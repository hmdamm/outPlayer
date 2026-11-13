// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "ui/MainWindow.h"
#include "ui/OsdOverlay.h"

using namespace outplayer;

class TestMainWindowHotkeys : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testPlaybackSpeedUpAndDown();
    void testPlaybackSpeedNormal();
    void testPlaybackSpeedClamping();
    void testPlaybackSpeedPresets();
    void testFrameStepCommands();
    void testAllCommandsDispatchedWithoutError();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestMainWindowHotkeys::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());

    m_testVideoPath = m_tempDir.filePath("test_hotkey_video.mp4");

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

void TestMainWindowHotkeys::cleanup()
{
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestMainWindowHotkeys::testPlaybackSpeedUpAndDown()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.engine() != nullptr);
    QVERIFY(window.osdOverlay() != nullptr);

    // Initial speed should be 1.0x
    window.engine()->setSpeed(1.0);
    QCOMPARE(window.engine()->speed(), 1.0);

    // 1. Execute CMD_SPEED_UP (C)
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_UP);
    QVERIFY2(ok, "CMD_SPEED_UP handler must be registered");
    QVERIFY(std::abs(window.engine()->speed() - 1.1) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 1.1x"));

    // 2. Execute CMD_SPEED_UP again -> 1.2x
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_UP);
    QVERIFY(ok);
    QVERIFY(std::abs(window.engine()->speed() - 1.2) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 1.2x"));

    // 3. Execute CMD_SPEED_DOWN (X) -> 1.1x
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_DOWN);
    QVERIFY2(ok, "CMD_SPEED_DOWN handler must be registered");
    QVERIFY(std::abs(window.engine()->speed() - 1.1) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 1.1x"));

    // 4. Execute CMD_SPEED_DOWN again -> 1.0x
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_DOWN);
    QVERIFY(ok);
    QVERIFY(std::abs(window.engine()->speed() - 1.0) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 1.0x"));

    // 5. Execute CMD_SPEED_DOWN again -> 0.9x
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_DOWN);
    QVERIFY(ok);
    QVERIFY(std::abs(window.engine()->speed() - 0.9) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 0.9x"));
}

void TestMainWindowHotkeys::testPlaybackSpeedNormal()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Change speed to 1.5x
    window.engine()->setSpeed(1.5);
    QCOMPARE(window.engine()->speed(), 1.5);

    // Execute CMD_SPEED_NORMAL (Z) -> resets to 1.0x with OSD
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_NORMAL);
    QVERIFY2(ok, "CMD_SPEED_NORMAL handler must be registered");
    QCOMPARE(window.engine()->speed(), 1.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 1.0x (Normal)"));
}

void TestMainWindowHotkeys::testPlaybackSpeedClamping()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Test minimum clamping: 0.2x
    window.engine()->setSpeed(0.25);
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_DOWN);
    QVERIFY(ok);
    // 0.25 - 0.1 = 0.15 -> clamped to 0.2
    QVERIFY(window.engine()->speed() >= 0.2);

    window.engine()->setSpeed(0.2);
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_DOWN);
    QVERIFY(ok);
    QCOMPARE(window.engine()->speed(), 0.2);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 0.2x"));

    // Test maximum clamping: 12.0x
    window.engine()->setSpeed(11.95);
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_UP);
    QVERIFY(ok);
    QVERIFY(window.engine()->speed() <= 12.0);

    window.engine()->setSpeed(12.0);
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_UP);
    QVERIFY(ok);
    QCOMPARE(window.engine()->speed(), 12.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 12.0x"));
}

void TestMainWindowHotkeys::testPlaybackSpeedPresets()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // CMD_SPEED_FAST_2X
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_FAST_2X);
    QVERIFY2(ok, "CMD_SPEED_FAST_2X handler must be registered");
    QCOMPARE(window.engine()->speed(), 2.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 2.0x"));

    // CMD_SPEED_SLOW_HALF
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SPEED_SLOW_HALF);
    QVERIFY2(ok, "CMD_SPEED_SLOW_HALF handler must be registered");
    QCOMPARE(window.engine()->speed(), 0.5);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Speed: 0.5x"));
}

void TestMainWindowHotkeys::testFrameStepCommands()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // CMD_FRAME_STEP / CMD_FRAME_NEXT
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_FRAME_STEP);
    QVERIFY2(ok, "CMD_FRAME_STEP handler must be registered");
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Frame Step"));

    // CMD_FRAME_BACK / CMD_FRAME_PREV
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_FRAME_BACK);
    QVERIFY2(ok, "CMD_FRAME_BACK handler must be registered");
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Frame Step Back"));
}

void TestMainWindowHotkeys::testAllCommandsDispatchedWithoutError()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const auto commands = core::CommandRegistry::instance().allCommands();
    QVERIFY(commands.size() >= 253);

    for (const auto& cmd : commands) {
        bool ok = core::CommandRegistry::instance().execute(cmd.id);
        QVERIFY2(ok, qPrintable(QString("Command must execute without error: %1").arg(cmd.name)));
    }
}

QTEST_MAIN(TestMainWindowHotkeys)
#include "test_main_window_hotkeys.moc"
