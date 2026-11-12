// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <functional>
#include <unordered_map>

#include "core/CommandId.h"

namespace outplayer::core {

class CommandRegistry : public QObject {
    Q_OBJECT

public:
    using Handler = std::function<void(const QVariantMap& params)>;

    static CommandRegistry& instance();

    explicit CommandRegistry(QObject* parent = nullptr);
    ~CommandRegistry() override = default;

    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry& operator=(const CommandRegistry&) = delete;
    CommandRegistry(CommandRegistry&&) = delete;
    CommandRegistry& operator=(CommandRegistry&&) = delete;

    bool registerHandler(CommandId id, Handler handler);
    void unregisterHandler(CommandId id);
    void clearHandlers();

    bool execute(CommandId id, const QVariantMap& params = {});
    bool execute(const QString& commandIdString, const QVariantMap& params = {});

    [[nodiscard]] const CommandInfo* getInfo(CommandId id) const;
    [[nodiscard]] const CommandInfo* getInfo(const QString& commandIdString) const;
    [[nodiscard]] QList<CommandInfo> allCommands() const;

signals:
    void commandExecuted(CommandId id, const QVariantMap& params);

private:
    QList<CommandInfo> m_commands;
    std::unordered_map<CommandId, size_t> m_idToIndex;
    std::unordered_map<CommandId, Handler> m_handlers;
};

} // namespace outplayer::core
