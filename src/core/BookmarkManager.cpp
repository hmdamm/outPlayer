// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/BookmarkManager.h"
#include "core/SettingsManager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

namespace outplayer::core {

namespace {

QString formatDefaultTitle(double timestamp) {
    if (!std::isfinite(timestamp) || timestamp < 0.0) {
        timestamp = 0.0;
    }
    const auto totalSeconds = static_cast<qint64>(std::floor(timestamp));
    const qint64 h = totalSeconds / 3600;
    const qint64 m = (totalSeconds % 3600) / 60;
    const qint64 s = totalSeconds % 60;

    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }

    return QStringLiteral("%1:%2")
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

} // namespace

BookmarkManager::BookmarkManager(SettingsManager* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings ? settings : &SettingsManager::instance())
{
    load();
}

void BookmarkManager::addBookmark(const QString& filePath, double timestamp, const QString& title) {
    if (filePath.isEmpty()) {
        return;
    }

    auto& list = m_bookmarks[filePath];

    // Check duplicate within 0.1s threshold
    for (const auto& b : list) {
        if (std::abs(b.timestamp - timestamp) <= 0.1) {
            return;
        }
    }

    const QString effectiveTitle = title.trimmed().isEmpty() ? formatDefaultTitle(timestamp) : title;

    Bookmark bookmark{filePath, timestamp, effectiveTitle};

    // Keep chronological order
    auto it = std::lower_bound(list.begin(), list.end(), timestamp, [](const Bookmark& b, double t) {
        return b.timestamp < t;
    });
    list.insert(it, bookmark);

    emit bookmarksChanged(filePath);
}

void BookmarkManager::removeBookmark(const QString& filePath, double timestamp) {
    if (filePath.isEmpty() || !m_bookmarks.contains(filePath)) {
        return;
    }

    auto& list = m_bookmarks[filePath];
    bool removed = false;

    for (auto it = list.begin(); it != list.end();) {
        if (std::abs(it->timestamp - timestamp) <= 0.5) {
            it = list.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }

    if (list.isEmpty()) {
        m_bookmarks.remove(filePath);
    }

    if (removed) {
        emit bookmarksChanged(filePath);
    }
}

void BookmarkManager::clearBookmarks(const QString& filePath) {
    if (filePath.isEmpty()) {
        if (!m_bookmarks.isEmpty()) {
            m_bookmarks.clear();
            emit bookmarksChanged(QString());
        }
    } else {
        if (m_bookmarks.remove(filePath) > 0) {
            emit bookmarksChanged(filePath);
        }
    }
}

QList<Bookmark> BookmarkManager::bookmarksForFile(const QString& filePath) const {
    return m_bookmarks.value(filePath);
}

QList<Bookmark> BookmarkManager::allBookmarks() const {
    QList<Bookmark> result;
    for (const auto& list : m_bookmarks) {
        result.append(list);
    }
    return result;
}

std::optional<Bookmark> BookmarkManager::nextBookmark(const QString& filePath, double currentPos) const {
    const auto list = m_bookmarks.value(filePath);
    for (const auto& b : list) {
        if (b.timestamp > currentPos + 0.1) {
            return b;
        }
    }
    return std::nullopt;
}

std::optional<Bookmark> BookmarkManager::previousBookmark(const QString& filePath, double currentPos) const {
    const auto list = m_bookmarks.value(filePath);
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        if (it->timestamp < currentPos - 0.1) {
            return *it;
        }
    }
    return std::nullopt;
}

void BookmarkManager::save() {
    if (!m_settings) {
        return;
    }

    QJsonArray array;
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        for (const auto& bm : it.value()) {
            QJsonObject obj;
            obj[QStringLiteral("filePath")] = bm.filePath;
            obj[QStringLiteral("timestamp")] = bm.timestamp;
            obj[QStringLiteral("title")] = bm.title;
            array.append(obj);
        }
    }

    const QJsonDocument doc(array);
    const QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    m_settings->set(QStringLiteral("bookmarks/data"), jsonString);
    m_settings->sync();
}

void BookmarkManager::load() {
    if (!m_settings) {
        return;
    }

    m_bookmarks.clear();

    QVariant val = m_settings->get(QStringLiteral("bookmarks/data"));
    if (!val.isValid() || val.toString().isEmpty()) {
        val = m_settings->get(QStringLiteral("Bookmarks/data"));
    }
    if (!val.isValid() || val.toString().isEmpty()) {
        val = m_settings->get(QStringLiteral("bookmarks/items"));
    }

    if (!val.isValid()) {
        return;
    }

    QByteArray jsonData;
    if (val.typeId() == QMetaType::QByteArray) {
        jsonData = val.toByteArray();
    } else {
        jsonData = val.toString().toUtf8();
    }

    if (jsonData.isEmpty()) {
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return;
    }

    const QJsonArray array = doc.array();
    for (const auto& item : array) {
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject obj = item.toObject();
        Bookmark bm;
        bm.filePath = obj.value(QStringLiteral("filePath")).toString();
        bm.timestamp = obj.value(QStringLiteral("timestamp")).toDouble();
        bm.title = obj.value(QStringLiteral("title")).toString();
        if (bm.title.trimmed().isEmpty()) {
            bm.title = formatDefaultTitle(bm.timestamp);
        }
        if (!bm.filePath.isEmpty()) {
            m_bookmarks[bm.filePath].append(bm);
        }
    }

    for (auto it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(), [](const Bookmark& a, const Bookmark& b) {
            return a.timestamp < b.timestamp;
        });
    }

    emit bookmarksChanged(QString());
}

} // namespace outplayer::core
