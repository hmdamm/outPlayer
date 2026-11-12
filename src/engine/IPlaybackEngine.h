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
};

} // namespace outplayer::engine
