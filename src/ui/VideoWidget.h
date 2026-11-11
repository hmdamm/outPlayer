// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QOpenGLWidget>

struct mpv_render_context;

namespace outplayer::engine {
class MpvEngine;
}

namespace outplayer::ui {

class VideoWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit VideoWidget(engine::MpvEngine* engine, QWidget* parent = nullptr);
    ~VideoWidget() override;

    VideoWidget(const VideoWidget&) = delete;
    VideoWidget& operator=(const VideoWidget&) = delete;
    VideoWidget(VideoWidget&&) = delete;
    VideoWidget& operator=(VideoWidget&&) = delete;

    [[nodiscard]] engine::MpvEngine* engine() const noexcept { return m_engine; }
    [[nodiscard]] mpv_render_context* renderContext() const noexcept { return m_mpv_gl; }

public slots:
    void maybeUpdate();

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    static void onMpvUpdate(void* ctx);

    engine::MpvEngine* m_engine{nullptr};
    mpv_render_context* m_mpv_gl{nullptr};
};

} // namespace outplayer::ui
