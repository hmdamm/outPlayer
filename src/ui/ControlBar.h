// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QPointer>
#include <QString>

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
    void setVideoInfo(bool hwDec, const QString& videoCodec, const QString& audioCodec, const QString& channels);

    [[nodiscard]] TimelineSlider* timelineSlider() const noexcept;
    [[nodiscard]] QLabel* timeLabel() const noexcept;
    [[nodiscard]] QPushButton* playPauseButton() const noexcept;
    [[nodiscard]] QPushButton* stopButton() const noexcept;
    [[nodiscard]] QPushButton* prevButton() const noexcept;
    [[nodiscard]] QPushButton* nextButton() const noexcept;
    [[nodiscard]] QPushButton* openFileButton() const noexcept;
    [[nodiscard]] QPushButton* muteButton() const noexcept;
    [[nodiscard]] QSlider* volumeSlider() const noexcept;
    [[nodiscard]] QPushButton* subtitleButton() const noexcept;
    [[nodiscard]] QPushButton* playlistButton() const noexcept;
    [[nodiscard]] QPushButton* controlPanelButton() const noexcept;
    [[nodiscard]] QPushButton* settingsButton() const noexcept;

    [[nodiscard]] QLabel* hwBadge() const noexcept;
    [[nodiscard]] QLabel* videoCodecBadge() const noexcept;
    [[nodiscard]] QLabel* audioCodecBadge() const noexcept;
    [[nodiscard]] QLabel* audioChannelsBadge() const noexcept;

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] double volume() const noexcept;
    [[nodiscard]] double currentPosition() const noexcept;
    [[nodiscard]] double duration() const noexcept;
    [[nodiscard]] bool isHwDecoding() const noexcept;
    [[nodiscard]] QString videoCodec() const noexcept;
    [[nodiscard]] QString audioCodec() const noexcept;
    [[nodiscard]] QString audioChannels() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void playPauseClicked();
    void stopClicked();
    void prevClicked();
    void nextClicked();
    void openFileClicked();
    void volumeChanged(double volumePercent);
    void muteToggled();
    void playlistToggled();
    void controlPanelToggled();
    void subtitleToggled();
    void preferencesClicked();

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onPrevClicked();
    void onNextClicked();
    void onOpenFileClicked();
    void onMuteClicked();
    void onVolumeSliderValueChanged(int value);
    void onSubtitleClicked();
    void onPlaylistClicked();
    void onControlPanelClicked();
    void onSettingsClicked();

private:
    void setupUi();
    void applyStyle();

    QPointer<engine::IPlaybackEngine> m_engine;

    // Top tier
    TimelineSlider* m_timelineSlider{nullptr};
    QPushButton* m_muteBtn{nullptr};
    QSlider* m_volumeSlider{nullptr};

    // Bottom tier
    QPushButton* m_playPauseBtn{nullptr};
    QPushButton* m_stopBtn{nullptr};
    QPushButton* m_prevBtn{nullptr};
    QPushButton* m_nextBtn{nullptr};
    QPushButton* m_openFileBtn{nullptr};
    QLabel* m_timeLabel{nullptr};

    // Badges
    QLabel* m_hwBadge{nullptr};
    QLabel* m_videoCodecBadge{nullptr};
    QLabel* m_audioCodecBadge{nullptr};
    QLabel* m_audioChannelsBadge{nullptr};

    // Right-aligned utility buttons
    QPushButton* m_subBtn{nullptr};
    QPushButton* m_controlPanelBtn{nullptr};
    QPushButton* m_settingsBtn{nullptr};
    QPushButton* m_playlistBtn{nullptr};

    bool m_isPlaying{false};
    bool m_isMuted{false};
    double m_volume{100.0};
    double m_currentPosition{0.0};
    double m_duration{0.0};

    bool m_isHwDecoding{false};
    QString m_videoCodec;
    QString m_audioCodec;
    QString m_audioChannels;
};

} // namespace outplayer::ui
