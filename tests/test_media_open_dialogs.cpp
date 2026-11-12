// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "core/SettingsManager.h"
#include "engine/IPlaybackEngine.h"
#include "ui/MediaOpenController.h"
#include "ui/OpenUrlDialog.h"

using namespace outplayer;

class MockPlaybackEngine : public engine::IPlaybackEngine {
    Q_OBJECT
public:
    explicit MockPlaybackEngine(QObject* parent = nullptr) : engine::IPlaybackEngine(parent) {}

    bool initialize() override { return true; }
    bool loadFile(const QString&) override { return true; }
    void play() override {}
    void pause() override {}
    void togglePause() override {}
    void stop() override {}
    void seek(double, bool) override {}
    [[nodiscard]] double position() const override { return 0.0; }
    [[nodiscard]] double duration() const override { return 100.0; }
    [[nodiscard]] bool isPaused() const override { return false; }
    [[nodiscard]] QList<engine::MediaTrack> tracks(const QString&) const override { return {}; }
    bool setTrack(const QString&, int) override { return true; }
    [[nodiscard]] double volume() const override { return 100.0; }
    void setVolume(double) override {}
    [[nodiscard]] bool isMuted() const override { return false; }
    void setMuted(bool) override {}
    [[nodiscard]] double speed() const override { return 1.0; }
    void setSpeed(double) override {}
    [[nodiscard]] QString aspectRatio() const override { return QStringLiteral("auto"); }
    void setAspectRatio(const QString&) override {}
    [[nodiscard]] QList<engine::ChapterInfo> chapters() const override { return {}; }
    bool setChapter(int) override { return true; }
};

class TestMediaOpenDialogs : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testIsMediaFile();
    void testSupportedMediaFilter();
    void testScanDirectory();
    void testOpenUrlDialogProperties();
    void testOpenUrlDialogWhitespaceTrimming();
    void testOpenUrlDialogValidation();
    void testOpenUrlDialogPersistence();
    void testMediaOpenControllerEngineIntegration();

private:
    std::unique_ptr<QTemporaryDir> m_tempConfigDir;
};

void TestMediaOpenDialogs::initTestCase()
{
    m_tempConfigDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempConfigDir->isValid());
    core::SettingsManager::instance().init(m_tempConfigDir->path() + QStringLiteral("/test_settings.ini"));
}

void TestMediaOpenDialogs::cleanupTestCase()
{
    m_tempConfigDir.reset();
}

void TestMediaOpenDialogs::init()
{
    core::SettingsManager::instance().resetToDefaults();
}

void TestMediaOpenDialogs::testIsMediaFile()
{
    // Common video formats
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("video.mp4")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("movie.MKV")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("/path/to/clip.avi")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("sample.webm")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("stream.mov")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("media.wmv")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("flash.flv")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("broadcast.ts")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("disc.m2ts")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("dvd.vob")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("open.ogv")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("mobile.m4v")));

    // Common audio formats
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("music.mp3")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("lossless.FLAC")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("wave.wav")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("vorbis.ogg")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("audio.m4a")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("stream.aac")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("podcast.opus")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("sound.wma")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("track.aiff")));

    // Playlist formats
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("playlist.m3u")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("stream.m3u8")));
    QVERIFY(ui::MediaOpenController::isMediaFile(QStringLiteral("radio.pls")));

    // Non-media formats
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("readme.txt")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("program.exe")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("source.cpp")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("header.h")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("data.csv")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("archive.zip")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("document.pdf")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral("/just/a/directory/")));
    QVERIFY(!ui::MediaOpenController::isMediaFile(QStringLiteral(".hiddenfile")));
}

void TestMediaOpenDialogs::testSupportedMediaFilter()
{
    const QStringList filters = ui::MediaOpenController::supportedMediaFilter();
    QVERIFY(!filters.isEmpty());

    // Should contain a comprehensive filter and specific categories
    bool foundMedia = false;
    bool foundVideo = false;
    bool foundAudio = false;
    bool foundPlaylist = false;
    bool foundAll = false;

    for (const QString& filter : filters) {
        if (filter.contains(QStringLiteral("*.mp4")) && filter.contains(QStringLiteral("*.mp3"))) {
            foundMedia = true;
        }
        if (filter.contains(QStringLiteral("Video Files")) || filter.contains(QStringLiteral("*.mkv"))) {
            foundVideo = true;
        }
        if (filter.contains(QStringLiteral("Audio Files")) || filter.contains(QStringLiteral("*.flac"))) {
            foundAudio = true;
        }
        if (filter.contains(QStringLiteral("Playlist")) || filter.contains(QStringLiteral("*.m3u"))) {
            foundPlaylist = true;
        }
        if (filter.contains(QStringLiteral("*.*"))) {
            foundAll = true;
        }
    }

    QVERIFY(foundMedia);
    QVERIFY(foundVideo);
    QVERIFY(foundAudio);
    QVERIFY(foundPlaylist);
    QVERIFY(foundAll);

    // Filter string joining test
    const QString filterString = ui::MediaOpenController::supportedMediaFilterString();
    QVERIFY(filterString.contains(QStringLiteral(";;")));
    QVERIFY(filterString.contains(QStringLiteral("*.mp4")));
}

void TestMediaOpenDialogs::testScanDirectory()
{
    QTemporaryDir tempTree;
    QVERIFY(tempTree.isValid());

    const QDir rootDir(tempTree.path());
    rootDir.mkdir(QStringLiteral("sub1"));
    rootDir.mkdir(QStringLiteral("sub2"));
    rootDir.mkdir(QStringLiteral("sub1/nested"));

    auto createFile = [](const QString& path) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("dummy media content");
        file.close();
    };

    const QString f1 = rootDir.filePath(QStringLiteral("video1.mp4"));
    const QString f2 = rootDir.filePath(QStringLiteral("audio1.mp3"));
    const QString f3 = rootDir.filePath(QStringLiteral("notes.txt"));
    const QString f4 = rootDir.filePath(QStringLiteral("sub1/video2.mkv"));
    const QString f5 = rootDir.filePath(QStringLiteral("sub1/code.cpp"));
    const QString f6 = rootDir.filePath(QStringLiteral("sub1/nested/song2.flac"));
    const QString f7 = rootDir.filePath(QStringLiteral("sub2/playlist.m3u"));
    const QString f8 = rootDir.filePath(QStringLiteral("sub2/doc.pdf"));

    createFile(f1);
    createFile(f2);
    createFile(f3);
    createFile(f4);
    createFile(f5);
    createFile(f6);
    createFile(f7);
    createFile(f8);

    // Non-recursive scan of root directory: only media files in root directory
    const QStringList nonRec = ui::MediaOpenController::scanDirectory(rootDir.absolutePath(), false);
    QCOMPARE(nonRec.size(), 2);
    QVERIFY(nonRec.contains(QFileInfo(f1).absoluteFilePath()));
    QVERIFY(nonRec.contains(QFileInfo(f2).absoluteFilePath()));
    QVERIFY(!nonRec.contains(QFileInfo(f3).absoluteFilePath()));
    QVERIFY(!nonRec.contains(QFileInfo(f4).absoluteFilePath()));

    // Recursive scan: all 5 media files, ignores 3 non-media files
    const QStringList rec = ui::MediaOpenController::scanDirectory(rootDir.absolutePath(), true);
    QCOMPARE(rec.size(), 5);
    QVERIFY(rec.contains(QFileInfo(f1).absoluteFilePath()));
    QVERIFY(rec.contains(QFileInfo(f2).absoluteFilePath()));
    QVERIFY(rec.contains(QFileInfo(f4).absoluteFilePath()));
    QVERIFY(rec.contains(QFileInfo(f6).absoluteFilePath()));
    QVERIFY(rec.contains(QFileInfo(f7).absoluteFilePath()));

    // Ensure list is sorted
    for (int i = 0; i < rec.size() - 1; ++i) {
        QVERIFY(rec[i] <= rec[i + 1]);
    }

    // Scanning invalid directory returns empty list
    const QStringList invalidScan = ui::MediaOpenController::scanDirectory(QStringLiteral("/path/does/not/exist"));
    QVERIFY(invalidScan.isEmpty());
}

void TestMediaOpenDialogs::testOpenUrlDialogProperties()
{
    ui::OpenUrlDialog dlg;
    QVERIFY(dlg.url().isEmpty());

    const QStringList sampleUrls = {
        QStringLiteral("https://example.com/live/stream.m3u8"),
        QStringLiteral("rtmp://stream.example.org/live/channel1"),
        QStringLiteral("http://icecast.example.com/radio.mp3")
    };

    dlg.setRecentUrls(sampleUrls);
    QCOMPARE(dlg.recentUrls(), sampleUrls);

    dlg.setUrl(QStringLiteral("https://example.com/test.mp4"));
    QCOMPARE(dlg.url(), QStringLiteral("https://example.com/test.mp4"));
}

void TestMediaOpenDialogs::testOpenUrlDialogWhitespaceTrimming()
{
    ui::OpenUrlDialog dlg;
    dlg.setUrl(QStringLiteral("   \t  https://example.com/stream.m3u8  \n "));
    QCOMPARE(dlg.url(), QStringLiteral("https://example.com/stream.m3u8"));
}

void TestMediaOpenDialogs::testOpenUrlDialogValidation()
{
    ui::OpenUrlDialog dlg;
    dlg.setUrl(QStringLiteral("   "));
    // Attempting to accept with an empty URL should not succeed
    dlg.accept();
    QCOMPARE(dlg.result(), 0); // Not accepted

    dlg.setUrl(QStringLiteral("https://example.com/live.m3u8"));
    dlg.accept();
    QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
}

void TestMediaOpenDialogs::testOpenUrlDialogPersistence()
{
    // Test that OpenUrlDialog loads and stores recent URLs to SettingsManager
    core::SettingsManager::instance().set(QStringLiteral("network/recent_urls"),
        QStringList{ QStringLiteral("https://saved1.com/stream.m3u8"), QStringLiteral("https://saved2.com/video.mp4") });

    {
        ui::OpenUrlDialog dlg;
        QCOMPARE(dlg.recentUrls().size(), 2);
        QCOMPARE(dlg.recentUrls().first(), QStringLiteral("https://saved1.com/stream.m3u8"));

        // Enter a new URL and accept
        dlg.setUrl(QStringLiteral("https://brandnew.com/live.ts"));
        dlg.accept();
    }

    // SettingsManager should now have the newly added URL at the top
    const QStringList updated = core::SettingsManager::instance().get(QStringLiteral("network/recent_urls")).toStringList();
    QVERIFY(!updated.isEmpty());
    QCOMPARE(updated.first(), QStringLiteral("https://brandnew.com/live.ts"));
    QCOMPARE(updated.size(), 3);
}

void TestMediaOpenDialogs::testMediaOpenControllerEngineIntegration()
{
    MockPlaybackEngine mockEngine;
    ui::MediaOpenController controller(&mockEngine);
    QCOMPARE(controller.playbackEngine(), &mockEngine);

    MockPlaybackEngine mockEngine2;
    controller.setPlaybackEngine(&mockEngine2);
    QCOMPARE(controller.playbackEngine(), &mockEngine2);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    TestMediaOpenDialogs tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_media_open_dialogs.moc"
