// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

#include "core/BookmarkManager.h"
#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "playlist/PlaylistModel.h"
#include "ui/ContextMenuManager.h"
#include "ui/ControlBar.h"
#include "ui/ControlPanel.h"
#include "ui/MainWindow.h"
#include "ui/OsdOverlay.h"
#include "ui/ScreenCaptureController.h"

using namespace outplayer;

class TestMainWindowAv : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testAccessorsAndInitialization();
    void testControlPanelToggle();
    void testBookmarkCommands();
    void testFrameCaptureCommands();
    void testVideoEqualizerCommands();
    void testAudioAndSubtitleSyncCommands();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestMainWindowAv::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());

    m_testVideoPath = m_tempDir.filePath("test_av_video.mp4");

    const QStringList args = {
        "-y",
        "-f", "lavfi",
        "-i", "testsrc=duration=5:size=320x240:rate=10",
        "-f", "lavfi",
        "-i", "sine=duration=5:frequency=440",
        "-pix_fmt", "yuv420p",
        "-c:v", "libx264",
        "-c:a", "aac",
        m_testVideoPath
    };
    int exitCode = QProcess::execute("ffmpeg", args);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));
}

void TestMainWindowAv::cleanup()
{
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestMainWindowAv::testAccessorsAndInitialization()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Accessors must be non-null
    QVERIFY(window.controlPanel() != nullptr);
    QVERIFY(window.bookmarkManager() != nullptr);
    QVERIFY(window.screenCaptureController() != nullptr);

    // Initial state: ControlPanel is hidden
    QVERIFY(window.controlPanel()->isHidden());
}

void TestMainWindowAv::testControlPanelToggle()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ui::ControlPanel* cp = window.controlPanel();
    QVERIFY(cp != nullptr);
    QVERIFY(cp->isHidden());

    // 1. Toggle via CP button on ControlBar
    QVERIFY(window.controlBar() != nullptr);
    QVERIFY(window.controlBar()->controlPanelButton() != nullptr);
    window.controlBar()->controlPanelButton()->click();
    QVERIFY(cp->isVisible());

    window.controlBar()->controlPanelButton()->click();
    QVERIFY(cp->isHidden());

    // 2. Toggle via CMD_CONTROL_PANEL (F7)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_CONTROL_PANEL);
    QVERIFY(cp->isVisible());

    core::CommandRegistry::instance().execute(core::CommandId::CMD_CONTROL_PANEL);
    QVERIFY(cp->isHidden());

    // 3. Toggle via CMD_PANEL_CONTROL alias
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PANEL_CONTROL);
    QVERIFY(cp->isVisible());

    core::CommandRegistry::instance().execute(core::CommandId::CMD_PANEL_CONTROL);
    QVERIFY(cp->isHidden());

    // 4. Toggle via ContextMenuManager signal
    QVERIFY(window.contextMenuManager() != nullptr);
    emit window.contextMenuManager()->toggleControlPanelRequested();
    QVERIFY(cp->isVisible());

    emit window.contextMenuManager()->toggleControlPanelRequested();
    QVERIFY(cp->isHidden());
}

void TestMainWindowAv::testBookmarkCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.openMedia(m_testVideoPath));
    QTest::qWait(500);

    core::BookmarkManager* bm = window.bookmarkManager();
    QVERIFY(bm != nullptr);
    bm->clearBookmarks(m_testVideoPath);

    // Seek to 1.0s and add bookmark via CMD_BOOKMARK_ADD (H)
    window.engine()->seek(1.0);
    QTest::qWait(200);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_BOOKMARK_ADD);
    QTest::qWait(100);

    auto list1 = bm->bookmarksForFile(m_testVideoPath);
    QCOMPARE(list1.size(), 1);
    QVERIFY(std::abs(list1.first().timestamp - 1.0) < 0.5);

    QVERIFY(window.osdOverlay() != nullptr);
    QVERIFY(window.osdOverlay()->currentText().contains("Bookmark added:"));

    // Seek to 3.0s and add second bookmark
    window.engine()->seek(3.0);
    QTest::qWait(200);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_BOOKMARK_ADD);
    QTest::qWait(100);

    auto list2 = bm->bookmarksForFile(m_testVideoPath);
    QCOMPARE(list2.size(), 2);

    // From 3.0s, jump to previous bookmark via CMD_BOOKMARK_PREV (Shift+H)
    window.engine()->seek(2.5);
    QTest::qWait(100);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_BOOKMARK_PREV);
    QTest::qWait(200);

    QVERIFY(std::abs(window.engine()->position() - list2.first().timestamp) < 0.5);
    QVERIFY(window.osdOverlay()->currentText().contains("Bookmark:"));

    // Jump to next bookmark via CMD_BOOKMARK_NEXT (Ctrl+H)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_BOOKMARK_NEXT);
    QTest::qWait(200);

    QVERIFY(std::abs(window.engine()->position() - list2.last().timestamp) < 0.5);
    QVERIFY(window.osdOverlay()->currentText().contains("Bookmark:"));
}

void TestMainWindowAv::testFrameCaptureCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.openMedia(m_testVideoPath));
    QTest::qWait(500);

    // Frame Capture Save: CMD_CAPTURE_SAVE (Ctrl+E)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_CAPTURE_SAVE);
    QTest::qWait(300);

    QVERIFY(window.osdOverlay() != nullptr);
    QVERIFY(window.osdOverlay()->currentText().contains("Captured:"));

    // Frame Capture Copy: CMD_CAPTURE_COPY (Ctrl+C)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_CAPTURE_COPY);
    QTest::qWait(300);

    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Frame copied to clipboard"));
}

void TestMainWindowAv::testVideoEqualizerCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    engine::MpvEngine* eng = window.engine();
    QVERIFY(eng != nullptr);
    eng->resetVideoEqualizer();

    // Brightness
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_BRIGHTNESS_UP);
    QCOMPARE(eng->brightness(), 1);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Brightness: 1%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_BRIGHTNESS_DOWN);
    QCOMPARE(eng->brightness(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Brightness: 0%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_BRIGHTNESS_DOWN);
    QCOMPARE(eng->brightness(), -1);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Brightness: -1%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_BRIGHTNESS_RESET);
    QCOMPARE(eng->brightness(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Brightness: 0%"));

    // Contrast
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_CONTRAST_UP);
    QCOMPARE(eng->contrast(), 1);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Contrast: 1%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_CONTRAST_DOWN);
    QCOMPARE(eng->contrast(), 0);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_CONTRAST_RESET);
    QCOMPARE(eng->contrast(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Contrast: 0%"));

    // Saturation
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_SATURATION_UP);
    QCOMPARE(eng->saturation(), 1);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Saturation: 1%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_SATURATION_DOWN);
    QCOMPARE(eng->saturation(), 0);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_SATURATION_RESET);
    QCOMPARE(eng->saturation(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Saturation: 0%"));

    // Hue
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_HUE_UP);
    QCOMPARE(eng->hue(), 1);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Hue: 1%"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_HUE_DOWN);
    QCOMPARE(eng->hue(), 0);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_HUE_RESET);
    QCOMPARE(eng->hue(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Hue: 0%"));

    // Video Reset
    eng->setBrightness(10);
    eng->setContrast(10);
    eng->setSaturation(10);
    eng->setHue(10);
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_RESET);
    QCOMPARE(eng->brightness(), 0);
    QCOMPARE(eng->contrast(), 0);
    QCOMPARE(eng->saturation(), 0);
    QCOMPARE(eng->hue(), 0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Video adjustments reset"));
}

void TestMainWindowAv::testAudioAndSubtitleSyncCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    engine::MpvEngine* eng = window.engine();
    QVERIFY(eng != nullptr);

    // Audio Sync
    eng->setAudioDelay(0.0);
    core::CommandRegistry::instance().execute(core::CommandId::CMD_AUDIO_SYNC_FASTER);
    QVERIFY(std::abs(eng->audioDelay() - 0.05) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Audio Sync: +50 ms"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_AUDIO_SYNC_SLOWER);
    QVERIFY(std::abs(eng->audioDelay() - 0.0) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Audio Sync: 0 ms"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_AUDIO_SYNC_SLOWER);
    QVERIFY(std::abs(eng->audioDelay() - (-0.05)) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Audio Sync: -50 ms"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_AUDIO_SYNC_RESET);
    QCOMPARE(eng->audioDelay(), 0.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Audio Sync: 0 ms"));

    // Subtitle Sync
    eng->setSubtitleDelay(0.0);
    core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SYNC_FASTER);
    QVERIFY(std::abs(eng->subtitleDelay() - 0.5) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Sync: +0.5 s"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SYNC_SLOWER);
    QVERIFY(std::abs(eng->subtitleDelay() - 0.0) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Sync: 0.0 s"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SYNC_SLOWER);
    QVERIFY(std::abs(eng->subtitleDelay() - (-0.5)) < 1e-4);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Sync: -0.5 s"));

    core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SYNC_RESET);
    QCOMPARE(eng->subtitleDelay(), 0.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Sync: 0.0 s"));
}

QTEST_MAIN(TestMainWindowAv)
#include "test_main_window_av.moc"
