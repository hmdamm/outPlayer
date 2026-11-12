// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QKeySequence>
#include <QMouseEvent>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>
#include <QWidget>
#include <memory>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "input/InputManager.h"

class TestInputManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaultShortcutsLoaded();
    void testRebindShortcut();
    void testShortcutExecution();
    void testMouseFilter();
    void testMouseClickGestures();
    void testRebindMouseMapping();
    void testAttachToWindowLater();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

void TestInputManager::init() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    QString iniPath = m_tempDir->filePath("test_input_settings.ini");
    outplayer::core::SettingsManager::instance().init(iniPath);
    outplayer::core::CommandRegistry::instance().clearHandlers();
}

void TestInputManager::cleanup() {
    outplayer::core::CommandRegistry::instance().clearHandlers();
    m_tempDir.reset();
}

void TestInputManager::testDefaultShortcutsLoaded() {
    QWidget dummyWindow;
    outplayer::input::InputManager inputMgr(&dummyWindow);
    QKeySequence seq = inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QCOMPARE(seq, QKeySequence(Qt::Key_Space));

    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_VOLUME_UP), QKeySequence(Qt::Key_Up));
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_VOLUME_DOWN), QKeySequence(Qt::Key_Down));
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_SEEK_FORWARD_SHORT), QKeySequence(Qt::Key_Right));
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_SEEK_BACKWARD_SHORT), QKeySequence(Qt::Key_Left));
}

void TestInputManager::testRebindShortcut() {
    QWidget dummyWindow;
    outplayer::input::InputManager inputMgr(&dummyWindow);
    QVERIFY(inputMgr.setCustomShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE, QKeySequence(Qt::Key_P)));
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE), QKeySequence(Qt::Key_P));
    inputMgr.resetShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE), QKeySequence(Qt::Key_Space));
}

void TestInputManager::testShortcutExecution() {
    QWidget dummyWindow;
    dummyWindow.show();
    QVERIFY(QTest::qWaitForWindowActive(&dummyWindow));
    outplayer::input::InputManager inputMgr(&dummyWindow);

    bool executed = false;
    outplayer::core::CommandRegistry::instance().registerHandler(
        outplayer::core::CommandId::CMD_PLAY_PAUSE,
        [&executed](const QVariantMap&) {
            executed = true;
        }
    );

    // Simulate shortcut key activation
    QTest::keyClick(&dummyWindow, Qt::Key_Space);
    QVERIFY(executed);
}

void TestInputManager::testMouseFilter() {
    QWidget dummySurface;
    outplayer::input::InputManager inputMgr;
    inputMgr.attachMouseFilter(&dummySurface);
    inputMgr.setMouseMapping(outplayer::input::MouseAction::WheelUp, outplayer::core::CommandId::CMD_VOLUME_UP);
    QSignalSpy spy(&inputMgr, &outplayer::input::InputManager::mouseActionTriggered);

    // Simulate mouse wheel up event
    QWheelEvent wheelUpEv(QPointF(10, 10), QPointF(10, 10), QPoint(0, 120), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&dummySurface, &wheelUpEv);
    QCOMPARE(spy.count(), 1);
    auto firstSignal = spy.takeFirst();
    QCOMPARE(firstSignal.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::WheelUp);
    QCOMPARE(firstSignal.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_VOLUME_UP);

    // Simulate mouse wheel down event
    QWheelEvent wheelDownEv(QPointF(10, 10), QPointF(10, 10), QPoint(0, -120), QPoint(0, -120),
                            Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&dummySurface, &wheelDownEv);
    QCOMPARE(spy.count(), 1);
    auto secondSignal = spy.takeFirst();
    QCOMPARE(secondSignal.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::WheelDown);
    QCOMPARE(secondSignal.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_VOLUME_DOWN);
}

void TestInputManager::testMouseClickGestures() {
    QWidget dummySurface;
    outplayer::input::InputManager inputMgr;
    inputMgr.attachMouseFilter(&dummySurface);
    QSignalSpy spy(&inputMgr, &outplayer::input::InputManager::mouseActionTriggered);

    // Left click
    QMouseEvent leftClick(QEvent::MouseButtonPress, QPointF(20, 20), QPointF(20, 20),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&dummySurface, &leftClick);
    QCOMPARE(spy.count(), 1);
    auto sig1 = spy.takeFirst();
    QCOMPARE(sig1.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::LeftClick);
    QCOMPARE(sig1.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_PLAY_PAUSE);

    // Left double click
    QMouseEvent leftDblClick(QEvent::MouseButtonDblClick, QPointF(20, 20), QPointF(20, 20),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&dummySurface, &leftDblClick);
    QCOMPARE(spy.count(), 1);
    auto sig2 = spy.takeFirst();
    QCOMPARE(sig2.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::LeftDblClick);
    QCOMPARE(sig2.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_WINDOW_FULLSCREEN);

    // Middle click
    QMouseEvent midClick(QEvent::MouseButtonPress, QPointF(20, 20), QPointF(20, 20),
                         Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    QApplication::sendEvent(&dummySurface, &midClick);
    QCOMPARE(spy.count(), 1);
    auto sig3 = spy.takeFirst();
    QCOMPARE(sig3.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::MiddleClick);
    QCOMPARE(sig3.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_VOLUME_MUTE);

    // Right click
    QMouseEvent rightClick(QEvent::MouseButtonPress, QPointF(20, 20), QPointF(20, 20),
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&dummySurface, &rightClick);
    QCOMPARE(spy.count(), 1);
    auto sig4 = spy.takeFirst();
    QCOMPARE(sig4.at(0).value<outplayer::input::MouseAction>(), outplayer::input::MouseAction::RightClick);
    QCOMPARE(sig4.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_MOUSE_RIGHT_CLICK);
}

void TestInputManager::testRebindMouseMapping() {
    QWidget dummySurface;
    outplayer::input::InputManager inputMgr;
    inputMgr.attachMouseFilter(&dummySurface);

    inputMgr.setMouseMapping(outplayer::input::MouseAction::LeftClick, outplayer::core::CommandId::CMD_SEEK_FORWARD_SHORT);
    QCOMPARE(inputMgr.getMouseMapping(outplayer::input::MouseAction::LeftClick), outplayer::core::CommandId::CMD_SEEK_FORWARD_SHORT);

    QSignalSpy spy(&inputMgr, &outplayer::input::InputManager::mouseActionTriggered);
    QMouseEvent leftClick(QEvent::MouseButtonPress, QPointF(20, 20), QPointF(20, 20),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&dummySurface, &leftClick);
    QCOMPARE(spy.count(), 1);
    auto sig = spy.takeFirst();
    QCOMPARE(sig.at(1).value<outplayer::core::CommandId>(), outplayer::core::CommandId::CMD_SEEK_FORWARD_SHORT);
}

void TestInputManager::testAttachToWindowLater() {
    QWidget dummyWindow;
    dummyWindow.show();
    QVERIFY(QTest::qWaitForWindowActive(&dummyWindow));
    outplayer::input::InputManager inputMgr;
    inputMgr.attachToWindow(&dummyWindow);

    bool executed = false;
    outplayer::core::CommandRegistry::instance().registerHandler(
        outplayer::core::CommandId::CMD_PLAY_PAUSE,
        [&executed](const QVariantMap&) {
            executed = true;
        }
    );

    QTest::keyClick(&dummyWindow, Qt::Key_Space);
    QVERIFY(executed);
}

QTEST_MAIN(TestInputManager)
#include "test_input_manager.moc"
