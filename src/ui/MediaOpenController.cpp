// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MediaOpenController.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QSet>

#include "engine/IPlaybackEngine.h"
#include "ui/OpenUrlDialog.h"

namespace outplayer::ui {

namespace {

const QSet<QString>& videoExtensions()
{
    static const QSet<QString> s_video = {
        QStringLiteral("mp4"),  QStringLiteral("mkv"),  QStringLiteral("avi"),
        QStringLiteral("webm"), QStringLiteral("mov"),  QStringLiteral("wmv"),
        QStringLiteral("flv"),  QStringLiteral("ts"),   QStringLiteral("m2ts"),
        QStringLiteral("vob"),  QStringLiteral("ogv"),  QStringLiteral("m4v"),
        QStringLiteral("mka"),  QStringLiteral("mks"),  QStringLiteral("mk3d"),
        QStringLiteral("qt"),   QStringLiteral("divx"), QStringLiteral("mts"),
        QStringLiteral("trp"),  QStringLiteral("f4v"),  QStringLiteral("asf"),
        QStringLiteral("mpg"),  QStringLiteral("mpeg"), QStringLiteral("evo"),
        QStringLiteral("m2v"),  QStringLiteral("rm"),   QStringLiteral("rmvb"),
        QStringLiteral("3gp"),  QStringLiteral("3g2")
    };
    return s_video;
}

const QSet<QString>& audioExtensions()
{
    static const QSet<QString> s_audio = {
        QStringLiteral("mp3"),  QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("ogg"),  QStringLiteral("m4a"),  QStringLiteral("aac"),
        QStringLiteral("opus"), QStringLiteral("wma"),  QStringLiteral("aiff"),
        QStringLiteral("ape"),  QStringLiteral("wv"),   QStringLiteral("ra"),
        QStringLiteral("alac"), QStringLiteral("oga"),  QStringLiteral("ogx"),
        QStringLiteral("ac3"),  QStringLiteral("dts"),  QStringLiteral("mid")
    };
    return s_audio;
}

const QSet<QString>& playlistExtensions()
{
    static const QSet<QString> s_playlist = {
        QStringLiteral("m3u"), QStringLiteral("m3u8"), QStringLiteral("pls")
    };
    return s_playlist;
}

const QSet<QString>& allMediaExtensions()
{
    static const QSet<QString> s_all = []() {
        QSet<QString> combined = videoExtensions();
        combined.unite(audioExtensions());
        combined.unite(playlistExtensions());
        return combined;
    }();
    return s_all;
}

} // namespace

MediaOpenController::MediaOpenController(QObject* parent)
    : QObject(parent)
{
}

MediaOpenController::MediaOpenController(engine::IPlaybackEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

bool MediaOpenController::isMediaFile(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    const QFileInfo fileInfo(filePath);
    if (fileInfo.isDir()) {
        return false;
    }

    const QString ext = fileInfo.suffix().toLower();
    if (ext.isEmpty()) {
        return false;
    }

    return allMediaExtensions().contains(ext);
}

QStringList MediaOpenController::supportedMediaFilter()
{
    static const QStringList s_filters = []() {
        QStringList allWildcards;
        for (const QString& ext : allMediaExtensions()) {
            allWildcards.append(QStringLiteral("*.%1").arg(ext));
        }

        QStringList videoWildcards;
        for (const QString& ext : videoExtensions()) {
            videoWildcards.append(QStringLiteral("*.%1").arg(ext));
        }

        QStringList audioWildcards;
        for (const QString& ext : audioExtensions()) {
            audioWildcards.append(QStringLiteral("*.%1").arg(ext));
        }

        QStringList playlistWildcards;
        for (const QString& ext : playlistExtensions()) {
            playlistWildcards.append(QStringLiteral("*.%1").arg(ext));
        }

        return QStringList{
            QStringLiteral("All Supported Media (%1)").arg(allWildcards.join(QLatin1Char(' '))),
            QStringLiteral("Video Files (%1)").arg(videoWildcards.join(QLatin1Char(' '))),
            QStringLiteral("Audio Files (%1)").arg(audioWildcards.join(QLatin1Char(' '))),
            QStringLiteral("Playlist Files (%1)").arg(playlistWildcards.join(QLatin1Char(' '))),
            QStringLiteral("All Files (*.*)")
        };
    }();

    return s_filters;
}

QString MediaOpenController::supportedMediaFilterString()
{
    return supportedMediaFilter().join(QStringLiteral(";;"));
}

QStringList MediaOpenController::scanDirectory(const QString& dirPath, bool recursive)
{
    if (dirPath.isEmpty()) {
        return {};
    }

    const QDir dir(dirPath);
    if (!dir.exists()) {
        return {};
    }

    QStringList mediaFiles;
    const QDirIterator::IteratorFlags flags = recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(dirPath, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, flags);

    while (it.hasNext()) {
        const QString filePath = it.next();
        if (isMediaFile(filePath)) {
            mediaFiles.append(QFileInfo(filePath).absoluteFilePath());
        }
    }

    mediaFiles.sort(Qt::CaseInsensitive);
    return mediaFiles;
}

QStringList MediaOpenController::promptOpenFiles(QWidget* parent)
{
    return QFileDialog::getOpenFileNames(
        parent,
        tr("Open Media Files"),
        QString(),
        supportedMediaFilterString()
    );
}

QString MediaOpenController::promptOpenFolder(QWidget* parent)
{
    return QFileDialog::getExistingDirectory(
        parent,
        tr("Open Folder"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
}

QString MediaOpenController::promptOpenUrl(QWidget* parent)
{
    OpenUrlDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.url();
    }
    return {};
}

void MediaOpenController::setPlaybackEngine(engine::IPlaybackEngine* engine) noexcept
{
    m_engine = engine;
}

engine::IPlaybackEngine* MediaOpenController::playbackEngine() const noexcept
{
    return m_engine;
}

} // namespace outplayer::ui
