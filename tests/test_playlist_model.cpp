#include <QTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"

using namespace outplayer::playlist;

class TestPlaylistModel : public QObject {
    Q_OBJECT

private slots:
    void testEmptyModel();
    void testAddItems();
    void testRemoveItem();
    void testMoveItem();
    void testClear();
    void testCurrentIndexAndSignals();
    void testRepeatModes();
    void testSorting();
    void testShuffle();
    void testTotalDuration();
    void test10kItemsBenchmark();
};

void TestPlaylistModel::testEmptyModel() {
    PlaylistModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.currentIndex(), -1);
    QVERIFY(!model.currentItem().has_value());
    QCOMPARE(model.nextIndex(), -1);
    QCOMPARE(model.previousIndex(), -1);
    QCOMPARE(model.totalDuration(), 0.0);
    QVERIFY(!model.data(model.index(0, 0)).isValid());
}

void TestPlaylistModel::testAddItems() {
    PlaylistModel model;

    QSignalSpy rowsInsertedSpy(&model, &QAbstractItemModel::rowsInserted);

    PlaylistItem item1{"/media/video1.mp4", "Video 1", 120.5};
    model.addItem(item1);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(rowsInsertedSpy.count(), 1);

    QModelIndex idx0 = model.index(0, 0);
    QCOMPARE(model.data(idx0, Qt::DisplayRole).toString(), QString("Video 1"));
    QCOMPARE(model.data(idx0, PlaylistModel::TitleRole).toString(), QString("Video 1"));
    QCOMPARE(model.data(idx0, PlaylistModel::FilePathRole).toString(), QString("/media/video1.mp4"));
    QCOMPARE(model.data(idx0, PlaylistModel::DurationRole).toDouble(), 120.5);
    QCOMPARE(model.data(idx0, PlaylistModel::IsActiveRole).toBool(), false);

    // Batch add
    QList<PlaylistItem> batch = {
        {"/media/video2.mkv", "Video 2", 300.0},
        {"/media/video3.avi", "Video 3", 45.0}
    };
    model.addItems(batch);

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(rowsInsertedSpy.count(), 2);

    QModelIndex idx2 = model.index(2, 0);
    QCOMPARE(model.data(idx2, PlaylistModel::TitleRole).toString(), QString("Video 3"));
    QCOMPARE(model.data(idx2, PlaylistModel::DurationRole).toDouble(), 45.0);
}

void TestPlaylistModel::testRemoveItem() {
    PlaylistModel model;
    model.addItems({
        {"/media/a.mp4", "A", 10.0},
        {"/media/b.mp4", "B", 20.0},
        {"/media/c.mp4", "C", 30.0}
    });

    model.setCurrentIndex(1); // Item "B" is active
    QCOMPARE(model.currentIndex(), 1);

    QSignalSpy rowsRemovedSpy(&model, &QAbstractItemModel::rowsRemoved);

    // Remove item at index 0 ("A")
    model.removeItem(0);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(rowsRemovedSpy.count(), 1);
    // Active item was at 1, now shifted to 0
    QCOMPARE(model.currentIndex(), 0);
    QVERIFY(model.currentItem().has_value());
    QCOMPARE(model.currentItem()->title, QString("B"));

    // Remove current active item (index 0)
    model.removeItem(0);
    QCOMPARE(model.rowCount(), 1);
    // Active item was removed, resets to -1
    QCOMPARE(model.currentIndex(), -1);
    QVERIFY(!model.currentItem().has_value());

    // Out of bounds remove is safe
    model.removeItem(99);
    model.removeItem(-1);
    QCOMPARE(model.rowCount(), 1);
}

void TestPlaylistModel::testMoveItem() {
    PlaylistModel model;
    model.addItems({
        {"/media/a.mp4", "A", 10.0},
        {"/media/b.mp4", "B", 20.0},
        {"/media/c.mp4", "C", 30.0}
    });

    model.setCurrentIndex(0); // "A" is active

    QSignalSpy rowsMovedSpy(&model, &QAbstractItemModel::rowsMoved);

    // Move "A" from 0 to 2
    model.moveItem(0, 2);
    QCOMPARE(rowsMovedSpy.count(), 1);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("B"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("C"));
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::TitleRole).toString(), QString("A"));

    // Active item index must track the move to index 2
    QCOMPARE(model.currentIndex(), 2);
    QVERIFY(model.currentItem().has_value());
    QCOMPARE(model.currentItem()->title, QString("A"));

    // Move "C" from 1 to 0
    model.moveItem(1, 0);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("C"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("B"));
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::TitleRole).toString(), QString("A"));
    // "A" is still at 2
    QCOMPARE(model.currentIndex(), 2);
}

void TestPlaylistModel::testClear() {
    PlaylistModel model;
    model.addItems({
        {"/media/a.mp4", "A", 10.0},
        {"/media/b.mp4", "B", 20.0}
    });
    model.setCurrentIndex(1);

    QSignalSpy modelResetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy currentChangedSpy(&model, &PlaylistModel::currentItemChanged);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(modelResetSpy.count(), 1);
    QCOMPARE(model.currentIndex(), -1);
    QVERIFY(!model.currentItem().has_value());
    QCOMPARE(currentChangedSpy.count(), 1);
}

void TestPlaylistModel::testCurrentIndexAndSignals() {
    PlaylistModel model;
    model.addItems({
        {"/media/a.mp4", "A", 10.0},
        {"/media/b.mp4", "B", 20.0}
    });

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy currentChangedSpy(&model, &PlaylistModel::currentItemChanged);

    model.setCurrentIndex(1);
    QCOMPARE(model.currentIndex(), 1);
    QCOMPARE(currentChangedSpy.count(), 1);
    QCOMPARE(currentChangedSpy.at(0).at(0).toInt(), 1);
    PlaylistItem item = qvariant_cast<PlaylistItem>(currentChangedSpy.at(0).at(1));
    QCOMPARE(item.title, QString("B"));

    // Active row data changed
    QVERIFY(dataChangedSpy.count() >= 1);
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::IsActiveRole).toBool(), true);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::IsActiveRole).toBool(), false);

    // Setting same index does nothing
    dataChangedSpy.clear();
    currentChangedSpy.clear();
    model.setCurrentIndex(1);
    QCOMPARE(currentChangedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);

    // Setting out of bounds index is ignored
    model.setCurrentIndex(10);
    QCOMPARE(model.currentIndex(), 1);
    model.setCurrentIndex(-2);
    QCOMPARE(model.currentIndex(), 1);
}

void TestPlaylistModel::testRepeatModes() {
    PlaylistModel model;
    model.addItems({
        {"/media/1.mp4", "1", 10.0},
        {"/media/2.mp4", "2", 20.0},
        {"/media/3.mp4", "3", 30.0}
    });

    // Default: RepeatMode::None
    QCOMPARE(model.repeatMode(), RepeatMode::None);
    model.setCurrentIndex(0);
    QCOMPARE(model.previousIndex(), -1);
    QCOMPARE(model.nextIndex(), 1);

    model.setCurrentIndex(2);
    QCOMPARE(model.previousIndex(), 1);
    QCOMPARE(model.nextIndex(), -1);

    // RepeatMode::RepeatTrack
    model.setRepeatMode(RepeatMode::RepeatTrack);
    QCOMPARE(model.repeatMode(), RepeatMode::RepeatTrack);
    model.setCurrentIndex(1);
    QCOMPARE(model.previousIndex(), 1);
    QCOMPARE(model.nextIndex(), 1);

    // RepeatMode::RepeatAll
    model.setRepeatMode(RepeatMode::RepeatAll);
    QCOMPARE(model.repeatMode(), RepeatMode::RepeatAll);
    model.setCurrentIndex(0);
    QCOMPARE(model.previousIndex(), 2); // wraps to last
    QCOMPARE(model.nextIndex(), 1);

    model.setCurrentIndex(2);
    QCOMPARE(model.previousIndex(), 1);
    QCOMPARE(model.nextIndex(), 0); // wraps to first

    // RepeatMode::Shuffle
    model.setRepeatMode(RepeatMode::Shuffle);
    QCOMPARE(model.repeatMode(), RepeatMode::Shuffle);
    model.setCurrentIndex(1);
    int next = model.nextIndex();
    QVERIFY(next >= 0 && next < model.rowCount());
    int prev = model.previousIndex();
    QVERIFY(prev >= 0 && prev < model.rowCount());
}

void TestPlaylistModel::testSorting() {
    PlaylistModel model;
    model.addItems({
        {"/media/charlie.mp4", "Charlie", 30.0},
        {"/media/alice.mp4", "Alice", 10.0},
        {"/media/bob.mp4", "Bob", 20.0}
    });

    model.setCurrentIndex(1); // "Alice" is active

    // Sort by name Ascending -> Alice, Bob, Charlie
    model.sortByName(Qt::AscendingOrder);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("Alice"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("Bob"));
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::TitleRole).toString(), QString("Charlie"));
    // Active item "Alice" should now be at index 0
    QCOMPARE(model.currentIndex(), 0);
    QVERIFY(model.currentItem().has_value());
    QCOMPARE(model.currentItem()->title, QString("Alice"));

    // Sort by name Descending -> Charlie, Bob, Alice
    model.sortByName(Qt::DescendingOrder);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::TitleRole).toString(), QString("Charlie"));
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::TitleRole).toString(), QString("Bob"));
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::TitleRole).toString(), QString("Alice"));
    // Active item "Alice" should now be at index 2
    QCOMPARE(model.currentIndex(), 2);

    // Sort by duration Ascending -> Alice (10), Bob (20), Charlie (30)
    model.sortByDuration(Qt::AscendingOrder);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::DurationRole).toDouble(), 10.0);
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::DurationRole).toDouble(), 20.0);
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::DurationRole).toDouble(), 30.0);
    QCOMPARE(model.currentIndex(), 0);

    // Sort by duration Descending -> Charlie (30), Bob (20), Alice (10)
    model.sortByDuration(Qt::DescendingOrder);
    QCOMPARE(model.data(model.index(0, 0), PlaylistModel::DurationRole).toDouble(), 30.0);
    QCOMPARE(model.data(model.index(1, 0), PlaylistModel::DurationRole).toDouble(), 20.0);
    QCOMPARE(model.data(model.index(2, 0), PlaylistModel::DurationRole).toDouble(), 10.0);
    QCOMPARE(model.currentIndex(), 2);
}

void TestPlaylistModel::testShuffle() {
    PlaylistModel model;
    for (int i = 0; i < 50; ++i) {
        model.addItem({QString("/media/%1.mp4").arg(i), QString("Track %1").arg(i), 100.0 + i});
    }

    model.setCurrentIndex(15);
    PlaylistItem activeItem = *model.currentItem();
    QCOMPARE(activeItem.title, QString("Track 15"));

    model.shuffle();

    QCOMPARE(model.rowCount(), 50);
    // Active item index relocated, pointing to the exact same track
    QVERIFY(model.currentIndex() >= 0 && model.currentIndex() < 50);
    QVERIFY(model.currentItem().has_value());
    QCOMPARE(model.currentItem()->title, activeItem.title);
    QCOMPARE(model.currentItem()->filePath, activeItem.filePath);
}

void TestPlaylistModel::testTotalDuration() {
    PlaylistModel model;
    QCOMPARE(model.totalDuration(), 0.0);

    model.addItem({"/media/1.mp4", "1", 10.5});
    model.addItem({"/media/2.mp4", "2", 20.25});
    model.addItem({"/media/3.mp4", "3", 30.125});

    QCOMPARE(model.totalDuration(), 60.875);

    model.removeItem(1);
    QCOMPARE(model.totalDuration(), 40.625);

    model.clear();
    QCOMPARE(model.totalDuration(), 0.0);
}

void TestPlaylistModel::test10kItemsBenchmark() {
    PlaylistModel model;
    QList<PlaylistItem> items;
    items.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        items.append(PlaylistItem{
            QString("/media/video_%1.mkv").arg(i),
            QString("Video %1").arg(i),
            120.0 + (i % 300)
        });
    }

    QElapsedTimer timer;
    timer.start();
    model.addItems(items);
    qint64 elapsedMs = timer.elapsed();

    QCOMPARE(model.rowCount(), 10000);
    QVERIFY2(elapsedMs < 50, qPrintable(QString("Elapsed time was %1 ms (expected < 50 ms)").arg(elapsedMs)));
}

QTEST_MAIN(TestPlaylistModel)
#include "test_playlist_model.moc"
