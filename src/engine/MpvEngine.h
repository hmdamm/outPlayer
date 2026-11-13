// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "engine/IPlaybackEngine.h"

#include <mpv/client.h>
#include <array>

namespace outplayer::engine {

class MpvEngine : public IPlaybackEngine {
    Q_OBJECT

public:
    explicit MpvEngine(QObject* parent = nullptr);
    ~MpvEngine() override;

    MpvEngine(const MpvEngine&) = delete;
    MpvEngine& operator=(const MpvEngine&) = delete;
    MpvEngine(MpvEngine&&) = delete;
    MpvEngine& operator=(MpvEngine&&) = delete;

    bool initialize() override;
    bool loadFile(const QString& url) override;
    void play() override;
    void pause() override;
    void togglePause() override;
    void stop() override;
    void seek(double seconds, bool exact = false) override;
    void frameStep() override;
    void frameBackStep() override;
    [[nodiscard]] double position() const override;
    [[nodiscard]] double duration() const override;
    [[nodiscard]] bool isPaused() const override;

    [[nodiscard]] QList<MediaTrack> tracks(const QString& type = {}) const override;
    bool setTrack(const QString& type, int trackId) override;
    [[nodiscard]] double volume() const override;
    void setVolume(double vol) override;
    [[nodiscard]] bool isMuted() const override;
    void setMuted(bool mute) override;
    [[nodiscard]] double speed() const override;
    void setSpeed(double spd) override;
    [[nodiscard]] QString aspectRatio() const override;
    void setAspectRatio(const QString& ar) override;
    [[nodiscard]] QList<ChapterInfo> chapters() const override;
    bool setChapter(int chapterIndex) override;

    // Video Equalizer
    [[nodiscard]] int brightness() const override;
    void setBrightness(int val) override;
    [[nodiscard]] int contrast() const override;
    void setContrast(int val) override;
    [[nodiscard]] int saturation() const override;
    void setSaturation(int val) override;
    [[nodiscard]] int hue() const override;
    void setHue(int val) override;
    [[nodiscard]] int rotation() const override;
    void setRotation(int degrees) override;
    [[nodiscard]] bool deinterlace() const override;
    void setDeinterlace(bool enable) override;
    void resetVideoEqualizer() override;

    // Audio Equalizer & Sync
    [[nodiscard]] double audioDelay() const override;
    void setAudioDelay(double seconds) override;
    [[nodiscard]] double equalizerBand(int bandIndex) const override;
    void setEqualizerBand(int bandIndex, double gainDb) override;
    void resetAudioEqualizer() override;
    [[nodiscard]] bool isNormalizerEnabled() const override;
    void setNormalizerEnabled(bool enable) override;

    // Subtitle Controls & Sync
    [[nodiscard]] double subtitleDelay() const override;
    void setSubtitleDelay(double seconds) override;
    [[nodiscard]] double subtitleScale() const override;
    void setSubtitleScale(double scale) override;
    bool loadSubtitle(const QString& filePath) override;

    // Frame Capture
    bool captureFrame(const QString& destinationPath) override;

    [[nodiscard]] mpv_handle* mpvHandle() const noexcept { return m_mpv; }

    bool setProperty(const QString& name, const QString& value);
    [[nodiscard]] QString getPropertyString(const QString& name) const;
    void setVo(const QString& vo);

public slots:
    void handleMpvEvents();

private:
    static void onMpvWakeup(void* ctx);
    void processEvent(mpv_event* event);
    void updateAudioFilters();

    mpv_handle* m_mpv{nullptr};
    double m_position{0.0};
    double m_duration{0.0};
    bool m_paused{false};
    double m_volume{100.0};
    bool m_muted{false};
    double m_speed{1.0};
    QString m_aspectRatio;
    QList<MediaTrack> m_tracks;
    QList<ChapterInfo> m_chapters;

    int m_brightness{0};
    int m_contrast{0};
    int m_saturation{0};
    int m_hue{0};
    int m_rotation{0};
    bool m_deinterlace{false};

    double m_audioDelay{0.0};
    std::array<double, 10> m_eqBands{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    bool m_normalizerEnabled{false};

    double m_subtitleDelay{0.0};
    double m_subtitleScale{1.0};
};

} // namespace outplayer::engine
