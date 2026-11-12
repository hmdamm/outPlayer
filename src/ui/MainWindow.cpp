// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MainWindow.h"

#include <QCursor>
#include <QEvent>
#include <QFileInfo>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>

#include <algorithm>

#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "input/InputManager.h"
#include "ui/ControlBar.h"
#include "ui/CustomTitleBar.h"
#include "ui/OsdOverlay.h"
#include "ui/TimeFormatter.h"
#include "ui/VideoWidget.h"

namespace outplayer::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_engine(std::make_unique<engine::MpvEngine>())
{
    m_engine->initialize();

    const bool useSystemDeco = core::SettingsManager::instance().useSystemWindowDecorations();
    if (!useSystemDeco) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    }

    setMouseTracking(true);
    resize(1280, 720);
    QMainWindow::setWindowTitle(QStringLiteral("outPlayer"));

    // Center video presentation canvas
    m_videoWidget = new VideoWidget(m_engine.get(), this);
    m_videoWidget->setMouseTracking(true);
    setCentralWidget(m_videoWidget);

    // OSD overlay layer atop the video canvas
    m_osdOverlay = new OsdOverlay(m_videoWidget);

    // Custom 30px TitleBar at top
    m_titleBar = new CustomTitleBar(this);
    m_titleBar->setTitle(windowTitle());
    if (useSystemDeco) {
        m_titleBar->hide();
    } else {
        m_titleBar->show();
    }
    connect(m_titleBar, &CustomTitleBar::minimizeClicked, this, &MainWindow::showMinimized);
    connect(m_titleBar, &CustomTitleBar::maximizeRestoreClicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
            m_titleBar->setMaximizedState(false);
        } else {
            showMaximized();
            m_titleBar->setMaximizedState(true);
        }
        updateChromeLayout();
    });
    connect(m_titleBar, &CustomTitleBar::closeClicked, this, &MainWindow::close);

    // 40px ControlBar at bottom
    m_controlBar = new ControlBar(this);
    m_controlBar->setPlaybackEngine(m_engine.get());
    m_controlBar->show();

    // Input manager for keyboard shortcuts and mouse mapping
    m_inputManager = std::make_unique<input::InputManager>(this);
    m_inputManager->attachToWindow(this);
    m_inputManager->attachMouseFilter(m_videoWidget);

    // Event filter for mouse motion, borders, and chrome tracking
    m_videoWidget->installEventFilter(this);
    m_titleBar->installEventFilter(this);
    m_controlBar->installEventFilter(this);
    installEventFilter(this);

    // 2.5s mouse inactivity timer
    m_autohideTimer = new QTimer(this);
    m_autohideTimer->setSingleShot(true);
    m_autohideTimer->setInterval(2500);
    connect(m_autohideTimer, &QTimer::timeout, this, &MainWindow::onAutohideTimeout);

    setupEngineConnections();
    setupCommandHandlers();

    updateChromeLayout();
}

MainWindow::~MainWindow()
{
    // Unregister registered command handlers to avoid test/instance dangling pointers
    for (core::CommandId id : m_registeredCommands) {
        core::CommandRegistry::instance().unregisterHandler(id);
    }
    m_registeredCommands.clear();

    m_inputManager.reset();

    // Explicitly delete video widget and osd overlay before engine is destroyed
    // to guarantee mpv_render_context_free() happens before mpv_terminate_destroy().
    if (m_videoWidget) {
        delete m_videoWidget;
        m_videoWidget = nullptr;
        m_osdOverlay = nullptr;
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

CustomTitleBar* MainWindow::titleBar() const noexcept
{
    return m_titleBar;
}

ControlBar* MainWindow::controlBar() const noexcept
{
    return m_controlBar;
}

OsdOverlay* MainWindow::osdOverlay() const noexcept
{
    return m_osdOverlay;
}

input::InputManager* MainWindow::inputManager() const noexcept
{
    return m_inputManager.get();
}

bool MainWindow::isChromeVisible() const noexcept
{
    return m_chromeVisible;
}

void MainWindow::setChromeVisible(bool visible)
{
    m_chromeVisible = visible;
    if (m_chromeVisible) {
        if (!isFullScreen() && !core::SettingsManager::instance().useSystemWindowDecorations()) {
            if (m_titleBar) {
                m_titleBar->show();
                m_titleBar->raise();
            }
        } else {
            if (m_titleBar) {
                m_titleBar->hide();
            }
        }

        if (m_controlBar) {
            m_controlBar->show();
            m_controlBar->raise();
        }

        unsetCursor();
        if (m_videoWidget) {
            m_videoWidget->unsetCursor();
        }
    } else {
        if (m_titleBar) {
            m_titleBar->hide();
        }
        if (m_controlBar) {
            m_controlBar->hide();
        }

        setCursor(Qt::BlankCursor);
        if (m_videoWidget) {
            m_videoWidget->setCursor(Qt::BlankCursor);
        }
    }
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
        setChromeVisible(true);
    } else {
        setChromeVisible(false);
        showFullScreen();
    }
}

bool MainWindow::openMedia(const QString& pathOrUrl)
{
    if (!m_engine) {
        return false;
    }
    if (m_videoWidget) {
        m_videoWidget->ensureInitialized();
    }
    const bool loaded = m_engine->loadFile(pathOrUrl);
    if (loaded) {
        QString fileName = QFileInfo(pathOrUrl).fileName();
        if (fileName.isEmpty()) {
            fileName = pathOrUrl;
        }
        setWindowTitle(fileName + QStringLiteral(" - outPlayer"));

        if (!m_engine->isPaused()) {
            m_autohideTimer->start(2500);
        }
    }
    return loaded;
}

void MainWindow::setWindowTitle(const QString& title)
{
    QMainWindow::setWindowTitle(title);
    if (m_titleBar) {
        m_titleBar->setTitle(title);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateChromeLayout();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (m_titleBar) {
            m_titleBar->setMaximizedState(isMaximized());
            if (isFullScreen()) {
                m_titleBar->hide();
            }
        }
        updateChromeLayout();
    } else if (event->type() == QEvent::WindowTitleChange) {
        if (m_titleBar) {
            m_titleBar->setTitle(windowTitle());
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::updateChromeLayout()
{
    const int w = width();
    const int h = height();

    if (m_titleBar) {
        m_titleBar->setGeometry(0, 0, w, 30);
        if (m_chromeVisible && !isFullScreen() && !core::SettingsManager::instance().useSystemWindowDecorations()) {
            m_titleBar->show();
            m_titleBar->raise();
        } else {
            m_titleBar->hide();
        }
    }

    if (m_controlBar) {
        m_controlBar->setGeometry(0, h - 40, w, 40);
        if (m_chromeVisible) {
            m_controlBar->show();
            m_controlBar->raise();
        } else {
            m_controlBar->hide();
        }
    }
}

void MainWindow::setupEngineConnections()
{
    if (!m_engine) {
        return;
    }

    connect(m_engine.get(), &engine::IPlaybackEngine::stateChanged, this, [this](bool paused) {
        if (paused) {
            m_autohideTimer->stop();
            setChromeVisible(true);
        } else {
            setChromeVisible(true);
            m_autohideTimer->start(2500);
        }
    });

    connect(m_engine.get(), &engine::IPlaybackEngine::playbackFinished, this, [this]() {
        m_autohideTimer->stop();
        setChromeVisible(true);
    });

    connect(m_engine.get(), &engine::IPlaybackEngine::fileLoaded, this, [this]() {
        if (!m_engine->isPaused()) {
            m_autohideTimer->start(2500);
        }
    });
}

void MainWindow::setupCommandHandlers()
{
    registerCommand(core::CommandId::CMD_PLAY_PAUSE, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->togglePause();
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    m_engine->isPaused() ? QStringLiteral("Pause") : QStringLiteral("Play"),
                    1500, OsdPosition::Center);
            }
        }
    });

    registerCommand(core::CommandId::CMD_PLAY_STOP, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->stop();
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(QStringLiteral("Stop"), 1500, OsdPosition::Center);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VOLUME_UP, [this](const QVariantMap&) {
        if (m_engine) {
            const double newVol = std::min(100.0, m_engine->volume() + 5.0);
            m_engine->setVolume(newVol);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Volume: %1%").arg(static_cast<int>(newVol)),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VOLUME_DOWN, [this](const QVariantMap&) {
        if (m_engine) {
            const double newVol = std::max(0.0, m_engine->volume() - 5.0);
            m_engine->setVolume(newVol);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Volume: %1%").arg(static_cast<int>(newVol)),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VOLUME_MUTE, [this](const QVariantMap&) {
        if (m_engine) {
            const bool muted = !m_engine->isMuted();
            m_engine->setMuted(muted);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    muted ? QStringLiteral("Mute: On") : QStringLiteral("Mute: Off"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    auto seekRel = [this](double delta, const QString& label) {
        if (m_engine) {
            const double dur = std::max(0.0, m_engine->duration());
            const double target = std::clamp(m_engine->position() + delta, 0.0, dur > 0.0 ? dur : 86400.0);
            m_engine->seek(target);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("%1 (%2)").arg(label, TimeFormatter::formatTime(target)),
                    1500, OsdPosition::Center);
            }
        }
    };

    registerCommand(core::CommandId::CMD_SEEK_FORWARD_SHORT, [seekRel](const QVariantMap&) {
        seekRel(5.0, QStringLiteral("Seek: +5s"));
    });
    registerCommand(core::CommandId::CMD_SEEK_BACKWARD_SHORT, [seekRel](const QVariantMap&) {
        seekRel(-5.0, QStringLiteral("Seek: -5s"));
    });
    registerCommand(core::CommandId::CMD_SEEK_FORWARD_MEDIUM, [seekRel](const QVariantMap&) {
        seekRel(30.0, QStringLiteral("Seek: +30s"));
    });
    registerCommand(core::CommandId::CMD_SEEK_BACKWARD_MEDIUM, [seekRel](const QVariantMap&) {
        seekRel(-30.0, QStringLiteral("Seek: -30s"));
    });
    registerCommand(core::CommandId::CMD_SEEK_FORWARD_1MIN, [seekRel](const QVariantMap&) {
        seekRel(60.0, QStringLiteral("Seek: +1m"));
    });
    registerCommand(core::CommandId::CMD_SEEK_BACKWARD_1MIN, [seekRel](const QVariantMap&) {
        seekRel(-60.0, QStringLiteral("Seek: -1m"));
    });
    registerCommand(core::CommandId::CMD_SEEK_FORWARD_5MIN, [seekRel](const QVariantMap&) {
        seekRel(300.0, QStringLiteral("Seek: +5m"));
    });
    registerCommand(core::CommandId::CMD_SEEK_BACKWARD_5MIN, [seekRel](const QVariantMap&) {
        seekRel(-300.0, QStringLiteral("Seek: -5m"));
    });

    registerCommand(core::CommandId::CMD_WINDOW_FULLSCREEN, [this](const QVariantMap&) {
        toggleFullScreen();
    });

    registerCommand(core::CommandId::CMD_WINDOW_ESCAPE, [this](const QVariantMap&) {
        if (isFullScreen()) {
            toggleFullScreen();
        } else {
            showMinimized();
        }
    });

    registerCommand(core::CommandId::CMD_CLOSE_PLAYBACK, [this](const QVariantMap&) {
        close();
    });
}

void MainWindow::registerCommand(core::CommandId id, core::CommandRegistry::Handler handler)
{
    core::CommandRegistry::instance().registerHandler(id, std::move(handler));
    m_registeredCommands.push_back(id);
}

void MainWindow::handleMouseMove(QMouseEvent* event, QObject* /*watched*/)
{
    if (!m_chromeVisible) {
        setChromeVisible(true);
    }

    if (m_engine && !m_engine->isPaused()) {
        m_autohideTimer->start(2500);
    } else {
        m_autohideTimer->stop();
    }

    // Frameless edge hover cursor and resizing
    if (!core::SettingsManager::instance().useSystemWindowDecorations() && !isMaximized() && !isFullScreen()) {
        const QPoint globalPos = event->globalPosition().toPoint();
        const QPoint localPos = mapFromGlobal(globalPos);

        if (m_isResizing) {
            QRect geo = m_resizeStartGeometry;
            const QPoint delta = globalPos - m_resizeStartGlobalPos.toPoint();
            if (m_resizeEdges.testFlag(Qt::LeftEdge)) {
                const int newW = geo.width() - delta.x();
                if (newW >= minimumWidth()) {
                    geo.setLeft(geo.left() + delta.x());
                }
            }
            if (m_resizeEdges.testFlag(Qt::RightEdge)) {
                const int newW = geo.width() + delta.x();
                if (newW >= minimumWidth()) {
                    geo.setRight(geo.left() + newW);
                }
            }
            if (m_resizeEdges.testFlag(Qt::TopEdge)) {
                const int newH = geo.height() - delta.y();
                if (newH >= minimumHeight()) {
                    geo.setTop(geo.top() + delta.y());
                }
            }
            if (m_resizeEdges.testFlag(Qt::BottomEdge)) {
                const int newH = geo.height() + delta.y();
                if (newH >= minimumHeight()) {
                    geo.setBottom(geo.top() + newH);
                }
            }
            setGeometry(geo);
            return;
        }

        const int margin = 6;
        Qt::Edges edges;
        if (localPos.x() < margin) edges |= Qt::LeftEdge;
        else if (localPos.x() >= width() - margin) edges |= Qt::RightEdge;
        if (localPos.y() < margin) edges |= Qt::TopEdge;
        else if (localPos.y() >= height() - margin) edges |= Qt::BottomEdge;

        if (edges != 0) {
            if ((edges.testFlag(Qt::LeftEdge) && edges.testFlag(Qt::TopEdge)) ||
                (edges.testFlag(Qt::RightEdge) && edges.testFlag(Qt::BottomEdge))) {
                setCursor(Qt::SizeFDiagCursor);
            } else if ((edges.testFlag(Qt::RightEdge) && edges.testFlag(Qt::TopEdge)) ||
                       (edges.testFlag(Qt::LeftEdge) && edges.testFlag(Qt::BottomEdge))) {
                setCursor(Qt::SizeBDiagCursor);
            } else if (edges.testFlag(Qt::LeftEdge) || edges.testFlag(Qt::RightEdge)) {
                setCursor(Qt::SizeHorCursor);
            } else if (edges.testFlag(Qt::TopEdge) || edges.testFlag(Qt::BottomEdge)) {
                setCursor(Qt::SizeVerCursor);
            }
        } else if (cursor().shape() != Qt::ArrowCursor && m_chromeVisible) {
            unsetCursor();
        }
    }
}

bool MainWindow::handleMousePress(QMouseEvent* event, QObject* /*watched*/)
{
    if (event->button() == Qt::LeftButton &&
        !core::SettingsManager::instance().useSystemWindowDecorations() &&
        !isMaximized() && !isFullScreen()) {
        const QPoint globalPos = event->globalPosition().toPoint();
        const QPoint localPos = mapFromGlobal(globalPos);

        const int margin = 6;
        Qt::Edges edges;
        if (localPos.x() < margin) edges |= Qt::LeftEdge;
        else if (localPos.x() >= width() - margin) edges |= Qt::RightEdge;
        if (localPos.y() < margin) edges |= Qt::TopEdge;
        else if (localPos.y() >= height() - margin) edges |= Qt::BottomEdge;

        if (edges != 0) {
            m_isResizing = true;
            m_resizeEdges = edges;
            m_resizeStartGlobalPos = event->globalPosition();
            m_resizeStartGeometry = geometry();
            if (windowHandle()) {
                windowHandle()->startSystemResize(edges);
            }
            return true;
        }
    }
    return false;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_videoWidget || watched == this || watched == m_titleBar || watched == m_controlBar) {
        if (event->type() == QEvent::MouseMove) {
            handleMouseMove(static_cast<QMouseEvent*>(event), watched);
        } else if (event->type() == QEvent::MouseButtonPress) {
            if (handleMousePress(static_cast<QMouseEvent*>(event), watched)) {
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_isResizing) {
                m_isResizing = false;
                m_resizeEdges = {};
            }
        } else if (event->type() == QEvent::Leave) {
            if (m_chromeVisible && cursor().shape() != Qt::ArrowCursor) {
                unsetCursor();
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onAutohideTimeout()
{
    if (!m_engine || m_engine->isPaused()) {
        return;
    }

    // Check if mouse is hovering over titlebar or controlbar
    if (m_titleBar && m_titleBar->isVisible()) {
        const QPoint localPos = m_titleBar->mapFromGlobal(QCursor::pos());
        if (m_titleBar->rect().contains(localPos) && m_titleBar->underMouse()) {
            m_autohideTimer->start(2500);
            return;
        }
    }
    if (m_controlBar && m_controlBar->isVisible()) {
        const QPoint localPos = m_controlBar->mapFromGlobal(QCursor::pos());
        if (m_controlBar->rect().contains(localPos) && m_controlBar->underMouse()) {
            m_autohideTimer->start(2500);
            return;
        }
    }

    setChromeVisible(false);
}

} // namespace outplayer::ui
