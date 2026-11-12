// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/ScreenCaptureController.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QStandardPaths>
#include <QThread>

#include "core/SettingsManager.h"
#include "engine/IPlaybackEngine.h"

namespace outplayer::ui {

namespace {
const QString kCaptureFolderKey = QStringLiteral("capture/folder");
}

ScreenCaptureController::ScreenCaptureController(engine::IPlaybackEngine* engine,
                                                 core::SettingsManager* settings,
                                                 QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_settings(settings ? settings : &core::SettingsManager::instance())
{
    if (m_settings && m_settings->contains(kCaptureFolderKey)) {
        m_captureFolder = m_settings->get(kCaptureFolderKey).toString();
    }
}

QString ScreenCaptureController::captureFolder() const {
    if (m_settings && m_settings->contains(kCaptureFolderKey)) {
        const QString saved = m_settings->get(kCaptureFolderKey).toString();
        if (!saved.isEmpty()) {
            return saved;
        }
    }
    if (!m_captureFolder.isEmpty()) {
        return m_captureFolder;
    }
    const QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (!picturesDir.isEmpty()) {
        return QDir(picturesDir).filePath(QStringLiteral("outPlayer"));
    }
    return QDir::home().filePath(QStringLiteral("Pictures/outPlayer"));
}

void ScreenCaptureController::setCaptureFolder(const QString& path) {
    m_captureFolder = path;
    if (m_settings) {
        m_settings->set(kCaptureFolderKey, path);
    }
}

QString ScreenCaptureController::captureInstantFrame() {
    if (!m_engine) {
        emit captureError(QStringLiteral("Playback engine is not available"));
        return QString();
    }

    const QString folder = captureFolder();
    if (folder.isEmpty()) {
        emit captureError(QStringLiteral("Capture folder is not configured"));
        return QString();
    }

    QDir dir(folder);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            emit captureError(QStringLiteral("Failed to create capture directory: %1").arg(folder));
            return QString();
        }
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
    const QString filename = QStringLiteral("outPlayer_%1.png").arg(timestamp);
    const QString fullPath = dir.filePath(filename);

    if (!m_engine->captureFrame(fullPath)) {
        emit captureError(QStringLiteral("Playback engine failed to capture frame"));
        return QString();
    }

    emit frameCaptured(fullPath);
    return fullPath;
}

bool ScreenCaptureController::copyFrameToClipboard() {
    if (!m_engine) {
        emit captureError(QStringLiteral("Playback engine is not available"));
        return false;
    }

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString tempPath = QDir(tempDir).filePath(
        QStringLiteral("outplayer_clipboard_%1.png").arg(QDateTime::currentMSecsSinceEpoch()));

    if (!m_engine->captureFrame(tempPath)) {
        emit captureError(QStringLiteral("Playback engine failed to capture frame for clipboard"));
        return false;
    }

    if (!QFile::exists(tempPath)) {
        for (int i = 0; i < 5; ++i) {
            QThread::msleep(20);
            if (QFile::exists(tempPath)) {
                break;
            }
        }
    }

    QImage img(tempPath);
    QFile::remove(tempPath);

    if (img.isNull()) {
        emit captureError(QStringLiteral("Failed to load captured frame for clipboard"));
        return false;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        emit captureError(QStringLiteral("Clipboard is not available"));
        return false;
    }

    clipboard->setImage(img);
    emit frameCopiedToClipboard();
    return true;
}

} // namespace outplayer::ui
