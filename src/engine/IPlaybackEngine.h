// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QObject>
#include <QString>

namespace outplayer::engine {

struct MediaTrack {
    int id{-1};
    QString type; // "video", "audio", "sub"
    QString title;
    QString language;
    QString codec;
    bool selected{false};
    bool isDefault{false};

    bool operator==(const MediaTrack&) const = default;
};

struct ChapterInfo {
    int index{0};
    QString title;
    double time{0.0};

    bool operator==(const ChapterInfo&) const = default;
};

class IPlaybackEngine : public QObject {
    Q_OBJECT

public:
    explicit IPlaybackEngine(QObject* parent = nullptr) : QObject(parent) {}
    ~IPlaybackEngine() override = default;

    virtual bool initialize() = 0;
    virtual bool loadFile(const QString& url) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void togglePause() = 0;
    virtual void stop() = 0;
    virtual void seek(double seconds, bool exact = false) = 0;
    virtual void frameStep() {}
    virtual void frameBackStep() {}
    [[nodiscard]] virtual double position() const = 0;
    [[nodiscard]] virtual double duration() const = 0;
    [[nodiscard]] virtual bool isPaused() const = 0;

    [[nodiscard]] virtual QList<MediaTrack> tracks(const QString& type = {}) const = 0;
    virtual bool setTrack(const QString& type, int trackId) = 0;
    [[nodiscard]] virtual double volume() const = 0;
    virtual void setVolume(double vol) = 0;
    [[nodiscard]] virtual bool isMuted() const = 0;
    virtual void setMuted(bool mute) = 0;
    [[nodiscard]] virtual double speed() const = 0;
    virtual void setSpeed(double spd) = 0;
    [[nodiscard]] virtual QString aspectRatio() const = 0;
    virtual void setAspectRatio(const QString& ar) = 0;
    [[nodiscard]] virtual QList<ChapterInfo> chapters() const = 0;
    virtual bool setChapter(int chapterIndex) = 0;

    // Video Equalizer
    [[nodiscard]] virtual int brightness() const = 0;
    virtual void setBrightness(int val) = 0;
    [[nodiscard]] virtual int contrast() const = 0;
    virtual void setContrast(int val) = 0;
    [[nodiscard]] virtual int saturation() const = 0;
    virtual void setSaturation(int val) = 0;
    [[nodiscard]] virtual int hue() const = 0;
    virtual void setHue(int val) = 0;
    [[nodiscard]] virtual int rotation() const = 0;
    virtual void setRotation(int degrees) = 0;
    [[nodiscard]] virtual bool deinterlace() const = 0;
    virtual void setDeinterlace(bool enable) = 0;
    virtual void resetVideoEqualizer() = 0;

    // Audio Equalizer & Sync
    [[nodiscard]] virtual double audioDelay() const = 0;
    virtual void setAudioDelay(double seconds) = 0;
    [[nodiscard]] virtual double equalizerBand(int bandIndex) const = 0;
    virtual void setEqualizerBand(int bandIndex, double gainDb) = 0;
    virtual void resetAudioEqualizer() = 0;
    [[nodiscard]] virtual bool isNormalizerEnabled() const = 0;
    virtual void setNormalizerEnabled(bool enable) = 0;

    // Subtitle Controls & Sync
    [[nodiscard]] virtual double subtitleDelay() const = 0;
    virtual void setSubtitleDelay(double seconds) = 0;
    [[nodiscard]] virtual double subtitleScale() const = 0;
    virtual void setSubtitleScale(double scale) = 0;
    virtual bool loadSubtitle(const QString& filePath) = 0;

    // Frame Capture
    virtual bool captureFrame(const QString& destinationPath) = 0;

signals:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void stateChanged(bool paused);
    void fileLoaded();
    void playbackFinished();
    void tracksChanged();
    void volumeChanged(double vol);
    void muteChanged(bool muted);
    void speedChanged(double spd);
    void chaptersChanged();
    void videoEqualizerChanged();
    void audioEqualizerChanged();
    void subtitleSettingsChanged();
};

} // namespace outplayer::engine
