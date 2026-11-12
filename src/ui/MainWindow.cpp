// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MainWindow.h"

#include <QContextMenuEvent>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QWindow>

#include <algorithm>

#include "core/BookmarkManager.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "input/InputManager.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"
#include "ui/ContextMenuManager.h"
#include "ui/ControlBar.h"
#include "ui/ControlPanel.h"
#include "ui/CustomTitleBar.h"
#include "ui/MediaOpenController.h"
#include "ui/OsdOverlay.h"
#include "ui/PlaylistDock.h"
#include "ui/ScreenCaptureController.h"
#include "ui/TimeFormatter.h"
#include "ui/VideoWidget.h"

namespace outplayer::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_engine(std::make_unique<engine::MpvEngine>())
    , m_bookmarkManager(std::make_unique<core::BookmarkManager>(&core::SettingsManager::instance(), this))
    , m_captureController(std::make_unique<ScreenCaptureController>(m_engine.get(), &core::SettingsManager::instance(), this))
    , m_contextMenuManager(std::make_unique<ContextMenuManager>(this))
    , m_mediaOpenController(std::make_unique<MediaOpenController>(m_engine.get(), this))
{
    m_engine->initialize();

    const bool useSystemDeco = core::SettingsManager::instance().useSystemWindowDecorations();
    if (!useSystemDeco) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    }

    setAcceptDrops(true);
    setMouseTracking(true);
    resize(1280, 720);
    QMainWindow::setWindowTitle(QStringLiteral("outPlayer"));

    // Central layout containing VideoWidget and PlaylistDock horizontally
    m_centralContainer = new QWidget(this);
    auto* centralLayout = new QHBoxLayout(m_centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // Center video presentation canvas
    m_videoWidget = new VideoWidget(m_engine.get(), m_centralContainer);
    m_videoWidget->setMouseTracking(true);
    m_videoWidget->setAcceptDrops(true);
    centralLayout->addWidget(m_videoWidget, 1);

    // Playlist model and dock
    m_playlistModel = new playlist::PlaylistModel(this);
    m_playlistDock = new PlaylistDock(m_playlistModel, m_centralContainer);
    m_playlistDock->hide(); // hidden by default until toggled
    centralLayout->addWidget(m_playlistDock, 0);

    setCentralWidget(m_centralContainer);

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

    // Control panel (F7)
    m_controlPanel = new ControlPanel(m_engine.get(), this);
    m_controlPanel->hide();
    connect(m_controlPanel, &ControlPanel::closeRequested, this, [this]() {
        setControlPanelVisible(false);
    });

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
    setupNavigationConnections();

    updateChromeLayout();
}

MainWindow::~MainWindow()
{
    // Unregister registered command handlers to avoid test/instance dangling pointers
    for (core::CommandId id : m_registeredCommands) {
        core::CommandRegistry::instance().unregisterHandler(id);
    }
    m_registeredCommands.clear();

    if (m_controlPanel) {
        delete m_controlPanel;
        m_controlPanel = nullptr;
    }

    m_inputManager.reset();
    m_captureController.reset();
    m_bookmarkManager.reset();

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

QWidget* MainWindow::centralWidget() const noexcept
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

PlaylistDock* MainWindow::playlistDock() const noexcept
{
    return m_playlistDock;
}

playlist::PlaylistModel* MainWindow::playlistModel() const noexcept
{
    return m_playlistModel;
}

ContextMenuManager* MainWindow::contextMenuManager() const noexcept
{
    return m_contextMenuManager.get();
}

MediaOpenController* MainWindow::mediaOpenController() const noexcept
{
    return m_mediaOpenController.get();
}

ControlPanel* MainWindow::controlPanel() const noexcept
{
    return m_controlPanel;
}

core::BookmarkManager* MainWindow::bookmarkManager() const noexcept
{
    return m_bookmarkManager.get();
}

ScreenCaptureController* MainWindow::screenCaptureController() const noexcept
{
    return m_captureController.get();
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

void MainWindow::setPlaylistVisible(bool visible)
{
    if (!m_playlistDock) {
        return;
    }
    m_playlistDock->setVisible(visible);
    if (visible) {
        m_playlistDock->raise();
    }
    updateChromeLayout();
}

void MainWindow::togglePlaylist()
{
    if (m_playlistDock) {
        setPlaylistVisible(!m_playlistDock->isVisible());
    }
}

void MainWindow::setControlPanelVisible(bool visible)
{
    if (!m_controlPanel) {
        return;
    }
    m_controlPanel->setVisible(visible);
    if (visible) {
        m_controlPanel->raise();
        m_controlPanel->activateWindow();
    }
}

void MainWindow::toggleControlPanel()
{
    if (m_controlPanel) {
        setControlPanelVisible(!m_controlPanel->isVisible());
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
        m_currentFile = pathOrUrl;
        QString fileName = QFileInfo(pathOrUrl).fileName();
        if (fileName.isEmpty()) {
            fileName = pathOrUrl;
        }
        setWindowTitle(fileName + QStringLiteral(" - outPlayer"));

        if (!m_engine->isPaused()) {
            m_autohideTimer->start(2500);
        }

        if (m_playlistModel) {
            const auto current = m_playlistModel->currentItem();
            if (!current.has_value() || current->filePath != pathOrUrl) {
                int existingRow = -1;
                for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
                    const QModelIndex idx = m_playlistModel->index(i, 0);
                    if (idx.data(playlist::PlaylistModel::FilePathRole).toString() == pathOrUrl) {
                        existingRow = i;
                        break;
                    }
                }
                if (existingRow >= 0) {
                    m_playlistModel->setCurrentIndex(existingRow);
                } else {
                    playlist::PlaylistItem item;
                    item.filePath = pathOrUrl;
                    item.title = fileName;
                    m_playlistModel->addItem(item);
                    m_playlistModel->setCurrentIndex(m_playlistModel->rowCount() - 1);
                }
            }
        }
    }
    return loaded;
}

void MainWindow::openFiles(const QStringList& files, bool playFirst)
{
    if (files.isEmpty() || !m_playlistModel) {
        return;
    }
    const int startIdx = m_playlistModel->rowCount();
    QList<playlist::PlaylistItem> items;
    items.reserve(files.size());
    for (const auto& path : files) {
        playlist::PlaylistItem item;
        item.filePath = path;
        item.title = QFileInfo(path).fileName();
        if (item.title.isEmpty()) {
            item.title = path;
        }
        items.append(item);
    }
    m_playlistModel->addItems(items);

    if (playFirst || startIdx == 0 || m_playlistModel->currentIndex() < 0) {
        playPlaylistItem(startIdx);
    }
}

void MainWindow::openFolder(const QString& folderPath, bool playFirst)
{
    if (folderPath.isEmpty() || !m_playlistModel) {
        return;
    }
    const QStringList files = MediaOpenController::scanDirectory(folderPath, true);
    if (!files.isEmpty()) {
        openFiles(files, playFirst);
    }
}

void MainWindow::playPlaylistItem(int row)
{
    if (!m_playlistModel || row < 0 || row >= m_playlistModel->rowCount()) {
        return;
    }

    m_playlistModel->setCurrentIndex(row);
    const auto optItem = m_playlistModel->currentItem();
    if (!optItem.has_value()) {
        return;
    }

    if (m_videoWidget) {
        m_videoWidget->ensureInitialized();
    }

    if (m_engine) {
        m_currentFile = optItem->filePath;
        m_engine->loadFile(optItem->filePath);
        QString fileName = QFileInfo(optItem->filePath).fileName();
        if (fileName.isEmpty()) {
            fileName = optItem->filePath;
        }
        setWindowTitle(fileName + QStringLiteral(" - outPlayer"));

        if (!m_engine->isPaused()) {
            m_autohideTimer->start(2500);
        }
    }
}

void MainWindow::playNext()
{
    if (!m_playlistModel) {
        return;
    }
    const int nextIdx = m_playlistModel->nextIndex();
    if (nextIdx >= 0) {
        playPlaylistItem(nextIdx);
    }
}

void MainWindow::playPrevious()
{
    if (!m_playlistModel) {
        return;
    }
    const int prevIdx = m_playlistModel->previousIndex();
    if (prevIdx >= 0) {
        playPlaylistItem(prevIdx);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData() && (event->mimeData()->hasUrls() || event->mimeData()->hasText())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (!mimeData) {
        event->ignore();
        return;
    }

    QStringList filesToAdd;
    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        for (const auto& url : urls) {
            if (url.isLocalFile()) {
                const QString localPath = url.toLocalFile();
                QFileInfo fi(localPath);
                if (fi.isDir()) {
                    filesToAdd.append(MediaOpenController::scanDirectory(localPath, true));
                } else if (fi.isFile() && MediaOpenController::isMediaFile(localPath)) {
                    filesToAdd.append(localPath);
                }
            } else {
                filesToAdd.append(url.toString());
            }
        }
    } else if (mimeData->hasText()) {
        const QString text = mimeData->text().trimmed();
        if (!text.isEmpty()) {
            filesToAdd.append(text);
        }
    }

    if (!filesToAdd.isEmpty()) {
        const bool wasEmpty = (m_playlistModel && m_playlistModel->rowCount() == 0);
        openFiles(filesToAdd, wasEmpty || !m_engine || m_engine->isPaused() || m_playlistModel->currentIndex() < 0);
        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_contextMenuManager) {
        m_contextMenuManager->showContextMenu(event->globalPos(), this, m_engine.get());
        event->accept();
        return;
    }
    QMainWindow::contextMenuEvent(event);
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
        playNext();
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

    registerCommand(core::CommandId::CMD_PLAYLIST, [this](const QVariantMap&) {
        togglePlaylist();
    });

    registerCommand(core::CommandId::CMD_PLAYLIST_CLOSE, [this](const QVariantMap&) {
        setPlaylistVisible(false);
    });

    registerCommand(core::CommandId::CMD_PLAYLIST_NEXT, [this](const QVariantMap&) {
        playNext();
    });

    registerCommand(core::CommandId::CMD_PLAYLIST_PREV, [this](const QVariantMap&) {
        playPrevious();
    });

    registerCommand(core::CommandId::CMD_MOUSE_RIGHT_CLICK, [this](const QVariantMap&) {
        m_contextMenuManager->showContextMenu(QCursor::pos(), this, m_engine.get());
    });

    // Control panel toggle
    auto toggleCP = [this](const QVariantMap&) {
        toggleControlPanel();
    };
    registerCommand(core::CommandId::CMD_CONTROL_PANEL, toggleCP);
    registerCommand(core::CommandId::CMD_PANEL_CONTROL, toggleCP);

    // Bookmarks
    auto getCurrentFilePath = [this]() -> QString {
        if (!m_currentFile.isEmpty()) {
            return m_currentFile;
        }
        if (m_playlistModel) {
            auto opt = m_playlistModel->currentItem();
            if (opt.has_value()) {
                return opt->filePath;
            }
        }
        return QString();
    };

    registerCommand(core::CommandId::CMD_BOOKMARK_ADD, [this, getCurrentFilePath](const QVariantMap&) {
        const QString path = getCurrentFilePath();
        if (!path.isEmpty() && m_engine && m_bookmarkManager) {
            const double pos = m_engine->position();
            m_bookmarkManager->addBookmark(path, pos);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Bookmark added: %1").arg(TimeFormatter::formatTime(pos)),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_BOOKMARK_PREV, [this, getCurrentFilePath](const QVariantMap&) {
        const QString path = getCurrentFilePath();
        if (!path.isEmpty() && m_engine && m_bookmarkManager) {
            const double pos = m_engine->position();
            auto optBm = m_bookmarkManager->previousBookmark(path, pos);
            if (optBm.has_value()) {
                m_engine->seek(optBm->timestamp);
                if (m_osdOverlay) {
                    m_osdOverlay->showMessage(
                        QStringLiteral("Bookmark: %1").arg(TimeFormatter::formatTime(optBm->timestamp)),
                        1500, OsdPosition::TopLeft);
                }
            }
        }
    });

    registerCommand(core::CommandId::CMD_BOOKMARK_NEXT, [this, getCurrentFilePath](const QVariantMap&) {
        const QString path = getCurrentFilePath();
        if (!path.isEmpty() && m_engine && m_bookmarkManager) {
            const double pos = m_engine->position();
            auto optBm = m_bookmarkManager->nextBookmark(path, pos);
            if (optBm.has_value()) {
                m_engine->seek(optBm->timestamp);
                if (m_osdOverlay) {
                    m_osdOverlay->showMessage(
                        QStringLiteral("Bookmark: %1").arg(TimeFormatter::formatTime(optBm->timestamp)),
                        1500, OsdPosition::TopLeft);
                }
            }
        }
    });

    // Frame capture
    auto captureSave = [this](const QVariantMap&) {
        if (m_captureController) {
            const QString path = m_captureController->captureInstantFrame();
            if (!path.isEmpty() && m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Captured: %1").arg(QFileInfo(path).fileName()),
                    1500, OsdPosition::TopLeft);
            }
        }
    };
    registerCommand(core::CommandId::CMD_CAPTURE_SAVE_FRAME, captureSave);
    registerCommand(core::CommandId::CMD_CAPTURE_SAVE, captureSave);

    auto captureCopy = [this](const QVariantMap&) {
        if (m_captureController && m_captureController->copyFrameToClipboard()) {
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Frame copied to clipboard"),
                    1500, OsdPosition::TopLeft);
            }
        }
    };
    registerCommand(core::CommandId::CMD_CAPTURE_COPY_FRAME, captureCopy);
    registerCommand(core::CommandId::CMD_CAPTURE_COPY, captureCopy);

    // Video Equalizer
    registerCommand(core::CommandId::CMD_VIDEO_BRIGHTNESS_UP, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setBrightness(m_engine->brightness() + 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Brightness: %1%").arg(m_engine->brightness()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_BRIGHTNESS_DOWN, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setBrightness(m_engine->brightness() - 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Brightness: %1%").arg(m_engine->brightness()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_BRIGHTNESS_RESET, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setBrightness(0);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Brightness: 0%"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VIDEO_CONTRAST_UP, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setContrast(m_engine->contrast() + 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Contrast: %1%").arg(m_engine->contrast()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_CONTRAST_DOWN, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setContrast(m_engine->contrast() - 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Contrast: %1%").arg(m_engine->contrast()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_CONTRAST_RESET, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setContrast(0);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Contrast: 0%"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VIDEO_SATURATION_UP, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSaturation(m_engine->saturation() + 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Saturation: %1%").arg(m_engine->saturation()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_SATURATION_DOWN, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSaturation(m_engine->saturation() - 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Saturation: %1%").arg(m_engine->saturation()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_SATURATION_RESET, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSaturation(0);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Saturation: 0%"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VIDEO_HUE_UP, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setHue(m_engine->hue() + 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Hue: %1%").arg(m_engine->hue()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_HUE_DOWN, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setHue(m_engine->hue() - 1);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Hue: %1%").arg(m_engine->hue()),
                    1500, OsdPosition::TopLeft);
            }
        }
    });
    registerCommand(core::CommandId::CMD_VIDEO_HUE_RESET, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->setHue(0);
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Hue: 0%"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    registerCommand(core::CommandId::CMD_VIDEO_RESET, [this](const QVariantMap&) {
        if (m_engine) {
            m_engine->resetVideoEqualizer();
            if (m_osdOverlay) {
                m_osdOverlay->showMessage(
                    QStringLiteral("Video adjustments reset"),
                    1500, OsdPosition::TopLeft);
            }
        }
    });

    // Audio Sync
    auto showAudioSyncOsd = [this]() {
        if (m_engine && m_osdOverlay) {
            const int ms = static_cast<int>(std::round(m_engine->audioDelay() * 1000.0));
            QString sign = (ms > 0) ? QStringLiteral("+") : QString();
            m_osdOverlay->showMessage(
                QStringLiteral("Audio Sync: %1%2 ms").arg(sign).arg(ms),
                1500, OsdPosition::TopLeft);
        }
    };

    registerCommand(core::CommandId::CMD_AUDIO_SYNC_FASTER, [this, showAudioSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setAudioDelay(m_engine->audioDelay() + 0.05);
            showAudioSyncOsd();
        }
    });
    registerCommand(core::CommandId::CMD_AUDIO_SYNC_SLOWER, [this, showAudioSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setAudioDelay(m_engine->audioDelay() - 0.05);
            showAudioSyncOsd();
        }
    });
    registerCommand(core::CommandId::CMD_AUDIO_SYNC_RESET, [this, showAudioSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setAudioDelay(0.0);
            showAudioSyncOsd();
        }
    });

    // Subtitle Sync
    auto showSubtitleSyncOsd = [this]() {
        if (m_engine && m_osdOverlay) {
            const double delay = m_engine->subtitleDelay();
            QString sign = (delay > 0.0) ? QStringLiteral("+") : QString();
            m_osdOverlay->showMessage(
                QStringLiteral("Subtitle Sync: %1%2 s").arg(sign).arg(QString::number(delay, 'f', 1)),
                1500, OsdPosition::TopLeft);
        }
    };

    registerCommand(core::CommandId::CMD_SUB_SYNC_FASTER, [this, showSubtitleSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSubtitleDelay(m_engine->subtitleDelay() + 0.5);
            showSubtitleSyncOsd();
        }
    });
    registerCommand(core::CommandId::CMD_SUB_SYNC_SLOWER, [this, showSubtitleSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSubtitleDelay(m_engine->subtitleDelay() - 0.5);
            showSubtitleSyncOsd();
        }
    });
    registerCommand(core::CommandId::CMD_SUB_SYNC_RESET, [this, showSubtitleSyncOsd](const QVariantMap&) {
        if (m_engine) {
            m_engine->setSubtitleDelay(0.0);
            showSubtitleSyncOsd();
        }
    });
}

void MainWindow::setupNavigationConnections()
{
    // Playlist Dock close button
    connect(m_playlistDock, &PlaylistDock::closeRequested, this, [this]() {
        setPlaylistVisible(false);
    });

    // Playlist Dock item activation (double-click or Enter)
    connect(m_playlistDock, &PlaylistDock::playItemRequested, this, &MainWindow::playPlaylistItem);

    // Playlist Dock add actions
    connect(m_playlistDock, &PlaylistDock::addFilesRequested, this, [this]() {
        const QStringList files = m_mediaOpenController->promptOpenFiles(this);
        if (!files.isEmpty()) {
            openFiles(files, false /* only play if empty */);
        }
    });

    connect(m_playlistDock, &PlaylistDock::addFolderRequested, this, [this]() {
        const QString folder = m_mediaOpenController->promptOpenFolder(this);
        if (!folder.isEmpty()) {
            openFolder(folder, false /* only play if empty */);
        }
    });

    connect(m_playlistDock, &PlaylistDock::addUrlRequested, this, [this]() {
        const QString url = m_mediaOpenController->promptOpenUrl(this);
        if (!url.isEmpty()) {
            playlist::PlaylistItem item;
            item.filePath = url;
            item.title = url;
            m_playlistModel->addItem(item);
            if (m_playlistModel->rowCount() == 1 || m_playlistModel->currentIndex() < 0) {
                playPlaylistItem(0);
            }
        }
    });

    // TitleBar logo click -> context menu
    connect(m_titleBar, &CustomTitleBar::logoClicked, this, [this]() {
        const QPoint pos = m_titleBar->mapToGlobal(QPoint(10, m_titleBar->height()));
        m_contextMenuManager->showContextMenu(pos, this, m_engine.get());
    });

    // Context menu dialog actions
    connect(m_contextMenuManager.get(), &ContextMenuManager::openFilesRequested, this, [this]() {
        const QStringList files = m_mediaOpenController->promptOpenFiles(this);
        if (!files.isEmpty()) {
            openFiles(files, true /* play first */);
        }
    });

    connect(m_contextMenuManager.get(), &ContextMenuManager::openFolderRequested, this, [this]() {
        const QString folder = m_mediaOpenController->promptOpenFolder(this);
        if (!folder.isEmpty()) {
            openFolder(folder, true /* play first */);
        }
    });

    connect(m_contextMenuManager.get(), &ContextMenuManager::openUrlRequested, this, [this]() {
        const QString url = m_mediaOpenController->promptOpenUrl(this);
        if (!url.isEmpty()) {
            openMedia(url);
        }
    });

    connect(m_contextMenuManager.get(), &ContextMenuManager::togglePlaylistRequested, this, &MainWindow::togglePlaylist);
    connect(m_contextMenuManager.get(), &ContextMenuManager::toggleControlPanelRequested, this, &MainWindow::toggleControlPanel);
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
                    geo.setWidth(newW);
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
                    geo.setHeight(newH);
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
        } else if (event->type() == QEvent::ContextMenu) {
            auto* ce = static_cast<QContextMenuEvent*>(event);
            if (m_contextMenuManager) {
                m_contextMenuManager->showContextMenu(ce->globalPos(), this, m_engine.get());
                return true;
            }
        } else if (event->type() == QEvent::DragEnter) {
            dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return true;
        } else if (event->type() == QEvent::Drop) {
            dropEvent(static_cast<QDropEvent*>(event));
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onAutohideTimeout()
{
    if (!m_engine || m_engine->isPaused()) {
        return;
    }

    // Check if mouse is hovering over titlebar, controlbar, or playlist dock
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
    if (m_playlistDock && m_playlistDock->isVisible()) {
        const QPoint localPos = m_playlistDock->mapFromGlobal(QCursor::pos());
        if (m_playlistDock->rect().contains(localPos) && m_playlistDock->underMouse()) {
            m_autohideTimer->start(2500);
            return;
        }
    }

    setChromeVisible(false);
}

} // namespace outplayer::ui
