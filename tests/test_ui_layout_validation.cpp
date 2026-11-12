// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QEnterEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QWidget>

#include "core/SettingsManager.h"
#include "playlist/PlaylistModel.h"
#include "ui/ControlBar.h"
#include "ui/ControlPanel.h"
#include "ui/CustomTitleBar.h"
#include "ui/MainWindow.h"
#include "ui/OsdOverlay.h"
#include "ui/PlaylistDock.h"
#include "ui/TimelineSlider.h"
#include "ui/VideoWidget.h"

using namespace outplayer;

class TestUiLayoutValidation : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    // Test 1: CustomTitleBar geometry & metrics
    void testCustomTitleBarGeometryAndMetrics();

    // Test 2: ControlBar geometry & metrics
    void testControlBarGeometryAndMetrics();

    // Test 3: TimelineSlider geometry & styling
    void testTimelineSliderGeometryAndStyling();

    // Test 4: PlaylistDock geometry & layout
    void testPlaylistDockGeometryAndLayout();

    // Test 5: ControlPanel window properties
    void testControlPanelWindowProperties();

    // Test 6: OsdOverlay window flags & behavior
    void testOsdOverlayWindowFlags();

    // Test 7: Full MainWindow composite layout & resize margins
    void testMainWindowCompositeLayout();

    // Test 8: Clean-room brand sanitization
    void testCleanRoomBrandSanitization();
};

void TestUiLayoutValidation::initTestCase() {
    QVERIFY(qApp != nullptr);
}

void TestUiLayoutValidation::cleanup() {
    // Restore default settings
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestUiLayoutValidation::testCustomTitleBarGeometryAndMetrics() {
    ui::CustomTitleBar titleBar;

    // Height contract: exactly 30px
    QCOMPARE(titleBar.sizeHint().height(), 30);
    QCOMPARE(titleBar.minimumHeight(), 30);
    QCOMPARE(titleBar.maximumHeight(), 30);
    QCOMPARE(titleBar.minimumSizeHint().height(), 30);

    // Default style verification: #1b1b1b background and #242424 border
    const QString style = titleBar.styleSheet();
    QVERIFY2(style.contains(QStringLiteral("#1b1b1b")), "TitleBar must use #1b1b1b base dark background");
    QVERIFY2(style.contains(QStringLiteral("#242424")), "TitleBar must use #242424 border divider");

    // Child elements existence and dimensions
    QVERIFY(titleBar.titleLabel() != nullptr);
    QVERIFY(titleBar.iconLabel() != nullptr);
    QVERIFY(titleBar.minimizeButton() != nullptr);
    QVERIFY(titleBar.maximizeButton() != nullptr);
    QVERIFY(titleBar.closeButton() != nullptr);

    // Window control buttons contract: 45x30px
    QCOMPARE(titleBar.minimizeButton()->width(), 45);
    QCOMPARE(titleBar.minimizeButton()->height(), 30);
    QCOMPARE(titleBar.maximizeButton()->width(), 45);
    QCOMPARE(titleBar.maximizeButton()->height(), 30);
    QCOMPARE(titleBar.closeButton()->width(), 45);
    QCOMPARE(titleBar.closeButton()->height(), 30);
}

void TestUiLayoutValidation::testControlBarGeometryAndMetrics() {
    ui::ControlBar controlBar;

    // Height contract: exactly 40px
    QCOMPARE(controlBar.sizeHint().height(), 40);
    QCOMPARE(controlBar.minimumHeight(), 40);
    QCOMPARE(controlBar.maximumHeight(), 40);
    QCOMPARE(controlBar.minimumSizeHint().height(), 40);

    // Style and palette verification: #252525 background, #2e2e2e top border
    const QString style = controlBar.styleSheet();
    QVERIFY2(style.contains(QStringLiteral("#252525")), "ControlBar must use #252525 base background");
    QVERIFY2(style.contains(QStringLiteral("#2e2e2e")), "ControlBar must use #2e2e2e border separator");

    // Sub-controls presence and metrics
    QVERIFY(controlBar.playPauseButton() != nullptr);
    QCOMPARE(controlBar.playPauseButton()->width(), 28);
    QCOMPARE(controlBar.playPauseButton()->height(), 28);

    QVERIFY(controlBar.stopButton() != nullptr);
    QCOMPARE(controlBar.stopButton()->width(), 24);
    QCOMPARE(controlBar.stopButton()->height(), 24);

    QVERIFY(controlBar.prevButton() != nullptr);
    QCOMPARE(controlBar.prevButton()->width(), 24);
    QCOMPARE(controlBar.prevButton()->height(), 24);

    QVERIFY(controlBar.nextButton() != nullptr);
    QCOMPARE(controlBar.nextButton()->width(), 24);
    QCOMPARE(controlBar.nextButton()->height(), 24);

    QVERIFY(controlBar.muteButton() != nullptr);
    QCOMPARE(controlBar.muteButton()->width(), 24);
    QCOMPARE(controlBar.muteButton()->height(), 24);

    QVERIFY(controlBar.volumeSlider() != nullptr);
    QCOMPARE(controlBar.volumeSlider()->width(), 70);

    QVERIFY(controlBar.subtitleButton() != nullptr);
    QCOMPARE(controlBar.subtitleButton()->width(), 24);
    QCOMPARE(controlBar.subtitleButton()->height(), 24);

    QVERIFY(controlBar.playlistButton() != nullptr);
    QCOMPARE(controlBar.playlistButton()->width(), 24);
    QCOMPARE(controlBar.playlistButton()->height(), 24);

    QVERIFY(controlBar.controlPanelButton() != nullptr);
    QCOMPARE(controlBar.controlPanelButton()->width(), 24);
    QCOMPARE(controlBar.controlPanelButton()->height(), 24);

    QVERIFY(controlBar.timeLabel() != nullptr);
    QVERIFY(controlBar.timelineSlider() != nullptr);
}

void TestUiLayoutValidation::testTimelineSliderGeometryAndStyling() {
    ui::TimelineSlider slider;

    // Height contract: exactly 14px
    QCOMPARE(slider.sizeHint().height(), 14);
    QCOMPARE(slider.minimumHeight(), 14);
    QCOMPARE(slider.maximumHeight(), 14);
    QCOMPARE(slider.minimumSizeHint().height(), 14);
    QCOMPARE(slider.sizeHint().width(), 200);

    slider.resize(200, 14);
    slider.setDuration(100.0);
    slider.setPosition(50.0); // 50% played

    // Offscreen paint test using QPixmap(200, 14) and render()
    QPixmap pixmap(200, 14);
    pixmap.fill(Qt::transparent);
    slider.render(&pixmap);

    QVERIFY(!pixmap.isNull());
    QImage img = pixmap.toImage();

    int nonTransparentCount = 0;
    for (int y = 0; y < 14; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (img.pixelColor(x, y).alpha() > 0) {
                nonTransparentCount++;
            }
        }
    }
    QVERIFY2(nonTransparentCount > 100, "Rendered timeline slider must not be empty");

    // Validate groove height (4px) centered vertically at (14 - 4) / 2 = 5 (y: 5..8)
    // At y=7 (centerline):
    // In unhovered idle state:
    // First 50% (x=30) has progress color #e09b1f
    const QColor progressColor = img.pixelColor(30, 7);
    QCOMPARE(progressColor.rgb(), QColor(0xe0, 0x9b, 0x1f).rgb());

    // Remaining 50% (x=170) has groove color #383838
    const QColor grooveColor = img.pixelColor(170, 7);
    QCOMPARE(grooveColor.rgb(), QColor(0x38, 0x38, 0x38).rgb());

    // Test hover transition to #ffaa00 via enterEvent
    slider.show();
    QEnterEvent enterEv(QPointF(30, 7), QPointF(30, 7), slider.mapToGlobal(QPoint(30, 7)));
    QCoreApplication::sendEvent(&slider, &enterEv);

    QPixmap hoverPixmap(200, 14);
    hoverPixmap.fill(Qt::transparent);
    slider.render(&hoverPixmap);

    QImage hoverImg = hoverPixmap.toImage();
    const QColor hoverProgressColor = hoverImg.pixelColor(30, 7);
    QCOMPARE(hoverProgressColor.rgb(), QColor(0xff, 0xaa, 0x00).rgb());
}

void TestUiLayoutValidation::testPlaylistDockGeometryAndLayout() {
    playlist::PlaylistModel model;
    ui::PlaylistDock dock(&model);

    // Dimension constraints: default 300px, min 200px, max 600px
    QCOMPARE(dock.sizeHint().width(), 300);
    QCOMPARE(dock.width(), 300);
    QCOMPARE(dock.minimumWidth(), 200);
    QCOMPARE(dock.maximumWidth(), 600);

    // Category tabs count: 4 tabs
    QVERIFY(dock.tabBar() != nullptr);
    QCOMPARE(dock.tabBar()->count(), 4);

    // Tab names contract
    QCOMPARE(dock.tabBar()->tabText(0), QStringLiteral("Default"));
    QCOMPARE(dock.tabBar()->tabText(1), QStringLiteral("Favorites"));
    QCOMPARE(dock.tabBar()->tabText(2), QStringLiteral("Albums"));
    QCOMPARE(dock.tabBar()->tabText(3), QStringLiteral("History"));

    // Sub-components presence
    QVERIFY(dock.searchEdit() != nullptr);
    QVERIFY(dock.listView() != nullptr);
    QVERIFY(dock.addButton() != nullptr);
    QVERIFY(dock.removeButton() != nullptr);
    QVERIFY(dock.sortButton() != nullptr);
    QVERIFY(dock.menuButton() != nullptr);
    QVERIFY(dock.closeButton() != nullptr);
}

void TestUiLayoutValidation::testControlPanelWindowProperties() {
    ui::ControlPanel panel(nullptr);

    // Window flags include Qt::Tool
    QVERIFY2(panel.windowFlags().testFlag(Qt::Tool), "ControlPanel must have Qt::Tool window flag");

    // 4 tabs present with exact names
    QVERIFY(panel.tabWidget() != nullptr);
    QCOMPARE(panel.tabWidget()->count(), 4);
    QCOMPARE(panel.tabWidget()->tabText(0), QStringLiteral("Video"));
    QCOMPARE(panel.tabWidget()->tabText(1), QStringLiteral("Audio / EQ"));
    QCOMPARE(panel.tabWidget()->tabText(2), QStringLiteral("Subtitles"));
    QCOMPARE(panel.tabWidget()->tabText(3), QStringLiteral("Playback"));

    // Dimensions ~420x360px
    QCOMPARE(panel.sizeHint(), QSize(440, 380));
    QCOMPARE(panel.minimumSize(), QSize(400, 340));
    QVERIFY(panel.sizeHint().width() >= 400 && panel.sizeHint().width() <= 460);
    QVERIFY(panel.sizeHint().height() >= 340 && panel.sizeHint().height() <= 400);
}

void TestUiLayoutValidation::testOsdOverlayWindowFlags() {
    ui::OsdOverlay osd;

    // OSD overlay flags: transparent for mouse events and translucent background
    QVERIFY(osd.testAttribute(Qt::WA_TransparentForMouseEvents));
    QVERIFY(osd.testAttribute(Qt::WA_TranslucentBackground));
    QVERIFY(osd.testAttribute(Qt::WA_NoSystemBackground));

    // Parent geometry tracking
    QWidget parentWidget;
    parentWidget.resize(800, 600);
    parentWidget.show();

    ui::OsdOverlay attachedOsd(&parentWidget);
    QCOMPARE(attachedOsd.geometry(), parentWidget.rect());

    parentWidget.resize(1024, 768);
    QCOMPARE(attachedOsd.geometry(), parentWidget.rect());
}

void TestUiLayoutValidation::testMainWindowCompositeLayout() {
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);

    ui::MainWindow window;
    window.resize(1280, 720);
    window.show();
    QVERIFY(window.isVisible());

    // 1. CustomTitleBar at top (y=0, h=30, w=1280)
    QVERIFY(window.titleBar() != nullptr);
    QVERIFY(!window.titleBar()->isHidden());
    QCOMPARE(window.titleBar()->geometry(), QRect(0, 0, 1280, 30));

    // 2. ControlBar at bottom (y=680, h=40, w=1280)
    QVERIFY(window.controlBar() != nullptr);
    QVERIFY(!window.controlBar()->isHidden());
    QCOMPARE(window.controlBar()->geometry(), QRect(0, 720 - 40, 1280, 40));

    // 3. Central container contains VideoWidget and PlaylistDock
    QVERIFY(window.videoWidget() != nullptr);
    QVERIFY(window.playlistDock() != nullptr);
    QVERIFY(window.playlistDock()->isHidden()); // Hidden by default

    // Toggle playlist visible
    window.setPlaylistVisible(true);
    QVERIFY(window.playlistDock()->isVisible());
    QCOMPARE(window.playlistDock()->width(), 300);

    window.setPlaylistVisible(false);
    QVERIFY(window.playlistDock()->isHidden());

    // 4. Frameless resize margin verification (margin = 6px)
    auto sendMouseMove = [&window](const QPoint& pt) {
        const QPoint globalPt = window.mapToGlobal(pt);
        QMouseEvent ev(QEvent::MouseMove, pt, globalPt, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &ev);
    };

    // Left edge (x=2, y=360) -> Qt::SizeHorCursor
    sendMouseMove(QPoint(2, 360));
    QCOMPARE(window.cursor().shape(), Qt::SizeHorCursor);

    // Right edge (x=1278, y=360) -> Qt::SizeHorCursor
    sendMouseMove(QPoint(1278, 360));
    QCOMPARE(window.cursor().shape(), Qt::SizeHorCursor);

    // Top edge (x=640, y=2) -> Qt::SizeVerCursor
    sendMouseMove(QPoint(640, 2));
    QCOMPARE(window.cursor().shape(), Qt::SizeVerCursor);

    // Bottom edge (x=640, y=718) -> Qt::SizeVerCursor
    sendMouseMove(QPoint(640, 718));
    QCOMPARE(window.cursor().shape(), Qt::SizeVerCursor);

    // Top-Left corner (x=2, y=2) -> Qt::SizeFDiagCursor
    sendMouseMove(QPoint(2, 2));
    QCOMPARE(window.cursor().shape(), Qt::SizeFDiagCursor);

    // Top-Right corner (x=1278, y=2) -> Qt::SizeBDiagCursor
    sendMouseMove(QPoint(1278, 2));
    QCOMPARE(window.cursor().shape(), Qt::SizeBDiagCursor);

    // Center area (x=640, y=360) inside chrome -> Arrow / unset cursor
    sendMouseMove(QPoint(640, 360));
    QVERIFY(window.cursor().shape() != Qt::SizeHorCursor);
    QVERIFY(window.cursor().shape() != Qt::SizeVerCursor);
    QVERIFY(window.cursor().shape() != Qt::SizeFDiagCursor);
    QVERIFY(window.cursor().shape() != Qt::SizeBDiagCursor);
}

void TestUiLayoutValidation::testCleanRoomBrandSanitization() {
    ui::MainWindow window;
    const QString winTitle = window.windowTitle();
    const QString titleBarText = window.titleBar()->title();

    const QStringList forbidden = {
        QStringLiteral("potplayer"),
        QStringLiteral("daum"),
        QStringLiteral("kmplayer"),
        QStringLiteral("gomplayer")
    };

    for (const auto& term : forbidden) {
        QVERIFY2(!winTitle.toLower().contains(term),
                 qPrintable(QString("Window title leaked brand term: %1").arg(term)));
        QVERIFY2(!titleBarText.toLower().contains(term),
                 qPrintable(QString("TitleBar leaked brand term: %1").arg(term)));
    }

    // Verify ControlPanel tabs do not leak brand
    if (window.controlPanel() && window.controlPanel()->tabWidget()) {
        auto* tw = window.controlPanel()->tabWidget();
        for (int i = 0; i < tw->count(); ++i) {
            for (const auto& term : forbidden) {
                QVERIFY2(!tw->tabText(i).toLower().contains(term),
                         qPrintable(QString("ControlPanel tab %1 leaked brand term: %2").arg(i).arg(term)));
            }
        }
    }

    // Verify PlaylistDock tabs do not leak brand
    if (window.playlistDock() && window.playlistDock()->tabBar()) {
        auto* tb = window.playlistDock()->tabBar();
        for (int i = 0; i < tb->count(); ++i) {
            for (const auto& term : forbidden) {
                QVERIFY2(!tb->tabText(i).toLower().contains(term),
                         qPrintable(QString("PlaylistDock tab %1 leaked brand term: %2").arg(i).arg(term)));
            }
        }
    }
}

QTEST_MAIN(TestUiLayoutValidation)
#include "test_ui_layout_validation.moc"
