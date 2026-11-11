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

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestMpvEngine::initTestCase() {
    std::setlocale(LC_NUMERIC, "C");
    QVERIFY(m_tempDir.isValid());
    m_testVideoPath = m_tempDir.filePath("test_video.mp4");

    // Synthesize a tiny valid 2-second MP4 video using ffmpeg
    const QStringList args = {
        "-y",
        "-f", "lavfi",
        "-i", "testsrc=duration=2:size=320x240:rate=10",
        "-pix_fmt", "yuv420p",
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

QTEST_GUILESS_MAIN(TestMpvEngine)
#include "test_mpv_engine.moc"
