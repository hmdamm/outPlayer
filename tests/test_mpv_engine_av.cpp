// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "engine/IPlaybackEngine.h"
#include "engine/MpvEngine.h"

#include <clocale>
#include <cmath>

using namespace outplayer::engine;

class TestMpvEngineAv : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testVideoEqualizer();
    void testVideoRotationAndDeinterlace();
    void testAudioSyncAndEqualizer();
    void testSubtitleControls();
    void testFrameCapture();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
    QString m_testSubPath;
};

void TestMpvEngineAv::initTestCase()
{
    std::setlocale(LC_NUMERIC, "C");
    QVERIFY(m_tempDir.isValid());

    m_testVideoPath = m_tempDir.filePath("test_video.mp4");
    m_testSubPath = m_tempDir.filePath("test_sub.srt");

    // Create a simple test subtitle file
    QFile subFile(m_testSubPath);
    QVERIFY(subFile.open(QIODevice::WriteOnly | QIODevice::Text));
    subFile.write("1\n00:00:00,000 --> 00:00:02,000\nHello Test Subtitle\n\n");
    subFile.close();

    // Synthesize a valid 2-second MP4 with video and audio using ffmpeg
    const QStringList args = {
        "-y",
        "-f", "lavfi",
        "-i", "testsrc=duration=2:size=320x240:rate=10",
        "-f", "lavfi",
        "-i", "sine=duration=2:frequency=440",
        "-pix_fmt", "yuv420p",
        "-c:v", "libx264",
        "-c:a", "aac",
        m_testVideoPath
    };
    int exitCode = QProcess::execute("ffmpeg", args);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));
}

void TestMpvEngineAv::testVideoEqualizer()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Initial default values should be neutral 0
    QCOMPARE(engine.brightness(), 0);
    QCOMPARE(engine.contrast(), 0);
    QCOMPARE(engine.saturation(), 0);
    QCOMPARE(engine.hue(), 0);

    // Test setting valid ranges [-100, 100]
    engine.setBrightness(25);
    QCOMPARE(engine.brightness(), 25);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("brightness").toDouble())), 25);

    engine.setContrast(-40);
    QCOMPARE(engine.contrast(), -40);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("contrast").toDouble())), -40);

    engine.setSaturation(50);
    QCOMPARE(engine.saturation(), 50);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("saturation").toDouble())), 50);

    engine.setHue(-15);
    QCOMPARE(engine.hue(), -15);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("hue").toDouble())), -15);

    // Test clamping beyond boundaries
    engine.setBrightness(150);
    QCOMPARE(engine.brightness(), 100);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("brightness").toDouble())), 100);

    engine.setBrightness(-200);
    QCOMPARE(engine.brightness(), -100);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("brightness").toDouble())), -100);

    engine.setContrast(200);
    QCOMPARE(engine.contrast(), 100);

    engine.setSaturation(-150);
    QCOMPARE(engine.saturation(), -100);

    engine.setHue(180);
    QCOMPARE(engine.hue(), 100);

    // Test reset
    engine.resetVideoEqualizer();
    QCOMPARE(engine.brightness(), 0);
    QCOMPARE(engine.contrast(), 0);
    QCOMPARE(engine.saturation(), 0);
    QCOMPARE(engine.hue(), 0);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("brightness").toDouble())), 0);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("contrast").toDouble())), 0);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("saturation").toDouble())), 0);
    QCOMPARE(static_cast<int>(std::round(engine.getPropertyString("hue").toDouble())), 0);
}

void TestMpvEngineAv::testVideoRotationAndDeinterlace()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Default rotation is 0
    QCOMPARE(engine.rotation(), 0);

    // Valid rotations: 0, 90, 180, 270
    engine.setRotation(90);
    QCOMPARE(engine.rotation(), 90);
    QCOMPARE(engine.getPropertyString("video-rotate").toInt(), 90);

    engine.setRotation(180);
    QCOMPARE(engine.rotation(), 180);
    QCOMPARE(engine.getPropertyString("video-rotate").toInt(), 180);

    engine.setRotation(270);
    QCOMPARE(engine.rotation(), 270);
    QCOMPARE(engine.getPropertyString("video-rotate").toInt(), 270);

    engine.setRotation(360);
    QCOMPARE(engine.rotation(), 0);
    QCOMPARE(engine.getPropertyString("video-rotate").toInt(), 0);

    // Deinterlace default is false
    QCOMPARE(engine.deinterlace(), false);

    engine.setDeinterlace(true);
    QCOMPARE(engine.deinterlace(), true);
    QCOMPARE(engine.getPropertyString("deinterlace"), QStringLiteral("yes"));

    engine.setDeinterlace(false);
    QCOMPARE(engine.deinterlace(), false);
    QCOMPARE(engine.getPropertyString("deinterlace"), QStringLiteral("no"));
}

void TestMpvEngineAv::testAudioSyncAndEqualizer()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Audio delay default is 0.0
    QCOMPARE(engine.audioDelay(), 0.0);

    engine.setAudioDelay(0.150);
    QVERIFY(std::abs(engine.audioDelay() - 0.150) < 1e-4);
    QVERIFY(std::abs(engine.getPropertyString("audio-delay").toDouble() - 0.150) < 1e-4);

    engine.setAudioDelay(-0.050);
    QVERIFY(std::abs(engine.audioDelay() - (-0.050)) < 1e-4);
    QVERIFY(std::abs(engine.getPropertyString("audio-delay").toDouble() - (-0.050)) < 1e-4);

    // 10-band Equalizer
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(engine.equalizerBand(i), 0.0);
    }

    // Set individual band gains (-12.0 to 12.0 dB)
    engine.setEqualizerBand(0, 5.5);
    QCOMPARE(engine.equalizerBand(0), 5.5);

    engine.setEqualizerBand(9, -8.0);
    QCOMPARE(engine.equalizerBand(9), -8.0);

    // Clamping test
    engine.setEqualizerBand(1, 20.0);
    QCOMPARE(engine.equalizerBand(1), 12.0);

    engine.setEqualizerBand(2, -25.0);
    QCOMPARE(engine.equalizerBand(2), -12.0);

    // Out-of-bounds indices
    QCOMPARE(engine.equalizerBand(-1), 0.0);
    QCOMPARE(engine.equalizerBand(10), 0.0);
    engine.setEqualizerBand(-1, 5.0); // should safely do nothing
    engine.setEqualizerBand(15, 5.0); // should safely do nothing

    // Audio filter should reflect non-neutral EQ in mpv
    QString afStr = engine.getPropertyString("af");
    QVERIFY(afStr.contains("equalizer"));

    // Reset EQ
    engine.resetAudioEqualizer();
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(engine.equalizerBand(i), 0.0);
    }

    // Audio Normalizer
    QCOMPARE(engine.isNormalizerEnabled(), false);
    engine.setNormalizerEnabled(true);
    QCOMPARE(engine.isNormalizerEnabled(), true);
    QString normAf = engine.getPropertyString("af");
    QVERIFY(normAf.contains("dynaudnorm"));

    engine.setNormalizerEnabled(false);
    QCOMPARE(engine.isNormalizerEnabled(), false);
}

void TestMpvEngineAv::testSubtitleControls()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Subtitle delay default 0.0
    QCOMPARE(engine.subtitleDelay(), 0.0);
    engine.setSubtitleDelay(1.5);
    QVERIFY(std::abs(engine.subtitleDelay() - 1.5) < 1e-4);
    QVERIFY(std::abs(engine.getPropertyString("sub-delay").toDouble() - 1.5) < 1e-4);

    // Subtitle scale default 1.0 (clamped between 0.5 and 3.0)
    QCOMPARE(engine.subtitleScale(), 1.0);
    engine.setSubtitleScale(2.0);
    QCOMPARE(engine.subtitleScale(), 2.0);
    QVERIFY(std::abs(engine.getPropertyString("sub-scale").toDouble() - 2.0) < 1e-4);

    engine.setSubtitleScale(0.1);
    QCOMPARE(engine.subtitleScale(), 0.5);

    engine.setSubtitleScale(5.0);
    QCOMPARE(engine.subtitleScale(), 3.0);

    // Subtitle loading (requires a media file to be loaded)
    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));
    QVERIFY(engine.loadSubtitle(m_testSubPath));
}

void TestMpvEngineAv::testFrameCapture()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));
    QTest::qWait(300);

    QString destPath = m_tempDir.filePath("snapshots/frame.png");
    QVERIFY(engine.captureFrame(destPath));

    // Wait up to 2 seconds for the screenshot file to be written to disk
    bool fileWritten = false;
    for (int i = 0; i < 20; ++i) {
        if (QFile::exists(destPath) && QFileInfo(destPath).size() > 0) {
            fileWritten = true;
            break;
        }
        QTest::qWait(100);
    }
    QVERIFY(fileWritten);
}

QTEST_MAIN(TestMpvEngineAv)
#include "test_mpv_engine_av.moc"
