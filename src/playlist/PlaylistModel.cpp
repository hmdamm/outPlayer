#include "playlist/PlaylistModel.h"
#include <QFileInfo>
#include <algorithm>

namespace outplayer::playlist {

PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_rng(std::random_device{}())
{
}

int PlaylistModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const auto& item = m_items[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return item.title.isEmpty() ? QFileInfo(item.filePath).fileName() : item.title;
    case TitleRole:
        return item.title;
    case FilePathRole:
        return item.filePath;
    case DurationRole:
        return item.duration;
    case IsActiveRole:
        return (index.row() == m_currentIndex);
    default:
        return {};
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[FilePathRole] = "filePath";
    roles[TitleRole] = "title";
    roles[DurationRole] = "duration";
    roles[IsActiveRole] = "isActive";
    return roles;
}

void PlaylistModel::addItem(const PlaylistItem& item) {
    int row = static_cast<int>(m_items.size());
    beginInsertRows(QModelIndex(), row, row);
    m_items.push_back(item);
    endInsertRows();
}

void PlaylistModel::addItems(const QList<PlaylistItem>& items) {
    if (items.isEmpty()) {
        return;
    }
    int first = static_cast<int>(m_items.size());
    int last = first + items.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    m_items.reserve(m_items.size() + items.size());
    for (const auto& item : items) {
        m_items.push_back(item);
    }
    endInsertRows();
}

void PlaylistModel::removeItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();

    if (m_currentIndex == index) {
        m_currentIndex = -1;
        emit currentItemChanged(-1, PlaylistItem{});
    } else if (m_currentIndex > index) {
        --m_currentIndex;
    }
}

void PlaylistModel::clear() {
    if (m_items.empty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    bool hadActive = (m_currentIndex != -1);
    m_currentIndex = -1;
    endResetModel();

    if (hadActive) {
        emit currentItemChanged(-1, PlaylistItem{});
    }
}

void PlaylistModel::moveItem(int fromIndex, int toIndex) {
    int count = static_cast<int>(m_items.size());
    if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex >= count || fromIndex == toIndex) {
        return;
    }

    int destChild = (toIndex > fromIndex) ? toIndex + 1 : toIndex;
    if (!beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), destChild)) {
        return;
    }

    PlaylistItem item = std::move(m_items[fromIndex]);
    m_items.erase(m_items.begin() + fromIndex);
    m_items.insert(m_items.begin() + toIndex, std::move(item));

    if (m_currentIndex == fromIndex) {
        m_currentIndex = toIndex;
    } else if (fromIndex < m_currentIndex && toIndex >= m_currentIndex) {
        --m_currentIndex;
    } else if (fromIndex > m_currentIndex && toIndex <= m_currentIndex) {
        ++m_currentIndex;
    }

    endMoveRows();
}

int PlaylistModel::currentIndex() const {
    return m_currentIndex;
}

void PlaylistModel::setCurrentIndex(int index) {
    if (index == m_currentIndex) {
        return;
    }
    if (index < -1 || index >= static_cast<int>(m_items.size())) {
        return;
    }

    int oldIndex = m_currentIndex;
    m_currentIndex = index;

    if (oldIndex >= 0 && oldIndex < static_cast<int>(m_items.size())) {
        QModelIndex oldModelIndex = this->index(oldIndex, 0);
        emit dataChanged(oldModelIndex, oldModelIndex, {IsActiveRole});
    }

    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_items.size())) {
        QModelIndex newModelIndex = this->index(m_currentIndex, 0);
        emit dataChanged(newModelIndex, newModelIndex, {IsActiveRole});
        emit currentItemChanged(m_currentIndex, m_items[m_currentIndex]);
    } else {
        emit currentItemChanged(-1, PlaylistItem{});
    }
}

std::optional<PlaylistItem> PlaylistModel::currentItem() const {
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_items.size())) {
        return m_items[m_currentIndex];
    }
    return std::nullopt;
}

int PlaylistModel::nextIndex() const {
    int count = static_cast<int>(m_items.size());
    if (count == 0) {
        return -1;
    }

    switch (m_repeatMode) {
    case RepeatMode::RepeatTrack:
        return (m_currentIndex >= 0 && m_currentIndex < count) ? m_currentIndex : 0;

    case RepeatMode::RepeatAll:
        if (m_currentIndex < 0) {
            return 0;
        }
        return (m_currentIndex + 1) % count;

    case RepeatMode::Shuffle: {
        if (count == 1) {
            return 0;
        }
        std::uniform_int_distribution<int> dist(0, count - 2);
        int pick = dist(m_rng);
        if (m_currentIndex >= 0 && pick >= m_currentIndex) {
            ++pick;
        }
        return pick;
    }

    case RepeatMode::None:
    default:
        if (m_currentIndex < 0) {
            return 0;
        }
        if (m_currentIndex + 1 < count) {
            return m_currentIndex + 1;
        }
        return -1;
    }
}

int PlaylistModel::previousIndex() const {
    int count = static_cast<int>(m_items.size());
    if (count == 0) {
        return -1;
    }

    switch (m_repeatMode) {
    case RepeatMode::RepeatTrack:
        return (m_currentIndex >= 0 && m_currentIndex < count) ? m_currentIndex : 0;

    case RepeatMode::RepeatAll:
        if (m_currentIndex <= 0) {
            return count - 1;
        }
        return m_currentIndex - 1;

    case RepeatMode::Shuffle: {
        if (count == 1) {
            return 0;
        }
        std::uniform_int_distribution<int> dist(0, count - 2);
        int pick = dist(m_rng);
        if (m_currentIndex >= 0 && pick >= m_currentIndex) {
            ++pick;
        }
        return pick;
    }

    case RepeatMode::None:
    default:
        if (m_currentIndex < 0) {
            return count - 1;
        }
        if (m_currentIndex - 1 >= 0) {
            return m_currentIndex - 1;
        }
        return -1;
    }
}

RepeatMode PlaylistModel::repeatMode() const {
    return m_repeatMode;
}

void PlaylistModel::setRepeatMode(RepeatMode mode) {
    m_repeatMode = mode;
}

void PlaylistModel::sortByName(Qt::SortOrder order) {
    if (m_items.size() <= 1) {
        return;
    }

    std::optional<PlaylistItem> activeItem = currentItem();

    beginResetModel();
    if (order == Qt::AscendingOrder) {
        std::stable_sort(m_items.begin(), m_items.end(), [](const PlaylistItem& a, const PlaylistItem& b) {
            int cmp = QString::compare(a.title, b.title, Qt::CaseInsensitive);
            if (cmp != 0) return cmp < 0;
            return QString::compare(a.filePath, b.filePath, Qt::CaseInsensitive) < 0;
        });
    } else {
        std::stable_sort(m_items.begin(), m_items.end(), [](const PlaylistItem& a, const PlaylistItem& b) {
            int cmp = QString::compare(a.title, b.title, Qt::CaseInsensitive);
            if (cmp != 0) return cmp > 0;
            return QString::compare(a.filePath, b.filePath, Qt::CaseInsensitive) > 0;
        });
    }

    if (activeItem.has_value()) {
        auto it = std::find(m_items.begin(), m_items.end(), *activeItem);
        if (it != m_items.end()) {
            m_currentIndex = static_cast<int>(std::distance(m_items.begin(), it));
        } else {
            m_currentIndex = -1;
        }
    } else {
        m_currentIndex = -1;
    }
    endResetModel();
}

void PlaylistModel::sortByDuration(Qt::SortOrder order) {
    if (m_items.size() <= 1) {
        return;
    }

    std::optional<PlaylistItem> activeItem = currentItem();

    beginResetModel();
    if (order == Qt::AscendingOrder) {
        std::stable_sort(m_items.begin(), m_items.end(), [](const PlaylistItem& a, const PlaylistItem& b) {
            if (a.duration != b.duration) {
                return a.duration < b.duration;
            }
            int cmp = QString::compare(a.title, b.title, Qt::CaseInsensitive);
            if (cmp != 0) return cmp < 0;
            return QString::compare(a.filePath, b.filePath, Qt::CaseInsensitive) < 0;
        });
    } else {
        std::stable_sort(m_items.begin(), m_items.end(), [](const PlaylistItem& a, const PlaylistItem& b) {
            if (a.duration != b.duration) {
                return a.duration > b.duration;
            }
            int cmp = QString::compare(a.title, b.title, Qt::CaseInsensitive);
            if (cmp != 0) return cmp > 0;
            return QString::compare(a.filePath, b.filePath, Qt::CaseInsensitive) > 0;
        });
    }

    if (activeItem.has_value()) {
        auto it = std::find(m_items.begin(), m_items.end(), *activeItem);
        if (it != m_items.end()) {
            m_currentIndex = static_cast<int>(std::distance(m_items.begin(), it));
        } else {
            m_currentIndex = -1;
        }
    } else {
        m_currentIndex = -1;
    }
    endResetModel();
}

void PlaylistModel::shuffle() {
    if (m_items.size() <= 1) {
        return;
    }

    std::optional<PlaylistItem> activeItem = currentItem();

    beginResetModel();
    std::shuffle(m_items.begin(), m_items.end(), m_rng);

    if (activeItem.has_value()) {
        auto it = std::find(m_items.begin(), m_items.end(), *activeItem);
        if (it != m_items.end()) {
            m_currentIndex = static_cast<int>(std::distance(m_items.begin(), it));
        } else {
            m_currentIndex = -1;
        }
    } else {
        m_currentIndex = -1;
    }
    endResetModel();
}

double PlaylistModel::totalDuration() const {
    double total = 0.0;
    for (const auto& item : m_items) {
        total += item.duration;
    }
    return total;
}

} // namespace outplayer::playlist
