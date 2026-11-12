// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::core {
class SettingsManager;
}

namespace outplayer::ui {

class ScreenCaptureController : public QObject {
    Q_OBJECT

public:
    explicit ScreenCaptureController(engine::IPlaybackEngine* engine,
                                     core::SettingsManager* settings = nullptr,
                                     QObject* parent = nullptr);
    ~ScreenCaptureController() override = default;

    [[nodiscard]] QString captureFolder() const;
    void setCaptureFolder(const QString& path);

    QString captureInstantFrame();
    bool copyFrameToClipboard();

signals:
    void frameCaptured(const QString& filePath);
    void frameCopiedToClipboard();
    void captureError(const QString& message);

private:
    engine::IPlaybackEngine* m_engine{nullptr};
    core::SettingsManager* m_settings{nullptr};
    QString m_captureFolder;
};

} // namespace outplayer::ui
