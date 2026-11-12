// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "engine/IPlaybackEngine.h"

#include <mpv/client.h>

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

    [[nodiscard]] mpv_handle* mpvHandle() const noexcept { return m_mpv; }

    bool setProperty(const QString& name, const QString& value);
    [[nodiscard]] QString getPropertyString(const QString& name) const;
    void setVo(const QString& vo);

public slots:
    void handleMpvEvents();

private:
    static void onMpvWakeup(void* ctx);
    void processEvent(mpv_event* event);

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
};

} // namespace outplayer::engine
