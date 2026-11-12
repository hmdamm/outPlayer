// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "core/BookmarkManager.h"
#include "core/SettingsManager.h"

using namespace outplayer::core;

class TestBookmarkManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testAddBookmarkOrderAndTitle();
    void testAddBookmarkDuplicateThreshold();
    void testRemoveBookmarkTolerance();
    void testClearBookmarks();
    void testAllBookmarksAndFileBookmarks();
    void testNextAndPreviousNavigation();
    void testPersistenceRoundTrip();
    void testSignalEmissions();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<SettingsManager> m_settings;
};

void TestBookmarkManager::init() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    QString iniPath = m_tempDir->filePath("test_bookmarks.ini");
    m_settings = std::make_unique<SettingsManager>();
    m_settings->init(iniPath);
}

void TestBookmarkManager::cleanup() {
    m_settings.reset();
    m_tempDir.reset();
}

void TestBookmarkManager::testAddBookmarkOrderAndTitle() {
    BookmarkManager manager(m_settings.get());
    const QString file = "/path/to/media.mp4";

    // Add out of chronological order
    manager.addBookmark(file, 120.0, "Second Bookmark");
    manager.addBookmark(file, 30.0); // Empty title -> should generate MM:SS ("00:30")
    manager.addBookmark(file, 3665.0); // Empty title -> should generate HH:MM:SS ("01:01:05")

    const auto bookmarks = manager.bookmarksForFile(file);
    QCOMPARE(bookmarks.size(), 3);

    // Verify chronological ordering
    QCOMPARE(bookmarks[0].timestamp, 30.0);
    QCOMPARE(bookmarks[0].title, QString("00:30"));

    QCOMPARE(bookmarks[1].timestamp, 120.0);
    QCOMPARE(bookmarks[1].title, QString("Second Bookmark"));

    QCOMPARE(bookmarks[2].timestamp, 3665.0);
    QCOMPARE(bookmarks[2].title, QString("01:01:05"));
}

void TestBookmarkManager::testAddBookmarkDuplicateThreshold() {
    BookmarkManager manager(m_settings.get());
    const QString file = "/path/to/video.mkv";

    manager.addBookmark(file, 10.0, "Bookmark 1");
    // Duplicate within 0.1s threshold: 10.05s is 0.05s away
    manager.addBookmark(file, 10.05, "Duplicate 1");
    QCOMPARE(manager.bookmarksForFile(file).size(), 1);

    // 10.15s is > 0.1s away -> should be added
    manager.addBookmark(file, 10.15, "Bookmark 2");
    QCOMPARE(manager.bookmarksForFile(file).size(), 2);
}

void TestBookmarkManager::testRemoveBookmarkTolerance() {
    BookmarkManager manager(m_settings.get());
    const QString file = "/path/to/video.mkv";

    manager.addBookmark(file, 20.0, "At 20s");
    manager.addBookmark(file, 50.0, "At 50s");
    QCOMPARE(manager.bookmarksForFile(file).size(), 2);

    // Remove within 0.5s tolerance (20.3 is 0.3s away)
    manager.removeBookmark(file, 20.3);
    auto bookmarks = manager.bookmarksForFile(file);
    QCOMPARE(bookmarks.size(), 1);
    QCOMPARE(bookmarks[0].timestamp, 50.0);

    // Attempt remove outside tolerance (51.0 is 1.0s away)
    manager.removeBookmark(file, 51.0);
    QCOMPARE(manager.bookmarksForFile(file).size(), 1);
}

void TestBookmarkManager::testClearBookmarks() {
    BookmarkManager manager(m_settings.get());
    const QString file1 = "/path/to/video1.mkv";
    const QString file2 = "/path/to/video2.mkv";

    manager.addBookmark(file1, 10.0);
    manager.addBookmark(file1, 20.0);
    manager.addBookmark(file2, 15.0);

    QCOMPARE(manager.bookmarksForFile(file1).size(), 2);
    QCOMPARE(manager.bookmarksForFile(file2).size(), 1);

    // Clear bookmarks for file1 only
    manager.clearBookmarks(file1);
    QCOMPARE(manager.bookmarksForFile(file1).size(), 0);
    QCOMPARE(manager.bookmarksForFile(file2).size(), 1);

    // Clear all bookmarks
    manager.addBookmark(file1, 5.0);
    manager.clearBookmarks(); // Empty filePath clears all
    QCOMPARE(manager.allBookmarks().size(), 0);
}

void TestBookmarkManager::testAllBookmarksAndFileBookmarks() {
    BookmarkManager manager(m_settings.get());
    const QString file1 = "/path/to/alpha.mkv";
    const QString file2 = "/path/to/beta.mkv";

    manager.addBookmark(file1, 10.0, "Alpha 1");
    manager.addBookmark(file2, 20.0, "Beta 1");
    manager.addBookmark(file1, 30.0, "Alpha 2");

    const auto all = manager.allBookmarks();
    QCOMPARE(all.size(), 3);

    // Unknown file returns empty list
    QCOMPARE(manager.bookmarksForFile("/nonexistent").size(), 0);
}

void TestBookmarkManager::testNextAndPreviousNavigation() {
    BookmarkManager manager(m_settings.get());
    const QString file = "/path/to/movie.mp4";

    manager.addBookmark(file, 10.0, "B1");
    manager.addBookmark(file, 30.0, "B2");
    manager.addBookmark(file, 60.0, "B3");

    // Before first bookmark (0.0s)
    auto prev = manager.previousBookmark(file, 0.0);
    QVERIFY(!prev.has_value());
    auto next = manager.nextBookmark(file, 0.0);
    QVERIFY(next.has_value());
    QCOMPARE(next->timestamp, 10.0);

    // Exactly at B1 (10.0s) - next should skip within 0.1s threshold
    next = manager.nextBookmark(file, 10.0);
    QVERIFY(next.has_value());
    QCOMPARE(next->timestamp, 30.0);

    // Between B1 and B2 (20.0s)
    prev = manager.previousBookmark(file, 20.0);
    QVERIFY(prev.has_value());
    QCOMPARE(prev->timestamp, 10.0);

    next = manager.nextBookmark(file, 20.0);
    QVERIFY(next.has_value());
    QCOMPARE(next->timestamp, 30.0);

    // Exactly at B2 (30.0s) - previous should skip within 0.1s threshold
    prev = manager.previousBookmark(file, 30.0);
    QVERIFY(prev.has_value());
    QCOMPARE(prev->timestamp, 10.0);

    // Past last bookmark (70.0s)
    next = manager.nextBookmark(file, 70.0);
    QVERIFY(!next.has_value());
    prev = manager.previousBookmark(file, 70.0);
    QVERIFY(prev.has_value());
    QCOMPARE(prev->timestamp, 60.0);

    // Non-existent file
    QVERIFY(!manager.nextBookmark("/other.mp4", 0.0).has_value());
    QVERIFY(!manager.previousBookmark("/other.mp4", 100.0).has_value());
}

void TestBookmarkManager::testPersistenceRoundTrip() {
    const QString file1 = "/movies/film1.mkv";
    const QString file2 = "/movies/film2.mkv";

    {
        BookmarkManager manager(m_settings.get());
        manager.addBookmark(file1, 45.0, "Intro Scene");
        manager.addBookmark(file1, 95.5, "Climax");
        manager.addBookmark(file2, 12.0, "Start");
        manager.save();
    }

    // New manager instance reading the same settings
    {
        BookmarkManager restoredManager(m_settings.get());
        // Constructor loads automatically, or explicit load can be called
        const auto f1Bookmarks = restoredManager.bookmarksForFile(file1);
        QCOMPARE(f1Bookmarks.size(), 2);
        QCOMPARE(f1Bookmarks[0].timestamp, 45.0);
        QCOMPARE(f1Bookmarks[0].title, QString("Intro Scene"));
        QCOMPARE(f1Bookmarks[1].timestamp, 95.5);
        QCOMPARE(f1Bookmarks[1].title, QString("Climax"));

        const auto f2Bookmarks = restoredManager.bookmarksForFile(file2);
        QCOMPARE(f2Bookmarks.size(), 1);
        QCOMPARE(f2Bookmarks[0].timestamp, 12.0);
        QCOMPARE(f2Bookmarks[0].title, QString("Start"));
    }
}

void TestBookmarkManager::testSignalEmissions() {
    BookmarkManager manager(m_settings.get());
    const QString file = "/path/to/clip.mp4";

    QSignalSpy spy(&manager, &BookmarkManager::bookmarksChanged);

    // Adding bookmark emits bookmarksChanged(file)
    manager.addBookmark(file, 15.0, "Test");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), file);

    // Duplicate does not emit
    manager.addBookmark(file, 15.02, "Duplicate");
    QCOMPARE(spy.count(), 0);

    // Removing bookmark emits bookmarksChanged(file)
    manager.removeBookmark(file, 15.0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), file);

    // Removing non-existent does not emit
    manager.removeBookmark(file, 99.0);
    QCOMPARE(spy.count(), 0);

    // Clear file bookmarks emits
    manager.addBookmark(file, 25.0);
    spy.clear();
    manager.clearBookmarks(file);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), file);
}

QTEST_MAIN(TestBookmarkManager)
#include "test_bookmark_manager.moc"
