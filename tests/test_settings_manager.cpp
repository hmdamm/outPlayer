// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "core/SettingsManager.h"

class TestSettingsManager : public QObject {
    Q_OBJECT
private slots:
    void testGetAndSet();
    void testSignals();
    void testDefaults();
    void testTypedAccessors();
    void testContainsAndRemove();
    void testExportImport();
};

void TestSettingsManager::testGetAndSet() {
    QTemporaryDir tempDir;
    QString iniPath = tempDir.filePath("test_outplayer.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.init(iniPath);
    settings.set("Playback/Volume", 85.0);
    QCOMPARE(settings.get("Playback/Volume").toDouble(), 85.0);
}

void TestSettingsManager::testSignals() {
    auto& settings = outplayer::core::SettingsManager::instance();
    QSignalSpy spy(&settings, &outplayer::core::SettingsManager::settingChanged);
    settings.set("Video/Deinterlace", QString("auto"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("Video/Deinterlace"));
}

void TestSettingsManager::testDefaults() {
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.resetToDefaults();
    QCOMPARE(settings.hardwareDecoder(), QString("auto-safe"));
}

void TestSettingsManager::testTypedAccessors() {
    QTemporaryDir tempDir;
    QString iniPath = tempDir.filePath("typed_accessors.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.init(iniPath);

    settings.setAllowMultipleInstances(true);
    QCOMPARE(settings.allowMultipleInstances(), true);
    QCOMPARE(settings.get("General/AllowMultipleInstances").toBool(), true);

    settings.setDefaultVolume(75.5);
    QCOMPARE(settings.defaultVolume(), 75.5);
    QCOMPARE(settings.get("Playback/Volume").toDouble(), 75.5);

    settings.setHardwareDecoder("vaapi");
    QCOMPARE(settings.hardwareDecoder(), QString("vaapi"));
    QCOMPARE(settings.get("Video/HardwareDecoder").toString(), QString("vaapi"));

    settings.setUseSystemWindowDecorations(true);
    QCOMPARE(settings.useSystemWindowDecorations(), true);
    QCOMPARE(settings.get("General/UseSystemWindowDecorations").toBool(), true);
}

void TestSettingsManager::testContainsAndRemove() {
    QTemporaryDir tempDir;
    QString iniPath = tempDir.filePath("test_contains.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.init(iniPath);

    QVERIFY(!settings.contains("Custom/Foo"));
    settings.set("Custom/Foo", "Bar");
    QVERIFY(settings.contains("Custom/Foo"));
    settings.remove("Custom/Foo");
    QVERIFY(!settings.contains("Custom/Foo"));
}

void TestSettingsManager::testExportImport() {
    QTemporaryDir tempDir;
    QString exportPath = tempDir.filePath("exported.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.set("General/CustomSetting", 12345);
    QVERIFY(settings.exportToFile(exportPath));
    settings.set("General/CustomSetting", 0);
    QVERIFY(settings.importFromFile(exportPath));
    QCOMPARE(settings.get("General/CustomSetting").toInt(), 12345);
}

QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
