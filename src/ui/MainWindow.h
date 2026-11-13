// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"

class QTimer;
class QMouseEvent;
class QDragEnterEvent;
class QDropEvent;
class QContextMenuEvent;

namespace outplayer::core {
class BookmarkManager;
class DeviceStreamManager;
}

namespace outplayer::engine {
class MpvEngine;
}

namespace outplayer::input {
class InputManager;
}

namespace outplayer::playlist {
class PlaylistModel;
}

namespace outplayer::ui {

class ControlBar;
class CustomTitleBar;
class OsdOverlay;
class VideoWidget;
class PlaylistDock;
class ContextMenuManager;
class MediaOpenController;
class ControlPanel;
class ScreenCaptureController;
class PreferencesDialog;
class DesktopIntegration;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    [[nodiscard]] engine::MpvEngine* engine() const noexcept;
    [[nodiscard]] VideoWidget* videoWidget() const noexcept;
    [[nodiscard]] QWidget* centralWidget() const noexcept;
    [[nodiscard]] CustomTitleBar* titleBar() const noexcept;
    [[nodiscard]] ControlBar* controlBar() const noexcept;
    [[nodiscard]] OsdOverlay* osdOverlay() const noexcept;
    [[nodiscard]] input::InputManager* inputManager() const noexcept;
    [[nodiscard]] PlaylistDock* playlistDock() const noexcept;
    [[nodiscard]] playlist::PlaylistModel* playlistModel() const noexcept;
    [[nodiscard]] ContextMenuManager* contextMenuManager() const noexcept;
    [[nodiscard]] MediaOpenController* mediaOpenController() const noexcept;
    [[nodiscard]] ControlPanel* controlPanel() const noexcept;
    [[nodiscard]] core::BookmarkManager* bookmarkManager() const noexcept;
    [[nodiscard]] ScreenCaptureController* screenCaptureController() const noexcept;
    [[nodiscard]] PreferencesDialog* preferencesDialog() const noexcept;
    [[nodiscard]] core::DeviceStreamManager* deviceStreamManager() const noexcept;
    [[nodiscard]] DesktopIntegration* desktopIntegration() const noexcept;

    [[nodiscard]] bool isChromeVisible() const noexcept;
    void setChromeVisible(bool visible);
    void toggleFullScreen();

    void setPlaylistVisible(bool visible);
    void togglePlaylist();

    void setControlPanelVisible(bool visible);
    void toggleControlPanel();

    bool openMedia(const QString& pathOrUrl);
    void openFiles(const QStringList& files, bool playFirst = true);
    void openFolder(const QString& folderPath, bool playFirst = true);
    void playPlaylistItem(int row);
    void playNext();
    void playPrevious();
    void stopPlayback();

    // Drag and drop events declared public for unit testing access
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void setWindowTitle(const QString& title);
    void showPreferences();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onAutohideTimeout();

private:
    void updateChromeLayout();
    void setupEngineConnections();
    void setupCommandHandlers();
    void setupNavigationConnections();
    void updateVideoInfoBadges();
    void registerCommand(core::CommandId id, core::CommandRegistry::Handler handler);
    void handleMouseMove(QMouseEvent* event, QObject* watched);
    bool handleMousePress(QMouseEvent* event, QObject* watched);

    std::unique_ptr<engine::MpvEngine> m_engine;
    QWidget* m_centralContainer{nullptr};
    VideoWidget* m_videoWidget{nullptr};
    PlaylistDock* m_playlistDock{nullptr};
    playlist::PlaylistModel* m_playlistModel{nullptr};
    ControlPanel* m_controlPanel{nullptr};
    std::unique_ptr<core::BookmarkManager> m_bookmarkManager;
    std::unique_ptr<ScreenCaptureController> m_captureController;
    std::unique_ptr<ContextMenuManager> m_contextMenuManager;
    std::unique_ptr<MediaOpenController> m_mediaOpenController;
    std::unique_ptr<PreferencesDialog> m_preferencesDialog;
    std::unique_ptr<core::DeviceStreamManager> m_deviceStreamManager;
    std::unique_ptr<DesktopIntegration> m_desktopIntegration;

    QString m_currentFile;

    CustomTitleBar* m_titleBar{nullptr};
    ControlBar* m_controlBar{nullptr};
    OsdOverlay* m_osdOverlay{nullptr};
    std::unique_ptr<input::InputManager> m_inputManager;

    QTimer* m_autohideTimer{nullptr};
    bool m_chromeVisible{true};
    bool m_userStopped{false};

    bool m_isResizing{false};
    Qt::Edges m_resizeEdges{};
    QPointF m_resizeStartGlobalPos;
    QRect m_resizeStartGeometry;

    std::vector<core::CommandId> m_registeredCommands;
};

} // namespace outplayer::ui
