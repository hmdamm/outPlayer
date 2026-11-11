// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMainWindow>
#include <memory>

namespace outplayer::engine {
class MpvEngine;
}

namespace outplayer::ui {

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

    bool openMedia(const QString& pathOrUrl);

private:
    std::unique_ptr<engine::MpvEngine> m_engine;
    VideoWidget* m_videoWidget{nullptr};
};

} // namespace outplayer::ui
