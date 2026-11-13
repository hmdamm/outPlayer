// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>
#include <QVBoxLayout>

#include "ui/ControlBar.h"
#include "ui/TimelineSlider.h"

using namespace outplayer;

class TestControlBarLayout : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testDimensionsAndHierarchy();
    void testTopRowControls();
    void testBottomRowControls();
    void testSignalEmissions();
    void testVideoInfoBadges();
    void testTimeFormatting();
};

void TestControlBarLayout::initTestCase()
{
}

void TestControlBarLayout::testDimensionsAndHierarchy()
{
    ui::ControlBar bar;

    // Height contract: exactly 56px for the 2-tier layout
    QCOMPARE(bar.height(), 56);
    QCOMPARE(bar.minimumHeight(), 56);
    QCOMPARE(bar.maximumHeight(), 56);
    QCOMPARE(bar.sizeHint().height(), 56);
    QCOMPARE(bar.minimumSizeHint().height(), 56);

    // Verify outer layout is a vertical box layout with two rows
    auto* mainLayout = qobject_cast<QVBoxLayout*>(bar.layout());
    QVERIFY(mainLayout != nullptr);
    QVERIFY(mainLayout->count() >= 2);

    auto* topLayout = qobject_cast<QHBoxLayout*>(mainLayout->itemAt(0)->layout());
    QVERIFY(topLayout != nullptr);

    auto* bottomLayout = qobject_cast<QHBoxLayout*>(mainLayout->itemAt(1)->layout());
    QVERIFY(bottomLayout != nullptr);
}

void TestControlBarLayout::testTopRowControls()
{
    ui::ControlBar bar;

    // Timeline Slider
    QVERIFY(bar.timelineSlider() != nullptr);

    // Mute button: 20x20 in top tier
    QVERIFY(bar.muteButton() != nullptr);
    QCOMPARE(bar.muteButton()->width(), 20);
    QCOMPARE(bar.muteButton()->height(), 20);

    // Volume slider: compact horizontal slider, 70px wide
    QVERIFY(bar.volumeSlider() != nullptr);
    QCOMPARE(bar.volumeSlider()->orientation(), Qt::Horizontal);
    QCOMPARE(bar.volumeSlider()->width(), 70);
}

void TestControlBarLayout::testBottomRowControls()
{
    ui::ControlBar bar;

    // Play/Pause button: 28x28
    QVERIFY(bar.playPauseButton() != nullptr);
    QCOMPARE(bar.playPauseButton()->width(), 28);
    QCOMPARE(bar.playPauseButton()->height(), 28);

    // Stop button: 24x24
    QVERIFY(bar.stopButton() != nullptr);
    QCOMPARE(bar.stopButton()->width(), 24);
    QCOMPARE(bar.stopButton()->height(), 24);

    // Prev button: 24x24
    QVERIFY(bar.prevButton() != nullptr);
    QCOMPARE(bar.prevButton()->width(), 24);
    QCOMPARE(bar.prevButton()->height(), 24);

    // Next button: 24x24
    QVERIFY(bar.nextButton() != nullptr);
    QCOMPARE(bar.nextButton()->width(), 24);
    QCOMPARE(bar.nextButton()->height(), 24);

    // Open File button: 24x24, glyph ⏏
    QVERIFY(bar.openFileButton() != nullptr);
    QCOMPARE(bar.openFileButton()->width(), 24);
    QCOMPARE(bar.openFileButton()->height(), 24);
    QCOMPARE(bar.openFileButton()->text(), QStringLiteral("⏏"));

    // Time Label
    QVERIFY(bar.timeLabel() != nullptr);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("00:00:00 / 00:00:00"));

    // Video info badges
    QVERIFY(bar.hwBadge() != nullptr);
    QCOMPARE(bar.hwBadge()->text(), QStringLiteral("H/W"));
    QVERIFY(bar.hwBadge()->isHidden());

    QVERIFY(bar.videoCodecBadge() != nullptr);
    QVERIFY(bar.videoCodecBadge()->isHidden());

    QVERIFY(bar.audioCodecBadge() != nullptr);
    QVERIFY(bar.audioCodecBadge()->isHidden());

    QVERIFY(bar.audioChannelsBadge() != nullptr);
    QVERIFY(bar.audioChannelsBadge()->isHidden());

    // Right-aligned utility buttons
    QVERIFY(bar.subtitleButton() != nullptr);
    QCOMPARE(bar.subtitleButton()->width(), 24);
    QCOMPARE(bar.subtitleButton()->height(), 24);

    QVERIFY(bar.controlPanelButton() != nullptr);
    QCOMPARE(bar.controlPanelButton()->width(), 24);
    QCOMPARE(bar.controlPanelButton()->height(), 24);

    QVERIFY(bar.settingsButton() != nullptr);
    QCOMPARE(bar.settingsButton()->width(), 24);
    QCOMPARE(bar.settingsButton()->height(), 24);
    QCOMPARE(bar.settingsButton()->text(), QStringLiteral("⚙"));

    QVERIFY(bar.playlistButton() != nullptr);
    QCOMPARE(bar.playlistButton()->width(), 24);
    QCOMPARE(bar.playlistButton()->height(), 24);
}

void TestControlBarLayout::testSignalEmissions()
{
    ui::ControlBar bar;

    QSignalSpy spyOpen(&bar, &ui::ControlBar::openFileClicked);
    QSignalSpy spyPref(&bar, &ui::ControlBar::preferencesClicked);
    QSignalSpy spySub(&bar, &ui::ControlBar::subtitleToggled);
    QSignalSpy spyPl(&bar, &ui::ControlBar::playlistToggled);
    QSignalSpy spyCp(&bar, &ui::ControlBar::controlPanelToggled);

    bar.openFileButton()->click();
    QCOMPARE(spyOpen.count(), 1);

    bar.settingsButton()->click();
    QCOMPARE(spyPref.count(), 1);

    bar.subtitleButton()->click();
    QCOMPARE(spySub.count(), 1);

    bar.playlistButton()->click();
    QCOMPARE(spyPl.count(), 1);

    bar.controlPanelButton()->click();
    QCOMPARE(spyCp.count(), 1);
}

void TestControlBarLayout::testVideoInfoBadges()
{
    ui::ControlBar bar;

    // Badges initially hidden
    QVERIFY(bar.hwBadge()->isHidden());
    QVERIFY(bar.videoCodecBadge()->isHidden());
    QVERIFY(bar.audioCodecBadge()->isHidden());
    QVERIFY(bar.audioChannelsBadge()->isHidden());

    // Update with stream info
    bar.setVideoInfo(true, QStringLiteral("AVC1"), QStringLiteral("AAC"), QStringLiteral("2.0"));

    QVERIFY(bar.isHwDecoding());
    QCOMPARE(bar.videoCodec(), QStringLiteral("AVC1"));
    QCOMPARE(bar.audioCodec(), QStringLiteral("AAC"));
    QCOMPARE(bar.audioChannels(), QStringLiteral("2.0"));

    QVERIFY(!bar.hwBadge()->isHidden());
    QCOMPARE(bar.hwBadge()->text(), QStringLiteral("H/W"));

    QVERIFY(!bar.videoCodecBadge()->isHidden());
    QCOMPARE(bar.videoCodecBadge()->text(), QStringLiteral("AVC1"));

    QVERIFY(!bar.audioCodecBadge()->isHidden());
    QCOMPARE(bar.audioCodecBadge()->text(), QStringLiteral("AAC"));

    QVERIFY(!bar.audioChannelsBadge()->isHidden());
    QCOMPARE(bar.audioChannelsBadge()->text(), QStringLiteral("2.0"));

    // Reset / clear stream info
    bar.setVideoInfo(false, QString(), QString(), QString());

    QVERIFY(!bar.isHwDecoding());
    QVERIFY(bar.videoCodec().isEmpty());
    QVERIFY(bar.audioCodec().isEmpty());
    QVERIFY(bar.audioChannels().isEmpty());

    QVERIFY(bar.hwBadge()->isHidden());
    QVERIFY(bar.videoCodecBadge()->isHidden());
    QVERIFY(bar.audioCodecBadge()->isHidden());
    QVERIFY(bar.audioChannelsBadge()->isHidden());
}

void TestControlBarLayout::testTimeFormatting()
{
    ui::ControlBar bar;

    // Default label text
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("00:00:00 / 00:00:00"));

    // Formats with hours when duration >= 3600
    bar.setTime(3665.0, 7200.0);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("01:01:05 / 02:00:00"));

    // Formats MM:SS when under an hour
    bar.setTime(75.0, 180.0);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("01:15 / 03:00"));
}

QTEST_MAIN(TestControlBarLayout)
#include "test_control_bar_layout.moc"
