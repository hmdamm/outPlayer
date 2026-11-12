// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "engine/IPlaybackEngine.h"
#include "engine/MpvEngine.h"

#include <clocale>

using namespace outplayer::engine;

class TestMpvEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitializeAndHandle();
    void testPlaybackControls();
    void testSignals();
    void testSeekAndStop();
    void testVolumeAndMute();
    void testSpeed();
    void testAspectRatio();
    void testTracksAndChapters();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestMpvEngine::initTestCase() {
    std::setlocale(LC_NUMERIC, "C");
    QVERIFY(m_tempDir.isValid());
    m_testVideoPath = m_tempDir.filePath("test_video.mp4");

    QString metaPath = m_tempDir.filePath("chapters.txt");
    QFile metaFile(metaPath);
    QVERIFY(metaFile.open(QIODevice::WriteOnly | QIODevice::Text));
    metaFile.write(";FFMETADATA1\n"
                   "[CHAPTER]\n"
                   "TIMEBASE=1/1000\n"
                   "START=0\n"
                   "END=1000\n"
                   "title=Intro\n"
                   "[CHAPTER]\n"
                   "TIMEBASE=1/1000\n"
                   "START=1000\n"
                   "END=2000\n"
                   "title=Outro\n");
    metaFile.close();

    // Synthesize a valid 2-second MP4 with video, audio, and chapters
    const QStringList args = {
        "-y",
        "-f", "lavfi",
        "-i", "testsrc=duration=2:size=320x240:rate=10",
        "-f", "lavfi",
        "-i", "sine=duration=2:frequency=440",
        "-i", metaPath,
        "-map_metadata", "2",
        "-map_chapters", "2",
        "-pix_fmt", "yuv420p",
        "-c:v", "libx264",
        "-c:a", "aac",
        m_testVideoPath
    };
    int exitCode = QProcess::execute("ffmpeg", args);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));
}

void TestMpvEngine::testInitializeAndHandle() {
    MpvEngine engine;
    QVERIFY(engine.mpvHandle() == nullptr);
    QVERIFY(!engine.isPaused());

    QVERIFY(engine.initialize());
    QVERIFY(engine.mpvHandle() != nullptr);

    // Re-initialization should be safe and return true
    QVERIFY(engine.initialize());
}

void TestMpvEngine::testPlaybackControls() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Initial state before loading file
    QVERIFY(!engine.isPaused());

    // Pause / Play / Toggle
    engine.pause();
    QVERIFY(engine.isPaused());

    engine.play();
    QVERIFY(!engine.isPaused());

    engine.togglePause();
    QVERIFY(engine.isPaused());

    engine.togglePause();
    QVERIFY(!engine.isPaused());
}

void TestMpvEngine::testSignals() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QSignalSpy spyState(&engine, &IPlaybackEngine::stateChanged);
    QSignalSpy spyDuration(&engine, &IPlaybackEngine::durationChanged);

    // Load synthesized test video
    QVERIFY(engine.loadFile(m_testVideoPath));

    // Wait for fileLoaded signal
    QVERIFY(spyLoaded.wait(5000));
    QCOMPARE(spyLoaded.count(), 1);

    // Verify duration changed and is positive
    if (spyDuration.isEmpty()) {
        spyDuration.wait(2000);
    }
    QVERIFY(engine.duration() > 0.0);

    // Test pause signal
    engine.pause();
    if (spyState.isEmpty()) {
        spyState.wait(2000);
    }
    QVERIFY(!spyState.isEmpty());
    QCOMPARE(spyState.takeLast().at(0).toBool(), true);

    // Test play signal
    engine.play();
    if (spyState.isEmpty()) {
        spyState.wait(2000);
    }
    QVERIFY(!spyState.isEmpty());
    QCOMPARE(spyState.takeLast().at(0).toBool(), false);
}

void TestMpvEngine::testSeekAndStop() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));

    // Seek exact and inexact
    engine.seek(0.5, false);
    engine.seek(1.0, true);

    // Stop playback
    engine.stop();
}

void TestMpvEngine::testVolumeAndMute() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QCOMPARE(engine.volume(), 100.0);
    QCOMPARE(engine.isMuted(), false);

    QSignalSpy spyVolume(&engine, &IPlaybackEngine::volumeChanged);
    QSignalSpy spyMute(&engine, &IPlaybackEngine::muteChanged);

    engine.setVolume(75.0);
    QCOMPARE(engine.volume(), 75.0);
    QVERIFY(!spyVolume.isEmpty());
    QCOMPARE(spyVolume.takeLast().at(0).toDouble(), 75.0);

    engine.setMuted(true);
    QVERIFY(engine.isMuted());
    QVERIFY(!spyMute.isEmpty());
    QCOMPARE(spyMute.takeLast().at(0).toBool(), true);

    engine.setMuted(false);
    QVERIFY(!engine.isMuted());
    QVERIFY(!spyMute.isEmpty());
    QCOMPARE(spyMute.takeLast().at(0).toBool(), false);

    engine.setVolume(120.0);
    QCOMPARE(engine.volume(), 120.0);
}

void TestMpvEngine::testSpeed() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QCOMPARE(engine.speed(), 1.0);

    QSignalSpy spySpeed(&engine, &IPlaybackEngine::speedChanged);

    // 0.5x speed
    engine.setSpeed(0.5);
    QCOMPARE(engine.speed(), 0.5);
    QVERIFY(!spySpeed.isEmpty());
    QCOMPARE(spySpeed.takeLast().at(0).toDouble(), 0.5);

    // 1.0x speed
    engine.setSpeed(1.0);
    QCOMPARE(engine.speed(), 1.0);
    QVERIFY(!spySpeed.isEmpty());
    QCOMPARE(spySpeed.takeLast().at(0).toDouble(), 1.0);

    // 2.0x speed
    engine.setSpeed(2.0);
    QCOMPARE(engine.speed(), 2.0);
    QVERIFY(!spySpeed.isEmpty());
    QCOMPARE(spySpeed.takeLast().at(0).toDouble(), 2.0);
}

void TestMpvEngine::testAspectRatio() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    engine.setAspectRatio(QStringLiteral("16:9"));
    QCOMPARE(engine.aspectRatio(), QStringLiteral("16:9"));

    engine.setAspectRatio(QStringLiteral("4:3"));
    QCOMPARE(engine.aspectRatio(), QStringLiteral("4:3"));

    engine.setAspectRatio(QString());
    QVERIFY(engine.aspectRatio().isEmpty() || engine.aspectRatio() == QStringLiteral("-1"));
}

void TestMpvEngine::testTracksAndChapters() {
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QSignalSpy spyTracks(&engine, &IPlaybackEngine::tracksChanged);
    QSignalSpy spyChapters(&engine, &IPlaybackEngine::chaptersChanged);

    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));

    // Wait for tracks and chapters if not already received
    int attempts = 0;
    while ((engine.tracks().isEmpty() || engine.chapters().isEmpty()) && attempts < 20) {
        QTest::qWait(100);
        ++attempts;
    }

    // Check all tracks vs filtered tracks
    const auto allTracks = engine.tracks();
    QVERIFY(!allTracks.isEmpty());

    const auto videoTracks = engine.tracks(QStringLiteral("video"));
    QVERIFY(!videoTracks.isEmpty());
    QCOMPARE(videoTracks.first().type, QStringLiteral("video"));
    QVERIFY(videoTracks.first().id > 0);

    const auto audioTracks = engine.tracks(QStringLiteral("audio"));
    QVERIFY(!audioTracks.isEmpty());
    QCOMPARE(audioTracks.first().type, QStringLiteral("audio"));
    QVERIFY(audioTracks.first().id > 0);

    const auto subTracks = engine.tracks(QStringLiteral("sub"));
    QVERIFY(subTracks.isEmpty()); // No subtitles in synthesized video

    // Track selection
    QVERIFY(engine.setTrack(QStringLiteral("video"), videoTracks.first().id));
    QVERIFY(engine.setTrack(QStringLiteral("video"), -1)); // disable video track
    QVERIFY(!engine.setTrack(QStringLiteral("invalid_type"), 1));

    // Chapters
    const auto chapters = engine.chapters();
    QCOMPARE(chapters.size(), 2);
    QCOMPARE(chapters[0].index, 0);
    QCOMPARE(chapters[0].title, QStringLiteral("Intro"));
    QCOMPARE(chapters[1].index, 1);
    QCOMPARE(chapters[1].title, QStringLiteral("Outro"));

    // Set chapter
    QVERIFY(engine.setChapter(0));
    QVERIFY(engine.setChapter(1));
    QVERIFY(!engine.setChapter(99));
    QVERIFY(!engine.setChapter(-1));
}

QTEST_GUILESS_MAIN(TestMpvEngine)
#include "test_mpv_engine.moc"
