// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "core/DeviceStreamManager.h"

using namespace outplayer::core;

class TestDeviceStreamManager : public QObject {
    Q_OBJECT

private slots:
    void testIsStreamUrl();
    void testFormatDvdUrl();
    void testFormatBlurayUrl();
    void testFormatV4l2Url();
    void testAvailableVideoDevicesEmpty();
    void testAvailableVideoDevicesMock();
    void testAvailableOpticalDrivesEmpty();
    void testAvailableOpticalDrivesMock();
    void testDeviceInfoEquality();
};

void TestDeviceStreamManager::testIsStreamUrl() {
    // Valid stream schemes
    QVERIFY(DeviceStreamManager::isStreamUrl("http://example.com/live/stream.m3u8"));
    QVERIFY(DeviceStreamManager::isStreamUrl("https://stream.provider.org/video.mp4"));
    QVERIFY(DeviceStreamManager::isStreamUrl("rtsp://192.168.1.100:554/live"));
    QVERIFY(DeviceStreamManager::isStreamUrl("rtmp://live.streaming.tv/app/stream"));
    QVERIFY(DeviceStreamManager::isStreamUrl("mms://ms.media.server/stream"));
    QVERIFY(DeviceStreamManager::isStreamUrl("hls://cdn.net/hls/manifest"));
    QVERIFY(DeviceStreamManager::isStreamUrl("dash://cdn.net/dash/manifest.mpd"));
    QVERIFY(DeviceStreamManager::isStreamUrl("dvdnav:///dev/sr0"));
    QVERIFY(DeviceStreamManager::isStreamUrl("dvd:///media/dvd"));
    QVERIFY(DeviceStreamManager::isStreamUrl("bd:///dev/sr0"));
    QVERIFY(DeviceStreamManager::isStreamUrl("bluray:///dev/sr0"));
    QVERIFY(DeviceStreamManager::isStreamUrl("v4l2:///dev/video0"));
    QVERIFY(DeviceStreamManager::isStreamUrl("avdevice://v4l2:/dev/video0"));
    QVERIFY(DeviceStreamManager::isStreamUrl("ytdl://https://youtube.com/watch?v=123"));

    // Case-insensitivity & leading whitespace
    QVERIFY(DeviceStreamManager::isStreamUrl("HTTP://stream.com/live"));
    QVERIFY(DeviceStreamManager::isStreamUrl("  https://secure.stream.org/feed  "));
    QVERIFY(DeviceStreamManager::isStreamUrl("RTSP://10.0.0.1/stream"));

    // Non-stream URLs / local files
    QVERIFY(!DeviceStreamManager::isStreamUrl(""));
    QVERIFY(!DeviceStreamManager::isStreamUrl("   "));
    QVERIFY(!DeviceStreamManager::isStreamUrl("/home/user/Videos/movie.mkv"));
    QVERIFY(!DeviceStreamManager::isStreamUrl("C:/Videos/movie.mp4"));
    QVERIFY(!DeviceStreamManager::isStreamUrl("file:///home/user/movie.mkv"));
    QVERIFY(!DeviceStreamManager::isStreamUrl("ftp://fileserver/movie.mp4"));
    QVERIFY(!DeviceStreamManager::isStreamUrl("relative/path/video.avi"));
}

void TestDeviceStreamManager::testFormatDvdUrl() {
    QCOMPARE(DeviceStreamManager::formatDvdUrl(), QStringLiteral("dvdnav://"));
    QCOMPARE(DeviceStreamManager::formatDvdUrl(""), QStringLiteral("dvdnav://"));
    QCOMPARE(DeviceStreamManager::formatDvdUrl("/dev/sr0"), QStringLiteral("dvdnav:///dev/sr0"));
    QCOMPARE(DeviceStreamManager::formatDvdUrl("/media/dvd"), QStringLiteral("dvdnav:///media/dvd"));
    QCOMPARE(DeviceStreamManager::formatDvdUrl("dvdnav:///media/dvd"), QStringLiteral("dvdnav:///media/dvd"));
}

void TestDeviceStreamManager::testFormatBlurayUrl() {
    QCOMPARE(DeviceStreamManager::formatBlurayUrl(), QStringLiteral("bd://"));
    QCOMPARE(DeviceStreamManager::formatBlurayUrl(""), QStringLiteral("bd://"));
    QCOMPARE(DeviceStreamManager::formatBlurayUrl("/dev/sr0"), QStringLiteral("bd:///dev/sr0"));
    QCOMPARE(DeviceStreamManager::formatBlurayUrl("/mnt/bluray"), QStringLiteral("bd:///mnt/bluray"));
    QCOMPARE(DeviceStreamManager::formatBlurayUrl("bd:///mnt/bluray"), QStringLiteral("bd:///mnt/bluray"));
}

void TestDeviceStreamManager::testFormatV4l2Url() {
    QCOMPARE(DeviceStreamManager::formatV4l2Url(), QStringLiteral("avdevice://v4l2"));
    QCOMPARE(DeviceStreamManager::formatV4l2Url(""), QStringLiteral("avdevice://v4l2"));
    QCOMPARE(DeviceStreamManager::formatV4l2Url("/dev/video0"), QStringLiteral("avdevice://v4l2:/dev/video0"));
    QCOMPARE(DeviceStreamManager::formatV4l2Url("avdevice://v4l2:/dev/video0"), QStringLiteral("avdevice://v4l2:/dev/video0"));
}

void TestDeviceStreamManager::testAvailableVideoDevicesEmpty() {
    DeviceStreamManager manager;
    // Missing directory should safely return empty list without throwing or crashing
    auto devices = manager.availableVideoDevices("/non_existent_dev_dir_12345", "/non_existent_sys_dir_12345");
    QVERIFY(devices.isEmpty());
}

void TestDeviceStreamManager::testAvailableVideoDevicesMock() {
    DeviceStreamManager manager;
    QTemporaryDir tempDev;
    QTemporaryDir tempSys;
    QVERIFY(tempDev.isValid());
    QVERIFY(tempSys.isValid());

    // Create mock devices: video0, video1, and a non-video node audio0
    QFile v0(tempDev.filePath("video0"));
    QVERIFY(v0.open(QIODevice::WriteOnly));
    v0.close();

    QFile v1(tempDev.filePath("video1"));
    QVERIFY(v1.open(QIODevice::WriteOnly));
    v1.close();

    QFile a0(tempDev.filePath("audio0"));
    QVERIFY(a0.open(QIODevice::WriteOnly));
    a0.close();

    // Create sysfs info for video0
    QDir sysDir(tempSys.path());
    QVERIFY(sysDir.mkpath("video0"));
    QFile sysName(tempSys.filePath("video0/name"));
    QVERIFY(sysName.open(QIODevice::WriteOnly));
    sysName.write("Integrated Webcam HD\n");
    sysName.close();

    // video1 has no sysfs name (fallback test)

    auto devices = manager.availableVideoDevices(tempDev.path(), tempSys.path());
    QCOMPARE(devices.size(), 2);

    QCOMPARE(devices[0].path, tempDev.filePath("video0"));
    QCOMPARE(devices[0].name, QStringLiteral("Integrated Webcam HD"));
    QCOMPARE(devices[0].type, QStringLiteral("v4l2"));

    QCOMPARE(devices[1].path, tempDev.filePath("video1"));
    QCOMPARE(devices[1].name, QStringLiteral("Video Device %1").arg(tempDev.filePath("video1")));
    QCOMPARE(devices[1].type, QStringLiteral("v4l2"));
}

void TestDeviceStreamManager::testAvailableOpticalDrivesEmpty() {
    DeviceStreamManager manager;
    auto drives = manager.availableOpticalDrives("/non_existent_optical_dev_dir");
    QVERIFY(drives.isEmpty());
}

void TestDeviceStreamManager::testAvailableOpticalDrivesMock() {
    DeviceStreamManager manager;
    QTemporaryDir tempDev;
    QTemporaryDir tempSys;
    QVERIFY(tempDev.isValid());
    QVERIFY(tempSys.isValid());

    // Create mock nodes: cdrom, dvd, sr0, and random_file
    QFile cdrom(tempDev.filePath("cdrom"));
    QVERIFY(cdrom.open(QIODevice::WriteOnly));
    cdrom.close();

    QFile dvd(tempDev.filePath("dvd"));
    QVERIFY(dvd.open(QIODevice::WriteOnly));
    dvd.close();

    QFile sr0(tempDev.filePath("sr0"));
    QVERIFY(sr0.open(QIODevice::WriteOnly));
    sr0.close();

    QFile other(tempDev.filePath("sda"));
    QVERIFY(other.open(QIODevice::WriteOnly));
    other.close();

    // Create sysfs model for sr0
    QDir sysDir(tempSys.path());
    QVERIFY(sysDir.mkpath("sr0/device"));
    QFile sysModel(tempSys.filePath("sr0/device/model"));
    QVERIFY(sysModel.open(QIODevice::WriteOnly));
    sysModel.write("BD-RE WH16NS40\n");
    sysModel.close();

    auto drives = manager.availableOpticalDrives(tempDev.path(), tempSys.path());
    QCOMPARE(drives.size(), 3);

    // Verify types are optical
    for (const auto& drive : drives) {
        QCOMPARE(drive.type, QStringLiteral("optical"));
    }

    // Verify sr0 gets model name from sysfs
    auto itSr0 = std::find_if(drives.begin(), drives.end(), [&](const DeviceInfo& d) {
        return d.path == tempDev.filePath("sr0");
    });
    QVERIFY(itSr0 != drives.end());
    QVERIFY(itSr0->name.contains("BD-RE WH16NS40"));
}

void TestDeviceStreamManager::testDeviceInfoEquality() {
    DeviceInfo d1{ "Cam", "/dev/video0", "v4l2" };
    DeviceInfo d2{ "Cam", "/dev/video0", "v4l2" };
    DeviceInfo d3{ "Other", "/dev/video1", "v4l2" };

    QVERIFY(d1 == d2);
    QVERIFY(!(d1 == d3));
}

QTEST_MAIN(TestDeviceStreamManager)
#include "test_device_stream_manager.moc"
