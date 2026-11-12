// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QSignalSpy>
#include "core/CommandRegistry.h"
#include "core/CommandId.h"

class TestCommandRegistry : public QObject {
    Q_OBJECT
private slots:
    void cleanup() {
        outplayer::core::CommandRegistry::instance().clearHandlers();
    }
    void testCommandCatalog();
    void testExecuteCommand();
    void testExecuteCommandWithString();
    void testExecuteUnknownCommand();
};

void TestCommandRegistry::testCommandCatalog() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    QVERIFY(registry.allCommands().size() >= 250);
    const auto* info = registry.getInfo(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QVERIFY(info != nullptr);
    QCOMPARE(info->defaultKey, QString("Space"));

    const auto* infoStr = registry.getInfo(QStringLiteral("CMD_PLAY_PAUSE"));
    QVERIFY(infoStr != nullptr);
    QCOMPARE(infoStr->id, outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QCOMPARE(infoStr->name, QString("Play/Pause Playback"));

    QVERIFY(registry.getInfo(QStringLiteral("CMD_NONEXISTENT_XYZ")) == nullptr);
}

void TestCommandRegistry::testExecuteCommand() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    QSignalSpy spy(&registry, &outplayer::core::CommandRegistry::commandExecuted);

    bool executed = false;
    QVariantMap receivedParams;
    registry.registerHandler(outplayer::core::CommandId::CMD_PLAY_PAUSE, [&](const QVariantMap& params) {
        executed = true;
        receivedParams = params;
    });

    QVariantMap params;
    params["source"] = "test";
    QVERIFY(registry.execute(outplayer::core::CommandId::CMD_PLAY_PAUSE, params));
    QVERIFY(executed);
    QCOMPARE(receivedParams.value("source").toString(), QString("test"));
    QCOMPARE(spy.count(), 1);
}

void TestCommandRegistry::testExecuteCommandWithString() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    bool executed = false;
    registry.registerHandler(outplayer::core::CommandId::CMD_SEEK_FORWARD_SHORT, [&](const QVariantMap&) {
        executed = true;
    });

    QVERIFY(registry.execute(QStringLiteral("CMD_SEEK_FORWARD_SHORT")));
    QVERIFY(executed);
}

void TestCommandRegistry::testExecuteUnknownCommand() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    QVERIFY(!registry.execute("CMD_NONEXISTENT_XYZ"));
    QVERIFY(!registry.execute(outplayer::core::CommandId::CMD_PLAY_STOP));
}

QTEST_MAIN(TestCommandRegistry)
#include "test_command_registry.moc"
