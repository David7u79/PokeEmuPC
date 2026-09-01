#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

namespace Pocket::Emulator {

// PersistentGameSave represents persistent cartridge SRAM/Flash/EEPROM save files (.sav)
// Explicitly decoupled from SaveState snapshots to avoid confusion.
class PersistentGameSave {
public:
    PersistentGameSave() = default;
    explicit PersistentGameSave(const std::string& savePath) : m_savePath(savePath) {}

    const std::string& savePath() const { return m_savePath; }
    const std::vector<uint8_t>& data() const { return m_data; }

    bool isEmpty() const { return m_data.empty(); }
    size_t size() const { return m_data.size(); }

    void setData(std::vector<uint8_t> data) {
        m_data = std::move(data);
    }

    bool loadFromFile(const std::string& path) {
        m_savePath = path;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) return false;

        file.seekg(0, std::ios::beg);
        m_data.resize(static_cast<size_t>(fileSize));
        return file.read(reinterpret_cast<char*>(m_data.data()), fileSize).good();
    }

    bool saveToFile(const std::string& path) const {
        if (m_data.empty()) return false; // 12-Step Save Safety: Never write 0-byte uninitialized save
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
        return file.good();
    }

private:
    std::string m_savePath;
    std::vector<uint8_t> m_data;
};

} // namespace Pocket::Emulator
