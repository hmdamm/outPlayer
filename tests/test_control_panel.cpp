// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>

#include "engine/IPlaybackEngine.h"
#include "ui/ControlPanel.h"

using namespace outplayer;

class MockPlaybackEngine : public engine::IPlaybackEngine {
    Q_OBJECT
public:
    explicit MockPlaybackEngine(QObject* parent = nullptr) : engine::IPlaybackEngine(parent) {}

    bool initialize() override { return true; }
    bool loadFile(const QString&) override { return true; }
    void play() override { m_paused = false; emit stateChanged(m_paused); }
    void pause() override { m_paused = true; emit stateChanged(m_paused); }
    void togglePause() override {
        m_paused = !m_paused;
        m_togglePauseCalls++;
        emit stateChanged(m_paused);
    }
    void stop() override { m_paused = true; emit stateChanged(m_paused); }
    void seek(double seconds, bool) override { m_position = seconds; emit positionChanged(m_position); }
    [[nodiscard]] double position() const override { return m_position; }
    [[nodiscard]] double duration() const override { return m_duration; }
    [[nodiscard]] bool isPaused() const override { return m_paused; }

    [[nodiscard]] QList<engine::MediaTrack> tracks(const QString& type = {}) const override {
        if (type.isEmpty()) return m_tracks;
        QList<engine::MediaTrack> filtered;
        for (const auto& trk : m_tracks) {
            if (trk.type == type) filtered.append(trk);
        }
        return filtered;
    }
    bool setTrack(const QString& type, int trackId) override {
        if (type == "sub") {
            m_selectedSubTrackId = trackId;
            for (auto& trk : m_tracks) {
                if (trk.type == "sub") {
                    trk.selected = (trk.id == trackId);
                }
            }
            emit tracksChanged();
            return true;
        }
        return false;
    }

    [[nodiscard]] double volume() const override { return 100.0; }
    void setVolume(double) override {}
    [[nodiscard]] bool isMuted() const override { return false; }
    void setMuted(bool) override {}
    [[nodiscard]] double speed() const override { return m_speed; }
    void setSpeed(double spd) override {
        m_speed = spd;
        m_setSpeedCalls++;
        emit speedChanged(spd);
    }
    [[nodiscard]] QString aspectRatio() const override { return QStringLiteral("auto"); }
    void setAspectRatio(const QString&) override {}
    [[nodiscard]] QList<engine::ChapterInfo> chapters() const override { return {}; }
    bool setChapter(int) override { return true; }

    // Video Equalizer
    [[nodiscard]] int brightness() const override { return m_brightness; }
    void setBrightness(int val) override {
        m_brightness = val;
        m_setBrightnessCalls++;
        emit videoEqualizerChanged();
    }
    [[nodiscard]] int contrast() const override { return m_contrast; }
    void setContrast(int val) override {
        m_contrast = val;
        m_setContrastCalls++;
        emit videoEqualizerChanged();
    }
    [[nodiscard]] int saturation() const override { return m_saturation; }
    void setSaturation(int val) override {
        m_saturation = val;
        m_setSaturationCalls++;
        emit videoEqualizerChanged();
    }
    [[nodiscard]] int hue() const override { return m_hue; }
    void setHue(int val) override {
        m_hue = val;
        m_setHueCalls++;
        emit videoEqualizerChanged();
    }
    [[nodiscard]] int rotation() const override { return m_rotation; }
    void setRotation(int degrees) override {
        m_rotation = degrees;
        m_setRotationCalls++;
        emit videoEqualizerChanged();
    }
    [[nodiscard]] bool deinterlace() const override { return m_deinterlace; }
    void setDeinterlace(bool enable) override {
        m_deinterlace = enable;
        m_setDeinterlaceCalls++;
        emit videoEqualizerChanged();
    }
    void resetVideoEqualizer() override {
        m_brightness = 0;
        m_contrast = 0;
        m_saturation = 0;
        m_hue = 0;
        m_resetVideoEqualizerCalls++;
        emit videoEqualizerChanged();
    }

    // Audio Equalizer & Sync
    [[nodiscard]] double audioDelay() const override { return m_audioDelay; }
    void setAudioDelay(double seconds) override {
        m_audioDelay = seconds;
        m_setAudioDelayCalls++;
        emit audioEqualizerChanged();
    }
    [[nodiscard]] double equalizerBand(int bandIndex) const override {
        if (bandIndex >= 0 && bandIndex < 10) return m_eqBands[bandIndex];
        return 0.0;
    }
    void setEqualizerBand(int bandIndex, double gainDb) override {
        if (bandIndex >= 0 && bandIndex < 10) {
            m_eqBands[bandIndex] = gainDb;
            m_setEqualizerBandCalls++;
            emit audioEqualizerChanged();
        }
    }
    void resetAudioEqualizer() override {
        for (int i = 0; i < 10; ++i) m_eqBands[i] = 0.0;
        m_resetAudioEqualizerCalls++;
        emit audioEqualizerChanged();
    }
    [[nodiscard]] bool isNormalizerEnabled() const override { return m_normalizerEnabled; }
    void setNormalizerEnabled(bool enable) override {
        m_normalizerEnabled = enable;
        m_setNormalizerCalls++;
        emit audioEqualizerChanged();
    }

    // Subtitle Controls & Sync
    [[nodiscard]] double subtitleDelay() const override { return m_subtitleDelay; }
    void setSubtitleDelay(double seconds) override {
        m_subtitleDelay = seconds;
        m_setSubtitleDelayCalls++;
        emit subtitleSettingsChanged();
    }
    [[nodiscard]] double subtitleScale() const override { return m_subtitleScale; }
    void setSubtitleScale(double scale) override {
        m_subtitleScale = scale;
        m_setSubtitleScaleCalls++;
        emit subtitleSettingsChanged();
    }
    bool loadSubtitle(const QString& filePath) override {
        m_loadedSubtitle = filePath;
        return true;
    }

    bool captureFrame(const QString&) override { return true; }

    double m_position{0.0};
    double m_duration{100.0};
    bool m_paused{false};
    int m_togglePauseCalls{0};

    int m_brightness{0};
    int m_contrast{0};
    int m_saturation{0};
    int m_hue{0};
    int m_rotation{0};
    bool m_deinterlace{false};
    int m_setBrightnessCalls{0};
    int m_setContrastCalls{0};
    int m_setSaturationCalls{0};
    int m_setHueCalls{0};
    int m_setRotationCalls{0};
    int m_setDeinterlaceCalls{0};
    int m_resetVideoEqualizerCalls{0};

    double m_audioDelay{0.0};
    double m_eqBands[10]{0.0};
    int m_setAudioDelayCalls{0};
    int m_setEqualizerBandCalls{0};
    int m_resetAudioEqualizerCalls{0};
    bool m_normalizerEnabled{false};
    int m_setNormalizerCalls{0};

    double m_subtitleDelay{0.0};
    double m_subtitleScale{1.0};
    int m_setSubtitleDelayCalls{0};
    int m_setSubtitleScaleCalls{0};
    QString m_loadedSubtitle;
    QList<engine::MediaTrack> m_tracks;
    int m_selectedSubTrackId{-1};

    double m_speed{1.0};
    int m_setSpeedCalls{0};
};

class TestControlPanel : public QObject {
    Q_OBJECT

private slots:
    void testConstructionAndDefaults();
    void testCloseSignal();
    void testVideoTabControls();
    void testAudioTabControls();
    void testSubtitleTabControls();
    void testPlaybackTabControls();
    void testUpdateFromEngineWithoutFeedbackLoop();
};

void TestControlPanel::testConstructionAndDefaults() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    QVERIFY(panel.windowFlags().testFlag(Qt::Tool));
    QVERIFY(panel.tabWidget() != nullptr);
    QCOMPARE(panel.tabWidget()->count(), 4);
    QCOMPARE(panel.tabWidget()->tabText(0), QStringLiteral("Video"));
    QCOMPARE(panel.tabWidget()->tabText(1), QStringLiteral("Audio / EQ"));
    QCOMPARE(panel.tabWidget()->tabText(2), QStringLiteral("Subtitles"));
    QCOMPARE(panel.tabWidget()->tabText(3), QStringLiteral("Playback"));

    QVERIFY(panel.closeButton() != nullptr);
}

void TestControlPanel::testCloseSignal() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    QSignalSpy spy(&panel, &ui::ControlPanel::closeRequested);
    panel.closeButton()->click();
    QCOMPARE(spy.count(), 1);
}

void TestControlPanel::testVideoTabControls() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    // Sliders default to 0 with range -100 to 100
    QVERIFY(panel.brightnessSlider() != nullptr);
    QCOMPARE(panel.brightnessSlider()->minimum(), -100);
    QCOMPARE(panel.brightnessSlider()->maximum(), 100);
    QCOMPARE(panel.brightnessSlider()->value(), 0);
    QCOMPARE(panel.brightnessLabel()->text(), QStringLiteral("0"));

    QVERIFY(panel.contrastSlider() != nullptr);
    QCOMPARE(panel.contrastSlider()->minimum(), -100);
    QCOMPARE(panel.contrastSlider()->maximum(), 100);
    QCOMPARE(panel.contrastSlider()->value(), 0);

    QVERIFY(panel.saturationSlider() != nullptr);
    QCOMPARE(panel.saturationSlider()->minimum(), -100);
    QCOMPARE(panel.saturationSlider()->maximum(), 100);
    QCOMPARE(panel.saturationSlider()->value(), 0);

    QVERIFY(panel.hueSlider() != nullptr);
    QCOMPARE(panel.hueSlider()->minimum(), -100);
    QCOMPARE(panel.hueSlider()->maximum(), 100);
    QCOMPARE(panel.hueSlider()->value(), 0);

    // Moving brightness slider updates engine
    panel.brightnessSlider()->setValue(25);
    QCOMPARE(engine.brightness(), 25);
    QCOMPARE(panel.brightnessLabel()->text(), QStringLiteral("25"));

    // Reset button for brightness resets to 0
    panel.brightnessResetButton()->click();
    QCOMPARE(panel.brightnessSlider()->value(), 0);
    QCOMPARE(engine.brightness(), 0);
    QCOMPARE(panel.brightnessLabel()->text(), QStringLiteral("0"));

    // Moving contrast, saturation, hue
    panel.contrastSlider()->setValue(-15);
    QCOMPARE(engine.contrast(), -15);
    panel.saturationSlider()->setValue(30);
    QCOMPARE(engine.saturation(), 30);
    panel.hueSlider()->setValue(-40);
    QCOMPARE(engine.hue(), -40);

    // Rotation combobox
    QVERIFY(panel.rotationComboBox() != nullptr);
    QCOMPARE(panel.rotationComboBox()->count(), 4);
    panel.rotationComboBox()->setCurrentIndex(1); // 90°
    QCOMPARE(engine.rotation(), 90);

    // Deinterlacing checkbox
    QVERIFY(panel.deinterlaceCheckBox() != nullptr);
    panel.deinterlaceCheckBox()->setChecked(true);
    QCOMPARE(engine.deinterlace(), true);

    // Reset All Video Adjustments
    QVERIFY(panel.resetAllVideoButton() != nullptr);
    panel.resetAllVideoButton()->click();
    QCOMPARE(panel.brightnessSlider()->value(), 0);
    QCOMPARE(panel.contrastSlider()->value(), 0);
    QCOMPARE(panel.saturationSlider()->value(), 0);
    QCOMPARE(panel.hueSlider()->value(), 0);
    QCOMPARE(panel.rotationComboBox()->currentIndex(), 0);
    QCOMPARE(panel.deinterlaceCheckBox()->isChecked(), false);
    QVERIFY(engine.m_resetVideoEqualizerCalls > 0);
}

void TestControlPanel::testAudioTabControls() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    // 10 EQ sliders
    auto eqSliders = panel.equalizerSliders();
    QCOMPARE(eqSliders.size(), 10);
    for (int i = 0; i < 10; ++i) {
        QVERIFY(eqSliders[i] != nullptr);
        QCOMPARE(eqSliders[i]->orientation(), Qt::Vertical);
    }

    // Adjust band 0 (31Hz) slider: range -24 to +24 (-12dB to +12dB with 0.5dB step)
    // When set to 10 (which is +5.0dB)
    eqSliders[0]->setValue(10);
    QCOMPARE(engine.equalizerBand(0), 5.0);

    // Reset EQ
    QVERIFY(panel.resetEqButton() != nullptr);
    panel.resetEqButton()->click();
    QCOMPARE(eqSliders[0]->value(), 0);
    QCOMPARE(engine.equalizerBand(0), 0.0);
    QVERIFY(engine.m_resetAudioEqualizerCalls > 0);

    // Normalizer checkbox
    QVERIFY(panel.normalizerCheckBox() != nullptr);
    panel.normalizerCheckBox()->setChecked(true);
    QCOMPARE(engine.isNormalizerEnabled(), true);

    // Audio Delay spinbox (-10.0 to 10.0)
    QVERIFY(panel.audioDelaySpinBox() != nullptr);
    panel.audioDelaySpinBox()->setValue(0.25);
    QCOMPARE(engine.audioDelay(), 0.25);

    // Audio Delay reset button
    QVERIFY(panel.audioDelayResetButton() != nullptr);
    panel.audioDelayResetButton()->click();
    QCOMPARE(panel.audioDelaySpinBox()->value(), 0.0);
    QCOMPARE(engine.audioDelay(), 0.0);
}

void TestControlPanel::testSubtitleTabControls() {
    MockPlaybackEngine engine;
    engine.m_tracks = {
        {1, "sub", "English", "en", "subrip", true, true},
        {2, "sub", "Spanish", "es", "subrip", false, false}
    };
    ui::ControlPanel panel(&engine);

    // Subtitle delay
    QVERIFY(panel.subtitleDelaySpinBox() != nullptr);
    panel.subtitleDelaySpinBox()->setValue(1.2);
    QCOMPARE(engine.subtitleDelay(), 1.2);

    // Quick minus/plus 0.5s buttons
    QVERIFY(panel.subtitleDelayMinusButton() != nullptr);
    QVERIFY(panel.subtitleDelayPlusButton() != nullptr);
    panel.subtitleDelayPlusButton()->click();
    QCOMPARE(panel.subtitleDelaySpinBox()->value(), 1.7);
    panel.subtitleDelayMinusButton()->click();
    QCOMPARE(panel.subtitleDelaySpinBox()->value(), 1.2);

    // Subtitle delay reset
    QVERIFY(panel.subtitleDelayResetButton() != nullptr);
    panel.subtitleDelayResetButton()->click();
    QCOMPARE(panel.subtitleDelaySpinBox()->value(), 0.0);
    QCOMPARE(engine.subtitleDelay(), 0.0);

    // Subtitle scale slider (50% to 200%, default 100%)
    QVERIFY(panel.subtitleScaleSlider() != nullptr);
    QCOMPARE(panel.subtitleScaleSlider()->minimum(), 50);
    QCOMPARE(panel.subtitleScaleSlider()->maximum(), 200);
    QCOMPARE(panel.subtitleScaleSlider()->value(), 100);
    QCOMPARE(panel.subtitleScaleLabel()->text(), QStringLiteral("100%"));

    panel.subtitleScaleSlider()->setValue(150);
    QCOMPARE(engine.subtitleScale(), 1.5);
    QCOMPARE(panel.subtitleScaleLabel()->text(), QStringLiteral("150%"));

    QVERIFY(panel.subtitleScaleResetButton() != nullptr);
    panel.subtitleScaleResetButton()->click();
    QCOMPARE(panel.subtitleScaleSlider()->value(), 100);
    QCOMPARE(engine.subtitleScale(), 1.0);

    // Subtitle track combo box
    QVERIFY(panel.subtitleTrackComboBox() != nullptr);
    QCOMPARE(panel.subtitleTrackComboBox()->count(), 2);
    panel.subtitleTrackComboBox()->setCurrentIndex(1);
    QCOMPARE(engine.m_selectedSubTrackId, 2);

    // Load subtitle button exists
    QVERIFY(panel.loadSubtitleButton() != nullptr);
}

void TestControlPanel::testPlaybackTabControls() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    // Speed slider (0.2x to 12.0x, default 1.0x)
    QVERIFY(panel.speedSlider() != nullptr);
    QCOMPARE(panel.speedLabel()->text(), QStringLiteral("1.0x"));

    // Changing speed slider
    panel.speedSlider()->setValue(15); // e.g. 1.5x (value in tenths or tenths)
    QCOMPARE(engine.speed(), 1.5);
    QCOMPARE(panel.speedLabel()->text(), QStringLiteral("1.5x"));

    // Reset speed
    QVERIFY(panel.speedResetButton() != nullptr);
    panel.speedResetButton()->click();
    QCOMPARE(panel.speedSlider()->value(), 10);
    QCOMPARE(engine.speed(), 1.0);
    QCOMPARE(panel.speedLabel()->text(), QStringLiteral("1.0x"));

    // Play/Pause button
    QVERIFY(panel.playPauseButton() != nullptr);
    panel.playPauseButton()->click();
    QCOMPARE(engine.m_togglePauseCalls, 1);
}

void TestControlPanel::testUpdateFromEngineWithoutFeedbackLoop() {
    MockPlaybackEngine engine;
    ui::ControlPanel panel(&engine);

    int initialBrightnessCalls = engine.m_setBrightnessCalls;
    int initialSpeedCalls = engine.m_setSpeedCalls;
    int initialAudioDelayCalls = engine.m_setAudioDelayCalls;
    int initialSubtitleDelayCalls = engine.m_setSubtitleDelayCalls;

    // Simulate external change in engine
    engine.m_brightness = 40;
    emit engine.videoEqualizerChanged();

    // Verify UI updated
    QCOMPARE(panel.brightnessSlider()->value(), 40);
    QCOMPARE(panel.brightnessLabel()->text(), QStringLiteral("40"));
    // Ensure engine was not called back recursively
    QCOMPARE(engine.m_setBrightnessCalls, initialBrightnessCalls);

    // Speed change in engine
    engine.m_speed = 2.0;
    emit engine.speedChanged(2.0);
    QCOMPARE(panel.speedSlider()->value(), 20);
    QCOMPARE(panel.speedLabel()->text(), QStringLiteral("2.0x"));
    QCOMPARE(engine.m_setSpeedCalls, initialSpeedCalls);

    // Audio delay change in engine
    engine.m_audioDelay = -0.5;
    emit engine.audioEqualizerChanged();
    QCOMPARE(panel.audioDelaySpinBox()->value(), -0.5);
    QCOMPARE(engine.m_setAudioDelayCalls, initialAudioDelayCalls);

    // Subtitle delay change in engine
    engine.m_subtitleDelay = 2.5;
    emit engine.subtitleSettingsChanged();
    QCOMPARE(panel.subtitleDelaySpinBox()->value(), 2.5);
    QCOMPARE(engine.m_setSubtitleDelayCalls, initialSubtitleDelayCalls);
}

QTEST_MAIN(TestControlPanel)
#include "test_control_panel.moc"
