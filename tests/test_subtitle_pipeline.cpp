// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QDropEvent>
#include <QFile>
#include <QMimeData>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "ui/MainWindow.h"
#include "ui/MediaOpenController.h"
#include "ui/OsdOverlay.h"

using namespace outplayer;

class TestSubtitlePipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testSubtitleExtensionsAndDetection();
    void testMpvEngineSubtitleAutoAndVisibility();
    void testMpvEngineLoadSubtitleAndTrack();
    void testSubtitleCommandsAndOsd();
    void testDropEventSubtitleFile();

private:
    QTemporaryDir m_tempDir;
    QString m_testVideoPath;
    QString m_testSubPath;
};

void TestSubtitlePipeline::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());

    m_testVideoPath = m_tempDir.filePath("test_sub_video.mp4");
    m_testSubPath = m_tempDir.filePath("test_sub_video.srt");

    const QStringList ffmpegArgs = {
        "-y",
        "-f", "lavfi",
        "-i", "testsrc=duration=5:size=320x240:rate=10",
        "-f", "lavfi",
        "-i", "sine=duration=5:frequency=440",
        "-pix_fmt", "yuv420p",
        "-c:v", "libx264",
        "-c:a", "aac",
        m_testVideoPath
    };
    int exitCode = QProcess::execute("ffmpeg", ffmpegArgs);
    QCOMPARE(exitCode, 0);
    QVERIFY(QFile::exists(m_testVideoPath));

    QFile srtFile(m_testSubPath);
    QVERIFY(srtFile.open(QIODevice::WriteOnly | QIODevice::Text));
    const char* srtContent =
        "1\n"
        "00:00:01,000 --> 00:00:03,000\n"
        "Hello outPlayer Subtitles\n\n"
        "2\n"
        "00:00:03,500 --> 00:00:04,800\n"
        "Second subtitle line\n\n";
    srtFile.write(srtContent);
    srtFile.close();
    QVERIFY(QFile::exists(m_testSubPath));
}

void TestSubtitlePipeline::cleanup()
{
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestSubtitlePipeline::testSubtitleExtensionsAndDetection()
{
    const auto& exts = ui::MediaOpenController::subtitleExtensions();
    QVERIFY(exts.contains(QStringLiteral("srt")));
    QVERIFY(exts.contains(QStringLiteral("ass")));
    QVERIFY(exts.contains(QStringLiteral("ssa")));
    QVERIFY(exts.contains(QStringLiteral("vtt")));
    QVERIFY(exts.contains(QStringLiteral("sub")));
    QVERIFY(exts.contains(QStringLiteral("idx")));
    QVERIFY(exts.contains(QStringLiteral("sup")));

    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.srt")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.ASS")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.ssa")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.VTT")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.sub")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.idx")));
    QVERIFY(ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.sup")));

    QVERIFY(!ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/movie.mp4")));
    QVERIFY(!ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/audio.mp3")));
    QVERIFY(!ui::MediaOpenController::isSubtitleFile(QStringLiteral("/path/to/notes.txt")));
    QVERIFY(!ui::MediaOpenController::isSubtitleFile(QStringLiteral("")));
    QVERIFY(!ui::MediaOpenController::isSubtitleFile(m_tempDir.path()));
}

void TestSubtitlePipeline::testMpvEngineSubtitleAutoAndVisibility()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    // Check sub-auto is set to fuzzy
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-auto")), QStringLiteral("fuzzy"));

    // Check default subtitle visibility is true / yes
    QVERIFY(engine.isSubtitleVisible());
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-visibility")), QStringLiteral("yes"));

    // Toggle visibility
    engine.setSubtitleVisible(false);
    QVERIFY(!engine.isSubtitleVisible());
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-visibility")), QStringLiteral("no"));

    engine.setSubtitleVisible(true);
    QVERIFY(engine.isSubtitleVisible());
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-visibility")), QStringLiteral("yes"));
}

void TestSubtitlePipeline::testMpvEngineLoadSubtitleAndTrack()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    QSignalSpy spyLoaded(&engine, &engine::IPlaybackEngine::fileLoaded);
    QVERIFY(engine.loadFile(m_testVideoPath));
    QVERIFY(spyLoaded.wait(5000));

    // Load subtitle file
    QVERIFY(engine.loadSubtitle(m_testSubPath));

    // Give mpv an event cycle to register the subtitle track
    QTest::qWait(500);

    const auto subTracks = engine.tracks(QStringLiteral("sub"));
    QVERIFY(!subTracks.isEmpty());

    // Because sub-add with select was used, current subtitle track should be > 0
    const int curTrack = engine.currentSubtitleTrack();
    QVERIFY(curTrack > 0);

    // Test changing track
    QVERIFY(engine.setSubtitleTrack(0));
    QTest::qWait(200);
    QCOMPARE(engine.currentSubtitleTrack(), 0);

    QVERIFY(engine.setSubtitleTrack(curTrack));
    QTest::qWait(200);
    QCOMPARE(engine.currentSubtitleTrack(), curTrack);
}

void TestSubtitlePipeline::testSubtitleCommandsAndOsd()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.engine() != nullptr);
    QVERIFY(window.osdOverlay() != nullptr);

    // Initial subtitle visibility
    window.engine()->setSubtitleVisible(true);
    QVERIFY(window.engine()->isSubtitleVisible());

    // 1. Alt+H -> Toggle visibility (off)
    bool ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_TOGGLE_VISIBILITY);
    QVERIFY2(ok, "CMD_SUB_TOGGLE_VISIBILITY handler must be registered");
    QVERIFY(!window.engine()->isSubtitleVisible());
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitles: Off"));

    // 2. Alt+H -> Toggle visibility (on)
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_TOGGLE_VISIBILITY);
    QVERIFY(ok);
    QVERIFY(window.engine()->isSubtitleVisible());
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitles: On"));

    // 3. Subtitle size scaling
    window.engine()->setSubtitleScale(1.0);
    QCOMPARE(window.engine()->subtitleScale(), 1.0);

    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SIZE_UP);
    QVERIFY2(ok, "CMD_SUB_SIZE_UP handler must be registered");
    QVERIFY(std::abs(window.engine()->subtitleScale() - 1.1) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Size: 110%"));

    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SIZE_DOWN);
    QVERIFY2(ok, "CMD_SUB_SIZE_DOWN handler must be registered");
    QVERIFY(std::abs(window.engine()->subtitleScale() - 1.0) < 0.001);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Size: 100%"));

    window.engine()->setSubtitleScale(1.5);
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_SIZE_RESET);
    QVERIFY2(ok, "CMD_SUB_SIZE_RESET handler must be registered");
    QCOMPARE(window.engine()->subtitleScale(), 1.0);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle Size: 100%"));

    // 4. Subtitle track cycling commands exist and can execute
    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_TRACK_NEXT);
    QVERIFY2(ok, "CMD_SUB_TRACK_NEXT handler must be registered");

    ok = core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_TRACK_PREV);
    QVERIFY2(ok, "CMD_SUB_TRACK_PREV handler must be registered");
}

void TestSubtitlePipeline::testDropEventSubtitleFile()
{
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.openMedia(m_testVideoPath));
    QTest::qWait(1000);

    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(m_testSubPath)});
    QDropEvent dropEvent(QPointF(100, 100), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    window.dropEvent(&dropEvent);
    QVERIFY(dropEvent.isAccepted());

    QFileInfo fi(m_testSubPath);
    QCOMPARE(window.osdOverlay()->currentText(), QStringLiteral("Subtitle loaded: %1").arg(fi.fileName()));
}

QTEST_MAIN(TestSubtitlePipeline)
#include "test_subtitle_pipeline.moc"
