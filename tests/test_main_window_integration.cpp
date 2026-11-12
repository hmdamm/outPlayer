// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QTest>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/DeviceStreamManager.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"
#include "ui/ContextMenuManager.h"
#include "ui/DesktopIntegration.h"
#include "ui/MainWindow.h"
#include "ui/PreferencesDialog.h"
#include "ui/ThemeManager.h"

using namespace outplayer;

class TestMainWindowIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testAccessorsAndInitialization();
    void testPreferencesCommandAndSignal();
    void testDesktopIntegrationSignals();
    void testScreensaverInhibition();
    void testOpenDvdAndDeviceCommands();
    void testPreferencesApplyThemeLive();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestMainWindowIntegration::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());

    m_testVideoPath = m_tempDir.filePath(QStringLiteral("test_integ_video.mp4"));

    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("lavfi"),
        QStringLiteral("-i"), QStringLiteral("testsrc=duration=5:size=320x240:rate=10"),
        QStringLiteral("-f"), QStringLiteral("lavfi"),
        QStringLiteral("-i"), QStringLiteral("sine=duration=5:frequency=440"),
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        m_testVideoPath
    };
    int exitCode = QProcess::execute(QStringLiteral("ffmpeg"), args);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));
}

void TestMainWindowIntegration::cleanup()
{
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestMainWindowIntegration::testAccessorsAndInitialization()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.preferencesDialog() != nullptr);
    QVERIFY(window.deviceStreamManager() != nullptr);
    QVERIFY(window.desktopIntegration() != nullptr);
}

void TestMainWindowIntegration::testPreferencesCommandAndSignal()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* prefDialog = window.preferencesDialog();
    QVERIFY(prefDialog != nullptr);
    QVERIFY(!prefDialog->isVisible());

    // 1. Command CMD_PREFERENCES (F5) opens PreferencesDialog
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PREFERENCES);
    QVERIFY(prefDialog->isVisible());

    prefDialog->hide();
    QVERIFY(!prefDialog->isVisible());

    // 2. ContextMenuManager preferencesRequested opens PreferencesDialog
    emit window.contextMenuManager()->preferencesRequested();
    QVERIFY(prefDialog->isVisible());

    prefDialog->hide();
    QVERIFY(!prefDialog->isVisible());

    // 3. DesktopIntegration preferencesRequested opens PreferencesDialog
    emit window.desktopIntegration()->preferencesRequested();
    QVERIFY(prefDialog->isVisible());

    prefDialog->hide();
}

void TestMainWindowIntegration::testDesktopIntegrationSignals()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* desktop = window.desktopIntegration();
    auto* engine = window.engine();
    auto* playlist = window.playlistModel();
    QVERIFY(desktop != nullptr);
    QVERIFY(engine != nullptr);
    QVERIFY(playlist != nullptr);

    // Populate playlist with two items
    playlist::PlaylistItem item1;
    item1.filePath = m_testVideoPath;
    item1.title = QStringLiteral("Track 1");
    playlist->addItem(item1);

    playlist::PlaylistItem item2;
    item2.filePath = m_testVideoPath;
    item2.title = QStringLiteral("Track 2");
    playlist->addItem(item2);

    window.playPlaylistItem(0);
    QCOMPARE(playlist->currentIndex(), 0);

    // Test nextRequested signal from DesktopIntegration
    emit desktop->nextRequested();
    QCOMPARE(playlist->currentIndex(), 1);

    // Test previousRequested signal from DesktopIntegration
    emit desktop->previousRequested();
    QCOMPARE(playlist->currentIndex(), 0);

    // Test playPauseRequested signal from DesktopIntegration
    const bool wasPaused = engine->isPaused();
    emit desktop->playPauseRequested();
    QTRY_COMPARE_WITH_TIMEOUT(engine->isPaused(), !wasPaused, 1000);
    emit desktop->playPauseRequested();
    QTRY_COMPARE_WITH_TIMEOUT(engine->isPaused(), wasPaused, 1000);

    // Test stopRequested signal from DesktopIntegration
    emit desktop->stopRequested();
    QTRY_VERIFY_WITH_TIMEOUT(engine->isPaused() || desktop->playbackStatus() == QStringLiteral("Stopped"), 1000);

    // Test raiseWindowRequested signal from DesktopIntegration
    window.showMinimized();
    emit desktop->raiseWindowRequested();
    QVERIFY(!window.isMinimized());
}

void TestMainWindowIntegration::testScreensaverInhibition()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* desktop = window.desktopIntegration();
    auto* engine = window.engine();
    QVERIFY(desktop != nullptr);
    QVERIFY(engine != nullptr);

    window.openMedia(m_testVideoPath);
    engine->play();
    QTRY_VERIFY_WITH_TIMEOUT(!engine->isPaused(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(desktop->isScreenSaverInhibited(), 3000);

    engine->pause();
    QTRY_VERIFY_WITH_TIMEOUT(engine->isPaused(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(!desktop->isScreenSaverInhibited(), 1000);

    engine->play();
    QTRY_VERIFY_WITH_TIMEOUT(!engine->isPaused(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(desktop->isScreenSaverInhibited(), 1000);
}

void TestMainWindowIntegration::testOpenDvdAndDeviceCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* playlist = window.playlistModel();
    QVERIFY(playlist != nullptr);

    // Execute CMD_OPEN_DVD
    core::CommandRegistry::instance().execute(core::CommandId::CMD_OPEN_DVD);
    const auto dvdItem = playlist->currentItem();
    QVERIFY(dvdItem.has_value());
    QVERIFY(dvdItem->filePath.startsWith(QStringLiteral("dvdnav://")));

    // Execute CMD_OPEN_DEVICE
    core::CommandRegistry::instance().execute(core::CommandId::CMD_OPEN_DEVICE);
    const auto devItem = playlist->currentItem();
    QVERIFY(devItem.has_value());
    QVERIFY(devItem->filePath.startsWith(QStringLiteral("avdevice://v4l2")) ||
            devItem->filePath.startsWith(QStringLiteral("v4l2://")));
}

void TestMainWindowIntegration::testPreferencesApplyThemeLive()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* prefDialog = window.preferencesDialog();
    QVERIFY(prefDialog != nullptr);

    // Set accent color to Crimson and opacity to 85%
    prefDialog->accentColorComboBox()->setCurrentText(QStringLiteral("Crimson"));
    prefDialog->opacitySlider()->setValue(85);
    prefDialog->saveSettings();

    QCOMPARE(ui::ThemeManager::instance().accentColor(), ui::ThemeManager::AccentColor::Crimson);
    QCOMPARE(static_cast<int>(window.windowOpacity() * 100.0 + 0.5), 85);

    // Reset back to Amber and 100%
    prefDialog->accentColorComboBox()->setCurrentText(QStringLiteral("Amber"));
    prefDialog->opacitySlider()->setValue(100);
    prefDialog->saveSettings();

    QCOMPARE(ui::ThemeManager::instance().accentColor(), ui::ThemeManager::AccentColor::Amber);
    QCOMPARE(static_cast<int>(window.windowOpacity() * 100.0 + 0.5), 100);
}

QTEST_MAIN(TestMainWindowIntegration)
#include "test_main_window_integration.moc"
