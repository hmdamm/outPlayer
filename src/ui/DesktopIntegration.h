// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QVariantMap>

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::playlist {
class PlaylistModel;
}

class QWidget;

namespace outplayer::ui {

class DesktopIntegration;

class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
    explicit MprisRootAdaptor(DesktopIntegration* parent);

    [[nodiscard]] bool canQuit() const;
    [[nodiscard]] bool canRaise() const;
    [[nodiscard]] bool hasTrackList() const;
    [[nodiscard]] QString identity() const;
    [[nodiscard]] QStringList supportedUriSchemes() const;
    [[nodiscard]] QStringList supportedMimeTypes() const;

public slots:
    void Raise();
    void Quit();

private:
    DesktopIntegration* m_integration{nullptr};
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

public:
    explicit MprisPlayerAdaptor(DesktopIntegration* parent);

    [[nodiscard]] QString playbackStatus() const;
    [[nodiscard]] double rate() const;
    void setRate(double r);
    [[nodiscard]] QVariantMap metadata() const;
    [[nodiscard]] double volume() const;
    void setVolume(double v);
    [[nodiscard]] qlonglong position() const;
    [[nodiscard]] double minimumRate() const;
    [[nodiscard]] double maximumRate() const;
    [[nodiscard]] bool canGoNext() const;
    [[nodiscard]] bool canGoPrevious() const;
    [[nodiscard]] bool canPlay() const;
    [[nodiscard]] bool canPause() const;
    [[nodiscard]] bool canSeek() const;
    [[nodiscard]] bool canControl() const;

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath& TrackId, qlonglong Position);
    void OpenUri(const QString& Uri);

signals:
    void Seeked(qlonglong Position);

private:
    DesktopIntegration* m_integration{nullptr};
};

class DesktopIntegration : public QObject {
    Q_OBJECT

public:
    explicit DesktopIntegration(engine::IPlaybackEngine* engine,
                               playlist::PlaylistModel* playlist = nullptr,
                               QWidget* mainWindow = nullptr,
                               QObject* parent = nullptr);
    ~DesktopIntegration() override;

    [[nodiscard]] bool isMprisActive() const;
    [[nodiscard]] bool isScreenSaverInhibited() const;
    [[nodiscard]] bool isSystemTrayAvailable() const;

    void updateMetadata(const QString& title, const QString& artist, double duration);
    void setScreenSaverInhibited(bool inhibit);

    // MPRIS & Control methods
    void play();
    void pause();
    void playPause();
    void stop();
    void next();
    void previous();
    void seek(qlonglong offsetMicroseconds);
    void setPosition(const QDBusObjectPath& trackId, qlonglong positionMicroseconds);
    void openUri(const QString& uri);
    void raise();
    void quit();

    [[nodiscard]] QString playbackStatus() const;
    [[nodiscard]] double rate() const;
    void setRate(double rate);
    [[nodiscard]] QVariantMap metadata() const;
    [[nodiscard]] double volume() const;
    void setVolume(double volume);
    [[nodiscard]] qlonglong position() const;
    [[nodiscard]] bool canGoNext() const;
    [[nodiscard]] bool canGoPrevious() const;

    [[nodiscard]] QSystemTrayIcon* trayIcon() const;
    [[nodiscard]] QMenu* trayMenu() const;

signals:
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void raiseWindowRequested();
    void preferencesRequested();

private slots:
    void onEngineStateChanged(bool paused);
    void onEnginePlaybackFinished();
    void onEngineFileLoaded();
    void onEngineVolumeChanged(double vol);

private:
    void setupTray();
    void registerMpris();
    void unregisterMpris();
    void notifyPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties);

    engine::IPlaybackEngine* m_engine{nullptr};
    playlist::PlaylistModel* m_playlist{nullptr};
    QWidget* m_mainWindow{nullptr};

    bool m_mprisActive{false};
    QString m_registeredServiceName;

    bool m_screenSaverInhibited{false};
    uint m_screensaverCookie{0};

    QString m_currentTitle;
    QString m_currentArtist;
    double m_currentDuration{0.0};
    bool m_hasActiveMedia{false};

    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_trayMenu{nullptr};
    QAction* m_playPauseAction{nullptr};
    QAction* m_stopAction{nullptr};
    QAction* m_nextAction{nullptr};
    QAction* m_prevAction{nullptr};
    QAction* m_showHideAction{nullptr};
    QAction* m_preferencesAction{nullptr};
    QAction* m_exitAction{nullptr};
};

} // namespace outplayer::ui
