// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/PlaylistDock.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistItem.h"
#include "ui/MediaOpenController.h"
#include "ui/TimeFormatter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabBar>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFileInfo>
#include <QUrl>
#include <algorithm>
#include <vector>

namespace outplayer::ui {

// ==============================================================================
// PlaylistFilterProxyModel
// ==============================================================================

class PlaylistFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit PlaylistFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setFilterPattern(const QString& pattern) {
        m_pattern = pattern.trimmed();
        invalidate();
    }

    [[nodiscard]] const QString& filterPattern() const noexcept {
        return m_pattern;
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (m_pattern.isEmpty()) {
            return true;
        }

        const auto* srcModel = sourceModel();
        if (!srcModel) {
            return true;
        }

        const QModelIndex idx = srcModel->index(sourceRow, 0, sourceParent);
        const QString title = idx.data(playlist::PlaylistModel::TitleRole).toString();
        const QString filePath = idx.data(playlist::PlaylistModel::FilePathRole).toString();
        const QString display = idx.data(Qt::DisplayRole).toString();

        return title.contains(m_pattern, Qt::CaseInsensitive)
            || filePath.contains(m_pattern, Qt::CaseInsensitive)
            || display.contains(m_pattern, Qt::CaseInsensitive);
    }

private:
    QString m_pattern;
};

// ==============================================================================
// PlaylistDelegate
// ==============================================================================

class PlaylistDelegate : public QStyledItemDelegate {
public:
    explicit PlaylistDelegate(QSortFilterProxyModel* proxy, QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_proxy(proxy)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(option.rect.width() > 0 ? option.rect.width() : 200, 28);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        // Row background
        QColor bg = (index.row() % 2 == 0) ? QColor(0x1e, 0x1e, 0x1e) : QColor(0x22, 0x22, 0x22);
        if (option.state & QStyle::State_Selected) {
            bg = QColor(0x38, 0x38, 0x38);
        } else if (option.state & QStyle::State_MouseOver) {
            bg = QColor(0x2a, 0x2a, 0x2a);
        }
        painter->fillRect(option.rect, bg);

        const bool isActive = index.data(playlist::PlaylistModel::IsActiveRole).toBool();
        QString title = index.data(playlist::PlaylistModel::TitleRole).toString();
        if (title.isEmpty()) {
            title = index.data(Qt::DisplayRole).toString();
        }
        const double duration = index.data(playlist::PlaylistModel::DurationRole).toDouble();
        const QString durationStr = TimeFormatter::formatTime(duration);

        // Map to source row for track index
        int sourceRow = index.row();
        if (m_proxy) {
            const QModelIndex srcIdx = m_proxy->mapToSource(index);
            if (srcIdx.isValid()) {
                sourceRow = srcIdx.row();
            }
        }
        const QString trackNumStr = QString("%1").arg(sourceRow + 1, 2, 10, QChar('0'));

        const QColor textColor = isActive ? QColor(0xff, 0xaa, 0x00) : QColor(0xff, 0xff, 0xff);
        const QColor indexColor = isActive ? QColor(0xff, 0xaa, 0x00) : QColor(0x88, 0x88, 0x88);
        const QColor durColor = isActive ? QColor(0xff, 0xaa, 0x00) : QColor(0xaa, 0xaa, 0xaa);

        const QRect contentRect = option.rect.adjusted(6, 0, -6, 0);

        // Track index column (right-aligned in 24px box)
        const int indexWidth = 24;
        const QRect indexRect(contentRect.left(), contentRect.top(), indexWidth, contentRect.height());
        painter->setPen(indexColor);
        painter->drawText(indexRect, Qt::AlignVCenter | Qt::AlignRight, trackNumStr);

        // Active indicator glyph ▶ column
        const int glyphWidth = 14;
        const QRect glyphRect(indexRect.right() + 4, contentRect.top(), glyphWidth, contentRect.height());
        if (isActive) {
            painter->setPen(QColor(0xff, 0xaa, 0x00));
            painter->drawText(glyphRect, Qt::AlignCenter, QStringLiteral("▶"));
        }

        // Duration column (right-aligned)
        const int durWidth = 50;
        const QRect durRect(contentRect.right() - durWidth, contentRect.top(), durWidth, contentRect.height());
        painter->setPen(durColor);
        painter->drawText(durRect, Qt::AlignVCenter | Qt::AlignRight, durationStr);

        // Title column (left-aligned, elided)
        const int titleLeft = glyphRect.right() + 4;
        const int titleRight = durRect.left() - 8;
        const int availableTitleWidth = std::max(0, titleRight - titleLeft);
        const QRect titleRect(titleLeft, contentRect.top(), availableTitleWidth, contentRect.height());

        painter->setPen(textColor);
        const QString elidedTitle = option.fontMetrics.elidedText(title, Qt::ElideRight, availableTitleWidth);
        painter->drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);

        painter->restore();
    }

private:
    QSortFilterProxyModel* m_proxy{nullptr};
};

// ==============================================================================
// PlaylistDock
// ==============================================================================

PlaylistDock::PlaylistDock(playlist::PlaylistModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    setMinimumWidth(200);
    setMaximumWidth(600);
    resize(300, 500);
    setAcceptDrops(true);

    setupUi();
    setupStyle();
    updateStatusLabel();
}

void PlaylistDock::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Top bar: TabBar + Close button
    auto* topBarWidget = new QWidget(this);
    topBarWidget->setObjectName("topBar");
    auto* topBarLayout = new QHBoxLayout(topBarWidget);
    topBarLayout->setContentsMargins(4, 2, 2, 2);
    topBarLayout->setSpacing(2);

    m_tabBar = new QTabBar(topBarWidget);
    m_tabBar->setObjectName("playlistTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->addTab(tr("Default"));
    m_tabBar->addTab(tr("Favorites"));
    m_tabBar->addTab(tr("Albums"));
    m_tabBar->addTab(tr("History"));
    topBarLayout->addWidget(m_tabBar, 1);

    m_closeBtn = new QPushButton(QStringLiteral("✕"), topBarWidget);
    m_closeBtn->setObjectName("playlistCloseBtn");
    m_closeBtn->setToolTip(tr("Close Playlist"));
    m_closeBtn->setFixedSize(26, 24);
    connect(m_closeBtn, &QPushButton::clicked, this, &PlaylistDock::closeRequested);
    topBarLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(topBarWidget);

    // 2. Search filter bar
    auto* searchWidget = new QWidget(this);
    searchWidget->setObjectName("searchWidget");
    auto* searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(6, 4, 6, 4);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(searchWidget);
    m_searchEdit->setObjectName("playlistSearchEdit");
    m_searchEdit->setPlaceholderText(tr("Search playlist..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);

    mainLayout->addWidget(searchWidget);

    // 3. Proxy Model and List View
    m_proxyModel = new PlaylistFilterProxyModel(this);
    if (m_model) {
        m_proxyModel->setSourceModel(m_model);
    }

    m_listView = new QListView(this);
    m_listView->setObjectName("playlistView");
    m_listView->setModel(m_proxyModel);
    m_listView->setUniformItemSizes(true);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setMouseTracking(true);
    m_listView->setAcceptDrops(false);
    if (m_listView->viewport()) {
        m_listView->viewport()->setAcceptDrops(false);
    }

    m_delegate = new PlaylistDelegate(m_proxyModel, this);
    m_listView->setItemDelegate(m_delegate);

    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& proxyIndex) {
        if (m_proxyModel) {
            const QModelIndex srcIndex = m_proxyModel->mapToSource(proxyIndex);
            if (srcIndex.isValid()) {
                emit playItemRequested(srcIndex.row());
            }
        }
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxyModel->setFilterPattern(text);
        updateStatusLabel();
    });

    mainLayout->addWidget(m_listView, 1);

    // 4. Bottom Toolbar
    auto* bottomBarWidget = new QWidget(this);
    bottomBarWidget->setObjectName("bottomBar");
    auto* bottomBarLayout = new QHBoxLayout(bottomBarWidget);
    bottomBarLayout->setContentsMargins(6, 4, 6, 4);
    bottomBarLayout->setSpacing(4);

    m_addBtn = new QPushButton(QStringLiteral("+"), bottomBarWidget);
    m_addBtn->setObjectName("addBtn");
    m_addBtn->setToolTip(tr("Add Media"));
    m_addMenu = new QMenu(this);
    auto* addFilesAct = m_addMenu->addAction(tr("Add File(s)..."));
    connect(addFilesAct, &QAction::triggered, this, &PlaylistDock::addFilesRequested);
    auto* addFolderAct = m_addMenu->addAction(tr("Add Folder..."));
    connect(addFolderAct, &QAction::triggered, this, &PlaylistDock::addFolderRequested);
    auto* addUrlAct = m_addMenu->addAction(tr("Add URL..."));
    connect(addUrlAct, &QAction::triggered, this, &PlaylistDock::addUrlRequested);
    m_addBtn->setMenu(m_addMenu);
    bottomBarLayout->addWidget(m_addBtn);

    m_removeBtn = new QPushButton(QStringLiteral("-"), bottomBarWidget);
    m_removeBtn->setObjectName("removeBtn");
    m_removeBtn->setToolTip(tr("Remove Selected"));
    connect(m_removeBtn, &QPushButton::clicked, this, &PlaylistDock::removeSelected);
    bottomBarLayout->addWidget(m_removeBtn);

    m_sortBtn = new QPushButton(QStringLiteral("⇅"), bottomBarWidget);
    m_sortBtn->setObjectName("sortBtn");
    m_sortBtn->setToolTip(tr("Sort Playlist"));
    m_sortMenu = new QMenu(this);
    auto* sortNameAct = m_sortMenu->addAction(tr("Sort by Name"));
    connect(sortNameAct, &QAction::triggered, this, &PlaylistDock::sortByName);
    auto* sortDurationAct = m_sortMenu->addAction(tr("Sort by Duration"));
    connect(sortDurationAct, &QAction::triggered, this, &PlaylistDock::sortByDuration);
    auto* shuffleAct = m_sortMenu->addAction(tr("Shuffle"));
    connect(shuffleAct, &QAction::triggered, this, &PlaylistDock::shuffle);
    m_sortBtn->setMenu(m_sortMenu);
    bottomBarLayout->addWidget(m_sortBtn);

    m_menuBtn = new QPushButton(QStringLiteral("☰"), bottomBarWidget);
    m_menuBtn->setObjectName("menuBtn");
    m_menuBtn->setToolTip(tr("Playlist Options"));
    m_playlistMenu = new QMenu(this);
    auto* clearAct = m_playlistMenu->addAction(tr("Clear Playlist"));
    connect(clearAct, &QAction::triggered, this, &PlaylistDock::clearPlaylist);
    m_menuBtn->setMenu(m_playlistMenu);
    bottomBarLayout->addWidget(m_menuBtn);

    bottomBarLayout->addStretch(1);

    m_statusLabel = new QLabel(bottomBarWidget);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bottomBarLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(bottomBarWidget);

    // Connect model notifications to status label
    if (m_model) {
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &PlaylistDock::updateStatusLabel);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &PlaylistDock::updateStatusLabel);
        connect(m_model, &QAbstractItemModel::modelReset, this, &PlaylistDock::updateStatusLabel);
        connect(m_model, &QAbstractItemModel::layoutChanged, this, &PlaylistDock::updateStatusLabel);
        connect(m_model, &QAbstractItemModel::dataChanged, this, &PlaylistDock::updateStatusLabel);
    }
}

void PlaylistDock::setupStyle() {
    setStyleSheet(QStringLiteral(
        "PlaylistDock, QWidget#topBar, QWidget#searchWidget {"
        "    background-color: #1e1e1e;"
        "    color: #ffffff;"
        "}"
        "QTabBar {"
        "    background-color: #252525;"
        "}"
        "QTabBar::tab {"
        "    background-color: #252525;"
        "    color: #aaaaaa;"
        "    padding: 6px 10px;"
        "    border: none;"
        "    font-size: 11px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #1b1b1b;"
        "    color: #ffffff;"
        "    border-bottom: 2px solid #ffaa00;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: #2a2a2a;"
        "    color: #ffffff;"
        "}"
        "QLineEdit#playlistSearchEdit {"
        "    background-color: #181818;"
        "    color: #ffffff;"
        "    border: 1px solid #333333;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    font-size: 12px;"
        "}"
        "QLineEdit#playlistSearchEdit:focus {"
        "    border: 1px solid #ffaa00;"
        "}"
        "QListView#playlistView {"
        "    background-color: #1e1e1e;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QWidget#bottomBar {"
        "    background-color: #252525;"
        "    border-top: 1px solid #303030;"
        "}"
        "QPushButton {"
        "    background-color: #252525;"
        "    color: #cccccc;"
        "    border: 1px solid #383838;"
        "    border-radius: 3px;"
        "    padding: 3px 6px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #323232;"
        "    color: #ffffff;"
        "    border-color: #4a4a4a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #404040;"
        "}"
        "QPushButton#playlistCloseBtn {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #888888;"
        "    font-size: 13px;"
        "    padding: 2px 4px;"
        "}"
        "QPushButton#playlistCloseBtn:hover {"
        "    color: #ffffff;"
        "    background-color: #e81123;"
        "}"
        "QLabel#statusLabel {"
        "    color: #888888;"
        "    font-size: 11px;"
        "}"
        "QMenu {"
        "    background-color: #252525;"
        "    color: #ffffff;"
        "    border: 1px solid #383838;"
        "}"
        "QMenu::item {"
        "    padding: 5px 20px 5px 20px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #383838;"
        "    color: #ffaa00;"
        "}"
    ));
}

void PlaylistDock::setSearchFilter(const QString& pattern) {
    if (m_searchEdit) {
        m_searchEdit->setText(pattern);
    }
}

QString PlaylistDock::searchFilter() const {
    return m_searchEdit ? m_searchEdit->text() : QString();
}

bool PlaylistDock::isFilterActive() const {
    return m_searchEdit && !m_searchEdit->text().trimmed().isEmpty();
}

int PlaylistDock::selectedIndex() const {
    if (!m_listView || !m_listView->selectionModel() || !m_proxyModel) {
        return -1;
    }

    QModelIndex proxyIdx = m_listView->currentIndex();
    if (!proxyIdx.isValid()) {
        const auto selected = m_listView->selectionModel()->selectedIndexes();
        if (!selected.isEmpty()) {
            proxyIdx = selected.first();
        }
    }

    if (!proxyIdx.isValid()) {
        return -1;
    }

    const QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
    return srcIdx.isValid() ? srcIdx.row() : -1;
}

int PlaylistDock::currentTabIndex() const {
    return m_tabBar ? m_tabBar->currentIndex() : -1;
}

void PlaylistDock::setCurrentTabIndex(int index) {
    if (m_tabBar && index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setCurrentIndex(index);
    }
}

playlist::PlaylistModel* PlaylistDock::model() const noexcept {
    return m_model.data();
}

QSortFilterProxyModel* PlaylistDock::proxyModel() const noexcept {
    return m_proxyModel;
}

QListView* PlaylistDock::listView() const noexcept {
    return m_listView;
}

QLineEdit* PlaylistDock::searchEdit() const noexcept {
    return m_searchEdit;
}

QTabBar* PlaylistDock::tabBar() const noexcept {
    return m_tabBar;
}

QPushButton* PlaylistDock::closeButton() const noexcept {
    return m_closeBtn;
}

QPushButton* PlaylistDock::addButton() const noexcept {
    return m_addBtn;
}

QPushButton* PlaylistDock::removeButton() const noexcept {
    return m_removeBtn;
}

QPushButton* PlaylistDock::sortButton() const noexcept {
    return m_sortBtn;
}

QPushButton* PlaylistDock::menuButton() const noexcept {
    return m_menuBtn;
}

QLabel* PlaylistDock::statusLabel() const noexcept {
    return m_statusLabel;
}

QMenu* PlaylistDock::addMenu() const noexcept {
    return m_addMenu;
}

QMenu* PlaylistDock::sortMenu() const noexcept {
    return m_sortMenu;
}

QMenu* PlaylistDock::playlistMenu() const noexcept {
    return m_playlistMenu;
}

QSize PlaylistDock::sizeHint() const {
    return QSize(300, 500);
}

void PlaylistDock::addFiles() {
    emit addFilesRequested();
}

void PlaylistDock::addFolder() {
    emit addFolderRequested();
}

void PlaylistDock::addUrl() {
    emit addUrlRequested();
}

void PlaylistDock::removeSelected() {
    if (!m_listView || !m_listView->selectionModel() || !m_model || !m_proxyModel) {
        return;
    }

    auto selected = m_listView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        const QModelIndex curr = m_listView->currentIndex();
        if (curr.isValid()) {
            selected.append(curr);
        }
    }

    if (selected.isEmpty()) {
        return;
    }

    std::vector<int> rows;
    rows.reserve(selected.size());
    for (const auto& idx : selected) {
        const auto src = m_proxyModel->mapToSource(idx);
        if (src.isValid()) {
            rows.push_back(src.row());
        }
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    for (int r : rows) {
        m_model->removeItem(r);
    }
}

void PlaylistDock::clearPlaylist() {
    if (m_model) {
        m_model->clear();
    }
}

void PlaylistDock::sortByName() {
    if (m_model) {
        m_model->sortByName();
    }
}

void PlaylistDock::sortByDuration() {
    if (m_model) {
        m_model->sortByDuration();
    }
}

void PlaylistDock::shuffle() {
    if (m_model) {
        m_model->shuffle();
    }
}

void PlaylistDock::updateStatusLabel() {
    if (!m_statusLabel) {
        return;
    }

    int count = 0;
    double totalSec = 0.0;

    if (m_proxyModel) {
        count = m_proxyModel->rowCount();
        for (int r = 0; r < count; ++r) {
            const QModelIndex idx = m_proxyModel->index(r, 0);
            totalSec += idx.data(playlist::PlaylistModel::DurationRole).toDouble();
        }
    } else if (m_model) {
        count = m_model->rowCount();
        totalSec = m_model->totalDuration();
    }

    const QString itemWord = (count == 1) ? QStringLiteral("item") : QStringLiteral("items");
    const QString durFormatted = TimeFormatter::formatTime(totalSec);
    m_statusLabel->setText(QStringLiteral("%1 %2 [%3]").arg(count).arg(itemWord, durFormatted));
}

void PlaylistDock::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PlaylistDock::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PlaylistDock::dropEvent(QDropEvent* event) {
    const QMimeData* mime = event->mimeData();
    if (!mime || !mime->hasUrls() || !m_model) {
        event->ignore();
        return;
    }

    QList<playlist::PlaylistItem> itemsToAdd;
    const auto urls = mime->urls();
    for (const QUrl& url : urls) {
        const QString path = url.toLocalFile();
        if (path.isEmpty()) {
            continue;
        }

        const QFileInfo fi(path);
        if (fi.isDir()) {
            const QStringList scanned = MediaOpenController::scanDirectory(path, true);
            for (const QString& file : scanned) {
                const QFileInfo sfi(file);
                itemsToAdd.append(playlist::PlaylistItem{file, sfi.fileName(), 0.0});
            }
        } else if (fi.isFile() && MediaOpenController::isMediaFile(path)) {
            itemsToAdd.append(playlist::PlaylistItem{path, fi.fileName(), 0.0});
        }
    }

    if (!itemsToAdd.isEmpty()) {
        m_model->addItems(itemsToAdd);
    }
    event->acceptProposedAction();
}

} // namespace outplayer::ui
