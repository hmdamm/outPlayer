// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/VideoWidget.h"
#include "engine/MpvEngine.h"

#include <QMetaObject>
#include <QOpenGLContext>
#include <QWidget>
#include <QWindow>

#include <mpv/render.h>
#include <mpv/render_gl.h>

namespace outplayer::ui {

namespace {

void* getProcAddress(void* ctx, const char* name)
{
    Q_UNUSED(ctx);
    QOpenGLContext* glCtx = QOpenGLContext::currentContext();
    if (!glCtx) {
        return nullptr;
    }
    return reinterpret_cast<void*>(glCtx->getProcAddress(QByteArray(name)));
}

} // namespace

VideoWidget::VideoWidget(engine::MpvEngine* engine, QWidget* parent)
    : QOpenGLWidget(parent)
    , m_engine(engine)
{
}

VideoWidget::~VideoWidget()
{
    if (m_mpv_gl) {
        if (isValid()) {
            makeCurrent();
        }
        mpv_render_context_set_update_callback(m_mpv_gl, nullptr, nullptr);
        mpv_render_context_free(m_mpv_gl);
        m_mpv_gl = nullptr;
        if (isValid()) {
            doneCurrent();
        }
    }
}

void VideoWidget::initializeGL()
{
    if (m_mpv_gl || !m_engine || !m_engine->mpvHandle()) {
        return;
    }

    mpv_opengl_init_params gl_params{getProcAddress, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    int err = mpv_render_context_create(&m_mpv_gl, m_engine->mpvHandle(), params);
    if (err >= 0 && m_mpv_gl) {
        mpv_render_context_set_update_callback(m_mpv_gl, &VideoWidget::onMpvUpdate, this);
    }
}

void VideoWidget::paintGL()
{
    if (!m_mpv_gl) {
        return;
    }

    GLint fbo = static_cast<GLint>(defaultFramebufferObject());
    int w = static_cast<int>(width() * devicePixelRatioF());
    int h = static_cast<int>(height() * devicePixelRatioF());

    mpv_opengl_fbo mpv_fbo{fbo, w, h, 0};
    int flip_y = 1;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(m_mpv_gl, params);
}

void VideoWidget::onMpvUpdate(void* ctx)
{
    auto* self = static_cast<VideoWidget*>(ctx);
    if (!self) {
        return;
    }
    QMetaObject::invokeMethod(self, [self]() {
        self->maybeUpdate();
    }, Qt::QueuedConnection);
}

void VideoWidget::maybeUpdate()
{
    if (window() && window()->isMinimized()) {
        makeCurrent();
        paintGL();
        if (context() && context()->surface()) {
            context()->swapBuffers(context()->surface());
        }
        doneCurrent();
    } else {
        update();
    }
}

} // namespace outplayer::ui
