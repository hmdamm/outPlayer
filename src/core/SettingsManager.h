// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <memory>

class QSettings;

namespace outplayer::core {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;
    SettingsManager& operator=(SettingsManager&&) = delete;

    void init(const QString& customConfigPath = QString());

    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void set(const QString& key, const QVariant& value);
    bool contains(const QString& key) const;
    void remove(const QString& key);
    void sync();

    bool exportToFile(const QString& filePath) const;
    bool importFromFile(const QString& filePath);
    void resetToDefaults();

    // Strongly-typed accessors for core preferences
    bool allowMultipleInstances() const;
    void setAllowMultipleInstances(bool enable);
    double defaultVolume() const;
    void setDefaultVolume(double vol);
    QString hardwareDecoder() const;
    void setHardwareDecoder(const QString& hwdec);
    bool useSystemWindowDecorations() const;
    void setUseSystemWindowDecorations(bool enable);

    QString configFilePath() const;
    static QString defaultConfigurationPath();

signals:
    void settingChanged(const QString& key, const QVariant& newValue);

private:
    void ensureInitialized() const;
    void initDefaults();

    mutable std::unique_ptr<QSettings> m_settings;
    QMap<QString, QVariant> m_defaultSettings;
};

} // namespace outplayer::core
