// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"

class QTimer;
class QMouseEvent;

namespace outplayer::engine {
class MpvEngine;
}

namespace outplayer::input {
class InputManager;
}

namespace outplayer::ui {

class ControlBar;
class CustomTitleBar;
class OsdOverlay;
class VideoWidget;

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
    [[nodiscard]] CustomTitleBar* titleBar() const noexcept;
    [[nodiscard]] ControlBar* controlBar() const noexcept;
    [[nodiscard]] OsdOverlay* osdOverlay() const noexcept;
    [[nodiscard]] input::InputManager* inputManager() const noexcept;

    [[nodiscard]] bool isChromeVisible() const noexcept;
    void setChromeVisible(bool visible);
    void toggleFullScreen();

    bool openMedia(const QString& pathOrUrl);

public slots:
    void setWindowTitle(const QString& title);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onAutohideTimeout();

private:
    void updateChromeLayout();
    void setupEngineConnections();
    void setupCommandHandlers();
    void registerCommand(core::CommandId id, core::CommandRegistry::Handler handler);
    void handleMouseMove(QMouseEvent* event, QObject* watched);
    bool handleMousePress(QMouseEvent* event, QObject* watched);

    std::unique_ptr<engine::MpvEngine> m_engine;
    VideoWidget* m_videoWidget{nullptr};
    CustomTitleBar* m_titleBar{nullptr};
    ControlBar* m_controlBar{nullptr};
    OsdOverlay* m_osdOverlay{nullptr};
    std::unique_ptr<input::InputManager> m_inputManager;

    QTimer* m_autohideTimer{nullptr};
    bool m_chromeVisible{true};

    bool m_isResizing{false};
    Qt::Edges m_resizeEdges{};
    QPointF m_resizeStartGlobalPos;
    QRect m_resizeStartGeometry;

    std::vector<core::CommandId> m_registeredCommands;
};

} // namespace outplayer::ui
