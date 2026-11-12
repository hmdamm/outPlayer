// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QList>

namespace outplayer::core {

struct DeviceInfo {
    QString name;
    QString path;
    QString type; // "v4l2", "optical", "network"

    bool operator==(const DeviceInfo&) const = default;
};

class DeviceStreamManager : public QObject {
    Q_OBJECT

public:
    explicit DeviceStreamManager(QObject* parent = nullptr);
    ~DeviceStreamManager() override = default;

    QList<DeviceInfo> availableVideoDevices(const QString& devRoot = QStringLiteral("/dev"),
                                           const QString& sysfsClassRoot = QStringLiteral("/sys/class/video4linux")) const;
    QList<DeviceInfo> availableOpticalDrives(const QString& devRoot = QStringLiteral("/dev"),
                                            const QString& sysfsBlockRoot = QStringLiteral("/sys/class/block")) const;

    static bool isStreamUrl(const QString& url);
    static QString formatDvdUrl(const QString& deviceOrFolder = QString());
    static QString formatBlurayUrl(const QString& deviceOrFolder = QString());
    static QString formatV4l2Url(const QString& deviceNode = QString());
};

} // namespace outplayer::core
