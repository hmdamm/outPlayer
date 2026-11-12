#pragma once

#include <QAbstractListModel>
#include <QList>
#include <optional>
#include <vector>
#include <random>
#include "playlist/PlaylistItem.h"

namespace outplayer::playlist {

enum class RepeatMode {
    None,
    RepeatTrack,
    RepeatAll,
    Shuffle
};

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        TitleRole,
        DurationRole,
        IsActiveRole
    };

    explicit PlaylistModel(QObject* parent = nullptr);
    ~PlaylistModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addItem(const PlaylistItem& item);
    void addItems(const QList<PlaylistItem>& items);
    void removeItem(int index);
    void clear();
    void moveItem(int fromIndex, int toIndex);

    int currentIndex() const;
    void setCurrentIndex(int index);
    std::optional<PlaylistItem> currentItem() const;

    int nextIndex() const;
    int previousIndex() const;

    RepeatMode repeatMode() const;
    void setRepeatMode(RepeatMode mode);

    void sortByName(Qt::SortOrder order = Qt::AscendingOrder);
    void sortByDuration(Qt::SortOrder order = Qt::AscendingOrder);
    void shuffle();

    double totalDuration() const;

signals:
    void currentItemChanged(int index, const PlaylistItem& item);

private:
    std::vector<PlaylistItem> m_items;
    int m_currentIndex = -1;
    RepeatMode m_repeatMode = RepeatMode::None;
    mutable std::mt19937 m_rng;
};

} // namespace outplayer::playlist
