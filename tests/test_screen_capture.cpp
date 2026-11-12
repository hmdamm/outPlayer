// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "core/SettingsManager.h"
#include "engine/IPlaybackEngine.h"
#include "ui/ScreenCaptureController.h"

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
    void seek(double, bool = false) override {}
    double position() const override { return 10.0; }
    double duration() const override { return 100.0; }
    bool isPaused() const override { return false; }

    QList<engine::MediaTrack> tracks(const QString& = {}) const override { return {}; }
    bool setTrack(const QString&, int) override { return true; }
    double volume() const override { return 100.0; }
    void setVolume(double) override {}
    bool isMuted() const override { return false; }
    void setMuted(bool) override {}
    double speed() const override { return 1.0; }
    void setSpeed(double) override {}
    QString aspectRatio() const override { return QStringLiteral("16:9"); }
    void setAspectRatio(const QString&) override {}
    QList<engine::ChapterInfo> chapters() const override { return {}; }
    bool setChapter(int) override { return true; }

    int brightness() const override { return 0; }
    void setBrightness(int) override {}
    int contrast() const override { return 0; }
    void setContrast(int) override {}
    int saturation() const override { return 0; }
    void setSaturation(int) override {}
    int hue() const override { return 0; }
    void setHue(int) override {}
    int rotation() const override { return 0; }
    void setRotation(int) override {}
    bool deinterlace() const override { return false; }
    void setDeinterlace(bool) override {}
    void resetVideoEqualizer() override {}

    double audioDelay() const override { return 0.0; }
    void setAudioDelay(double) override {}
    double equalizerBand(int) const override { return 0.0; }
    void setEqualizerBand(int, double) override {}
    void resetAudioEqualizer() override {}
    bool isNormalizerEnabled() const override { return false; }
    void setNormalizerEnabled(bool) override {}

    double subtitleDelay() const override { return 0.0; }
    void setSubtitleDelay(double) override {}
    double subtitleScale() const override { return 1.0; }
    void setSubtitleScale(double) override {}
    bool loadSubtitle(const QString&) override { return true; }

    bool captureFrame(const QString& destinationPath) override {
        m_capturedPaths.append(destinationPath);
        if (!m_captureSucceeds) {
            return false;
        }
        if (m_writeImageFile) {
            QFileInfo fi(destinationPath);
            QDir dir = fi.dir();
            if (!dir.exists()) {
                dir.mkpath(QStringLiteral("."));
            }
            QImage dummy(80, 60, QImage::Format_RGB32);
            dummy.fill(Qt::blue);
            return dummy.save(destinationPath, "PNG");
        }
        return true;
    }

    bool m_captureSucceeds{true};
    bool m_writeImageFile{true};
    QStringList m_capturedPaths;
};

class TestScreenCapture : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDefaultCaptureFolder();
    void testSetAndPersistCaptureFolder();
    void testCaptureInstantFrameSuccess();
    void testCaptureInstantFrameEngineFailure();
    void testCaptureInstantFrameDirectoryCreationFailure();
    void testCopyFrameToClipboardSuccess();
    void testCopyFrameToClipboardEngineFailure();
    void testCopyFrameToClipboardImageLoadFailure();
    void testNullEngineHandling();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<core::SettingsManager> m_settings;
    std::unique_ptr<MockPlaybackEngine> m_engine;
};

void TestScreenCapture::init() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    const QString iniPath = m_tempDir->filePath("test_capture_settings.ini");

    m_settings = std::make_unique<core::SettingsManager>();
    m_settings->init(iniPath);

    m_engine = std::make_unique<MockPlaybackEngine>();
}

void TestScreenCapture::cleanup() {
    m_engine.reset();
    m_settings.reset();
    m_tempDir.reset();
}

void TestScreenCapture::testDefaultCaptureFolder() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());

    const QString defaultFolder = controller.captureFolder();
    const QString expectedBase = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString expectedFolder = expectedBase.isEmpty()
        ? QDir::home().filePath(QStringLiteral("Pictures/outPlayer"))
        : QDir(expectedBase).filePath(QStringLiteral("outPlayer"));

    QCOMPARE(defaultFolder, expectedFolder);
}

void TestScreenCapture::testSetAndPersistCaptureFolder() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    const QString customFolder = m_tempDir->filePath("custom_snapshots");

    controller.setCaptureFolder(customFolder);
    QCOMPARE(controller.captureFolder(), customFolder);

    // Verify persisted into SettingsManager
    QCOMPARE(m_settings->get(QStringLiteral("capture/folder")).toString(), customFolder);

    // Verify a new controller reads the persisted folder
    ui::ScreenCaptureController controller2(m_engine.get(), m_settings.get());
    QCOMPARE(controller2.captureFolder(), customFolder);
}

void TestScreenCapture::testCaptureInstantFrameSuccess() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    const QString captureDir = m_tempDir->filePath("captures_sub");
    controller.setCaptureFolder(captureDir);

    QSignalSpy spyCaptured(&controller, &ui::ScreenCaptureController::frameCaptured);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const QString capturedFile = controller.captureInstantFrame();
    QVERIFY(!capturedFile.isEmpty());
    QVERIFY(QFile::exists(capturedFile));

    // Verify file is placed in configured directory
    QFileInfo fi(capturedFile);
    QCOMPARE(fi.dir().absolutePath(), QDir(captureDir).absolutePath());

    // Filename format: outPlayer_YYYYMMDD_HHmmss_zzz.png
    const QRegularExpression nameRegex(QStringLiteral("^outPlayer_\\d{8}_\\d{6}_\\d{3}\\.png$"));
    QVERIFY2(nameRegex.match(fi.fileName()).hasMatch(), qPrintable(fi.fileName()));

    // Verify signal emission
    QCOMPARE(spyCaptured.count(), 1);
    QCOMPARE(spyCaptured.at(0).at(0).toString(), capturedFile);
    QCOMPARE(spyError.count(), 0);

    // Verify engine was invoked with the exact path
    QCOMPARE(m_engine->m_capturedPaths.size(), 1);
    QCOMPARE(m_engine->m_capturedPaths.first(), capturedFile);
}

void TestScreenCapture::testCaptureInstantFrameEngineFailure() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    controller.setCaptureFolder(m_tempDir->filePath("captures_fail"));
    m_engine->m_captureSucceeds = false;

    QSignalSpy spyCaptured(&controller, &ui::ScreenCaptureController::frameCaptured);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const QString capturedFile = controller.captureInstantFrame();
    QVERIFY(capturedFile.isEmpty());
    QCOMPARE(spyCaptured.count(), 0);
    QCOMPARE(spyError.count(), 1);
    QVERIFY(!spyError.at(0).at(0).toString().isEmpty());
}

void TestScreenCapture::testCaptureInstantFrameDirectoryCreationFailure() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    // An invalid directory path that cannot be created
    const QString invalidDir = QStringLiteral("/proc/sys/outplayer_invalid_dir");
    controller.setCaptureFolder(invalidDir);

    QSignalSpy spyCaptured(&controller, &ui::ScreenCaptureController::frameCaptured);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const QString capturedFile = controller.captureInstantFrame();
    QVERIFY(capturedFile.isEmpty());
    QCOMPARE(spyCaptured.count(), 0);
    QCOMPARE(spyError.count(), 1);
}

void TestScreenCapture::testCopyFrameToClipboardSuccess() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());

    QSignalSpy spyCopied(&controller, &ui::ScreenCaptureController::frameCopiedToClipboard);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const bool result = controller.copyFrameToClipboard();
    QVERIFY(result);
    QCOMPARE(spyCopied.count(), 1);
    QCOMPARE(spyError.count(), 0);

    // Verify clipboard contains the image
    QClipboard* clipboard = QGuiApplication::clipboard();
    QVERIFY(clipboard != nullptr);
    const QImage clipImg = clipboard->image();
    QVERIFY(!clipImg.isNull());
    QCOMPARE(clipImg.size(), QSize(80, 60));

    // Verify engine captured to a temp path and the temp file was removed afterwards
    QVERIFY(!m_engine->m_capturedPaths.isEmpty());
    const QString tempCaptured = m_engine->m_capturedPaths.last();
    QVERIFY(!QFile::exists(tempCaptured));
}

void TestScreenCapture::testCopyFrameToClipboardEngineFailure() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    m_engine->m_captureSucceeds = false;

    QSignalSpy spyCopied(&controller, &ui::ScreenCaptureController::frameCopiedToClipboard);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const bool result = controller.copyFrameToClipboard();
    QVERIFY(!result);
    QCOMPARE(spyCopied.count(), 0);
    QCOMPARE(spyError.count(), 1);
}

void TestScreenCapture::testCopyFrameToClipboardImageLoadFailure() {
    ui::ScreenCaptureController controller(m_engine.get(), m_settings.get());
    // Engine reports success but does not write a valid image file
    m_engine->m_writeImageFile = false;

    QSignalSpy spyCopied(&controller, &ui::ScreenCaptureController::frameCopiedToClipboard);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const bool result = controller.copyFrameToClipboard();
    QVERIFY(!result);
    QCOMPARE(spyCopied.count(), 0);
    QCOMPARE(spyError.count(), 1);
}

void TestScreenCapture::testNullEngineHandling() {
    ui::ScreenCaptureController controller(nullptr, m_settings.get());

    QSignalSpy spyCaptured(&controller, &ui::ScreenCaptureController::frameCaptured);
    QSignalSpy spyCopied(&controller, &ui::ScreenCaptureController::frameCopiedToClipboard);
    QSignalSpy spyError(&controller, &ui::ScreenCaptureController::captureError);

    const QString instantResult = controller.captureInstantFrame();
    QVERIFY(instantResult.isEmpty());
    QCOMPARE(spyCaptured.count(), 0);
    QCOMPARE(spyError.count(), 1);

    const bool clipResult = controller.copyFrameToClipboard();
    QVERIFY(!clipResult);
    QCOMPARE(spyCopied.count(), 0);
    QCOMPARE(spyError.count(), 2);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TestScreenCapture tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_screen_capture.moc"
