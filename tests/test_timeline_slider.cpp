// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include "ui/TimeFormatter.h"
#include "ui/TimelineSlider.h"
#include "engine/IPlaybackEngine.h"

class TestTimelineSlider : public QObject {
    Q_OBJECT

private slots:
    void testTimeFormatterBasic();
    void testTimeFormatterPair();
    void testSliderProperties();
    void testClickToSeek();
    void testScrubbingLifecycle();
    void testChaptersAndBuffer();
    void testPaintEventExecution();
};

void TestTimelineSlider::testTimeFormatterBasic()
{
    using outplayer::ui::TimeFormatter;

    QCOMPARE(TimeFormatter::formatTime(0), QStringLiteral("00:00"));
    QCOMPARE(TimeFormatter::formatTime(59), QStringLiteral("00:59"));
    QCOMPARE(TimeFormatter::formatTime(60), QStringLiteral("01:00"));
    QCOMPARE(TimeFormatter::formatTime(3599), QStringLiteral("59:59"));
    QCOMPARE(TimeFormatter::formatTime(3600), QStringLiteral("01:00:00"));
    QCOMPARE(TimeFormatter::formatTime(3661), QStringLiteral("01:01:01"));

    // showHours explicitly requested
    QCOMPARE(TimeFormatter::formatTime(0, true), QStringLiteral("00:00:00"));
    QCOMPARE(TimeFormatter::formatTime(65, true), QStringLiteral("00:01:05"));

    // Negative seconds clamped to 0
    QCOMPARE(TimeFormatter::formatTime(-10), QStringLiteral("00:00"));
}

void TestTimelineSlider::testTimeFormatterPair()
{
    using outplayer::ui::TimeFormatter;

    // Both without hours when total < 3600
    QCOMPARE(TimeFormatter::formatTimePair(65, 120), QStringLiteral("01:05 / 02:00"));

    // Both with hours when total >= 3600
    QCOMPARE(TimeFormatter::formatTimePair(65, 3600), QStringLiteral("00:01:05 / 01:00:00"));
    QCOMPARE(TimeFormatter::formatTimePair(3661, 7200), QStringLiteral("01:01:01 / 02:00:00"));
    QCOMPARE(TimeFormatter::formatTimePair(0, 0), QStringLiteral("00:00 / 00:00"));
}

void TestTimelineSlider::testSliderProperties()
{
    outplayer::ui::TimelineSlider slider;
    slider.resize(200, 14);

    QCOMPARE(slider.durationSeconds(), 0.0);
    QCOMPARE(slider.currentSeconds(), 0.0);
    QVERIFY(!slider.isScrubbing());
    QCOMPARE(slider.sizeHint().height(), 14);

    slider.setDuration(120.0);
    QCOMPARE(slider.durationSeconds(), 120.0);

    slider.setPosition(45.5);
    QCOMPARE(slider.currentSeconds(), 45.5);

    // Negative position clamped to 0
    slider.setPosition(-5.0);
    QCOMPARE(slider.currentSeconds(), 0.0);

    // Position beyond duration clamped
    slider.setPosition(150.0);
    QCOMPARE(slider.currentSeconds(), 120.0);
}

void TestTimelineSlider::testClickToSeek()
{
    outplayer::ui::TimelineSlider slider;
    slider.resize(200, 14);
    slider.setDuration(100.0);
    slider.show();
    QVERIFY(slider.isVisible());

    QSignalSpy spySeek(&slider, &outplayer::ui::TimelineSlider::seekRequested);
    QSignalSpy spyScrubStarted(&slider, &outplayer::ui::TimelineSlider::scrubStarted);
    QSignalSpy spyScrubEnded(&slider, &outplayer::ui::TimelineSlider::scrubEnded);

    // Click at 50% width (x = 100 in 200px width)
    QPoint clickPoint(100, 7);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, clickPoint);

    QVERIFY(slider.isScrubbing());
    QCOMPARE(spyScrubStarted.count(), 1);
    QCOMPARE(spySeek.count(), 1);
    QCOMPARE(spySeek.takeFirst().at(0).toDouble(), 50.0);
    QCOMPARE(slider.currentSeconds(), 50.0);

    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, clickPoint);
    QVERIFY(!slider.isScrubbing());
    QCOMPARE(spyScrubEnded.count(), 1);
    QCOMPARE(spyScrubEnded.takeFirst().at(0).toDouble(), 50.0);
}

void TestTimelineSlider::testScrubbingLifecycle()
{
    outplayer::ui::TimelineSlider slider;
    slider.resize(200, 14);
    slider.setDuration(200.0);
    slider.show();

    QSignalSpy spyStarted(&slider, &outplayer::ui::TimelineSlider::scrubStarted);
    QSignalSpy spyMoved(&slider, &outplayer::ui::TimelineSlider::scrubMoved);
    QSignalSpy spySeek(&slider, &outplayer::ui::TimelineSlider::seekRequested);
    QSignalSpy spyEnded(&slider, &outplayer::ui::TimelineSlider::scrubEnded);

    // Press at x = 20 (10% -> 20.0s)
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(20, 7));
    QVERIFY(slider.isScrubbing());
    QCOMPARE(spyStarted.count(), 1);
    QCOMPARE(spySeek.count(), 1);
    QCOMPARE(spySeek.takeFirst().at(0).toDouble(), 20.0);

    // Drag move to x = 80 (40% -> 80.0s)
    QTest::mouseMove(&slider, QPoint(80, 7));
    QVERIFY(slider.isScrubbing());
    QCOMPARE(spyMoved.count(), 1);
    QCOMPARE(spyMoved.takeFirst().at(0).toDouble(), 80.0);
    QCOMPARE(spySeek.count(), 1);
    QCOMPARE(spySeek.takeFirst().at(0).toDouble(), 80.0);

    // Release at x = 80
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(80, 7));
    QVERIFY(!slider.isScrubbing());
    QCOMPARE(spyEnded.count(), 1);
    QCOMPARE(spyEnded.takeFirst().at(0).toDouble(), 80.0);
}

void TestTimelineSlider::testChaptersAndBuffer()
{
    outplayer::ui::TimelineSlider slider;
    slider.resize(200, 14);
    slider.setDuration(100.0);

    slider.setBufferProgress(0.65);

    QList<outplayer::engine::ChapterInfo> chapters = {
        {0, QStringLiteral("Intro"), 0.0},
        {1, QStringLiteral("Scene 1"), 25.0},
        {2, QStringLiteral("Scene 2"), 75.0}
    };
    slider.setChapters(chapters);

    slider.show();
    QVERIFY(slider.isVisible());
}

void TestTimelineSlider::testPaintEventExecution()
{
    outplayer::ui::TimelineSlider slider;
    slider.resize(200, 14);
    slider.setDuration(100.0);
    slider.setPosition(40.0);
    slider.setBufferProgress(0.8);

    QList<outplayer::engine::ChapterInfo> chapters = {
        {0, QStringLiteral("Chapter 1"), 30.0},
        {1, QStringLiteral("Chapter 2"), 60.0}
    };
    slider.setChapters(chapters);

    slider.show();
    slider.repaint();

    // Hover
    QTest::mouseMove(&slider, QPoint(100, 7));
    slider.repaint();

    // Scrubbing paint
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(100, 7));
    slider.repaint();
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(100, 7));
    slider.repaint();
}

QTEST_MAIN(TestTimelineSlider)
#include "test_timeline_slider.moc"
