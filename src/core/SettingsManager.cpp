// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>

namespace outplayer::core {

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
{
    initDefaults();
}

SettingsManager::~SettingsManager() {
    if (m_settings) {
        m_settings->sync();
    }
}

void SettingsManager::initDefaults() {
    m_defaultSettings = {
        {QStringLiteral("General/AllowMultipleInstances"), false},
        {QStringLiteral("General/UseSystemWindowDecorations"), false},
        {QStringLiteral("Playback/Volume"), 100.0},
        {QStringLiteral("Video/HardwareDecoder"), QStringLiteral("auto-safe")},
        {QStringLiteral("Audio/Channels"), QStringLiteral("stereo")},
        {QStringLiteral("Subtitles/Font"), QStringLiteral("sans-serif")},
        {QStringLiteral("Video/Deinterlace"), QStringLiteral("auto")}
    };
}

QString SettingsManager::defaultConfigurationPath() {
    const QString xdgConfig = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (!xdgConfig.isEmpty()) {
        return QDir(xdgConfig).filePath(QStringLiteral("outplayer/outplayer.ini"));
    }

    const QString appConfig = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!appConfig.isEmpty()) {
        return QDir(appConfig).filePath(QStringLiteral("outplayer.ini"));
    }

    return QDir::home().filePath(QStringLiteral(".config/outplayer/outplayer.ini"));
}

void SettingsManager::ensureInitialized() const {
    if (!m_settings) {
        const_cast<SettingsManager*>(this)->init();
    }
}

void SettingsManager::init(const QString& customConfigPath) {
    QString configPath = customConfigPath;
    if (configPath.isEmpty()) {
        configPath = defaultConfigurationPath();
    }

    QFileInfo fileInfo(configPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    m_settings = std::make_unique<QSettings>(configPath, QSettings::IniFormat);
}

QVariant SettingsManager::get(const QString& key, const QVariant& defaultValue) const {
    ensureInitialized();
    if (m_settings->contains(key)) {
        return m_settings->value(key);
    }
    if (defaultValue.isValid()) {
        return defaultValue;
    }
    const auto it = m_defaultSettings.constFind(key);
    if (it != m_defaultSettings.constEnd()) {
        return it.value();
    }
    return QVariant();
}

void SettingsManager::set(const QString& key, const QVariant& value) {
    ensureInitialized();
    m_settings->setValue(key, value);
    emit settingChanged(key, value);
}

bool SettingsManager::contains(const QString& key) const {
    ensureInitialized();
    return m_settings->contains(key);
}

void SettingsManager::remove(const QString& key) {
    ensureInitialized();
    m_settings->remove(key);
}

void SettingsManager::sync() {
    ensureInitialized();
    m_settings->sync();
}

bool SettingsManager::exportToFile(const QString& filePath) const {
    ensureInitialized();
    if (filePath.isEmpty()) {
        return false;
    }

    QFileInfo fi(filePath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    m_settings->sync();

    QSettings exportSettings(filePath, QSettings::IniFormat);
    exportSettings.clear();
    const auto keys = m_settings->allKeys();
    for (const QString& key : keys) {
        exportSettings.setValue(key, m_settings->value(key));
    }
    exportSettings.sync();
    return (exportSettings.status() == QSettings::NoError);
}

bool SettingsManager::importFromFile(const QString& filePath) {
    ensureInitialized();
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        return false;
    }

    QSettings importSettings(filePath, QSettings::IniFormat);
    if (importSettings.status() != QSettings::NoError) {
        return false;
    }

    const auto keys = importSettings.allKeys();
    for (const QString& key : keys) {
        set(key, importSettings.value(key));
    }
    sync();
    return true;
}

void SettingsManager::resetToDefaults() {
    ensureInitialized();
    m_settings->clear();
    for (auto it = m_defaultSettings.constBegin(); it != m_defaultSettings.constEnd(); ++it) {
        set(it.key(), it.value());
    }
    sync();
}

bool SettingsManager::allowMultipleInstances() const {
    return get(QStringLiteral("General/AllowMultipleInstances"), false).toBool();
}

void SettingsManager::setAllowMultipleInstances(bool enable) {
    set(QStringLiteral("General/AllowMultipleInstances"), enable);
}

double SettingsManager::defaultVolume() const {
    return get(QStringLiteral("Playback/Volume"), 100.0).toDouble();
}

void SettingsManager::setDefaultVolume(double vol) {
    set(QStringLiteral("Playback/Volume"), vol);
}

QString SettingsManager::hardwareDecoder() const {
    return get(QStringLiteral("Video/HardwareDecoder"), QStringLiteral("auto-safe")).toString();
}

void SettingsManager::setHardwareDecoder(const QString& hwdec) {
    set(QStringLiteral("Video/HardwareDecoder"), hwdec);
}

bool SettingsManager::useSystemWindowDecorations() const {
    return get(QStringLiteral("General/UseSystemWindowDecorations"), false).toBool();
}

void SettingsManager::setUseSystemWindowDecorations(bool enable) {
    set(QStringLiteral("General/UseSystemWindowDecorations"), enable);
}

QString SettingsManager::configFilePath() const {
    ensureInitialized();
    return m_settings->fileName();
}

} // namespace outplayer::core
