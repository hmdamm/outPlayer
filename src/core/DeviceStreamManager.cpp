// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/DeviceStreamManager.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStringList>

namespace outplayer::core {

DeviceStreamManager::DeviceStreamManager(QObject* parent)
    : QObject(parent)
{
}

QList<DeviceInfo> DeviceStreamManager::availableVideoDevices(const QString& devRoot,
                                                             const QString& sysfsClassRoot) const
{
    QList<DeviceInfo> devices;
    QDir devDir(devRoot);
    if (!devDir.exists()) {
        return devices;
    }

    QStringList filters;
    filters << QStringLiteral("video*");
    const QFileInfoList entries = devDir.entryInfoList(
        filters,
        QDir::Files | QDir::System | QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::Name
    );

    for (const auto& entry : entries) {
        const QString fileName = entry.fileName();
        const QString devPath = entry.filePath();
        QString devName;

        // Try reading sysfs name e.g. /sys/class/video4linux/video0/name
        const QString sysfsNamePath = QDir(sysfsClassRoot).filePath(fileName + QStringLiteral("/name"));
        QFile nameFile(sysfsNamePath);
        if (nameFile.exists() && nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            devName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        if (devName.isEmpty()) {
            devName = QStringLiteral("Video Device %1").arg(devPath);
        }

        DeviceInfo info;
        info.name = devName;
        info.path = devPath;
        info.type = QStringLiteral("v4l2");
        devices.append(info);
    }

    return devices;
}

QList<DeviceInfo> DeviceStreamManager::availableOpticalDrives(const QString& devRoot,
                                                              const QString& sysfsBlockRoot) const
{
    QList<DeviceInfo> drives;
    QDir devDir(devRoot);
    if (!devDir.exists()) {
        return drives;
    }

    QStringList filters;
    filters << QStringLiteral("cdrom*") << QStringLiteral("dvd*") << QStringLiteral("sr*");
    const QFileInfoList entries = devDir.entryInfoList(
        filters,
        QDir::Files | QDir::System | QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::Name
    );

    for (const auto& entry : entries) {
        const QString fileName = entry.fileName();
        const QString devPath = entry.filePath();
        QString driveName;

        // Try reading sysfs block model e.g. /sys/class/block/sr0/device/model
        const QString sysfsModelPath = QDir(sysfsBlockRoot).filePath(fileName + QStringLiteral("/device/model"));
        QFile modelFile(sysfsModelPath);
        if (modelFile.exists() && modelFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString model = QString::fromUtf8(modelFile.readAll()).trimmed();
            if (!model.isEmpty()) {
                driveName = QStringLiteral("%1 (%2)").arg(model, devPath);
            }
            modelFile.close();
        }

        if (driveName.isEmpty()) {
            if (fileName.contains(QStringLiteral("cdrom"), Qt::CaseInsensitive)) {
                driveName = QStringLiteral("CD-ROM Drive (%1)").arg(devPath);
            } else if (fileName.contains(QStringLiteral("dvd"), Qt::CaseInsensitive)) {
                driveName = QStringLiteral("DVD Drive (%1)").arg(devPath);
            } else {
                driveName = QStringLiteral("Optical Drive (%1)").arg(devPath);
            }
        }

        DeviceInfo info;
        info.name = driveName;
        info.path = devPath;
        info.type = QStringLiteral("optical");
        drives.append(info);
    }

    return drives;
}

bool DeviceStreamManager::isStreamUrl(const QString& url)
{
    const QString trimmed = url.trimmed().toLower();
    if (trimmed.isEmpty()) {
        return false;
    }

    static const QStringList prefixes = {
        QStringLiteral("http://"),
        QStringLiteral("https://"),
        QStringLiteral("rtsp://"),
        QStringLiteral("rtmp://"),
        QStringLiteral("mms://"),
        QStringLiteral("hls://"),
        QStringLiteral("dash://"),
        QStringLiteral("dvdnav://"),
        QStringLiteral("dvd://"),
        QStringLiteral("bd://"),
        QStringLiteral("bluray://"),
        QStringLiteral("v4l2://"),
        QStringLiteral("avdevice://"),
        QStringLiteral("ytdl://")
    };

    for (const auto& prefix : prefixes) {
        if (trimmed.startsWith(prefix)) {
            return true;
        }
    }
    return false;
}

QString DeviceStreamManager::formatDvdUrl(const QString& deviceOrFolder)
{
    if (deviceOrFolder.trimmed().isEmpty()) {
        return QStringLiteral("dvdnav://");
    }
    if (deviceOrFolder.startsWith(QStringLiteral("dvdnav://"), Qt::CaseInsensitive)) {
        return deviceOrFolder;
    }
    return QStringLiteral("dvdnav://") + deviceOrFolder;
}

QString DeviceStreamManager::formatBlurayUrl(const QString& deviceOrFolder)
{
    if (deviceOrFolder.trimmed().isEmpty()) {
        return QStringLiteral("bd://");
    }
    if (deviceOrFolder.startsWith(QStringLiteral("bd://"), Qt::CaseInsensitive)) {
        return deviceOrFolder;
    }
    return QStringLiteral("bd://") + deviceOrFolder;
}

QString DeviceStreamManager::formatV4l2Url(const QString& deviceNode)
{
    if (deviceNode.trimmed().isEmpty()) {
        return QStringLiteral("avdevice://v4l2");
    }
    if (deviceNode.startsWith(QStringLiteral("avdevice://"), Qt::CaseInsensitive) ||
        deviceNode.startsWith(QStringLiteral("v4l2://"), Qt::CaseInsensitive)) {
        return deviceNode;
    }
    return QStringLiteral("avdevice://v4l2:") + deviceNode;
}

} // namespace outplayer::core
