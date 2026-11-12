// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QPointer>
#include <QString>

class QTabBar;
class QLineEdit;
class QListView;
class QLabel;
class QPushButton;
class QMenu;
class QSortFilterProxyModel;
class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;

namespace outplayer::playlist {
class PlaylistModel;
}

namespace outplayer::ui {

class PlaylistFilterProxyModel;
class PlaylistDelegate;

class PlaylistDock : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistDock(playlist::PlaylistModel* model, QWidget* parent = nullptr);
    ~PlaylistDock() override = default;

    void setSearchFilter(const QString& pattern);
    [[nodiscard]] QString searchFilter() const;
    [[nodiscard]] bool isFilterActive() const;
    [[nodiscard]] int selectedIndex() const;

    [[nodiscard]] int currentTabIndex() const;
    void setCurrentTabIndex(int index);

    [[nodiscard]] playlist::PlaylistModel* model() const noexcept;
    [[nodiscard]] QSortFilterProxyModel* proxyModel() const noexcept;
    [[nodiscard]] QListView* listView() const noexcept;
    [[nodiscard]] QLineEdit* searchEdit() const noexcept;
    [[nodiscard]] QTabBar* tabBar() const noexcept;
    [[nodiscard]] QPushButton* closeButton() const noexcept;
    [[nodiscard]] QPushButton* addButton() const noexcept;
    [[nodiscard]] QPushButton* removeButton() const noexcept;
    [[nodiscard]] QPushButton* sortButton() const noexcept;
    [[nodiscard]] QPushButton* menuButton() const noexcept;
    [[nodiscard]] QLabel* statusLabel() const noexcept;

    [[nodiscard]] QMenu* addMenu() const noexcept;
    [[nodiscard]] QMenu* sortMenu() const noexcept;
    [[nodiscard]] QMenu* playlistMenu() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;

    // Drag and drop events declared public for unit testing access
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void addFiles();
    void addFolder();
    void addUrl();
    void removeSelected();
    void clearPlaylist();
    void sortByName();
    void sortByDuration();
    void shuffle();

signals:
    void playItemRequested(int modelIndex);
    void addFilesRequested();
    void addFolderRequested();
    void addUrlRequested();
    void closeRequested();

private slots:
    void updateStatusLabel();

private:
    void setupUi();
    void setupStyle();

    QPointer<playlist::PlaylistModel> m_model{nullptr};
    PlaylistFilterProxyModel* m_proxyModel{nullptr};
    PlaylistDelegate* m_delegate{nullptr};

    QTabBar* m_tabBar{nullptr};
    QPushButton* m_closeBtn{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QListView* m_listView{nullptr};

    QPushButton* m_addBtn{nullptr};
    QPushButton* m_removeBtn{nullptr};
    QPushButton* m_sortBtn{nullptr};
    QPushButton* m_menuBtn{nullptr};
    QLabel* m_statusLabel{nullptr};

    QMenu* m_addMenu{nullptr};
    QMenu* m_sortMenu{nullptr};
    QMenu* m_playlistMenu{nullptr};
};

} // namespace outplayer::ui

namespace outplayer {
using ui::PlaylistDock;
}
