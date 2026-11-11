// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>

namespace outplayer::engine {

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

signals:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void stateChanged(bool paused);
    void fileLoaded();
    void playbackFinished();
};

} // namespace outplayer::engine
