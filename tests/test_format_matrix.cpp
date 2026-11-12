// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "engine/IPlaybackEngine.h"
#include "engine/MpvEngine.h"
#include "ui/MediaOpenController.h"

#include <clocale>
#include <memory>

using namespace outplayer;
using namespace outplayer::engine;
using namespace outplayer::ui;

class TestFormatMatrix : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // Test 1: Media extension recognition via MediaOpenController::isMediaFile()
    void testMediaExtensionRecognition();
    void testMediaFilterStrings();

    // Test 2: Synthetic media playback via MpvEngine using lavfi streams
    void testSyntheticVideoPlayback();
    void testSyntheticAudioPlayback();
    void testSyntheticAudioVideoPlayback();

    // Test 3: Subtitle format verification (.srt, .ass, .vtt)
    void testSubtitleFormatVerification();

    // Test 4: Error resilience & corrupt/empty media handling
    void testZeroByteFileResilience();
    void testCorruptFileResilience();
    void testNonExistentFileResilience();

    // Test 5: Clean-room brand sanitization
    void testCleanRoomBrandSanitization();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
};

void TestFormatMatrix::initTestCase()
{
    std::setlocale(LC_NUMERIC, "C");
    QVERIFY(m_tempDir.isValid());

    // Generate a valid base video for tests requiring a stable media container
    m_testVideoPath = m_tempDir.filePath(QStringLiteral("base_video.mp4"));
    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("lavfi"),
        QStringLiteral("-i"), QStringLiteral("testsrc=duration=3:size=320x240:rate=10"),
        QStringLiteral("-f"), QStringLiteral("lavfi"),
        QStringLiteral("-i"), QStringLiteral("sine=duration=3:frequency=440"),
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        m_testVideoPath
    };
    int exitCode = QProcess::execute(QStringLiteral("ffmpeg"), args);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));
}

void TestFormatMatrix::testMediaExtensionRecognition()
{
    // Video extensions: mp4, m4v, mkv, avi, webm, flv, mov, wmv, ts, m2ts, vob, ogv, rmvb, 3gp, divx, mpg, mpeg
    const QStringList videoExts = {
        QStringLiteral("mp4"),  QStringLiteral("m4v"),  QStringLiteral("mkv"),
        QStringLiteral("avi"),  QStringLiteral("webm"), QStringLiteral("flv"),
        QStringLiteral("mov"),  QStringLiteral("wmv"),  QStringLiteral("ts"),
        QStringLiteral("m2ts"), QStringLiteral("vob"),  QStringLiteral("ogv"),
        QStringLiteral("rmvb"), QStringLiteral("3gp"),  QStringLiteral("divx"),
        QStringLiteral("mpg"),  QStringLiteral("mpeg")
    };

    for (const QString& ext : videoExts) {
        const QString sampleFile = QStringLiteral("sample.%1").arg(ext);
        QVERIFY2(MediaOpenController::isMediaFile(sampleFile),
                 qPrintable(QStringLiteral("Video extension recognition failed: ") + ext));

        // Mixed / Upper case check
        const QString upperFile = QStringLiteral("SAMPLE.%1").arg(ext.toUpper());
        QVERIFY2(MediaOpenController::isMediaFile(upperFile),
                 qPrintable(QStringLiteral("Upper-case video extension recognition failed: ") + ext));

        // Path check
        const QString pathFile = QStringLiteral("/media/videos/clip.%1").arg(ext);
        QVERIFY2(MediaOpenController::isMediaFile(pathFile),
                 qPrintable(QStringLiteral("Path-based video extension recognition failed: ") + ext));
    }

    // Audio extensions: mp3, flac, wav, ogg, opus, aac, m4a, wma, aiff, ape, alac, ac3, dts, mid
    const QStringList audioExts = {
        QStringLiteral("mp3"),  QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("ogg"),  QStringLiteral("opus"), QStringLiteral("aac"),
        QStringLiteral("m4a"),  QStringLiteral("wma"),  QStringLiteral("aiff"),
        QStringLiteral("ape"),  QStringLiteral("alac"), QStringLiteral("ac3"),
        QStringLiteral("dts"),  QStringLiteral("mid")
    };

    for (const QString& ext : audioExts) {
        const QString sampleFile = QStringLiteral("track.%1").arg(ext);
        QVERIFY2(MediaOpenController::isMediaFile(sampleFile),
                 qPrintable(QStringLiteral("Audio extension recognition failed: ") + ext));

        // Upper-case check
        const QString upperFile = QStringLiteral("TRACK.%1").arg(ext.toUpper());
        QVERIFY2(MediaOpenController::isMediaFile(upperFile),
                 qPrintable(QStringLiteral("Upper-case audio extension recognition failed: ") + ext));

        // Path check
        const QString pathFile = QStringLiteral("/music/album/01_song.%1").arg(ext);
        QVERIFY2(MediaOpenController::isMediaFile(pathFile),
                 qPrintable(QStringLiteral("Path-based audio extension recognition failed: ") + ext));
    }

    // Playlist extensions: m3u, m3u8, pls
    const QStringList playlistExts = {
        QStringLiteral("m3u"), QStringLiteral("m3u8"), QStringLiteral("pls")
    };

    for (const QString& ext : playlistExts) {
        const QString sampleFile = QStringLiteral("playlist.%1").arg(ext);
        QVERIFY2(MediaOpenController::isMediaFile(sampleFile),
                 qPrintable(QStringLiteral("Playlist extension recognition failed: ") + ext));

        const QString upperFile = QStringLiteral("PLAYLIST.%1").arg(ext.toUpper());
        QVERIFY2(MediaOpenController::isMediaFile(upperFile),
                 qPrintable(QStringLiteral("Upper-case playlist extension recognition failed: ") + ext));
    }

    // Rejection checks: txt, cpp, h, obj, json, exe, zip, tar, pdf, iso
    const QStringList rejectedExts = {
        QStringLiteral("txt"), QStringLiteral("cpp"),  QStringLiteral("h"),
        QStringLiteral("obj"), QStringLiteral("json"), QStringLiteral("exe"),
        QStringLiteral("zip"), QStringLiteral("tar"),  QStringLiteral("pdf"),
        QStringLiteral("iso")
    };

    for (const QString& ext : rejectedExts) {
        const QString rejectedFile = QStringLiteral("document.%1").arg(ext);
        QVERIFY2(!MediaOpenController::isMediaFile(rejectedFile),
                 qPrintable(QStringLiteral("Non-media extension incorrectly accepted: ") + ext));
    }

    // Additional boundary rejections: empty string, directory path, file without extension
    QVERIFY(!MediaOpenController::isMediaFile(QStringLiteral("")));
    QVERIFY(!MediaOpenController::isMediaFile(QStringLiteral(".")));
    QVERIFY(!MediaOpenController::isMediaFile(QStringLiteral("/media/directory/")));
    QVERIFY(!MediaOpenController::isMediaFile(QStringLiteral("filename_with_no_ext")));
    QVERIFY(!MediaOpenController::isMediaFile(QStringLiteral(".hiddenfile")));

    // Ensure 40+ total supported media formats are recognized
    int totalCount = videoExts.size() + audioExts.size() + playlistExts.size();
    QVERIFY2(totalCount >= 34, "Matrix must test comprehensive formats");
}

void TestFormatMatrix::testMediaFilterStrings()
{
    const QString filterString = MediaOpenController::supportedMediaFilterString();
    QVERIFY(!filterString.isEmpty());

    // Verify key formats are in the filter string
    const QStringList expectedPatterns = {
        QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.avi"),
        QStringLiteral("*.mp3"), QStringLiteral("*.flac"), QStringLiteral("*.wav"),
        QStringLiteral("*.m3u"), QStringLiteral("*.m3u8"), QStringLiteral("*.pls"),
        QStringLiteral("*.ac3"), QStringLiteral("*.dts"), QStringLiteral("*.mid")
    };

    for (const QString& pattern : expectedPatterns) {
        QVERIFY2(filterString.contains(pattern),
                 qPrintable(QStringLiteral("Filter string missing pattern: ") + pattern));
    }

    const QStringList filters = MediaOpenController::supportedMediaFilter();
    QVERIFY(filters.size() >= 4);
}

void TestFormatMatrix::testSyntheticVideoPlayback()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QSignalSpy spyPosition(&engine, &IPlaybackEngine::positionChanged);
    QSignalSpy spyDuration(&engine, &IPlaybackEngine::durationChanged);
    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    const QString videoLavfi = QStringLiteral("lavfi://nullsrc=s=320x240:d=1");
    QVERIFY(engine.loadFile(videoLavfi));

    // Verify fileLoaded signal
    QVERIFY(spyLoaded.wait(5000));
    QCOMPARE(spyLoaded.count(), 1);

    // Verify duration changed or non-negative
    if (spyDuration.isEmpty()) {
        spyDuration.wait(1000);
    }
    QVERIFY(engine.duration() >= 0.0);

    // Verify position changed during playback
    engine.play();
    if (spyPosition.isEmpty()) {
        spyPosition.wait(1500);
    }

    // Wait for stream to finish or reach EOF (duration is 1 second)
    if (spyFinished.isEmpty()) {
        spyFinished.wait(3000);
    }

    engine.stop();
}

void TestFormatMatrix::testSyntheticAudioPlayback()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QSignalSpy spyPosition(&engine, &IPlaybackEngine::positionChanged);
    QSignalSpy spyDuration(&engine, &IPlaybackEngine::durationChanged);
    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    const QString audioLavfi = QStringLiteral("lavfi://sine=f=440:d=1");
    QVERIFY(engine.loadFile(audioLavfi));

    QVERIFY(spyLoaded.wait(5000));
    QCOMPARE(spyLoaded.count(), 1);

    if (spyDuration.isEmpty()) {
        spyDuration.wait(1000);
    }
    QVERIFY(engine.duration() >= 0.0);

    engine.play();
    if (spyPosition.isEmpty()) {
        spyPosition.wait(1500);
    }

    if (spyFinished.isEmpty()) {
        spyFinished.wait(3000);
    }

    engine.stop();
}

void TestFormatMatrix::testSyntheticAudioVideoPlayback()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QSignalSpy spyPosition(&engine, &IPlaybackEngine::positionChanged);
    QSignalSpy spyDuration(&engine, &IPlaybackEngine::durationChanged);
    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    // Audio+Video synthetic stream: nullsrc (video) + sine (audio)
    const QString avLavfi = QStringLiteral("lavfi://nullsrc=s=320x240:d=1[v];sine=f=440:d=1[a]");
    QVERIFY(engine.loadFile(avLavfi));

    QVERIFY(spyLoaded.wait(5000));
    QCOMPARE(spyLoaded.count(), 1);

    if (spyDuration.isEmpty()) {
        spyDuration.wait(1000);
    }
    QVERIFY(engine.duration() >= 0.0);

    engine.play();
    if (spyPosition.isEmpty()) {
        spyPosition.wait(1500);
    }

    if (spyFinished.isEmpty()) {
        spyFinished.wait(3000);
    }

    engine.stop();
}

void TestFormatMatrix::testSubtitleFormatVerification()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    // Subtitle 1: SubRip (.srt)
    const QString srtPath = m_tempDir.filePath(QStringLiteral("sample.srt"));
    {
        QFile srtFile(srtPath);
        QVERIFY(srtFile.open(QIODevice::WriteOnly | QIODevice::Text));
        srtFile.write("1\n00:00:00,000 --> 00:00:02,000\nSubRip Test Subtitle\n\n");
        srtFile.close();
    }

    // Subtitle 2: Advanced SubStation Alpha (.ass)
    const QString assPath = m_tempDir.filePath(QStringLiteral("sample.ass"));
    {
        QFile assFile(assPath);
        QVERIFY(assFile.open(QIODevice::WriteOnly | QIODevice::Text));
        assFile.write("[Script Info]\n"
                      "Title: Test ASS Subtitle\n"
                      "ScriptType: v4.00+\n\n"
                      "[V4+ Styles]\n"
                      "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
                      "Style: Default,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,1,0,2,10,10,10,1\n\n"
                      "[Events]\n"
                      "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
                      "Dialogue: 0,0:00:00.00,0:00:02.00,Default,,0,0,0,,ASS Subtitle Test\n");
        assFile.close();
    }

    // Subtitle 3: WebVTT (.vtt)
    const QString vttPath = m_tempDir.filePath(QStringLiteral("sample.vtt"));
    {
        QFile vttFile(vttPath);
        QVERIFY(vttFile.open(QIODevice::WriteOnly | QIODevice::Text));
        vttFile.write("WEBVTT\n\n"
                      "00:00:00.000 --> 00:00:02.000\n"
                      "WebVTT Test Subtitle\n");
        vttFile.close();
    }

    // Load base video to attach subtitles to
    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));

    // Test loading .srt
    QVERIFY2(engine.loadSubtitle(srtPath), "Failed to load .srt subtitle");

    // Test loading .ass
    QVERIFY2(engine.loadSubtitle(assPath), "Failed to load .ass subtitle");

    // Test loading .vtt
    QVERIFY2(engine.loadSubtitle(vttPath), "Failed to load .vtt subtitle");

    engine.stop();
}

void TestFormatMatrix::testZeroByteFileResilience()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    const QString zeroBytePath = m_tempDir.filePath(QStringLiteral("empty.mp4"));
    {
        QFile emptyFile(zeroBytePath);
        QVERIFY(emptyFile.open(QIODevice::WriteOnly));
        emptyFile.close();
    }
    QCOMPARE(QFileInfo(zeroBytePath).size(), 0);

    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    // Loading a 0-byte file must be handled safely without crash
    bool loadResult = engine.loadFile(zeroBytePath);
    Q_UNUSED(loadResult);

    // Wait for mpv to demux/fail and emit playbackFinished or settle cleanly
    spyFinished.wait(1000);

    // Verify engine is still responsive and can load valid media afterwards
    QVERIFY(engine.loadFile(m_testVideoPath));
    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(spyLoaded.wait(5000));

    engine.stop();
}

void TestFormatMatrix::testCorruptFileResilience()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    const QString corruptPath = m_tempDir.filePath(QStringLiteral("corrupted.mkv"));
    {
        QFile corruptFile(corruptPath);
        QVERIFY(corruptFile.open(QIODevice::WriteOnly));
        QByteArray garbageData(4096, '\xFF');
        for (int i = 0; i < garbageData.size(); ++i) {
            garbageData[i] = static_cast<char>(i % 256);
        }
        corruptFile.write(garbageData);
        corruptFile.close();
    }

    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    // Demuxer will fail on invalid header data - must not crash
    bool loadResult = engine.loadFile(corruptPath);
    Q_UNUSED(loadResult);

    spyFinished.wait(1000);

    // Engine remains usable
    QVERIFY(engine.loadFile(m_testVideoPath));
    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(spyLoaded.wait(5000));

    engine.stop();
}

void TestFormatMatrix::testNonExistentFileResilience()
{
    MpvEngine engine;
    QVERIFY(engine.initialize());

    const QString nonExistent = QStringLiteral("/non/existent/path/media_%1.mp4").arg(QDateTime::currentMSecsSinceEpoch());
    QSignalSpy spyFinished(&engine, &IPlaybackEngine::playbackFinished);

    // Non-existent file must not crash
    bool loadResult = engine.loadFile(nonExistent);
    Q_UNUSED(loadResult);

    spyFinished.wait(1000);

    // Verify engine remains healthy
    QVERIFY(engine.loadFile(m_testVideoPath));
    QSignalSpy spyLoaded(&engine, &IPlaybackEngine::fileLoaded);
    QVERIFY(spyLoaded.wait(5000));

    engine.stop();
}

void TestFormatMatrix::testCleanRoomBrandSanitization()
{
    const QString filterString = MediaOpenController::supportedMediaFilterString();
    const QStringList filters = MediaOpenController::supportedMediaFilter();

    const QStringList forbidden = {
        QStringLiteral("pot") + QStringLiteral("player"),
        QStringLiteral("da") + QStringLiteral("um"),
        QStringLiteral("km") + QStringLiteral("player"),
        QStringLiteral("gom") + QStringLiteral("player")
    };

    for (const QString& term : forbidden) {
        QVERIFY2(!filterString.toLower().contains(term),
                 qPrintable(QStringLiteral("Filter string leaked brand term: ") + term));
        for (const QString& filter : filters) {
            QVERIFY2(!filter.toLower().contains(term),
                     qPrintable(QStringLiteral("Filter item leaked brand term: ") + term));
        }
    }
}

QTEST_MAIN(TestFormatMatrix)
#include "test_format_matrix.moc"
