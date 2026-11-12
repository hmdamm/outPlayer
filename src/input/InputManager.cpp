// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/InputManager.h"

#include <QMouseEvent>
#include <QShortcut>
#include <QWheelEvent>
#include <QWidget>
#include <optional>

#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"

namespace outplayer::input {

namespace {

QString mouseActionToString(MouseAction action) {
    switch (action) {
    case MouseAction::LeftClick: return QStringLiteral("LeftClick");
    case MouseAction::LeftDblClick: return QStringLiteral("LeftDblClick");
    case MouseAction::MiddleClick: return QStringLiteral("MiddleClick");
    case MouseAction::RightClick: return QStringLiteral("RightClick");
    case MouseAction::WheelUp: return QStringLiteral("WheelUp");
    case MouseAction::WheelDown: return QStringLiteral("WheelDown");
    }
    return QStringLiteral("Unknown");
}

} // namespace

InputManager::InputManager(QWidget* parentWindow)
    : QObject(parentWindow)
{
    qRegisterMetaType<MouseAction>("MouseAction");
    qRegisterMetaType<MouseAction>("outplayer::input::MouseAction");

    initDefaultShortcuts();
    initDefaultMouseMappings();

    if (parentWindow) {
        attachToWindow(parentWindow);
    }
}

InputManager::~InputManager() {
    for (QWidget* surface : m_watchedSurfaces) {
        if (surface) {
            surface->removeEventFilter(this);
        }
    }
    m_watchedSurfaces.clear();

    for (auto& shortcut : m_windowShortcuts) {
        if (shortcut) {
            delete shortcut;
        }
    }
    m_windowShortcuts.clear();
}

void InputManager::initDefaultShortcuts() {
    m_defaultShortcuts.clear();
    m_shortcuts.clear();

    const auto catalog = core::createDefaultCommandCatalog();
    for (const auto& info : catalog) {
        const QString& rawKey = info.defaultKey;
        bool isKeyboardDefault = !rawKey.isEmpty() && rawKey != QStringLiteral("None") &&
            !rawKey.startsWith(QStringLiteral("LeftClick")) &&
            !rawKey.startsWith(QStringLiteral("LeftDblClick")) &&
            !rawKey.startsWith(QStringLiteral("LeftDrag")) &&
            !rawKey.startsWith(QStringLiteral("MiddleClick")) &&
            !rawKey.startsWith(QStringLiteral("MiddleDblClick")) &&
            !rawKey.startsWith(QStringLiteral("RightClick")) &&
            !rawKey.startsWith(QStringLiteral("Wheel")) &&
            !rawKey.startsWith(QStringLiteral("XButton"));

        QKeySequence defaultSeq;
        if (isKeyboardDefault) {
            QString keyStr = rawKey;
            if (keyStr == QStringLiteral("Numpad_Add")) {
                keyStr = QStringLiteral("+");
            } else if (keyStr == QStringLiteral("Numpad_Subtract")) {
                keyStr = QStringLiteral("-");
            } else if (keyStr.startsWith(QStringLiteral("Numpad")) && keyStr.length() > 6) {
                keyStr = keyStr.mid(6);
            }

            QString fullSeq;
            if (!info.defaultModifiers.isEmpty() && info.defaultModifiers != QStringLiteral("None")) {
                fullSeq = info.defaultModifiers + QStringLiteral("+") + keyStr;
            } else {
                fullSeq = keyStr;
            }

            defaultSeq = QKeySequence(fullSeq, QKeySequence::PortableText);
            if (!defaultSeq.isEmpty()) {
                m_defaultShortcuts[info.id] = defaultSeq;
            }
        }

        // Check if user has an override stored in settings
        QString settingKey = QStringLiteral("shortcuts/") + core::commandIdToString(info.id);
        if (core::SettingsManager::instance().contains(settingKey)) {
            QString customStr = core::SettingsManager::instance().get(settingKey).toString();
            m_shortcuts[info.id] = QKeySequence::fromString(customStr, QKeySequence::PortableText);
        } else if (!defaultSeq.isEmpty()) {
            m_shortcuts[info.id] = defaultSeq;
        }
    }
}

void InputManager::initDefaultMouseMappings() {
    m_mouseMappings[MouseAction::LeftClick] = core::CommandId::CMD_PLAY_PAUSE;
    m_mouseMappings[MouseAction::LeftDblClick] = core::CommandId::CMD_WINDOW_FULLSCREEN;
    m_mouseMappings[MouseAction::MiddleClick] = core::CommandId::CMD_VOLUME_MUTE;
    m_mouseMappings[MouseAction::RightClick] = core::CommandId::CMD_MOUSE_RIGHT_CLICK;
    m_mouseMappings[MouseAction::WheelUp] = core::CommandId::CMD_VOLUME_UP;
    m_mouseMappings[MouseAction::WheelDown] = core::CommandId::CMD_VOLUME_DOWN;

    // Load persisted mouse overrides if present
    for (auto it = m_mouseMappings.begin(); it != m_mouseMappings.end(); ++it) {
        QString key = QStringLiteral("mouse/") + mouseActionToString(it.key());
        if (core::SettingsManager::instance().contains(key)) {
            QString cmdStr = core::SettingsManager::instance().get(key).toString();
            auto optId = core::stringToCommandId(cmdStr);
            if (optId.has_value()) {
                it.value() = *optId;
            }
        }
    }
}

void InputManager::attachToWindow(QWidget* window) {
    for (auto& shortcut : m_windowShortcuts) {
        if (shortcut) {
            delete shortcut;
        }
    }
    m_windowShortcuts.clear();

    m_parentWindow = window;
    if (!m_parentWindow) {
        return;
    }

    rebuildShortcuts();
}

void InputManager::attachMouseFilter(QWidget* surface) {
    if (!surface || m_watchedSurfaces.contains(surface)) {
        return;
    }
    surface->installEventFilter(this);
    m_watchedSurfaces.insert(surface);
    connect(surface, &QObject::destroyed, this, [this, surface]() {
        m_watchedSurfaces.remove(surface);
    });
}

void InputManager::rebuildShortcuts() {
    if (!m_parentWindow) {
        return;
    }

    for (auto& shortcut : m_windowShortcuts) {
        if (shortcut) {
            delete shortcut;
        }
    }
    m_windowShortcuts.clear();

    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        core::CommandId id = it.key();
        const QKeySequence& seq = it.value();
        if (seq.isEmpty()) {
            continue;
        }

        auto* shortcut = new QShortcut(seq, m_parentWindow.data(), nullptr, nullptr, Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, [id]() {
            core::CommandRegistry::instance().execute(id);
        });
        m_windowShortcuts[id] = shortcut;
    }
}

bool InputManager::setCustomShortcut(core::CommandId id, const QKeySequence& shortcut) {
    if (core::CommandRegistry::instance().getInfo(id) == nullptr) {
        return false;
    }

    m_shortcuts[id] = shortcut;

    QString settingKey = QStringLiteral("shortcuts/") + core::commandIdToString(id);
    core::SettingsManager::instance().set(settingKey, shortcut.toString(QKeySequence::PortableText));

    if (m_parentWindow) {
        if (shortcut.isEmpty()) {
            if (m_windowShortcuts.contains(id) && m_windowShortcuts[id]) {
                delete m_windowShortcuts[id];
                m_windowShortcuts.remove(id);
            }
        } else {
            if (m_windowShortcuts.contains(id) && m_windowShortcuts[id]) {
                m_windowShortcuts[id]->setKey(shortcut);
            } else {
                auto* sc = new QShortcut(shortcut, m_parentWindow.data(), nullptr, nullptr, Qt::WindowShortcut);
                connect(sc, &QShortcut::activated, this, [id]() {
                    core::CommandRegistry::instance().execute(id);
                });
                m_windowShortcuts[id] = sc;
            }
        }
    }

    return true;
}

QKeySequence InputManager::getShortcut(core::CommandId id) const {
    auto it = m_shortcuts.find(id);
    if (it != m_shortcuts.end()) {
        return it.value();
    }
    return QKeySequence();
}

void InputManager::resetShortcut(core::CommandId id) {
    QString settingKey = QStringLiteral("shortcuts/") + core::commandIdToString(id);
    core::SettingsManager::instance().remove(settingKey);

    auto defIt = m_defaultShortcuts.find(id);
    if (defIt != m_defaultShortcuts.end()) {
        m_shortcuts[id] = defIt.value();
    } else {
        m_shortcuts.remove(id);
    }

    if (m_parentWindow) {
        QKeySequence seq = getShortcut(id);
        if (seq.isEmpty()) {
            if (m_windowShortcuts.contains(id) && m_windowShortcuts[id]) {
                delete m_windowShortcuts[id];
                m_windowShortcuts.remove(id);
            }
        } else {
            if (m_windowShortcuts.contains(id) && m_windowShortcuts[id]) {
                m_windowShortcuts[id]->setKey(seq);
            } else {
                auto* sc = new QShortcut(seq, m_parentWindow.data(), nullptr, nullptr, Qt::WindowShortcut);
                connect(sc, &QShortcut::activated, this, [id]() {
                    core::CommandRegistry::instance().execute(id);
                });
                m_windowShortcuts[id] = sc;
            }
        }
    }
}

void InputManager::setMouseMapping(MouseAction action, core::CommandId commandId) {
    m_mouseMappings[action] = commandId;
    QString key = QStringLiteral("mouse/") + mouseActionToString(action);
    core::SettingsManager::instance().set(key, core::commandIdToString(commandId));
}

core::CommandId InputManager::getMouseMapping(MouseAction action) const {
    auto it = m_mouseMappings.find(action);
    if (it != m_mouseMappings.end()) {
        return it.value();
    }

    switch (action) {
    case MouseAction::LeftClick: return core::CommandId::CMD_PLAY_PAUSE;
    case MouseAction::LeftDblClick: return core::CommandId::CMD_WINDOW_FULLSCREEN;
    case MouseAction::MiddleClick: return core::CommandId::CMD_VOLUME_MUTE;
    case MouseAction::RightClick: return core::CommandId::CMD_MOUSE_RIGHT_CLICK;
    case MouseAction::WheelUp: return core::CommandId::CMD_VOLUME_UP;
    case MouseAction::WheelDown: return core::CommandId::CMD_VOLUME_DOWN;
    }
    return core::CommandId::CMD_PLAY_PAUSE;
}

bool InputManager::eventFilter(QObject* watched, QEvent* event) {
    if (!event) {
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        std::optional<MouseAction> action;
        if (mouseEvent->button() == Qt::LeftButton) {
            action = MouseAction::LeftClick;
        } else if (mouseEvent->button() == Qt::MiddleButton) {
            action = MouseAction::MiddleClick;
        } else if (mouseEvent->button() == Qt::RightButton) {
            action = MouseAction::RightClick;
        }

        if (action.has_value()) {
            core::CommandId cmd = getMouseMapping(*action);
            core::CommandRegistry::instance().execute(cmd);
            emit mouseActionTriggered(*action, cmd);
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            MouseAction action = MouseAction::LeftDblClick;
            core::CommandId cmd = getMouseMapping(action);
            core::CommandRegistry::instance().execute(cmd);
            emit mouseActionTriggered(action, cmd);
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        const int deltaY = wheelEvent->angleDelta().y();
        if (deltaY != 0) {
            MouseAction action = (deltaY > 0) ? MouseAction::WheelUp : MouseAction::WheelDown;
            core::CommandId cmd = getMouseMapping(action);
            core::CommandRegistry::instance().execute(cmd);
            emit mouseActionTriggered(action, cmd);
            event->accept();
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

} // namespace outplayer::input
