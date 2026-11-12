// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QPointer>
#include <QList>

class QTabWidget;
class QPushButton;
class QSlider;
class QLabel;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QCloseEvent;

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::ui {

class ControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit ControlPanel(engine::IPlaybackEngine* engine, QWidget* parent = nullptr);
    ~ControlPanel() override = default;

    void updateFromEngine();

    // General & Tab navigation
    [[nodiscard]] QTabWidget* tabWidget() const noexcept { return m_tabWidget; }
    [[nodiscard]] QPushButton* closeButton() const noexcept { return m_closeBtn; }
    [[nodiscard]] int currentTabIndex() const;
    void setCurrentTabIndex(int index);

    // Tab 1: Video Controls
    [[nodiscard]] QSlider* brightnessSlider() const noexcept { return m_brightnessSlider; }
    [[nodiscard]] QLabel* brightnessLabel() const noexcept { return m_brightnessLabel; }
    [[nodiscard]] QPushButton* brightnessResetButton() const noexcept { return m_brightnessResetBtn; }

    [[nodiscard]] QSlider* contrastSlider() const noexcept { return m_contrastSlider; }
    [[nodiscard]] QLabel* contrastLabel() const noexcept { return m_contrastLabel; }
    [[nodiscard]] QPushButton* contrastResetButton() const noexcept { return m_contrastResetBtn; }

    [[nodiscard]] QSlider* saturationSlider() const noexcept { return m_saturationSlider; }
    [[nodiscard]] QLabel* saturationLabel() const noexcept { return m_saturationLabel; }
    [[nodiscard]] QPushButton* saturationResetButton() const noexcept { return m_saturationResetBtn; }

    [[nodiscard]] QSlider* hueSlider() const noexcept { return m_hueSlider; }
    [[nodiscard]] QLabel* hueLabel() const noexcept { return m_hueLabel; }
    [[nodiscard]] QPushButton* hueResetButton() const noexcept { return m_hueResetBtn; }

    [[nodiscard]] QCheckBox* deinterlaceCheckBox() const noexcept { return m_deinterlaceCheckBox; }
    [[nodiscard]] QComboBox* rotationComboBox() const noexcept { return m_rotationComboBox; }
    [[nodiscard]] QPushButton* resetAllVideoButton() const noexcept { return m_resetAllVideoBtn; }

    // Tab 2: Audio & Graphic Equalizer
    [[nodiscard]] QList<QSlider*> equalizerSliders() const { return m_eqSliders; }
    [[nodiscard]] QSlider* equalizerSlider(int bandIndex) const;
    [[nodiscard]] QPushButton* resetEqButton() const noexcept { return m_resetEqBtn; }
    [[nodiscard]] QCheckBox* normalizerCheckBox() const noexcept { return m_normalizerCheckBox; }
    [[nodiscard]] QDoubleSpinBox* audioDelaySpinBox() const noexcept { return m_audioDelaySpinBox; }
    [[nodiscard]] QPushButton* audioDelayResetButton() const noexcept { return m_audioDelayResetBtn; }

    // Tab 3: Subtitles
    [[nodiscard]] QDoubleSpinBox* subtitleDelaySpinBox() const noexcept { return m_subtitleDelaySpinBox; }
    [[nodiscard]] QPushButton* subtitleDelayMinusButton() const noexcept { return m_subtitleDelayMinusBtn; }
    [[nodiscard]] QPushButton* subtitleDelayPlusButton() const noexcept { return m_subtitleDelayPlusBtn; }
    [[nodiscard]] QPushButton* subtitleDelayResetButton() const noexcept { return m_subtitleDelayResetBtn; }

    [[nodiscard]] QSlider* subtitleScaleSlider() const noexcept { return m_subtitleScaleSlider; }
    [[nodiscard]] QLabel* subtitleScaleLabel() const noexcept { return m_subtitleScaleLabel; }
    [[nodiscard]] QPushButton* subtitleScaleResetButton() const noexcept { return m_subtitleScaleResetBtn; }

    [[nodiscard]] QComboBox* subtitleTrackComboBox() const noexcept { return m_subtitleTrackComboBox; }
    [[nodiscard]] QPushButton* loadSubtitleButton() const noexcept { return m_loadSubtitleBtn; }

    // Tab 4: Playback Controls
    [[nodiscard]] QSlider* speedSlider() const noexcept { return m_speedSlider; }
    [[nodiscard]] QLabel* speedLabel() const noexcept { return m_speedLabel; }
    [[nodiscard]] QPushButton* speedResetButton() const noexcept { return m_speedResetBtn; }
    [[nodiscard]] QPushButton* playPauseButton() const noexcept { return m_playPauseBtn; }

    [[nodiscard]] QSize sizeHint() const override;

signals:
    void closeRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateVideoFromEngine();
    void updateAudioFromEngine();
    void updateSubtitlesFromEngine();
    void updateSubtitleTracksFromEngine();
    void updatePlaybackFromEngine();

private:
    void setupUi();
    void setupVideoTab();
    void setupAudioTab();
    void setupSubtitlesTab();
    void setupPlaybackTab();
    void setupStyle();

    QPointer<engine::IPlaybackEngine> m_engine{nullptr};
    bool m_updatingFromEngine{false};

    QTabWidget* m_tabWidget{nullptr};
    QPushButton* m_closeBtn{nullptr};

    // Tab 1 Video
    QSlider* m_brightnessSlider{nullptr};
    QLabel* m_brightnessLabel{nullptr};
    QPushButton* m_brightnessResetBtn{nullptr};

    QSlider* m_contrastSlider{nullptr};
    QLabel* m_contrastLabel{nullptr};
    QPushButton* m_contrastResetBtn{nullptr};

    QSlider* m_saturationSlider{nullptr};
    QLabel* m_saturationLabel{nullptr};
    QPushButton* m_saturationResetBtn{nullptr};

    QSlider* m_hueSlider{nullptr};
    QLabel* m_hueLabel{nullptr};
    QPushButton* m_hueResetBtn{nullptr};

    QCheckBox* m_deinterlaceCheckBox{nullptr};
    QComboBox* m_rotationComboBox{nullptr};
    QPushButton* m_resetAllVideoBtn{nullptr};

    // Tab 2 Audio
    QList<QSlider*> m_eqSliders;
    QPushButton* m_resetEqBtn{nullptr};
    QCheckBox* m_normalizerCheckBox{nullptr};
    QDoubleSpinBox* m_audioDelaySpinBox{nullptr};
    QPushButton* m_audioDelayResetBtn{nullptr};

    // Tab 3 Subtitles
    QDoubleSpinBox* m_subtitleDelaySpinBox{nullptr};
    QPushButton* m_subtitleDelayMinusBtn{nullptr};
    QPushButton* m_subtitleDelayPlusBtn{nullptr};
    QPushButton* m_subtitleDelayResetBtn{nullptr};

    QSlider* m_subtitleScaleSlider{nullptr};
    QLabel* m_subtitleScaleLabel{nullptr};
    QPushButton* m_subtitleScaleResetBtn{nullptr};

    QComboBox* m_subtitleTrackComboBox{nullptr};
    QPushButton* m_loadSubtitleBtn{nullptr};

    // Tab 4 Playback
    QSlider* m_speedSlider{nullptr};
    QLabel* m_speedLabel{nullptr};
    QPushButton* m_speedResetBtn{nullptr};
    QPushButton* m_playPauseBtn{nullptr};

    QLabel* m_pointALabel{nullptr};
    QLabel* m_pointBLabel{nullptr};
    QPushButton* m_setPointABtn{nullptr};
    QPushButton* m_setPointBBtn{nullptr};
    QPushButton* m_clearABBtn{nullptr};
    double m_pointA{-1.0};
    double m_pointB{-1.0};
};

} // namespace outplayer::ui
