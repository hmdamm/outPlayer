// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "core/SettingsManager.h"

class TestSettingsManager : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void testGetAndSet();
    void testSignals();
    void testDefaults();
    void testTypedAccessors();
    void testContainsAndRemove();
    void testExportImport();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

void TestSettingsManager::init() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    QString iniPath = m_tempDir->filePath("test_outplayer.ini");
    outplayer::core::SettingsManager::instance().init(iniPath);
}

void TestSettingsManager::cleanup() {
    m_tempDir.reset();
}

void TestSettingsManager::testGetAndSet() {
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.set("Playback/Volume", 85.0);
    QCOMPARE(settings.get("Playback/Volume").toDouble(), 85.0);
}

void TestSettingsManager::testSignals() {
    auto& settings = outplayer::core::SettingsManager::instance();
    QSignalSpy spy(&settings, &outplayer::core::SettingsManager::settingChanged);
    settings.set("Video/Deinterlace", QString("auto"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("Video/Deinterlace"));

    // Setting identical value should not re-trigger signal
    settings.set("Video/Deinterlace", QString("auto"));
    QCOMPARE(spy.count(), 0);
}

void TestSettingsManager::testDefaults() {
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.resetToDefaults();
    QCOMPARE(settings.hardwareDecoder(), QString("auto-safe"));
}

void TestSettingsManager::testTypedAccessors() {
    auto& settings = outplayer::core::SettingsManager::instance();

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
    auto& settings = outplayer::core::SettingsManager::instance();

    QVERIFY(!settings.contains("Custom/Foo"));
    settings.set("Custom/Foo", "Bar");
    QVERIFY(settings.contains("Custom/Foo"));
    settings.remove("Custom/Foo");
    QVERIFY(!settings.contains("Custom/Foo"));
}

void TestSettingsManager::testExportImport() {
    QString exportPath = m_tempDir->filePath("exported.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.set("General/CustomSetting", 12345);
    QVERIFY(settings.exportToFile(exportPath));
    settings.set("General/CustomSetting", 0);
    QVERIFY(settings.importFromFile(exportPath));
    QCOMPARE(settings.get("General/CustomSetting").toInt(), 12345);
}

QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
