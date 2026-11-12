// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/DesktopIntegration.h"

#include <QAction>
#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusReply>
#include <QIcon>
#include <QPixmap>
#include <QWidget>

#include "engine/IPlaybackEngine.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"

namespace outplayer::ui {

// ============================================================================
// MprisRootAdaptor
// ============================================================================

MprisRootAdaptor::MprisRootAdaptor(DesktopIntegration* parent)
    : QDBusAbstractAdaptor(parent), m_integration(parent)
{
    setAutoRelaySignals(true);
}

bool MprisRootAdaptor::canQuit() const
{
    return true;
}

bool MprisRootAdaptor::canRaise() const
{
    return true;
}

bool MprisRootAdaptor::hasTrackList() const
{
    return false;
}

QString MprisRootAdaptor::identity() const
{
    return QStringLiteral("outPlayer");
}

QStringList MprisRootAdaptor::supportedUriSchemes() const
{
    return {QStringLiteral("file"), QStringLiteral("http"), QStringLiteral("https")};
}

QStringList MprisRootAdaptor::supportedMimeTypes() const
{
    return {QStringLiteral("video/*"), QStringLiteral("audio/*")};
}

void MprisRootAdaptor::Raise()
{
    if (m_integration) {
        m_integration->raise();
    }
}

void MprisRootAdaptor::Quit()
{
    if (m_integration) {
        m_integration->quit();
    }
}

// ============================================================================
// MprisPlayerAdaptor
// ============================================================================

MprisPlayerAdaptor::MprisPlayerAdaptor(DesktopIntegration* parent)
    : QDBusAbstractAdaptor(parent), m_integration(parent)
{
    setAutoRelaySignals(true);
}

QString MprisPlayerAdaptor::playbackStatus() const
{
    return m_integration ? m_integration->playbackStatus() : QStringLiteral("Stopped");
}

double MprisPlayerAdaptor::rate() const
{
    return m_integration ? m_integration->rate() : 1.0;
}

void MprisPlayerAdaptor::setRate(double r)
{
    if (m_integration) {
        m_integration->setRate(r);
    }
}

QVariantMap MprisPlayerAdaptor::metadata() const
{
    return m_integration ? m_integration->metadata() : QVariantMap{};
}

double MprisPlayerAdaptor::volume() const
{
    return m_integration ? m_integration->volume() : 1.0;
}

void MprisPlayerAdaptor::setVolume(double v)
{
    if (m_integration) {
        m_integration->setVolume(v);
    }
}

qlonglong MprisPlayerAdaptor::position() const
{
    return m_integration ? m_integration->position() : 0;
}

double MprisPlayerAdaptor::minimumRate() const
{
    return 0.1;
}

double MprisPlayerAdaptor::maximumRate() const
{
    return 16.0;
}

bool MprisPlayerAdaptor::canGoNext() const
{
    return m_integration ? m_integration->canGoNext() : false;
}

bool MprisPlayerAdaptor::canGoPrevious() const
{
    return m_integration ? m_integration->canGoPrevious() : false;
}

bool MprisPlayerAdaptor::canPlay() const
{
    return true;
}

bool MprisPlayerAdaptor::canPause() const
{
    return true;
}

bool MprisPlayerAdaptor::canSeek() const
{
    return true;
}

bool MprisPlayerAdaptor::canControl() const
{
    return true;
}

void MprisPlayerAdaptor::Next()
{
    if (m_integration) {
        m_integration->next();
    }
}

void MprisPlayerAdaptor::Previous()
{
    if (m_integration) {
        m_integration->previous();
    }
}

void MprisPlayerAdaptor::Pause()
{
    if (m_integration) {
        m_integration->pause();
    }
}

void MprisPlayerAdaptor::PlayPause()
{
    if (m_integration) {
        m_integration->playPause();
    }
}

void MprisPlayerAdaptor::Stop()
{
    if (m_integration) {
        m_integration->stop();
    }
}

void MprisPlayerAdaptor::Play()
{
    if (m_integration) {
        m_integration->play();
    }
}

void MprisPlayerAdaptor::Seek(qlonglong Offset)
{
    if (m_integration) {
        m_integration->seek(Offset);
    }
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath& TrackId, qlonglong Position)
{
    if (m_integration) {
        m_integration->setPosition(TrackId, Position);
    }
}

void MprisPlayerAdaptor::OpenUri(const QString& Uri)
{
    if (m_integration) {
        m_integration->openUri(Uri);
    }
}

// ============================================================================
// DesktopIntegration
// ============================================================================

DesktopIntegration::DesktopIntegration(engine::IPlaybackEngine* engine,
                                     playlist::PlaylistModel* playlist,
                                     QWidget* mainWindow,
                                     QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_playlist(playlist)
    , m_mainWindow(mainWindow)
{
    if (m_engine) {
        connect(m_engine, &engine::IPlaybackEngine::stateChanged, this, &DesktopIntegration::onEngineStateChanged);
        connect(m_engine, &engine::IPlaybackEngine::playbackFinished, this, &DesktopIntegration::onEnginePlaybackFinished);
        connect(m_engine, &engine::IPlaybackEngine::fileLoaded, this, &DesktopIntegration::onEngineFileLoaded);
        connect(m_engine, &engine::IPlaybackEngine::volumeChanged, this, &DesktopIntegration::onEngineVolumeChanged);

        if (!m_engine->isPaused()) {
            m_hasActiveMedia = true;
            setScreenSaverInhibited(true);
        }
    }

    if (m_playlist) {
        connect(m_playlist, &playlist::PlaylistModel::currentItemChanged, this,
                [this](int, const playlist::PlaylistItem& item) {
                    updateMetadata(item.title, QString(), item.duration);
                });
    }

    setupTray();
    registerMpris();
}

DesktopIntegration::~DesktopIntegration()
{
    if (m_screenSaverInhibited) {
        setScreenSaverInhibited(false);
    }
    unregisterMpris();
    delete m_trayMenu;
}

bool DesktopIntegration::isMprisActive() const
{
    return m_mprisActive;
}

bool DesktopIntegration::isScreenSaverInhibited() const
{
    return m_screenSaverInhibited;
}

bool DesktopIntegration::isSystemTrayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

QSystemTrayIcon* DesktopIntegration::trayIcon() const
{
    return m_trayIcon;
}

QMenu* DesktopIntegration::trayMenu() const
{
    return m_trayMenu;
}

void DesktopIntegration::setupTray()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayMenu = new QMenu();

    m_playPauseAction = m_trayMenu->addAction(tr("&Play/Pause"), this, [this]() {
        playPause();
    });

    m_stopAction = m_trayMenu->addAction(tr("&Stop"), this, [this]() {
        stop();
    });

    m_nextAction = m_trayMenu->addAction(tr("&Next"), this, [this]() {
        next();
    });

    m_prevAction = m_trayMenu->addAction(tr("Pre&vious"), this, [this]() {
        previous();
    });

    m_trayMenu->addSeparator();

    m_showHideAction = m_trayMenu->addAction(tr("Show/Hide Window"), this, [this]() {
        if (m_mainWindow) {
            if (m_mainWindow->isVisible()) {
                m_mainWindow->hide();
            } else {
                if (m_mainWindow->isMinimized()) {
                    m_mainWindow->showNormal();
                }
                m_mainWindow->show();
                m_mainWindow->raise();
                m_mainWindow->activateWindow();
                emit raiseWindowRequested();
            }
        } else {
            emit raiseWindowRequested();
        }
    });

    m_preferencesAction = m_trayMenu->addAction(tr("Preferences..."), this, [this]() {
        emit preferencesRequested();
    });

    m_trayMenu->addSeparator();

    m_exitAction = m_trayMenu->addAction(tr("Exit"), this, [this]() {
        quit();
    });

    m_trayIcon->setContextMenu(m_trayMenu);

    QIcon appIcon = QIcon::fromTheme(QStringLiteral("applications-multimedia"), QIcon());
    if (appIcon.isNull()) {
        QPixmap pix(16, 16);
        pix.fill(Qt::darkCyan);
        appIcon = QIcon(pix);
    }
    m_trayIcon->setIcon(appIcon);
    m_trayIcon->setToolTip(QStringLiteral("outPlayer"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            raise();
        }
    });

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon->show();
    }
}

void DesktopIntegration::registerMpris()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        m_mprisActive = false;
        return;
    }

    new MprisRootAdaptor(this);
    new MprisPlayerAdaptor(this);

    QString baseServiceName = QStringLiteral("org.mpris.MediaPlayer2.outPlayer");
    QString serviceName = baseServiceName;

    bool ok = bus.registerService(serviceName);
    if (!ok) {
        serviceName = QStringLiteral("org.mpris.MediaPlayer2.outPlayer.instance%1").arg(QCoreApplication::applicationPid());
        ok = bus.registerService(serviceName);
    }

    if (ok) {
        bool objOk = bus.registerObject(
            QStringLiteral("/org/mpris/MediaPlayer2"),
            this,
            QDBusConnection::ExportAdaptors
        );
        if (objOk) {
            m_registeredServiceName = serviceName;
            m_mprisActive = true;
        } else {
            bus.unregisterService(serviceName);
            m_mprisActive = false;
        }
    } else {
        m_mprisActive = false;
    }
}

void DesktopIntegration::unregisterMpris()
{
    if (!m_registeredServiceName.isEmpty()) {
        auto bus = QDBusConnection::sessionBus();
        if (bus.isConnected()) {
            bus.unregisterObject(QStringLiteral("/org/mpris/MediaPlayer2"));
            bus.unregisterService(m_registeredServiceName);
        }
        m_registeredServiceName.clear();
        m_mprisActive = false;
    }
}

void DesktopIntegration::setScreenSaverInhibited(bool inhibit)
{
    if (m_screenSaverInhibited == inhibit) {
        return;
    }

    m_screenSaverInhibited = inhibit;

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    if (inhibit) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.ScreenSaver"),
            QStringLiteral("/org/freedesktop/ScreenSaver"),
            QStringLiteral("org.freedesktop.ScreenSaver"),
            QStringLiteral("Inhibit"));
        msg << QStringLiteral("outPlayer") << QStringLiteral("Playing video");
        QDBusMessage reply = bus.call(msg);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            m_screensaverCookie = reply.arguments().at(0).toUInt();
        }
    } else {
        if (m_screensaverCookie != 0) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QStringLiteral("org.freedesktop.ScreenSaver"),
                QStringLiteral("/org/freedesktop/ScreenSaver"),
                QStringLiteral("org.freedesktop.ScreenSaver"),
                QStringLiteral("UnInhibit"));
            msg << m_screensaverCookie;
            bus.call(msg, QDBus::NoBlock);
            m_screensaverCookie = 0;
        }
    }
}

void DesktopIntegration::updateMetadata(const QString& title, const QString& artist, double duration)
{
    m_currentTitle = title;
    m_currentArtist = artist;
    m_currentDuration = duration;
    m_hasActiveMedia = true;

    QVariantMap changed;
    changed.insert(QStringLiteral("Metadata"), metadata());
    notifyPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void DesktopIntegration::onEngineStateChanged(bool paused)
{
    if (paused) {
        setScreenSaverInhibited(false);
    } else {
        m_hasActiveMedia = true;
        setScreenSaverInhibited(true);
    }

    QVariantMap changed;
    changed.insert(QStringLiteral("PlaybackStatus"), playbackStatus());
    notifyPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void DesktopIntegration::onEnginePlaybackFinished()
{
    m_hasActiveMedia = false;
    setScreenSaverInhibited(false);

    QVariantMap changed;
    changed.insert(QStringLiteral("PlaybackStatus"), playbackStatus());
    notifyPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void DesktopIntegration::onEngineFileLoaded()
{
    m_hasActiveMedia = true;
    if (m_engine && !m_engine->isPaused()) {
        setScreenSaverInhibited(true);
    }

    QVariantMap changed;
    changed.insert(QStringLiteral("PlaybackStatus"), playbackStatus());
    changed.insert(QStringLiteral("Metadata"), metadata());
    notifyPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void DesktopIntegration::onEngineVolumeChanged(double vol)
{
    QVariantMap changed;
    changed.insert(QStringLiteral("Volume"), vol / 100.0);
    notifyPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void DesktopIntegration::notifyPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties)
{
    if (!m_mprisActive) {
        return;
    }
    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/org/mpris/MediaPlayer2"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged")
    );
    signal << interfaceName << changedProperties << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

void DesktopIntegration::play()
{
    emit playPauseRequested();
    if (m_engine) {
        m_engine->play();
    }
}

void DesktopIntegration::pause()
{
    emit playPauseRequested();
    if (m_engine) {
        m_engine->pause();
    }
}

void DesktopIntegration::playPause()
{
    emit playPauseRequested();
    if (m_engine) {
        m_engine->togglePause();
    }
}

void DesktopIntegration::stop()
{
    emit stopRequested();
    if (m_engine) {
        m_engine->stop();
    }
}

void DesktopIntegration::next()
{
    emit nextRequested();
}

void DesktopIntegration::previous()
{
    emit previousRequested();
}

void DesktopIntegration::seek(qlonglong offsetMicroseconds)
{
    if (m_engine) {
        double currentPos = m_engine->position();
        double offsetSec = offsetMicroseconds / 1'000'000.0;
        m_engine->seek(currentPos + offsetSec);
    }
}

void DesktopIntegration::setPosition(const QDBusObjectPath& trackId, qlonglong positionMicroseconds)
{
    Q_UNUSED(trackId);
    if (m_engine) {
        double posSec = positionMicroseconds / 1'000'000.0;
        m_engine->seek(posSec, true);
    }
}

void DesktopIntegration::openUri(const QString& uri)
{
    if (m_engine) {
        m_engine->loadFile(uri);
    }
}

void DesktopIntegration::raise()
{
    if (m_mainWindow) {
        if (m_mainWindow->isMinimized()) {
            m_mainWindow->showNormal();
        }
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
    emit raiseWindowRequested();
}

void DesktopIntegration::quit()
{
    QCoreApplication::quit();
}

QString DesktopIntegration::playbackStatus() const
{
    if (!m_hasActiveMedia) {
        return QStringLiteral("Stopped");
    }
    if (m_engine) {
        if (m_engine->isPaused()) {
            return QStringLiteral("Paused");
        }
        return QStringLiteral("Playing");
    }
    return QStringLiteral("Stopped");
}

double DesktopIntegration::rate() const
{
    return m_engine ? m_engine->speed() : 1.0;
}

void DesktopIntegration::setRate(double rate)
{
    if (m_engine) {
        m_engine->setSpeed(rate);
    }
}

QVariantMap DesktopIntegration::metadata() const
{
    QVariantMap meta;
    meta.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/0"))));

    QString title = m_currentTitle;
    QString artist = m_currentArtist;
    double duration = m_currentDuration;

    if (title.isEmpty() && m_playlist) {
        auto current = m_playlist->currentItem();
        if (current.has_value()) {
            title = current->title;
            if (duration <= 0.0) {
                duration = current->duration;
            }
        }
    }

    if (duration <= 0.0 && m_engine) {
        duration = m_engine->duration();
    }

    if (!title.isEmpty()) {
        meta.insert(QStringLiteral("xesam:title"), title);
    }
    if (!artist.isEmpty()) {
        meta.insert(QStringLiteral("xesam:artist"), QStringList{artist});
    }
    if (duration > 0.0) {
        meta.insert(QStringLiteral("mpris:length"), static_cast<qlonglong>(duration * 1'000'000.0));
    }

    return meta;
}

double DesktopIntegration::volume() const
{
    return m_engine ? (m_engine->volume() / 100.0) : 1.0;
}

void DesktopIntegration::setVolume(double volume)
{
    if (m_engine) {
        m_engine->setVolume(volume * 100.0);
    }
}

qlonglong DesktopIntegration::position() const
{
    return m_engine ? static_cast<qlonglong>(m_engine->position() * 1'000'000.0) : 0;
}

bool DesktopIntegration::canGoNext() const
{
    return m_playlist ? (m_playlist->nextIndex() >= 0) : false;
}

bool DesktopIntegration::canGoPrevious() const
{
    return m_playlist ? (m_playlist->previousIndex() >= 0) : false;
}

} // namespace outplayer::ui
