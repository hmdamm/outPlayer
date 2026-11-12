// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <optional>

namespace outplayer::core {

class SettingsManager;

struct Bookmark {
    QString filePath;
    double timestamp = 0.0;
    QString title;

    bool operator==(const Bookmark&) const = default;
};

class BookmarkManager : public QObject {
    Q_OBJECT

public:
    explicit BookmarkManager(SettingsManager* settings = nullptr, QObject* parent = nullptr);
    ~BookmarkManager() override = default;

    BookmarkManager(const BookmarkManager&) = delete;
    BookmarkManager& operator=(const BookmarkManager&) = delete;
    BookmarkManager(BookmarkManager&&) = delete;
    BookmarkManager& operator=(BookmarkManager&&) = delete;

    void addBookmark(const QString& filePath, double timestamp, const QString& title = QString());
    void removeBookmark(const QString& filePath, double timestamp);
    void clearBookmarks(const QString& filePath = QString());

    QList<Bookmark> bookmarksForFile(const QString& filePath) const;
    QList<Bookmark> allBookmarks() const;

    std::optional<Bookmark> nextBookmark(const QString& filePath, double currentPos) const;
    std::optional<Bookmark> previousBookmark(const QString& filePath, double currentPos) const;

    void save();
    void load();

signals:
    void bookmarksChanged(const QString& filePath);

private:
    SettingsManager* m_settings = nullptr;
    QMap<QString, QList<Bookmark>> m_bookmarks;
};

} // namespace outplayer::core
