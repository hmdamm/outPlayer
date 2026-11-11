// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include "ui/VideoWidget.h"
#include "ui/MainWindow.h"
#include "engine/MpvEngine.h"

class TestVideoWidget : public QObject {
    Q_OBJECT

private slots:
    void testInitAndRender();
    void testPlaybackFrame();
    void testMainWindow();
    void testVideoWidgetNullEngine();
};

void TestVideoWidget::testInitAndRender()
{
    outplayer::engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    outplayer::ui::VideoWidget widget(&engine);
    QCOMPARE(widget.engine(), &engine);
    widget.resize(640, 480);
    widget.show();
    QVERIFY(widget.isVisible());
    QVERIFY(widget.ensureInitialized());
    QVERIFY(widget.renderContext() != nullptr);
    QCOMPARE(engine.getPropertyString(QStringLiteral("vo")), QStringLiteral("libmpv"));
}

void TestVideoWidget::testPlaybackFrame()
{
    outplayer::engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    outplayer::ui::VideoWidget widget(&engine);
    widget.resize(640, 480);
    widget.show();
    QVERIFY(widget.isVisible());
    QVERIFY(widget.ensureInitialized());
    QVERIFY(widget.renderContext() != nullptr);
    QCOMPARE(engine.getPropertyString(QStringLiteral("vo")), QStringLiteral("libmpv"));

    QVERIFY(engine.loadFile(QStringLiteral("lavfi://nullsrc=s=640x360:d=1")));

    // Verify frames are actually delivered to VideoWidget and rendered
    QTRY_VERIFY_WITH_TIMEOUT(widget.frameCount() > 0, 3000);
}

void TestVideoWidget::testMainWindow()
{
    outplayer::ui::MainWindow window;
    QVERIFY(window.engine() != nullptr);
    QVERIFY(window.videoWidget() != nullptr);
    QCOMPARE(window.centralWidget(), window.videoWidget());

    window.show();
    QVERIFY(window.isVisible());

    QVERIFY(window.openMedia(QStringLiteral("lavfi://nullsrc=s=640x360:d=1")));
    QVERIFY(window.videoWidget()->renderContext() != nullptr);
    QCOMPARE(window.engine()->getPropertyString(QStringLiteral("vo")), QStringLiteral("libmpv"));

    // Verify frames rendered through MainWindow
    QTRY_VERIFY_WITH_TIMEOUT(window.videoWidget()->frameCount() > 0, 3000);
}

void TestVideoWidget::testVideoWidgetNullEngine()
{
    outplayer::ui::VideoWidget widget(nullptr);
    QCOMPARE(widget.engine(), nullptr);
    QCOMPARE(widget.renderContext(), nullptr);
    QCOMPARE(widget.frameCount(), 0ULL);
    widget.resize(320, 240);
    widget.show();
    QVERIFY(widget.isVisible());
    QVERIFY(!widget.ensureInitialized());
    widget.maybeUpdate();
}

QTEST_MAIN(TestVideoWidget)
#include "test_video_widget.moc"
