#include <QTest>
#include <QSignalSpy>
#include <QListView>
#include <QLineEdit>
#include <QTabBar>
#include <QPushButton>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QTemporaryDir>
#include <QFile>
#include <QPainter>
#include <QPixmap>

#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistItem.h"
#include "ui/PlaylistDock.h"

using namespace outplayer::playlist;
using namespace outplayer::ui;

class TestPlaylistDock : public QObject {
    Q_OBJECT

private slots:
    void testConstructionAndDefaults();
    void testTabSwitching();
    void testCloseSignal();
    void testSearchFilter();
    void testDoubleClickEmitsPlayItemRequested();
    void testSelectedIndex();
    void testRemoveButton();
    void testBottomActionsAndMenus();
    void testDragAndDrop();
    void testDelegatePainting();
};

void TestPlaylistDock::testConstructionAndDefaults() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    QCOMPARE(dock.minimumWidth(), 200);
    QCOMPARE(dock.maximumWidth(), 600);
    QCOMPARE(dock.sizeHint().width(), 300);

    QVERIFY(dock.tabBar() != nullptr);
    QCOMPARE(dock.tabBar()->count(), 4);
    QCOMPARE(dock.tabBar()->tabText(0), QString("Default"));
    QCOMPARE(dock.tabBar()->tabText(1), QString("Favorites"));
    QCOMPARE(dock.tabBar()->tabText(2), QString("Albums"));
    QCOMPARE(dock.tabBar()->tabText(3), QString("History"));
    QCOMPARE(dock.currentTabIndex(), 0);

    QVERIFY(dock.searchEdit() != nullptr);
    QCOMPARE(dock.searchEdit()->placeholderText(), QString("Search playlist..."));
    QVERIFY(dock.searchEdit()->isClearButtonEnabled());
    QVERIFY(!dock.isFilterActive());

    QVERIFY(dock.statusLabel() != nullptr);
    QCOMPARE(dock.statusLabel()->text(), QString("0 items [00:00]"));

    QVERIFY(dock.acceptDrops());
    QVERIFY(dock.listView() != nullptr);
    QVERIFY(dock.listView()->uniformItemSizes());
}

void TestPlaylistDock::testTabSwitching() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    dock.setCurrentTabIndex(1);
    QCOMPARE(dock.currentTabIndex(), 1);
    QCOMPARE(dock.tabBar()->currentIndex(), 1);

    dock.setCurrentTabIndex(3);
    QCOMPARE(dock.currentTabIndex(), 3);
    QCOMPARE(dock.tabBar()->currentIndex(), 3);

    dock.setCurrentTabIndex(0);
    QCOMPARE(dock.currentTabIndex(), 0);
}

void TestPlaylistDock::testCloseSignal() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    QSignalSpy closeSpy(&dock, &PlaylistDock::closeRequested);
    QVERIFY(dock.closeButton() != nullptr);
    dock.closeButton()->click();

    QCOMPARE(closeSpy.count(), 1);
}

void TestPlaylistDock::testSearchFilter() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    model.addItem(PlaylistItem{"/media/bunny.mp4", "Big Buck Bunny", 600.0});
    model.addItem(PlaylistItem{"/media/sintel.mkv", "Sintel Teaser", 52.0});
    model.addItem(PlaylistItem{"/media/tears.avi", "Tears of Steel", 734.0});

    QCOMPARE(dock.proxyModel()->rowCount(), 3);
    QCOMPARE(dock.statusLabel()->text(), QString("3 items [23:06]"));

    // Filter by title
    dock.setSearchFilter("Bunny");
    QVERIFY(dock.isFilterActive());
    QCOMPARE(dock.proxyModel()->rowCount(), 1);
    QCOMPARE(dock.proxyModel()->index(0, 0).data(PlaylistModel::TitleRole).toString(), QString("Big Buck Bunny"));
    QCOMPARE(dock.statusLabel()->text(), QString("1 item [10:00]"));

    // Case-insensitive
    dock.setSearchFilter("sintel");
    QCOMPARE(dock.proxyModel()->rowCount(), 1);
    QCOMPARE(dock.proxyModel()->index(0, 0).data(PlaylistModel::TitleRole).toString(), QString("Sintel Teaser"));

    // Filter by file path extension
    dock.setSearchFilter("avi");
    QCOMPARE(dock.proxyModel()->rowCount(), 1);
    QCOMPARE(dock.proxyModel()->index(0, 0).data(PlaylistModel::TitleRole).toString(), QString("Tears of Steel"));

    // Clear filter
    dock.setSearchFilter("");
    QVERIFY(!dock.isFilterActive());
    QCOMPARE(dock.proxyModel()->rowCount(), 3);
    QCOMPARE(dock.statusLabel()->text(), QString("3 items [23:06]"));
}

void TestPlaylistDock::testDoubleClickEmitsPlayItemRequested() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    model.addItem(PlaylistItem{"/media/bunny.mp4", "Big Buck Bunny", 600.0});
    model.addItem(PlaylistItem{"/media/sintel.mkv", "Sintel Teaser", 52.0});
    model.addItem(PlaylistItem{"/media/tears.avi", "Tears of Steel", 734.0});

    QSignalSpy playSpy(&dock, &PlaylistDock::playItemRequested);

    // Double-click row 1 ("Sintel Teaser") without filter
    QModelIndex proxyIdx = dock.proxyModel()->index(1, 0);
    emit dock.listView()->doubleClicked(proxyIdx);

    QCOMPARE(playSpy.count(), 1);
    QCOMPARE(playSpy.takeFirst().at(0).toInt(), 1);

    // Filter for "Tears" -> only Tears of Steel visible at proxy row 0 (source row 2)
    dock.setSearchFilter("Tears");
    QCOMPARE(dock.proxyModel()->rowCount(), 1);
    proxyIdx = dock.proxyModel()->index(0, 0);
    emit dock.listView()->doubleClicked(proxyIdx);

    QCOMPARE(playSpy.count(), 1);
    QCOMPARE(playSpy.takeFirst().at(0).toInt(), 2);
}

void TestPlaylistDock::testSelectedIndex() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    QCOMPARE(dock.selectedIndex(), -1);

    model.addItem(PlaylistItem{"/media/bunny.mp4", "Big Buck Bunny", 600.0});
    model.addItem(PlaylistItem{"/media/sintel.mkv", "Sintel Teaser", 52.0});
    model.addItem(PlaylistItem{"/media/tears.avi", "Tears of Steel", 734.0});

    dock.listView()->setCurrentIndex(dock.proxyModel()->index(1, 0));
    QCOMPARE(dock.selectedIndex(), 1);

    // Filter to Tears (source row 2, proxy row 0)
    dock.setSearchFilter("Tears");
    dock.listView()->setCurrentIndex(dock.proxyModel()->index(0, 0));
    QCOMPARE(dock.selectedIndex(), 2);
}

void TestPlaylistDock::testRemoveButton() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    model.addItem(PlaylistItem{"/media/bunny.mp4", "Big Buck Bunny", 600.0});
    model.addItem(PlaylistItem{"/media/sintel.mkv", "Sintel Teaser", 52.0});
    model.addItem(PlaylistItem{"/media/tears.avi", "Tears of Steel", 734.0});

    dock.listView()->setCurrentIndex(dock.proxyModel()->index(1, 0));
    dock.removeButton()->click();

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("Big Buck Bunny"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("Tears of Steel"));
    QCOMPARE(dock.statusLabel()->text(), QString("2 items [22:14]"));
}

void TestPlaylistDock::testBottomActionsAndMenus() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    QSignalSpy addFilesSpy(&dock, &PlaylistDock::addFilesRequested);
    QSignalSpy addFolderSpy(&dock, &PlaylistDock::addFolderRequested);
    QSignalSpy addUrlSpy(&dock, &PlaylistDock::addUrlRequested);

    // Check Add Menu actions
    QVERIFY(dock.addMenu() != nullptr);
    const auto addActions = dock.addMenu()->actions();
    QCOMPARE(addActions.size(), 3);

    addActions.at(0)->trigger();
    QCOMPARE(addFilesSpy.count(), 1);

    addActions.at(1)->trigger();
    QCOMPARE(addFolderSpy.count(), 1);

    addActions.at(2)->trigger();
    QCOMPARE(addUrlSpy.count(), 1);

    // Check Sort Menu actions
    model.addItem(PlaylistItem{"/media/z.mp4", "Zebra", 300.0});
    model.addItem(PlaylistItem{"/media/a.mp4", "Alpha", 100.0});
    model.addItem(PlaylistItem{"/media/m.mp4", "Mango", 200.0});

    QVERIFY(dock.sortMenu() != nullptr);
    const auto sortActions = dock.sortMenu()->actions();
    QVERIFY(sortActions.size() >= 3);

    // Sort by name
    sortActions.at(0)->trigger();
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("Alpha"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("Mango"));
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::TitleRole).toString(), QString("Zebra"));

    // Sort by duration
    sortActions.at(1)->trigger();
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::DurationRole).toDouble(), 100.0);
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::DurationRole).toDouble(), 200.0);
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::DurationRole).toDouble(), 300.0);

    // Shuffle action triggers without crash
    sortActions.at(2)->trigger();
    QCOMPARE(model.rowCount(), 3);

    // Playlist options menu
    QVERIFY(dock.playlistMenu() != nullptr);
    const auto playlistActions = dock.playlistMenu()->actions();
    QVERIFY(!playlistActions.isEmpty());

    // Clear playlist action
    playlistActions.at(0)->trigger();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(dock.statusLabel()->text(), QString("0 items [00:00]"));
}

void TestPlaylistDock::testDragAndDrop() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString filePath1 = tempDir.filePath("video1.mp4");
    QString filePath2 = tempDir.filePath("track.mp3");
    QString ignoredFile = tempDir.filePath("notes.txt");

    {
        QFile f1(filePath1); QVERIFY(f1.open(QIODevice::WriteOnly)); f1.write("dummy");
        QFile f2(filePath2); QVERIFY(f2.open(QIODevice::WriteOnly)); f2.write("dummy");
        QFile f3(ignoredFile); QVERIFY(f3.open(QIODevice::WriteOnly)); f3.write("dummy");
    }

    PlaylistModel model;
    PlaylistDock dock(&model);

    // Simulate drop with individual files
    QMimeData mimeData;
    QList<QUrl> urls = {
        QUrl::fromLocalFile(filePath1),
        QUrl::fromLocalFile(filePath2),
        QUrl::fromLocalFile(ignoredFile)
    };
    mimeData.setUrls(urls);

    QDropEvent dropEvent(QPointF(50, 50), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    dock.dropEvent(&dropEvent);

    // Only media files (mp4 and mp3) should be added; notes.txt is filtered out by isMediaFile
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::FilePathRole).toString(), filePath1);
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::FilePathRole).toString(), filePath2);
}

void TestPlaylistDock::testDelegatePainting() {
    PlaylistModel model;
    PlaylistDock dock(&model);

    model.addItem(PlaylistItem{"/media/track1.mp4", "Active Track", 125.0});
    model.addItem(PlaylistItem{"/media/track2.mp4", "Inactive Track", 3661.0});

    model.setCurrentIndex(0); // Make track 0 active

    QVERIFY(dock.listView()->itemDelegate() != nullptr);

    QPixmap pixmap(300, 100);
    pixmap.fill(Qt::black);
    QPainter painter(&pixmap);

    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 300, 28);
    option.state = QStyle::State_Enabled | QStyle::State_Active;

    // Paint active item
    dock.listView()->itemDelegate()->paint(&painter, option, dock.proxyModel()->index(0, 0));

    // Paint hover/selected inactive item
    option.rect = QRect(0, 28, 300, 28);
    option.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_MouseOver;
    dock.listView()->itemDelegate()->paint(&painter, option, dock.proxyModel()->index(1, 0));

    painter.end();
}

QTEST_MAIN(TestPlaylistDock)
#include "test_playlist_dock.moc"
