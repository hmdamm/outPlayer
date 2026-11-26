// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSet>

#include "core/CommandId.h"

class QWidget;
class QShortcut;

namespace outplayer::input {

enum class MouseAction {
    LeftClick,
    LeftDblClick,
    MiddleClick,
    RightClick,
    WheelUp,
    WheelDown
};

class InputManager : public QObject {
    Q_OBJECT

public:
    explicit InputManager(QWidget* parentWindow = nullptr);
    ~InputManager() override;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;

    void attachToWindow(QWidget* window);
    void attachMouseFilter(QWidget* surface);

    bool setCustomShortcut(core::CommandId id, const QKeySequence& shortcut);
    QKeySequence getShortcut(core::CommandId id) const;
    void resetShortcut(core::CommandId id);

    void setMouseMapping(MouseAction action, core::CommandId commandId);
    core::CommandId getMouseMapping(MouseAction action) const;

signals:
    void mouseActionTriggered(MouseAction action, core::CommandId commandId);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initDefaultShortcuts();
    void initDefaultMouseMappings();
    void rebuildShortcuts();
    bool isWindowScopedCommand(core::CommandId id) const;
    void applyWindowShortcut(core::CommandId id, const QKeySequence& seq);

    QPointer<QWidget> m_parentWindow;
    QMap<core::CommandId, QKeySequence> m_defaultShortcuts;
    QMap<core::CommandId, QKeySequence> m_shortcuts;
    QMap<MouseAction, core::CommandId> m_mouseMappings;
    QMap<core::CommandId, QPointer<QShortcut>> m_windowShortcuts;
    QSet<QWidget*> m_watchedSurfaces;
};

} // namespace outplayer::input

Q_DECLARE_METATYPE(outplayer::input::MouseAction)
