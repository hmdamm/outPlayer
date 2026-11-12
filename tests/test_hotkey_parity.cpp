// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QKeySequence>
#include <QList>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QTextStream>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "input/InputManager.h"

namespace {

struct HotkeyEntry {
    QString id;
    QString functionName;
    QString key;
    QString modifiers;
    QString scope;
    QString menuPath;
    QString commandIdStr;
};

QString findHotkeysCsvPath() {
    const QStringList candidates = {
#ifdef PROJECT_SOURCE_DIR
        QStringLiteral(PROJECT_SOURCE_DIR) + QStringLiteral("/docs/spec/hotkeys.csv"),
#endif
        QStringLiteral("docs/spec/hotkeys.csv"),
        QStringLiteral("../docs/spec/hotkeys.csv"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../docs/spec/hotkeys.csv"),
        QDir::currentPath() + QStringLiteral("/docs/spec/hotkeys.csv")
    };
    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return {};
}

QStringList parseCsvLine(const QString& line) {
    QStringList fields;
    QString current;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.append(current.trimmed());
            current.clear();
        } else {
            current += c;
        }
    }
    fields.append(current.trimmed());
    return fields;
}

QList<HotkeyEntry> loadHotkeysCsv(const QString& path, QString* errorMsg = nullptr) {
    QList<HotkeyEntry> entries;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg) {
            *errorMsg = QStringLiteral("Could not open file: ") + path;
        }
        return entries;
    }

    QTextStream in(&file);
    bool firstLine = true;
    int lineNum = 0;

    while (!in.atEnd()) {
        lineNum++;
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (firstLine) {
            firstLine = false;
            // Verify header
            QStringList header = parseCsvLine(line);
            if (header.size() < 7 || header[0] != QStringLiteral("id") ||
                header[6] != QStringLiteral("command_id")) {
                if (errorMsg) {
                    *errorMsg = QStringLiteral("Invalid CSV header at line ") + QString::number(lineNum);
                }
                return {};
            }
            continue;
        }

        QStringList fields = parseCsvLine(line);
        if (fields.size() != 7) {
            if (errorMsg) {
                *errorMsg = QStringLiteral("Row has ") + QString::number(fields.size()) +
                            QStringLiteral(" columns, expected 7 at line ") + QString::number(lineNum);
            }
            return {};
        }

        entries.append(HotkeyEntry{
            .id = fields[0],
            .functionName = fields[1],
            .key = fields[2],
            .modifiers = fields[3],
            .scope = fields[4],
            .menuPath = fields[5],
            .commandIdStr = fields[6]
        });
    }

    return entries;
}

bool isMouseActionKey(const QString& key) {
    static const QStringList prefixes = {
        QStringLiteral("LeftClick"),
        QStringLiteral("LeftDblClick"),
        QStringLiteral("LeftDrag"),
        QStringLiteral("MiddleClick"),
        QStringLiteral("MiddleDblClick"),
        QStringLiteral("RightClick"),
        QStringLiteral("Wheel"),
        QStringLiteral("XButton")
    };
    for (const auto& prefix : prefixes) {
        if (key.startsWith(prefix)) {
            return true;
        }
    }
    return false;
}

QKeySequence parseShortcutSequence(const QString& key, const QString& modifiers) {
    if (key.isEmpty() || key == QStringLiteral("None") || isMouseActionKey(key)) {
        return {};
    }

    QString keyStr = key;
    if (keyStr == QStringLiteral("Numpad_Add")) {
        keyStr = QStringLiteral("+");
    } else if (keyStr == QStringLiteral("Numpad_Subtract")) {
        keyStr = QStringLiteral("-");
    } else if (keyStr.startsWith(QStringLiteral("Numpad")) && keyStr.length() > 6) {
        keyStr = keyStr.mid(6);
    }

    QString fullSeq;
    if (!modifiers.isEmpty() && modifiers != QStringLiteral("None")) {
        fullSeq = modifiers + QStringLiteral("+") + keyStr;
    } else {
        fullSeq = keyStr;
    }

    return QKeySequence(fullSeq, QKeySequence::PortableText);
}

} // namespace

class TestHotkeyParity : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. Exactly 253 rows parsed from hotkeys.csv
    void testExact253RowsParsed();

    // 2. Every row's command_id resolves to a valid core::CommandId
    void testCommandIdResolution();

    // 3. CommandRegistry::instance().getInfo(cmdId) exists with non-empty label and category
    void testCommandRegistryMetadata();

    // 4. Shortcut strings parse to valid QKeySequence (for non-empty keyboard shortcuts)
    void testShortcutSequences();

    // 5. Every command can be dispatched via execute(cmdId) without crashing
    void testCommandDispatch();

private:
    QString m_csvPath;
    QList<HotkeyEntry> m_entries;
};

void TestHotkeyParity::initTestCase() {
    m_csvPath = findHotkeysCsvPath();
    QVERIFY2(!m_csvPath.isEmpty(), "docs/spec/hotkeys.csv could not be located in candidate paths");

    QString errorMsg;
    m_entries = loadHotkeysCsv(m_csvPath, &errorMsg);
    QVERIFY2(errorMsg.isEmpty(), qPrintable(errorMsg));
}

void TestHotkeyParity::cleanupTestCase() {
    outplayer::core::CommandRegistry::instance().clearHandlers();
}

void TestHotkeyParity::testExact253RowsParsed() {
    // 1. Verify exactly 253 rows are parsed
    QCOMPARE(m_entries.size(), 253);

    // Verify all row IDs are unique and formatted correctly
    QSet<QString> seenIds;
    QSet<QString> seenCommandIds;

    for (const auto& entry : m_entries) {
        QVERIFY2(entry.id.startsWith(QStringLiteral("HK-")),
                 qPrintable(QStringLiteral("Invalid ID format: ") + entry.id));
        QVERIFY2(!seenIds.contains(entry.id),
                 qPrintable(QStringLiteral("Duplicate row ID: ") + entry.id));
        seenIds.insert(entry.id);

        QVERIFY2(!entry.commandIdStr.isEmpty(),
                 qPrintable(QStringLiteral("Empty command_id in row: ") + entry.id));
        QVERIFY2(!seenCommandIds.contains(entry.commandIdStr),
                 qPrintable(QStringLiteral("Duplicate command_id: ") + entry.commandIdStr));
        seenCommandIds.insert(entry.commandIdStr);
    }

    QCOMPARE(seenIds.size(), 253);
    QCOMPARE(seenCommandIds.size(), 253);
}

void TestHotkeyParity::testCommandIdResolution() {
    // 2. Every row's command_id resolves to a valid core::CommandId enum
    for (const auto& entry : m_entries) {
        auto optId = outplayer::core::stringToCommandId(entry.commandIdStr);
        QVERIFY2(optId.has_value(),
                 qPrintable(QStringLiteral("Failed to resolve CommandId for: ") + entry.commandIdStr));

        // Verify round-trip conversion
        QString backToStr = outplayer::core::commandIdToString(*optId);
        QCOMPARE(backToStr, entry.commandIdStr);
    }
}

void TestHotkeyParity::testCommandRegistryMetadata() {
    // 3. CommandRegistry::instance().getInfo(cmdId) exists with non-empty label and category
    auto& registry = outplayer::core::CommandRegistry::instance();

    // The registry should contain at least all 253 commands
    QVERIFY(registry.allCommands().size() >= 253);

    for (const auto& entry : m_entries) {
        auto optId = outplayer::core::stringToCommandId(entry.commandIdStr);
        QVERIFY(optId.has_value());

        // Test retrieval by enum ID
        const auto* infoById = registry.getInfo(*optId);
        QVERIFY2(infoById != nullptr,
                 qPrintable(QStringLiteral("CommandRegistry::getInfo(enum) returned null for: ") + entry.commandIdStr));

        // Test retrieval by string ID
        const auto* infoByStr = registry.getInfo(entry.commandIdStr);
        QVERIFY2(infoByStr != nullptr,
                 qPrintable(QStringLiteral("CommandRegistry::getInfo(string) returned null for: ") + entry.commandIdStr));

        QCOMPARE(infoById->id, *optId);
        QCOMPARE(infoByStr->id, *optId);

        // Verify label (human-readable name) is non-empty
        QVERIFY2(!infoById->name.trimmed().isEmpty(),
                 qPrintable(QStringLiteral("Empty name/label for command: ") + entry.commandIdStr));
        QCOMPARE(infoById->name, entry.functionName);

        // Verify category (menuPath) is non-empty
        QVERIFY2(!infoById->menuPath.trimmed().isEmpty(),
                 qPrintable(QStringLiteral("Empty menuPath/category for command: ") + entry.commandIdStr));
        QCOMPARE(infoById->menuPath, entry.menuPath);

        // Verify defaultKey, defaultModifiers, and scope match spec
        QCOMPARE(infoById->defaultKey, entry.key);
        QCOMPARE(infoById->defaultModifiers, entry.modifiers);
        QCOMPARE(infoById->scope, entry.scope);
    }
}

void TestHotkeyParity::testShortcutSequences() {
    // 4. The shortcut string parses to a valid QKeySequence (for non-empty shortcuts)
    int keyboardCount = 0;
    int mouseCount = 0;

    for (const auto& entry : m_entries) {
        if (isMouseActionKey(entry.key)) {
            mouseCount++;
            continue;
        }

        keyboardCount++;
        QKeySequence seq = parseShortcutSequence(entry.key, entry.modifiers);
        QVERIFY2(!seq.isEmpty(),
                 qPrintable(QString("Failed to parse valid QKeySequence for %1 (key: %2, mod: %3)")
                            .arg(entry.commandIdStr, entry.key, entry.modifiers)));

        // Verify that InputManager also produces a valid shortcut sequence for this command
        auto optId = outplayer::core::stringToCommandId(entry.commandIdStr);
        QVERIFY(optId.has_value());
    }

    // Spec verification: 233 keyboard shortcuts, 20 mouse gesture mappings
    QCOMPARE(keyboardCount, 233);
    QCOMPARE(mouseCount, 20);
    QCOMPARE(keyboardCount + mouseCount, 253);

    // Verify InputManager default shortcut catalog
    outplayer::input::InputManager inputMgr;
    for (const auto& entry : m_entries) {
        if (isMouseActionKey(entry.key)) {
            continue;
        }
        auto optId = outplayer::core::stringToCommandId(entry.commandIdStr);
        QVERIFY(optId.has_value());
        QKeySequence mgrSeq = inputMgr.getShortcut(*optId);
        QVERIFY2(!mgrSeq.isEmpty(),
                 qPrintable(QString("InputManager missing shortcut for %1").arg(entry.commandIdStr)));
    }
}

void TestHotkeyParity::testCommandDispatch() {
    // 5. Every command can be dispatched via CommandRegistry::instance().execute(cmdId) without crashing
    auto& registry = outplayer::core::CommandRegistry::instance();
    QSignalSpy spy(&registry, &outplayer::core::CommandRegistry::commandExecuted);

    for (const auto& entry : m_entries) {
        auto optId = outplayer::core::stringToCommandId(entry.commandIdStr);
        QVERIFY(optId.has_value());
        outplayer::core::CommandId cmdId = *optId;

        // 1. Dispatching with no handler should return false and not crash
        registry.unregisterHandler(cmdId);
        bool noHandlerResult = registry.execute(cmdId);
        QVERIFY(!noHandlerResult);

        // 2. Register a no-op handler
        bool handlerExecuted = false;
        QVariantMap receivedParams;
        bool registered = registry.registerHandler(cmdId, [&](const QVariantMap& params) {
            handlerExecuted = true;
            receivedParams = params;
        });
        QVERIFY2(registered, qPrintable(QString("Failed to register handler for: ") + entry.commandIdStr));

        // 3. Dispatch via enum ID
        int initialSpyCount = spy.count();
        QVariantMap testParams;
        testParams[QStringLiteral("command")] = entry.commandIdStr;
        testParams[QStringLiteral("iteration")] = 42;

        bool executeResult = registry.execute(cmdId, testParams);
        QVERIFY2(executeResult, qPrintable(QString("execute(enum) failed for: ") + entry.commandIdStr));
        QVERIFY(handlerExecuted);
        QCOMPARE(receivedParams.value(QStringLiteral("command")).toString(), entry.commandIdStr);
        QCOMPARE(receivedParams.value(QStringLiteral("iteration")).toInt(), 42);
        QCOMPARE(spy.count(), initialSpyCount + 1);

        // 4. Dispatch via string ID
        handlerExecuted = false;
        receivedParams.clear();
        bool executeStrResult = registry.execute(entry.commandIdStr, testParams);
        QVERIFY2(executeStrResult, qPrintable(QString("execute(string) failed for: ") + entry.commandIdStr));
        QVERIFY(handlerExecuted);
        QCOMPARE(spy.count(), initialSpyCount + 2);

        // Clean up handler
        registry.unregisterHandler(cmdId);
    }
}

QTEST_MAIN(TestHotkeyParity)
#include "test_hotkey_parity.moc"
