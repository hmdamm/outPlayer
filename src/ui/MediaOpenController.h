// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QStringList>

class QWidget;

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::ui {

class MediaOpenController : public QObject {
    Q_OBJECT

public:
    explicit MediaOpenController(QObject* parent = nullptr);
    explicit MediaOpenController(engine::IPlaybackEngine* engine, QObject* parent = nullptr);
    ~MediaOpenController() override = default;

    static bool isMediaFile(const QString& filePath);
    static QStringList supportedMediaFilter();
    static QString supportedMediaFilterString();
    static QStringList scanDirectory(const QString& dirPath, bool recursive = true);

    QStringList promptOpenFiles(QWidget* parent = nullptr);
    QString promptOpenFolder(QWidget* parent = nullptr);
    QString promptOpenUrl(QWidget* parent = nullptr);

    void setPlaybackEngine(engine::IPlaybackEngine* engine) noexcept;
    [[nodiscard]] engine::IPlaybackEngine* playbackEngine() const noexcept;

private:
    engine::IPlaybackEngine* m_engine{nullptr};
};

} // namespace outplayer::ui

namespace outplayer {
using ui::MediaOpenController;
}
