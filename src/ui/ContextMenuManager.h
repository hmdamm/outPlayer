// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QPoint>

class QAction;
class QMenu;
class QWidget;

namespace outplayer::engine {
class IPlaybackEngine;
}

namespace outplayer::core {
enum class CommandId : uint16_t;
using engine::IPlaybackEngine;
}

namespace outplayer::ui {

class ContextMenuManager : public QObject {
    Q_OBJECT

public:
    explicit ContextMenuManager(QObject* parent = nullptr);
    ~ContextMenuManager() override = default;

    QMenu* buildMainMenu(QWidget* parentWidget, engine::IPlaybackEngine* engine = nullptr);
    void showContextMenu(const QPoint& globalPos, QWidget* parentWidget, engine::IPlaybackEngine* engine = nullptr);

signals:
    void openFilesRequested();
    void openFolderRequested();
    void openUrlRequested();
    void togglePlaylistRequested();
    void toggleControlPanelRequested();
    void preferencesRequested();
    void aboutRequested();

private:
    void applyStyleSheet(QMenu* menu);
    QAction* addCommandAction(QMenu* menu, const QString& title, core::CommandId cmdId);
};

} // namespace outplayer::ui
