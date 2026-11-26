// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QSignalSpy>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>
#include <cmath>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "input/InputManager.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"
#include "ui/ControlBar.h"
#include "ui/CustomTitleBar.h"
#include "ui/DesktopIntegration.h"
#include "ui/MainWindow.h"
#include "ui/OsdOverlay.h"
#include "ui/PlaylistDock.h"
#include "ui/PreferencesDialog.h"

using namespace outplayer;

class TestAuditRegressions : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testMpvEngineTrackSwitchDoesNotEmitPlaybackFinished();
    void testMainWindowTrackSwitchDoesNotAutoAdvance();
    void testMpvEngineStopResetsStateSignals();
    void testMpvEngineAspectRatioAutoResetsInLibmpv();
    void testMpvEngineAudioEqualizerProducesValidLavfiFilter();
    void testDesktopIntegrationNoDoubleToggleOrStop();
    void testInputManagerNoPlaylistScopeAmbiguityAndLineEditTyping();
    void testMainWindowPlaylistDockNoChromeOverlap();
    void testCoreCommandsActuallyMutateState();
    void testPreferencesKeyConsistencyAndLiveApply();
    void testCorruptedConfigImportRejected();
    void testUnicodePersianSubtitleLoading();
    void testRepeatedOperationsAndResourceStability();

private:
    QTemporaryDir m_tempDir;
    QString m_clip1Path;
    QString m_clip2Path;
};

void TestAuditRegressions::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());

    core::SettingsManager::instance().init(m_tempDir.filePath(QStringLiteral("test_settings.ini")));

    m_clip1Path = m_tempDir.filePath(QStringLiteral("clip1.mp4"));
    m_clip2Path = m_tempDir.filePath(QStringLiteral("clip2.mp4"));

    auto makeClip = [](const QString& outPath, const QString& color) {
        const QStringList args = {
            QStringLiteral("-y"),
            QStringLiteral("-f"), QStringLiteral("lavfi"),
            QStringLiteral("-i"), QStringLiteral("color=c=%1:s=320x240:r=10:d=3").arg(color),
            QStringLiteral("-f"), QStringLiteral("lavfi"),
            QStringLiteral("-i"), QStringLiteral("sine=frequency=440:duration=3"),
            QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
            QStringLiteral("-c:v"), QStringLiteral("libx264"),
            QStringLiteral("-c:a"), QStringLiteral("aac"),
            outPath
        };
        const int rc = QProcess::execute(QStringLiteral("ffmpeg"), args);
        return rc == 0 && QFile::exists(outPath);
    };

    QVERIFY(makeClip(m_clip1Path, QStringLiteral("blue")));
    QVERIFY(makeClip(m_clip2Path, QStringLiteral("green")));
}

void TestAuditRegressions::cleanup()
{
    core::SettingsManager::instance().resetToDefaults();
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestAuditRegressions::testMpvEngineTrackSwitchDoesNotEmitPlaybackFinished()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    QSignalSpy loadedSpy(&engine, &engine::IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_clip1Path));
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));

    QSignalSpy finishedSpy(&engine, &engine::IPlaybackEngine::playbackFinished);
    loadedSpy.clear();

    QVERIFY(engine.loadFile(m_clip2Path));
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));
    QTest::qWait(250);

    QCOMPARE(finishedSpy.count(), 0);
}

void TestAuditRegressions::testMainWindowTrackSwitchDoesNotAutoAdvance()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* playlist = window.playlistModel();
    QVERIFY(playlist != nullptr);

    playlist::PlaylistItem item1{m_clip1Path, QStringLiteral("Clip 1"), 3.0};
    playlist::PlaylistItem item2{m_clip2Path, QStringLiteral("Clip 2"), 3.0};
    playlist::PlaylistItem item3{m_clip1Path, QStringLiteral("Clip 3"), 3.0};
    playlist->addItems({item1, item2, item3});

    QSignalSpy loadedSpy(window.engine(), &engine::IPlaybackEngine::fileLoaded);
    window.playPlaylistItem(0);
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));
    QCOMPARE(playlist->currentIndex(), 0);

    loadedSpy.clear();
    window.playPlaylistItem(1);
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));
    QTest::qWait(300);

    QCOMPARE(playlist->currentIndex(), 1);
}

void TestAuditRegressions::testMpvEngineStopResetsStateSignals()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    QSignalSpy loadedSpy(&engine, &engine::IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_clip1Path));
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1 && engine.duration() > 1.0; }, 4000));

    QSignalSpy finishedSpy(&engine, &engine::IPlaybackEngine::playbackFinished);
    QSignalSpy posSpy(&engine, &engine::IPlaybackEngine::positionChanged);
    QSignalSpy durSpy(&engine, &engine::IPlaybackEngine::durationChanged);
    QSignalSpy tracksSpy(&engine, &engine::IPlaybackEngine::tracksChanged);

    engine.stop();
    QTest::qWait(300);

    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(engine.position(), 0.0);
    QCOMPARE(engine.duration(), 0.0);
    QVERIFY(posSpy.count() >= 1);
    QVERIFY(durSpy.count() >= 1);
    QVERIFY(tracksSpy.count() >= 1);
}

void TestAuditRegressions::testMpvEngineAspectRatioAutoResetsInLibmpv()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    engine.setAspectRatio(QStringLiteral("16:9"));
    const QString ar169 = engine.getPropertyString(QStringLiteral("video-aspect-override"));
    QVERIFY(ar169.startsWith(QStringLiteral("1.77")));

    engine.setAspectRatio(QStringLiteral("auto"));
    QTest::qWait(100);
    const QString arAuto = engine.getPropertyString(QStringLiteral("video-aspect-override"));
    // "no" in mpv returns negative (-2.000000) or "no"; never stays stuck at 1.777778
    QVERIFY2(!arAuto.startsWith(QStringLiteral("1.77")),
             qPrintable(QStringLiteral("Expected aspect reset from 16:9, got: %1").arg(arAuto)));
}

void TestAuditRegressions::testMpvEngineAudioEqualizerProducesValidLavfiFilter()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    engine.setEqualizerBand(0, 5.0);
    engine.setNormalizerEnabled(true);

    const QString af = engine.getPropertyString(QStringLiteral("af"));
    QVERIFY2(!af.contains(QStringLiteral("equalizer=g1=")),
             qPrintable(QStringLiteral("Broken FFmpeg syntax in af: %1").arg(af)));
    QVERIFY2(af.contains(QStringLiteral("firequalizer")),
             qPrintable(QStringLiteral("Expected firequalizer in af, got: %1").arg(af)));
    QVERIFY2(af.contains(QStringLiteral("dynaudnorm")),
             qPrintable(QStringLiteral("Expected dynaudnorm in af, got: %1").arg(af)));
}

void TestAuditRegressions::testDesktopIntegrationNoDoubleToggleOrStop()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* engine = window.engine();
    auto* desktop = window.desktopIntegration();
    QVERIFY(engine != nullptr);
    QVERIFY(desktop != nullptr);

    QSignalSpy loadedSpy(engine, &engine::IPlaybackEngine::fileLoaded);
    window.openMedia(m_clip1Path);
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));
    QVERIFY(!engine->isPaused());

    // playPause() must toggle once -> paused
    desktop->playPause();
    QVERIFY(QTest::qWaitFor([&]() { return engine->isPaused(); }, 1000));

    // pause() while already paused must remain paused (no spurious playPauseRequested toggle!)
    desktop->pause();
    QTest::qWait(200);
    QVERIFY(engine->isPaused());

    // playPause() -> resumed
    desktop->playPause();
    QVERIFY(QTest::qWaitFor([&]() { return !engine->isPaused(); }, 1000));

    // play() while already playing must remain playing (no spurious toggle!)
    desktop->play();
    QTest::qWait(200);
    QVERIFY(!engine->isPaused());
}

void TestAuditRegressions::testInputManagerNoPlaylistScopeAmbiguityAndLineEditTyping()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Check no duplicate WindowShortcut key sequences exist on MainWindow
    const auto shortcuts = window.findChildren<QShortcut*>();
    QSet<QString> seenKeys;
    QStringList duplicates;
    for (QShortcut* sc : shortcuts) {
        if (!sc->isEnabled() || sc->context() != Qt::WindowShortcut) {
            continue;
        }
        const QString keyStr = sc->key().toString();
        if (keyStr.isEmpty()) {
            continue;
        }
        if (seenKeys.contains(keyStr)) {
            duplicates.append(keyStr);
        }
        seenKeys.insert(keyStr);
    }
    QVERIFY2(duplicates.isEmpty(),
             qPrintable(QStringLiteral("Duplicate WindowShortcuts detected: %1").arg(duplicates.join(", "))));

    // Typing single-key hotkey characters into PlaylistDock search bar must edit text, not trigger player hotkeys
    window.setPlaylistVisible(true);
    window.activateWindow();
    auto* searchEdit = window.playlistDock()->searchEdit();
    QVERIFY(searchEdit != nullptr);
    searchEdit->setFocus();
    QApplication::processEvents();
    if (!searchEdit->hasFocus()) {
        if (window.windowHandle()) {
            window.windowHandle()->requestActivate();
        }
        window.activateWindow();
        searchEdit->setFocus();
    }
    QVERIFY(QTest::qWaitFor([&]() { return searchEdit->hasFocus(); }, 1000));

    window.engine()->setSpeed(1.0);
    QTest::keyClicks(searchEdit, QStringLiteral("cmxz"));
    QTest::keyClick(searchEdit, Qt::Key_Space);
    QTest::keyClicks(searchEdit, QStringLiteral("a"));
    QApplication::processEvents();

    QCOMPARE(searchEdit->text(), QStringLiteral("cmxz a"));
    QVERIFY(std::abs(window.engine()->speed() - 1.0) < 0.001);
}

void TestAuditRegressions::testMainWindowPlaylistDockNoChromeOverlap()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.setPlaylistVisible(true);
    window.setChromeVisible(true);
    QApplication::processEvents();

    QVERIFY(window.titleBar()->isVisible());
    QVERIFY(window.controlBar()->isVisible());
    QVERIFY(window.playlistDock()->isVisible());

    const QRect titleRect = window.titleBar()->geometry();
    const QRect ctrlRect = window.controlBar()->geometry();

    auto* tabBar = window.playlistDock()->tabBar();
    QVERIFY(tabBar != nullptr);
    const QPoint tabTopLeft = tabBar->mapTo(&window, QPoint(0, 0));
    QVERIFY2(tabTopLeft.y() >= titleRect.bottom() + 1,
             qPrintable(QStringLiteral("PlaylistDock tabBar top %1 overlaps TitleBar bottom %2")
                            .arg(tabTopLeft.y()).arg(titleRect.bottom())));

    auto* dockToolbar = window.playlistDock()->findChild<QWidget*>(QStringLiteral("bottomBar"));
    QVERIFY(dockToolbar != nullptr);
    const QPoint tbBottomLeft = dockToolbar->mapTo(&window, QPoint(0, dockToolbar->height() - 1));
    QVERIFY2(tbBottomLeft.y() < ctrlRect.top(),
             qPrintable(QStringLiteral("PlaylistDock bottomBar bottom %1 overlaps ControlBar top %2")
                            .arg(tbBottomLeft.y()).arg(ctrlRect.top())));
}

void TestAuditRegressions::testCoreCommandsActuallyMutateState()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* engine = window.engine();
    QVERIFY(engine != nullptr);

    QCOMPARE(engine->rotation(), 0);
    QVERIFY(core::CommandRegistry::instance().execute(core::CommandId::CMD_VIDEO_ROTATE_90));
    QCOMPARE(engine->rotation(), 90);

    const bool normBefore = engine->isNormalizerEnabled();
    QVERIFY(core::CommandRegistry::instance().execute(core::CommandId::CMD_AUDIO_NORMALIZE));
    QCOMPARE(engine->isNormalizerEnabled(), !normBefore);

    QVERIFY(core::CommandRegistry::instance().execute(core::CommandId::CMD_ASPECT_RATIO_2));
    QCOMPARE(engine->aspectRatio(), QStringLiteral("4:3"));

    QVERIFY(core::CommandRegistry::instance().execute(core::CommandId::CMD_ASPECT_RATIO_3));
    QCOMPARE(engine->aspectRatio(), QStringLiteral("16:9"));
}

void TestAuditRegressions::testPreferencesKeyConsistencyAndLiveApply()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    auto& settings = core::SettingsManager::instance();
    settings.setHardwareDecoder(QStringLiteral("auto-safe"));
    settings.set(QStringLiteral("Video/Deinterlace"), true);

    ui::PreferencesDialog dlg(&settings);
    dlg.setPlaybackEngine(&engine);

    QCOMPARE(dlg.hwdecComboBox()->currentText(), QStringLiteral("auto-safe"));
    QCOMPARE(dlg.deinterlaceCheckBox()->isChecked(), true);

    dlg.aspectRatioComboBox()->setCurrentText(QStringLiteral("16:9"));
    dlg.normalizerCheckBox()->setChecked(true);
    dlg.deinterlaceCheckBox()->setChecked(true);
    dlg.speedSpinBox()->setValue(1.5);
    dlg.applyButton()->click();

    QCOMPARE(engine.aspectRatio(), QStringLiteral("16:9"));
    QCOMPARE(engine.isNormalizerEnabled(), true);
    QCOMPARE(engine.deinterlace(), true);
    QVERIFY(std::abs(engine.speed() - 1.5) < 0.001);
}

void TestAuditRegressions::testCorruptedConfigImportRejected()
{
    auto& settings = core::SettingsManager::instance();
    settings.setDefaultVolume(77.0);

    const QString corruptPath = m_tempDir.filePath(QStringLiteral("corrupt_config.ini"));
    QFile f(corruptPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("\x00\xff\xfe\xfd not a valid ini file without key=value pairs \x01\x02");
    f.close();

    QVERIFY(!settings.importFromFile(corruptPath));
    QCOMPARE(settings.defaultVolume(), 77.0);
}

void TestAuditRegressions::testUnicodePersianSubtitleLoading()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    engine.setVo(QStringLiteral("null"));

    const QString srtPath = m_tempDir.filePath(QStringLiteral("زیرنویس_فارسی_تست.srt"));
    QFile srt(srtPath);
    QVERIFY(srt.open(QIODevice::WriteOnly | QIODevice::Text));
    srt.write("1\n00:00:00,000 --> 00:00:02,500\nسلام دنیا — زیرنویس آزمایشی\n");
    srt.close();

    QSignalSpy loadedSpy(&engine, &engine::IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_clip1Path));
    QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));

    QVERIFY(engine.loadSubtitle(srtPath));
    QVERIFY(QTest::qWaitFor([&]() { return !engine.tracks(QStringLiteral("sub")).isEmpty(); }, 3000));
}

void TestAuditRegressions::testRepeatedOperationsAndResourceStability()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* engine = window.engine();
    QVERIFY(engine != nullptr);

    const QString srtPath = m_tempDir.filePath(QStringLiteral("stress_sub.srt"));
    QFile srt(srtPath);
    QVERIFY(srt.open(QIODevice::WriteOnly | QIODevice::Text));
    srt.write("1\n00:00:00,000 --> 00:00:02,000\nStress Subtitle Line\n");
    srt.close();

    // 1. Repeatedly open, play, seek, load subtitle, and stop across alternating files
    for (int i = 0; i < 6; ++i) {
        QSignalSpy loadedSpy(engine, &engine::IPlaybackEngine::fileLoaded);
        window.openMedia((i % 2 == 0) ? m_clip1Path : m_clip2Path);
        QVERIFY(QTest::qWaitFor([&]() { return loadedSpy.count() >= 1; }, 4000));
        engine->seek(0.2 * (i % 4), true);
        QVERIFY(engine->loadSubtitle(srtPath));
        engine->setSubtitleVisible(i % 2 == 0);
        QApplication::processEvents();
        window.stopPlayback();
        QTest::qWait(50);
    }

    // 2. Repeatedly toggle Preferences, Playlist, Control Panel, and Fullscreen transitions
    for (int i = 0; i < 5; ++i) {
        window.setPlaylistVisible(true);
        window.setControlPanelVisible(true);
        window.preferencesDialog()->show();
        QApplication::processEvents();
        window.preferencesDialog()->hide();
        window.setControlPanelVisible(false);
        window.setPlaylistVisible(false);
        window.showFullScreen();
        QApplication::processEvents();
        window.showNormal();
        QApplication::processEvents();
    }

    QVERIFY(window.controlBar() != nullptr);
    QVERIFY(window.titleBar() != nullptr);
    QVERIFY(window.controlBar()->isVisible());
    QVERIFY(window.titleBar()->isVisible());
}

QTEST_MAIN(TestAuditRegressions)
#include "test_audit_regressions.moc"
