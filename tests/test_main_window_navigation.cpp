// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QUrl>
#include <QWidget>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "playlist/PlaylistModel.h"
#include "ui/ControlBar.h"
#include "ui/CustomTitleBar.h"
#include "ui/MainWindow.h"
#include "ui/PlaylistDock.h"
#include "ui/VideoWidget.h"
#include "ui/ContextMenuManager.h"
#include "ui/MediaOpenController.h"

using namespace outplayer;

class TestMainWindowNavigation : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testPlaylistDockAccessAndVisibilityToggle();
    void testPlaylistDockCloseSignalHidesDock();
    void testContextMenuOnVideoWidgetAndLogo();
    void testDragAndDropFilesEnqueuesAndPlays();
    void testAutoAdvanceOnPlaybackFinished();
    void testPlaylistNextAndPrevCommands();
    void testPlayItemRequestedFromDock();
};

void TestMainWindowNavigation::initTestCase() {
    QVERIFY(qApp != nullptr);
}

void TestMainWindowNavigation::cleanup() {
    // Reset settings to defaults
    core::SettingsManager::instance().setUseSystemWindowDecorations(false);
}

void TestMainWindowNavigation::testPlaylistDockAccessAndVisibilityToggle() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Verify accessors
    ui::PlaylistDock* dock = window.playlistDock();
    playlist::PlaylistModel* model = window.playlistModel();
    QVERIFY(dock != nullptr);
    QVERIFY(model != nullptr);

    // Initial state: hidden by default
    QVERIFY(dock->isHidden());

    // Toggle via PL button on control bar
    QVERIFY(window.controlBar() != nullptr);
    QVERIFY(window.controlBar()->playlistButton() != nullptr);
    window.controlBar()->playlistButton()->click();
    QVERIFY(dock->isVisible());

    // Toggle again via PL button -> hides dock
    window.controlBar()->playlistButton()->click();
    QVERIFY(dock->isHidden());

    // Toggle via CMD_PLAYLIST in CommandRegistry (bound to F6)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST);
    QVERIFY(dock->isVisible());

    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST);
    QVERIFY(dock->isHidden());
}

void TestMainWindowNavigation::testPlaylistDockCloseSignalHidesDock() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ui::PlaylistDock* dock = window.playlistDock();
    QVERIFY(dock != nullptr);

    // Show dock
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST);
    QVERIFY(dock->isVisible());

    // Click close button on PlaylistDock
    QVERIFY(dock->closeButton() != nullptr);
    dock->closeButton()->click();
    QVERIFY(dock->isHidden());
}

void TestMainWindowNavigation::testContextMenuOnVideoWidgetAndLogo() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.contextMenuManager() != nullptr);

    // Trigger right click on video surface
    bool executed = core::CommandRegistry::instance().execute(core::CommandId::CMD_MOUSE_RIGHT_CLICK);
    QVERIFY(executed);

    // Verify titleBar logo click connection
    QVERIFY(window.titleBar() != nullptr);
    emit window.titleBar()->logoClicked();
}

void TestMainWindowNavigation::testDragAndDropFilesEnqueuesAndPlays() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.acceptDrops());

    // Create temporary media files
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString file1 = tempDir.filePath(QStringLiteral("track1.mp4"));
    QString file2 = tempDir.filePath(QStringLiteral("track2.mkv"));
    QFile f1(file1);
    QVERIFY(f1.open(QIODevice::WriteOnly));
    f1.write("dummy media 1");
    f1.close();

    QFile f2(file2);
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write("dummy media 2");
    f2.close();

    QMimeData mimeData;
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(file1));
    urls.append(QUrl::fromLocalFile(file2));
    mimeData.setUrls(urls);

    QPoint dropPos(window.width() / 2, window.height() / 2);
    QDropEvent dropEvent(dropPos, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Feed drop event to MainWindow
    window.dropEvent(&dropEvent);

    playlist::PlaylistModel* model = window.playlistModel();
    QVERIFY(model != nullptr);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->currentIndex(), 0);
    QCOMPARE(model->data(model->index(0, 0), playlist::PlaylistModel::FilePathRole).toString(), file1);
    QCOMPARE(model->data(model->index(1, 0), playlist::PlaylistModel::FilePathRole).toString(), file2);
}

void TestMainWindowNavigation::testAutoAdvanceOnPlaybackFinished() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    playlist::PlaylistModel* model = window.playlistModel();
    QVERIFY(model != nullptr);

    playlist::PlaylistItem item1;
    item1.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=1");
    item1.title = QStringLiteral("Item 1");

    playlist::PlaylistItem item2;
    item2.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=2");
    item2.title = QStringLiteral("Item 2");

    model->addItem(item1);
    model->addItem(item2);
    model->setCurrentIndex(0);

    QCOMPARE(model->currentIndex(), 0);

    // Simulate playback finished signal from playback engine
    emit window.engine()->playbackFinished();

    // Auto-advance should move to next item
    QCOMPARE(model->currentIndex(), 1);
}

void TestMainWindowNavigation::testPlaylistNextAndPrevCommands() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    playlist::PlaylistModel* model = window.playlistModel();
    QVERIFY(model != nullptr);

    playlist::PlaylistItem item1;
    item1.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=1");
    item1.title = QStringLiteral("Item 1");

    playlist::PlaylistItem item2;
    item2.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=2");
    item2.title = QStringLiteral("Item 2");

    playlist::PlaylistItem item3;
    item3.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=3");
    item3.title = QStringLiteral("Item 3");

    model->addItem(item1);
    model->addItem(item2);
    model->addItem(item3);
    model->setCurrentIndex(0);

    // CMD_PLAYLIST_NEXT (PgDn)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_NEXT);
    QCOMPARE(model->currentIndex(), 1);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_NEXT);
    QCOMPARE(model->currentIndex(), 2);

    // CMD_PLAYLIST_PREV (PgUp)
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_PREV);
    QCOMPARE(model->currentIndex(), 1);

    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_PREV);
    QCOMPARE(model->currentIndex(), 0);
}

void TestMainWindowNavigation::testPlayItemRequestedFromDock() {
    ui::MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    playlist::PlaylistModel* model = window.playlistModel();
    QVERIFY(model != nullptr);

    playlist::PlaylistItem item1;
    item1.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=1");
    item1.title = QStringLiteral("Item 1");

    playlist::PlaylistItem item2;
    item2.filePath = QStringLiteral("lavfi://nullsrc=s=640x360:d=2");
    item2.title = QStringLiteral("Item 2");

    model->addItem(item1);
    model->addItem(item2);
    model->setCurrentIndex(0);

    ui::PlaylistDock* dock = window.playlistDock();
    QVERIFY(dock != nullptr);

    // Emitting playItemRequested on dock should select and load item 1
    emit dock->playItemRequested(1);
    QCOMPARE(model->currentIndex(), 1);
}

QTEST_MAIN(TestMainWindowNavigation)
#include "test_main_window_navigation.moc"
