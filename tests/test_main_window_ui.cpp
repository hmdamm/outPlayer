// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "input/InputManager.h"
#include "ui/ControlBar.h"
#include "ui/CustomTitleBar.h"
#include "ui/MainWindow.h"
#include "ui/OsdOverlay.h"
#include "ui/VideoWidget.h"

using namespace outplayer;

class TestMainWindowUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void testCustomTitleBarProperties();
    void testWindowDecorationsFramelessByDefault();
    void testWindowDecorationsSystemWhenEnabled();
    void testTitleBarWindowActions();
    void testFullscreenToggleHidesChrome();
    void testAutohideChromeDuringPlayback();
    void testAutohideDoesNotTriggerWhenPaused();
    void testUiCompositionIntegrity();
};

void TestMainWindowUi::initTestCase() {
    QVERIFY(qApp != nullptr);
}

void TestMainWindowUi::cleanup() {
    // Restore default settings
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestMainWindowUi::testCustomTitleBarProperties() {
    ui::CustomTitleBar titleBar;
    QCOMPARE(titleBar.height(), 30);
    QCOMPARE(titleBar.sizeHint().height(), 30);

    titleBar.setTitle(QStringLiteral("Test Video - outPlayer"));
    QCOMPARE(titleBar.title(), QStringLiteral("Test Video - outPlayer"));

    QCOMPARE(titleBar.isMaximizedState(), false);
    titleBar.setMaximizedState(true);
    QCOMPARE(titleBar.isMaximizedState(), true);
    titleBar.setMaximizedState(false);
    QCOMPARE(titleBar.isMaximizedState(), false);

    QSignalSpy minSpy(&titleBar, &ui::CustomTitleBar::minimizeClicked);
    QSignalSpy maxSpy(&titleBar, &ui::CustomTitleBar::maximizeRestoreClicked);
    QSignalSpy closeSpy(&titleBar, &ui::CustomTitleBar::closeClicked);

    QVERIFY(titleBar.minimizeButton() != nullptr);
    QVERIFY(titleBar.maximizeButton() != nullptr);
    QVERIFY(titleBar.closeButton() != nullptr);

    titleBar.minimizeButton()->click();
    QCOMPARE(minSpy.count(), 1);

    titleBar.maximizeButton()->click();
    QCOMPARE(maxSpy.count(), 1);

    titleBar.closeButton()->click();
    QCOMPARE(closeSpy.count(), 1);
}

void TestMainWindowUi::testWindowDecorationsFramelessByDefault() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(window.titleBar() != nullptr);
    QCOMPARE(window.titleBar()->height(), 30);
    QVERIFY(!window.titleBar()->isHidden());
}

void TestMainWindowUi::testWindowDecorationsSystemWhenEnabled() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(true);

    ui::MainWindow window;
    QVERIFY(!window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(window.titleBar() != nullptr);
    QVERIFY(window.titleBar()->isHidden());
}

void TestMainWindowUi::testTitleBarWindowActions() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(window.isVisible());

    // Test maximize/restore toggle via titlebar
    QVERIFY(!window.isMaximized());
    window.titleBar()->maximizeButton()->click();
    QTRY_VERIFY(window.isMaximized());
    QCOMPARE(window.titleBar()->isMaximizedState(), true);

    window.titleBar()->maximizeButton()->click();
    QTRY_VERIFY(!window.isMaximized());
    QCOMPARE(window.titleBar()->isMaximizedState(), false);

    // Test minimize
    window.titleBar()->minimizeButton()->click();
    QTRY_VERIFY(window.isMinimized());
}

void TestMainWindowUi::testFullscreenToggleHidesChrome() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(window.isVisible());

    QVERIFY(!window.isFullScreen());
    QVERIFY(!window.titleBar()->isHidden());
    QVERIFY(!window.controlBar()->isHidden());

    // Toggle fullscreen via command
    core::CommandRegistry::instance().execute(core::CommandId::CMD_WINDOW_FULLSCREEN);
    QTRY_VERIFY(window.isFullScreen());

    // In fullscreen mode, titlebar and controlbar are hidden
    QVERIFY(window.titleBar()->isHidden());
    QVERIFY(window.controlBar()->isHidden());

    // Toggle fullscreen back to normal window
    core::CommandRegistry::instance().execute(core::CommandId::CMD_WINDOW_FULLSCREEN);
    QTRY_VERIFY(!window.isFullScreen());

    // Restored to windowed: titlebar and controlbar become visible
    QVERIFY(!window.titleBar()->isHidden());
    QVERIFY(!window.controlBar()->isHidden());
}

void TestMainWindowUi::testAutohideChromeDuringPlayback() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(window.isVisible());

    // Start synthetic playback
    QVERIFY(window.openMedia(QStringLiteral("lavfi://nullsrc=s=640x360:d=10")));
    window.engine()->play();
    QTRY_VERIFY(!window.engine()->isPaused());

    // Chrome starts visible
    QVERIFY(window.isChromeVisible());
    QVERIFY(!window.titleBar()->isHidden());
    QVERIFY(!window.controlBar()->isHidden());

    // Wait for 2.5s inactivity timer to hide chrome
    QTRY_VERIFY_WITH_TIMEOUT(!window.isChromeVisible(), 4000);
    QVERIFY(window.titleBar()->isHidden());
    QVERIFY(window.controlBar()->isHidden());
    QCOMPARE(window.cursor().shape(), Qt::BlankCursor);

    // Mouse motion triggers chrome restoration
    QTest::mouseMove(window.videoWidget(), QPoint(200, 200));
    QTRY_VERIFY(window.isChromeVisible());
    QVERIFY(!window.titleBar()->isHidden());
    QVERIFY(!window.controlBar()->isHidden());
    QVERIFY(window.cursor().shape() != Qt::BlankCursor);
}

void TestMainWindowUi::testAutohideDoesNotTriggerWhenPaused() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(window.isVisible());

    QVERIFY(window.openMedia(QStringLiteral("lavfi://nullsrc=s=640x360:d=10")));
    window.engine()->pause();
    QVERIFY(window.engine()->isPaused());

    // Ensure chrome is visible
    QVERIFY(window.isChromeVisible());

    // Wait 2.7s - since media is paused, chrome must NOT autohide
    QTest::qWait(2700);
    QVERIFY(window.isChromeVisible());
    QVERIFY(!window.titleBar()->isHidden());
    QVERIFY(!window.controlBar()->isHidden());
}

void TestMainWindowUi::testUiCompositionIntegrity() {
    ui::MainWindow window;
    QCOMPARE(window.centralWidget(), window.videoWidget());
    QVERIFY(window.engine() != nullptr);
    QVERIFY(window.videoWidget() != nullptr);
    QVERIFY(window.titleBar() != nullptr);
    QVERIFY(window.controlBar() != nullptr);
    QVERIFY(window.osdOverlay() != nullptr);
    QVERIFY(window.inputManager() != nullptr);
}

QTEST_MAIN(TestMainWindowUi)
#include "test_main_window_ui.moc"
