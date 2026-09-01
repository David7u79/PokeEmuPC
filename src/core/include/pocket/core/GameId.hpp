#pragma once

#include <string>

namespace Pocket::Core {

class GameId {
public:
    GameId();
    explicit GameId(std::string idStr);

    static GameId generate();

    const std::string& toString() const { return m_id; }
    bool isEmpty() const { return m_id.empty(); }

    bool operator==(const GameId& other) const { return m_id == other.m_id; }
    bool operator!=(const GameId& other) const { return m_id != other.m_id; }
    bool operator<(const GameId& other) const { return m_id < other.m_id; }

private:
    std::string m_id;
};

} // namespace Pocket::Core
