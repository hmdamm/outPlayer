#include "core/CommandRegistry.h"

namespace outplayer::core {

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry s_instance;
    return s_instance;
}

CommandRegistry::CommandRegistry(QObject* parent)
    : QObject(parent)
    , m_commands(createDefaultCommandCatalog())
{
    m_idToIndex.reserve(m_commands.size());
    for (qsizetype i = 0; i < m_commands.size(); ++i) {
        m_idToIndex[m_commands[i].id] = static_cast<size_t>(i);
    }
}

bool CommandRegistry::registerHandler(CommandId id, Handler handler) {
    if (!handler) {
        m_handlers.erase(id);
        return false;
    }
    if (m_idToIndex.find(id) == m_idToIndex.end()) {
        return false;
    }
    m_handlers[id] = std::move(handler);
    return true;
}

void CommandRegistry::unregisterHandler(CommandId id) {
    m_handlers.erase(id);
}

void CommandRegistry::clearHandlers() {
    m_handlers.clear();
}

bool CommandRegistry::execute(CommandId id, const QVariantMap& params) {
    const auto it = m_handlers.find(id);
    if (it == m_handlers.end()) {
        return false;
    }
    it->second(params);
    emit commandExecuted(id, params);
    return true;
}

bool CommandRegistry::execute(const QString& commandIdString, const QVariantMap& params) {
    const auto idOpt = stringToCommandId(commandIdString);
    if (!idOpt.has_value()) {
        return false;
    }
    return execute(*idOpt, params);
}

const CommandInfo* CommandRegistry::getInfo(CommandId id) const {
    const auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return nullptr;
    }
    return &m_commands[it->second];
}

const CommandInfo* CommandRegistry::getInfo(const QString& commandIdString) const {
    const auto idOpt = stringToCommandId(commandIdString);
    if (!idOpt.has_value()) {
        return nullptr;
    }
    return getInfo(*idOpt);
}

QList<CommandInfo> CommandRegistry::allCommands() const {
    return m_commands;
}

} // namespace outplayer::core
