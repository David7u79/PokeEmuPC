#include "pocket/core/GameId.hpp"
#include <QUuid>

namespace Pocket::Core {

GameId::GameId() = default;

GameId::GameId(std::string idStr)
    : m_id(std::move(idStr)) {}

GameId GameId::generate() {
    return GameId(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
}

} // namespace Pocket::Core
