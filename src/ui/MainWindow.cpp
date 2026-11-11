// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MainWindow.h"
#include "ui/VideoWidget.h"
#include "engine/MpvEngine.h"

namespace outplayer::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_engine(std::make_unique<engine::MpvEngine>(this))
{
    m_engine->initialize();

    m_videoWidget = new VideoWidget(m_engine.get(), this);
    setCentralWidget(m_videoWidget);

    resize(1280, 720);
    setWindowTitle(QStringLiteral("outPlayer"));
}

MainWindow::~MainWindow()
{
    // Explicitly delete central widget before engine is reset
    // to guarantee mpv_render_context_free() happens before mpv_terminate_destroy().
    if (m_videoWidget) {
        delete m_videoWidget;
        m_videoWidget = nullptr;
    }
    m_engine.reset();
}

engine::MpvEngine* MainWindow::engine() const noexcept
{
    return m_engine.get();
}

VideoWidget* MainWindow::videoWidget() const noexcept
{
    return m_videoWidget;
}

bool MainWindow::openMedia(const QString& pathOrUrl)
{
    if (!m_engine) {
        return false;
    }
    if (m_videoWidget) {
        m_videoWidget->ensureInitialized();
    }
    return m_engine->loadFile(pathOrUrl);
}

} // namespace outplayer::ui
