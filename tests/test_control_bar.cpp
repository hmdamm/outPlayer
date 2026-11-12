// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "engine/IPlaybackEngine.h"
#include "ui/ControlBar.h"
#include "ui/TimelineSlider.h"

using namespace outplayer;

class MockPlaybackEngine : public engine::IPlaybackEngine {
    Q_OBJECT
public:
    explicit MockPlaybackEngine(QObject* parent = nullptr) : engine::IPlaybackEngine(parent) {}

    bool initialize() override { return true; }
    bool loadFile(const QString&) override { return true; }
    void play() override { m_paused = false; emit stateChanged(m_paused); }
    void pause() override { m_paused = true; emit stateChanged(m_paused); }
    void togglePause() override { m_paused = !m_paused; emit stateChanged(m_paused); }
    void stop() override { m_paused = true; emit stateChanged(m_paused); }
    void seek(double seconds, bool) override { m_position = seconds; emit positionChanged(m_position); }
    [[nodiscard]] double position() const override { return m_position; }
    [[nodiscard]] double duration() const override { return m_duration; }
    [[nodiscard]] bool isPaused() const override { return m_paused; }

    [[nodiscard]] QList<engine::MediaTrack> tracks(const QString&) const override { return {}; }
    bool setTrack(const QString&, int) override { return true; }
    [[nodiscard]] double volume() const override { return m_volume; }
    void setVolume(double vol) override { m_volume = vol; emit volumeChanged(m_volume); }
    [[nodiscard]] bool isMuted() const override { return m_muted; }
    void setMuted(bool mute) override { m_muted = mute; emit muteChanged(m_muted); }
    [[nodiscard]] double speed() const override { return 1.0; }
    void setSpeed(double) override {}
    [[nodiscard]] QString aspectRatio() const override { return QStringLiteral("auto"); }
    void setAspectRatio(const QString&) override {}
    [[nodiscard]] QList<engine::ChapterInfo> chapters() const override { return m_chapters; }
    bool setChapter(int) override { return true; }

    void simulatePositionChange(double pos) {
        m_position = pos;
        emit positionChanged(pos);
    }

    void simulateDurationChange(double dur) {
        m_duration = dur;
        emit durationChanged(dur);
    }

    void simulateStateChange(bool paused) {
        m_paused = paused;
        emit stateChanged(paused);
    }

    void simulateVolumeChange(double vol) {
        m_volume = vol;
        emit volumeChanged(vol);
    }

    void simulateMuteChange(bool muted) {
        m_muted = muted;
        emit muteChanged(muted);
    }

private:
    double m_position{0.0};
    double m_duration{0.0};
    double m_volume{100.0};
    bool m_paused{true};
    bool m_muted{false};
    QList<engine::ChapterInfo> m_chapters;
};

class TestControlBar : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void testDimensionsAndDefaults();
    void testSetTimeUpdatesLabelAndSlider();
    void testPlayPauseToggle();
    void testButtonSignalsAndCommandRegistryDispatch();
    void testVolumeControls();
    void testPlaybackEngineSync();
};

void TestControlBar::initTestCase()
{
}

void TestControlBar::cleanup()
{
    core::CommandRegistry::instance().clearHandlers();
}

void TestControlBar::testDimensionsAndDefaults()
{
    ui::ControlBar bar;
    QCOMPARE(bar.height(), 40);
    QCOMPARE(bar.minimumHeight(), 40);
    QCOMPARE(bar.maximumHeight(), 40);
    QCOMPARE(bar.sizeHint().height(), 40);

    QVERIFY(bar.timelineSlider() != nullptr);
    QVERIFY(bar.timeLabel() != nullptr);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("00:00 / 00:00"));

    QVERIFY(!bar.isPlaying());
    QVERIFY(!bar.isMuted());
    QCOMPARE(bar.volume(), 100.0);
}

void TestControlBar::testSetTimeUpdatesLabelAndSlider()
{
    ui::ControlBar bar;

    bar.setTime(75.0, 180.0);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("01:15 / 03:00"));
    QCOMPARE(bar.timelineSlider()->currentSeconds(), 75.0);
    QCOMPARE(bar.timelineSlider()->durationSeconds(), 180.0);

    bar.setTime(3665.0, 7200.0);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("01:01:05 / 02:00:00"));
    QCOMPARE(bar.timelineSlider()->currentSeconds(), 3665.0);
    QCOMPARE(bar.timelineSlider()->durationSeconds(), 7200.0);
}

void TestControlBar::testPlayPauseToggle()
{
    ui::ControlBar bar;
    QVERIFY(!bar.isPlaying());
    QCOMPARE(bar.playPauseButton()->text(), QStringLiteral("▶"));

    bar.setPlaying(true);
    QVERIFY(bar.isPlaying());
    QCOMPARE(bar.playPauseButton()->text(), QStringLiteral("❚❚"));

    bar.setPlaying(false);
    QVERIFY(!bar.isPlaying());
    QCOMPARE(bar.playPauseButton()->text(), QStringLiteral("▶"));
}

void TestControlBar::testButtonSignalsAndCommandRegistryDispatch()
{
    ui::ControlBar bar;
    auto& registry = core::CommandRegistry::instance();

    bool playPauseDispatched = false;
    bool stopDispatched = false;
    bool prevDispatched = false;
    bool nextDispatched = false;
    bool muteDispatched = false;
    bool subDispatched = false;
    bool playlistDispatched = false;
    bool cpDispatched = false;

    registry.registerHandler(core::CommandId::CMD_PLAY_PAUSE, [&](const QVariantMap&) {
        playPauseDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_PLAY_STOP, [&](const QVariantMap&) {
        stopDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_PLAYLIST_PREV, [&](const QVariantMap&) {
        prevDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_PLAYLIST_NEXT, [&](const QVariantMap&) {
        nextDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_VOLUME_MUTE, [&](const QVariantMap&) {
        muteDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_SUB_TOGGLE_VISIBILITY, [&](const QVariantMap&) {
        subDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_PLAYLIST, [&](const QVariantMap&) {
        playlistDispatched = true;
    });
    registry.registerHandler(core::CommandId::CMD_CONTROL_PANEL, [&](const QVariantMap&) {
        cpDispatched = true;
    });

    QSignalSpy spyPlayPause(&bar, &ui::ControlBar::playPauseClicked);
    QSignalSpy spyStop(&bar, &ui::ControlBar::stopClicked);
    QSignalSpy spyPrev(&bar, &ui::ControlBar::prevClicked);
    QSignalSpy spyNext(&bar, &ui::ControlBar::nextClicked);
    QSignalSpy spyMute(&bar, &ui::ControlBar::muteToggled);
    QSignalSpy spySub(&bar, &ui::ControlBar::subtitleToggled);
    QSignalSpy spyPlaylist(&bar, &ui::ControlBar::playlistToggled);
    QSignalSpy spyCp(&bar, &ui::ControlBar::controlPanelToggled);

    bar.playPauseButton()->click();
    QCOMPARE(spyPlayPause.count(), 1);
    QVERIFY(playPauseDispatched);

    bar.stopButton()->click();
    QCOMPARE(spyStop.count(), 1);
    QVERIFY(stopDispatched);

    bar.prevButton()->click();
    QCOMPARE(spyPrev.count(), 1);
    QVERIFY(prevDispatched);

    bar.nextButton()->click();
    QCOMPARE(spyNext.count(), 1);
    QVERIFY(nextDispatched);

    bar.muteButton()->click();
    QCOMPARE(spyMute.count(), 1);
    QVERIFY(muteDispatched);

    bar.subtitleButton()->click();
    QCOMPARE(spySub.count(), 1);
    QVERIFY(subDispatched);

    bar.playlistButton()->click();
    QCOMPARE(spyPlaylist.count(), 1);
    QVERIFY(playlistDispatched);

    bar.controlPanelButton()->click();
    QCOMPARE(spyCp.count(), 1);
    QVERIFY(cpDispatched);
}

void TestControlBar::testVolumeControls()
{
    ui::ControlBar bar;
    QSignalSpy spyVol(&bar, &ui::ControlBar::volumeChanged);

    QCOMPARE(bar.volumeSlider()->width(), 70);
    QCOMPARE(bar.volumeSlider()->minimum(), 0);
    QCOMPARE(bar.volumeSlider()->maximum(), 100);

    bar.setVolume(65.0);
    QCOMPARE(bar.volume(), 65.0);
    QCOMPARE(bar.volumeSlider()->value(), 65);

    // User interacting with volume slider
    bar.volumeSlider()->setValue(40);
    QCOMPARE(spyVol.count(), 1);
    QCOMPARE(spyVol.takeFirst().at(0).toDouble(), 40.0);
    QCOMPARE(bar.volume(), 40.0);

    // Muted toggle state
    bar.setMuted(true);
    QVERIFY(bar.isMuted());
    bar.setMuted(false);
    QVERIFY(!bar.isMuted());
}

void TestControlBar::testPlaybackEngineSync()
{
    ui::ControlBar bar;
    MockPlaybackEngine engine;

    bar.setPlaybackEngine(&engine);

    // Engine emits stateChanged(false) -> playing
    engine.simulateStateChange(false);
    QVERIFY(bar.isPlaying());
    QCOMPARE(bar.playPauseButton()->text(), QStringLiteral("❚❚"));

    // Engine emits stateChanged(true) -> paused
    engine.simulateStateChange(true);
    QVERIFY(!bar.isPlaying());
    QCOMPARE(bar.playPauseButton()->text(), QStringLiteral("▶"));

    // Engine emits positionChanged and durationChanged
    engine.simulateDurationChange(300.0);
    engine.simulatePositionChange(45.0);
    QCOMPARE(bar.timeLabel()->text(), QStringLiteral("00:45 / 05:00"));
    QCOMPARE(bar.timelineSlider()->currentSeconds(), 45.0);
    QCOMPARE(bar.timelineSlider()->durationSeconds(), 300.0);

    // Engine emits volumeChanged
    engine.simulateVolumeChange(80.0);
    QCOMPARE(bar.volume(), 80.0);
    QCOMPARE(bar.volumeSlider()->value(), 80);

    // Engine emits muteChanged
    engine.simulateMuteChange(true);
    QVERIFY(bar.isMuted());

    // Seeking on timeline slider notifies engine
    bar.timelineSlider()->seekRequested(150.0);
    QCOMPARE(engine.position(), 150.0);

    // Dragging volume slider modifies engine volume
    bar.volumeSlider()->setValue(55);
    QCOMPARE(engine.volume(), 55.0);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    TestControlBar tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_control_bar.moc"
