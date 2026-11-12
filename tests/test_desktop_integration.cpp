// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QMenu>
#include <QSignalSpy>
#include <QSystemTrayIcon>
#include <QTest>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>

#include "engine/IPlaybackEngine.h"
#include "playlist/PlaylistModel.h"
#include "ui/DesktopIntegration.h"

using namespace outplayer;

class MockPlaybackEngine : public engine::IPlaybackEngine {
    Q_OBJECT
public:
    explicit MockPlaybackEngine(QObject* parent = nullptr) : engine::IPlaybackEngine(parent) {}

    bool initialize() override { return true; }
    bool loadFile(const QString& url) override {
        m_loadedUrl = url;
        m_paused = false;
        emit fileLoaded();
        emit stateChanged(m_paused);
        return true;
    }
    void play() override {
        m_paused = false;
        emit stateChanged(m_paused);
    }
    void pause() override {
        m_paused = true;
        emit stateChanged(m_paused);
    }
    void togglePause() override {
        m_paused = !m_paused;
        m_togglePauseCount++;
        emit stateChanged(m_paused);
    }
    void stop() override {
        m_paused = true;
        m_stopCount++;
        emit playbackFinished();
        emit stateChanged(m_paused);
    }
    void seek(double seconds, bool exact = false) override {
        Q_UNUSED(exact);
        m_position = seconds;
        emit positionChanged(m_position);
    }
    [[nodiscard]] double position() const override { return m_position; }
    [[nodiscard]] double duration() const override { return m_duration; }
    [[nodiscard]] bool isPaused() const override { return m_paused; }

    [[nodiscard]] QList<engine::MediaTrack> tracks(const QString& type = {}) const override {
        Q_UNUSED(type);
        return {};
    }
    bool setTrack(const QString& type, int trackId) override {
        Q_UNUSED(type);
        Q_UNUSED(trackId);
        return false;
    }
    [[nodiscard]] double volume() const override { return m_volume; }
    void setVolume(double vol) override {
        m_volume = vol;
        emit volumeChanged(m_volume);
    }
    [[nodiscard]] bool isMuted() const override { return m_muted; }
    void setMuted(bool mute) override {
        m_muted = mute;
        emit muteChanged(m_muted);
    }
    [[nodiscard]] double speed() const override { return m_speed; }
    void setSpeed(double spd) override {
        m_speed = spd;
        emit speedChanged(spd);
    }
    [[nodiscard]] QString aspectRatio() const override { return QStringLiteral("auto"); }
    void setAspectRatio(const QString&) override {}
    [[nodiscard]] QList<engine::ChapterInfo> chapters() const override { return {}; }
    bool setChapter(int) override { return false; }

    [[nodiscard]] int brightness() const override { return 0; }
    void setBrightness(int) override {}
    [[nodiscard]] int contrast() const override { return 0; }
    void setContrast(int) override {}
    [[nodiscard]] int saturation() const override { return 0; }
    void setSaturation(int) override {}
    [[nodiscard]] int hue() const override { return 0; }
    void setHue(int) override {}
    [[nodiscard]] int rotation() const override { return 0; }
    void setRotation(int) override {}
    [[nodiscard]] bool deinterlace() const override { return false; }
    void setDeinterlace(bool) override {}
    void resetVideoEqualizer() override {}

    [[nodiscard]] double audioDelay() const override { return 0.0; }
    void setAudioDelay(double) override {}
    [[nodiscard]] double equalizerBand(int) const override { return 0.0; }
    void setEqualizerBand(int, double) override {}
    void resetAudioEqualizer() override {}
    [[nodiscard]] bool isNormalizerEnabled() const override { return false; }
    void setNormalizerEnabled(bool) override {}

    [[nodiscard]] double subtitleDelay() const override { return 0.0; }
    void setSubtitleDelay(double) override {}
    [[nodiscard]] double subtitleScale() const override { return 1.0; }
    void setSubtitleScale(double) override {}
    bool loadSubtitle(const QString&) override { return false; }
    virtual bool captureFrame(const QString&) override { return false; }

    QString m_loadedUrl;
    double m_position{0.0};
    double m_duration{180.0};
    bool m_paused{true};
    double m_volume{80.0};
    bool m_muted{false};
    double m_speed{1.0};
    int m_togglePauseCount{0};
    int m_stopCount{0};
};

class TestDesktopIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testSafeInitializationWithoutCrash();
    void testScreensaverInhibitExplicit();
    void testScreensaverInhibitEngineStateTransitions();
    void testMetadataUpdates();
    void testSystemTrayMenuActions();
    void testMprisControlMethodsAndSignals();
    void testMprisDbusRegistrationIfConnected();
};

void TestDesktopIntegration::initTestCase()
{
    // Ensure headless application is ready
}

void TestDesktopIntegration::testSafeInitializationWithoutCrash()
{
    // Construction with nullptr arguments must never crash or throw
    ui::DesktopIntegration integration(nullptr, nullptr, nullptr);
    QCOMPARE(integration.isScreenSaverInhibited(), false);
    // If D-Bus is unavailable, isMprisActive() is gracefully false. If available, true.
    bool active = integration.isMprisActive();
    Q_UNUSED(active);
}

void TestDesktopIntegration::testScreensaverInhibitExplicit()
{
    MockPlaybackEngine engine;
    ui::DesktopIntegration integration(&engine);

    QCOMPARE(integration.isScreenSaverInhibited(), false);

    integration.setScreenSaverInhibited(true);
    QCOMPARE(integration.isScreenSaverInhibited(), true);

    integration.setScreenSaverInhibited(false);
    QCOMPARE(integration.isScreenSaverInhibited(), false);
}

void TestDesktopIntegration::testScreensaverInhibitEngineStateTransitions()
{
    MockPlaybackEngine engine;
    ui::DesktopIntegration integration(&engine);

    QCOMPARE(integration.isScreenSaverInhibited(), false);

    // When engine switches to playing (paused = false), screensaver must be inhibited
    engine.play();
    QCOMPARE(integration.isScreenSaverInhibited(), true);

    // When paused, inhibit must be released
    engine.pause();
    QCOMPARE(integration.isScreenSaverInhibited(), false);

    // Resuming playback re-inhibits
    engine.play();
    QCOMPARE(integration.isScreenSaverInhibited(), true);

    // Playback finished releases inhibit
    engine.stop();
    QCOMPARE(integration.isScreenSaverInhibited(), false);
}

void TestDesktopIntegration::testMetadataUpdates()
{
    MockPlaybackEngine engine;
    ui::DesktopIntegration integration(&engine);

    integration.updateMetadata(QStringLiteral("Big Buck Bunny"), QStringLiteral("Blender Foundation"), 596.0);

    QVariantMap meta = integration.metadata();
    QCOMPARE(meta.value(QStringLiteral("xesam:title")).toString(), QStringLiteral("Big Buck Bunny"));
    QStringList artists = meta.value(QStringLiteral("xesam:artist")).toStringList();
    QVERIFY(artists.contains(QStringLiteral("Blender Foundation")));
    QCOMPARE(meta.value(QStringLiteral("mpris:length")).toLongLong(), 596000000LL);
}

void TestDesktopIntegration::testSystemTrayMenuActions()
{
    MockPlaybackEngine engine;
    playlist::PlaylistModel playlist;
    QWidget mainWindow;
    ui::DesktopIntegration integration(&engine, &playlist, &mainWindow);

    QMenu* menu = integration.trayMenu();
    QVERIFY(menu != nullptr);

    QList<QAction*> actions = menu->actions();
    QVERIFY(!actions.isEmpty());

    QAction* playPauseAct = nullptr;
    QAction* stopAct = nullptr;
    QAction* nextAct = nullptr;
    QAction* prevAct = nullptr;
    QAction* showHideAct = nullptr;
    QAction* prefAct = nullptr;
    QAction* exitAct = nullptr;

    for (auto* act : actions) {
        QString text = act->text().remove('&');
        if (text.startsWith("Play/Pause")) playPauseAct = act;
        else if (text.startsWith("Stop")) stopAct = act;
        else if (text.startsWith("Next")) nextAct = act;
        else if (text.startsWith("Previous")) prevAct = act;
        else if (text.startsWith("Show/Hide")) showHideAct = act;
        else if (text.startsWith("Preferences")) prefAct = act;
        else if (text.startsWith("Exit")) exitAct = act;
    }

    QVERIFY(playPauseAct != nullptr);
    QVERIFY(stopAct != nullptr);
    QVERIFY(nextAct != nullptr);
    QVERIFY(prevAct != nullptr);
    QVERIFY(showHideAct != nullptr);
    QVERIFY(prefAct != nullptr);
    QVERIFY(exitAct != nullptr);

    QSignalSpy playPauseSpy(&integration, &ui::DesktopIntegration::playPauseRequested);
    QSignalSpy stopSpy(&integration, &ui::DesktopIntegration::stopRequested);
    QSignalSpy nextSpy(&integration, &ui::DesktopIntegration::nextRequested);
    QSignalSpy prevSpy(&integration, &ui::DesktopIntegration::previousRequested);
    QSignalSpy raiseSpy(&integration, &ui::DesktopIntegration::raiseWindowRequested);

    playPauseAct->trigger();
    QCOMPARE(playPauseSpy.count(), 1);

    stopAct->trigger();
    QCOMPARE(stopSpy.count(), 1);

    nextAct->trigger();
    QCOMPARE(nextSpy.count(), 1);

    prevAct->trigger();
    QCOMPARE(prevSpy.count(), 1);

    mainWindow.hide();
    showHideAct->trigger();
    QCOMPARE(raiseSpy.count(), 1);
    QVERIFY(mainWindow.isVisible());
}

void TestDesktopIntegration::testMprisControlMethodsAndSignals()
{
    MockPlaybackEngine engine;
    playlist::PlaylistModel playlist;
    ui::DesktopIntegration integration(&engine, &playlist);

    QSignalSpy playPauseSpy(&integration, &ui::DesktopIntegration::playPauseRequested);
    QSignalSpy stopSpy(&integration, &ui::DesktopIntegration::stopRequested);
    QSignalSpy nextSpy(&integration, &ui::DesktopIntegration::nextRequested);
    QSignalSpy prevSpy(&integration, &ui::DesktopIntegration::previousRequested);
    QSignalSpy raiseSpy(&integration, &ui::DesktopIntegration::raiseWindowRequested);

    integration.playPause();
    QCOMPARE(playPauseSpy.count(), 1);
    QCOMPARE(engine.m_togglePauseCount, 1);

    integration.stop();
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(engine.m_stopCount, 1);

    integration.next();
    QCOMPARE(nextSpy.count(), 1);

    integration.previous();
    QCOMPARE(prevSpy.count(), 1);

    integration.raise();
    QCOMPARE(raiseSpy.count(), 1);

    integration.openUri(QStringLiteral("file:///tmp/test.mkv"));
    QCOMPARE(engine.m_loadedUrl, QStringLiteral("file:///tmp/test.mkv"));

    // Playback status reflects engine state
    engine.play();
    QCOMPARE(integration.playbackStatus(), QStringLiteral("Playing"));

    engine.pause();
    QCOMPARE(integration.playbackStatus(), QStringLiteral("Paused"));

    engine.stop();
    QCOMPARE(integration.playbackStatus(), QStringLiteral("Stopped"));
}

void TestDesktopIntegration::testMprisDbusRegistrationIfConnected()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        QSKIP("D-Bus session bus not connected");
    }

    MockPlaybackEngine engine;
    playlist::PlaylistModel playlist;
    ui::DesktopIntegration integration(&engine, &playlist);

    QVERIFY(integration.isMprisActive());

    QDBusConnection bus = QDBusConnection::sessionBus();
    QVERIFY(bus.isConnected());

    // Test root interface
    QDBusInterface rootIface(
        QStringLiteral("org.mpris.MediaPlayer2.outPlayer"),
        QStringLiteral("/org/mpris/MediaPlayer2"),
        QStringLiteral("org.mpris.MediaPlayer2"),
        bus
    );
    QVERIFY(rootIface.isValid());
    QCOMPARE(rootIface.property("Identity").toString(), QStringLiteral("outPlayer"));
    QCOMPARE(rootIface.property("CanQuit").toBool(), true);
    QCOMPARE(rootIface.property("CanRaise").toBool(), true);

    // Test player interface
    QDBusInterface playerIface(
        QStringLiteral("org.mpris.MediaPlayer2.outPlayer"),
        QStringLiteral("/org/mpris/MediaPlayer2"),
        QStringLiteral("org.mpris.MediaPlayer2.Player"),
        bus
    );
    QVERIFY(playerIface.isValid());

    QSignalSpy spy(&integration, &ui::DesktopIntegration::playPauseRequested);
    playerIface.call(QStringLiteral("PlayPause"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestDesktopIntegration)
#include "test_desktop_integration.moc"
