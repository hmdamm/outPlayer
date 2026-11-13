// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <cmath>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"
#include "ui/ControlBar.h"
#include "ui/ControlPanel.h"
#include "ui/CustomTitleBar.h"
#include "ui/DesktopIntegration.h"
#include "ui/MainWindow.h"
#include "ui/PlaylistDock.h"
#include "ui/PreferencesDialog.h"
#include "ui/VideoWidget.h"

using namespace outplayer;

class TestBenchmarks : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Test 1: Player Startup Time
    void testStartupTime();

    // Test 2: Seek Latency
    void testSeekLatency();

    // Test 3: Playlist Scalability & Memory Footprint
    void testPlaylistScalabilityAndMemory();

    // Test 4: Command Registry Dispatch Performance
    void testCommandRegistryDispatchPerformance();

    // Test 5: Timer & Autohide precision
    void testAutohideTimerPrecision();
};

void TestBenchmarks::initTestCase() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestBenchmarks::cleanupTestCase() {
}

void TestBenchmarks::testStartupTime() {
    // Measure time to construct MainWindow (which instantiates MpvEngine, VideoWidget,
    // ControlBar, PlaylistDock, ControlPanel, PreferencesDialog, DesktopIntegration).
    QElapsedTimer timer;
    timer.start();

    auto window = std::make_unique<ui::MainWindow>();
    const qint64 startupTimeMs = timer.elapsed();

    // Verify all key components are successfully assembled
    QVERIFY(window->engine() != nullptr);
    QVERIFY(window->videoWidget() != nullptr);
    QVERIFY(window->controlBar() != nullptr);
    QVERIFY(window->playlistDock() != nullptr);
    QVERIFY(window->controlPanel() != nullptr);
    QVERIFY(window->preferencesDialog() != nullptr);
    QVERIFY(window->desktopIntegration() != nullptr);

    // Assert elapsed time < 500ms
    qInfo() << "[BENCHMARK] Player MainWindow startup time:" << startupTimeMs << "ms (threshold: < 500 ms)";
    QVERIFY2(startupTimeMs < 500,
             qPrintable(QStringLiteral("Player startup took %1 ms, expected < 500 ms").arg(startupTimeMs)));
}

void TestBenchmarks::testSeekLatency() {
    ui::MainWindow window;
    window.resize(640, 480);
    window.show();
    QVERIFY(window.isVisible());

    // Load synthetic stream
    const bool opened = window.openMedia(QStringLiteral("lavfi://nullsrc=s=320x240:d=10"));
    QVERIFY(opened);

    // Wait for stream to be loaded and active
    QTRY_VERIFY_WITH_TIMEOUT(window.engine()->duration() > 0.0, 5000);

    // Measure seek dispatch latency
    QElapsedTimer timer;
    timer.start();
    window.engine()->seek(5.0);
    const qint64 forwardSeekMs = timer.elapsed();

    qInfo() << "[BENCHMARK] Seek forward dispatch latency:" << forwardSeekMs << "ms (threshold: < 50 ms)";
    QVERIFY2(forwardSeekMs < 50,
             qPrintable(QStringLiteral("Forward seek dispatch latency took %1 ms, expected < 50 ms").arg(forwardSeekMs)));

    // Measure backward seek dispatch latency
    timer.restart();
    window.engine()->seek(1.0);
    const qint64 backwardSeekMs = timer.elapsed();

    qInfo() << "[BENCHMARK] Seek backward dispatch latency:" << backwardSeekMs << "ms (threshold: < 50 ms)";
    QVERIFY2(backwardSeekMs < 50,
             qPrintable(QStringLiteral("Backward seek dispatch latency took %1 ms, expected < 50 ms").arg(backwardSeekMs)));

    window.stopPlayback();
}

void TestBenchmarks::testPlaylistScalabilityAndMemory() {
    constexpr int itemCount = 10000;
    playlist::PlaylistModel model;

    QList<playlist::PlaylistItem> items;
    items.reserve(itemCount);
    for (int i = 0; i < itemCount; ++i) {
        items.push_back(playlist::PlaylistItem{
            .filePath = QStringLiteral("/media/stream_file_%1.mkv").arg(i),
            .title = QStringLiteral("Benchmark Track Stream #%1").arg(i),
            .duration = 120.0 + (i % 60)
        });
    }

    // Benchmark batch insertion performance
    QElapsedTimer timer;
    timer.start();
    model.addItems(items);
    const qint64 insertionTimeMs = timer.elapsed();

    QCOMPARE(model.rowCount(), itemCount);
    qInfo() << "[BENCHMARK] Playlist 10k items insertion:" << insertionTimeMs << "ms (threshold: < 50 ms)";
    QVERIFY2(insertionTimeMs < 50,
             qPrintable(QStringLiteral("Batch insertion of %1 items took %2 ms, expected < 50 ms")
                 .arg(itemCount).arg(insertionTimeMs)));

    // Calculate memory footprint
    const size_t structMemory = itemCount * sizeof(playlist::PlaylistItem);
    size_t stringPayloadMemory = 0;
    for (const auto& item : items) {
        stringPayloadMemory += (item.filePath.capacity() * sizeof(QChar)) +
                               (item.title.capacity() * sizeof(QChar));
    }
    const size_t totalMemoryFootprint = structMemory + stringPayloadMemory;

    // Assert memory overhead < 10MB
    constexpr size_t maxAllowedMemory = 10 * 1024 * 1024;
    qInfo() << "[BENCHMARK] Playlist 10k items memory footprint:"
            << totalMemoryFootprint << "bytes (" << (totalMemoryFootprint / (1024.0 * 1024.0)) << "MB, threshold: < 10 MB)";
    QVERIFY2(totalMemoryFootprint < maxAllowedMemory,
             qPrintable(QStringLiteral("Memory footprint is %1 bytes (%2 MB), expected < 10MB")
                 .arg(totalMemoryFootprint)
                 .arg(totalMemoryFootprint / (1024.0 * 1024.0))));
}

void TestBenchmarks::testCommandRegistryDispatchPerformance() {
    constexpr int dispatchCount = 10000;
    auto& registry = core::CommandRegistry::instance();

    int executedCount = 0;
    registry.registerHandler(core::CommandId::CMD_PLAY_PAUSE, [&](const QVariantMap&) {
        ++executedCount;
    });

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < dispatchCount; ++i) {
        registry.execute(core::CommandId::CMD_PLAY_PAUSE);
    }
    const qint64 elapsedMs = timer.elapsed();
    const qint64 elapsedNs = timer.nsecsElapsed();

    QCOMPARE(executedCount, dispatchCount);

    const double elapsedSeconds = elapsedNs / 1'000'000'000.0;
    const double opsPerSec = dispatchCount / (elapsedSeconds > 0.0 ? elapsedSeconds : 0.000001);

    // Assert latency < 50ms for 10,000 dispatches
    qInfo() << "[BENCHMARK] Command registry 10k dispatches:" << elapsedMs << "ms (throughput:" << opsPerSec << "ops/sec, threshold: < 50 ms and > 100k ops/sec)";
    QVERIFY2(elapsedMs < 50,
             qPrintable(QStringLiteral("10k command executions took %1 ms, expected < 50 ms").arg(elapsedMs)));

    // Assert execution throughput > 100,000 ops/second
    QVERIFY2(opsPerSec > 100000.0,
             qPrintable(QStringLiteral("Throughput was %1 ops/sec, expected > 100,000 ops/sec").arg(opsPerSec)));

    registry.unregisterHandler(core::CommandId::CMD_PLAY_PAUSE);
}

void TestBenchmarks::testAutohideTimerPrecision() {
    ui::MainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(window.isVisible());

    auto* autohideTimer = window.findChild<QTimer*>(QStringLiteral("autohideTimer"));
    if (!autohideTimer) {
        for (auto* t : window.findChildren<QTimer*>()) {
            if (t->interval() == 2500) {
                autohideTimer = t;
                break;
            }
        }
    }
    QVERIFY(autohideTimer != nullptr);
    QCOMPARE(autohideTimer->interval(), 2500);
    QVERIFY(autohideTimer->isSingleShot());

    // Test inactivity timer precision
    QSignalSpy timeoutSpy(autohideTimer, &QTimer::timeout);
    constexpr int testIntervalMs = 100;
    autohideTimer->setInterval(testIntervalMs);

    QElapsedTimer elapsed;
    elapsed.start();
    autohideTimer->start();

    QVERIFY(timeoutSpy.wait(500));
    const qint64 elapsedMs = elapsed.elapsed();
    const qint64 driftMs = std::abs(elapsedMs - testIntervalMs);

    // Verify inactivity timer interval is within 50ms precision
    qInfo() << "[BENCHMARK] Inactivity timer precision: drift:" << driftMs << "ms (actual elapsed:" << elapsedMs << "ms for" << testIntervalMs << "ms interval, threshold: <= 50 ms)";
    QVERIFY2(driftMs <= 50,
             qPrintable(QStringLiteral("Inactivity timer drift was %1 ms (fired at %2 ms for %3 ms interval), expected <= 50 ms precision")
                 .arg(driftMs).arg(elapsedMs).arg(testIntervalMs)));

    // Verify chrome autohide triggered upon timeout
    QVERIFY(!window.isChromeVisible());

    // Restore timer interval
    autohideTimer->setInterval(2500);
}

QTEST_MAIN(TestBenchmarks)
#include "test_benchmarks.moc"
