// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QSlider;

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::ui {

class TimelineSlider;

class ControlBar : public QWidget {
    Q_OBJECT

public:
    explicit ControlBar(QWidget* parent = nullptr);
    ~ControlBar() override = default;

    void setPlaybackEngine(engine::IPlaybackEngine* engine);
    void setPlaying(bool isPlaying);
    void setTime(double currentSeconds, double totalSeconds);
    void setVolume(double volumePercent);
    void setMuted(bool isMuted);

    [[nodiscard]] TimelineSlider* timelineSlider() const noexcept;
    [[nodiscard]] QLabel* timeLabel() const noexcept;
    [[nodiscard]] QPushButton* playPauseButton() const noexcept;
    [[nodiscard]] QPushButton* stopButton() const noexcept;
    [[nodiscard]] QPushButton* prevButton() const noexcept;
    [[nodiscard]] QPushButton* nextButton() const noexcept;
    [[nodiscard]] QPushButton* muteButton() const noexcept;
    [[nodiscard]] QSlider* volumeSlider() const noexcept;
    [[nodiscard]] QPushButton* subtitleButton() const noexcept;
    [[nodiscard]] QPushButton* playlistButton() const noexcept;
    [[nodiscard]] QPushButton* controlPanelButton() const noexcept;

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] double volume() const noexcept;
    [[nodiscard]] double currentPosition() const noexcept;
    [[nodiscard]] double duration() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void playPauseClicked();
    void stopClicked();
    void prevClicked();
    void nextClicked();
    void volumeChanged(double volumePercent);
    void muteToggled();
    void playlistToggled();
    void controlPanelToggled();
    void subtitleToggled();

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onPrevClicked();
    void onNextClicked();
    void onMuteClicked();
    void onVolumeSliderValueChanged(int value);
    void onSubtitleClicked();
    void onPlaylistClicked();
    void onControlPanelClicked();

private:
    void setupUi();
    void applyStyle();

    engine::IPlaybackEngine* m_engine{nullptr};

    QPushButton* m_playPauseBtn{nullptr};
    QPushButton* m_stopBtn{nullptr};
    QPushButton* m_prevBtn{nullptr};
    QPushButton* m_nextBtn{nullptr};
    QLabel* m_timeLabel{nullptr};
    TimelineSlider* m_timelineSlider{nullptr};
    QPushButton* m_muteBtn{nullptr};
    QSlider* m_volumeSlider{nullptr};
    QPushButton* m_subBtn{nullptr};
    QPushButton* m_playlistBtn{nullptr};
    QPushButton* m_controlPanelBtn{nullptr};

    bool m_isPlaying{false};
    bool m_isMuted{false};
    double m_volume{100.0};
    double m_currentPosition{0.0};
    double m_duration{0.0};
};

} // namespace outplayer::ui
