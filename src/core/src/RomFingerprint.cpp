#include "pocket/core/RomFingerprint.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <QCryptographicHash>
#include <QByteArray>

namespace Pocket::Core {

uint32_t RomFingerprint::calculateCrc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

std::string RomFingerprint::calculateSha256(const uint8_t* data, size_t length) {
    QByteArray hash = QCryptographicHash::hash(
        QByteArray(reinterpret_cast<const char*>(data), static_cast<qsizetype>(length)),
        QCryptographicHash::Sha256
    );
    return hash.toHex().toStdString();
}

RomFingerprint RomFingerprint::calculateFromBuffer(const std::vector<uint8_t>& buffer) {
    RomFingerprint fp;
    if (buffer.empty()) return fp;

    fp.fileSize = buffer.size();

    uint32_t crc = calculateCrc32(buffer.data(), buffer.size());
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << crc;
    fp.crc32 = ss.str();

    fp.sha256 = calculateSha256(buffer.data(), buffer.size());

    QByteArray md5Hash = QCryptographicHash::hash(
        QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(buffer.size())),
        QCryptographicHash::Md5
    );
    fp.md5 = md5Hash.toHex().toUpper().toStdString();

    return fp;
}

RomFingerprint RomFingerprint::calculate(const std::string& romFilePath) {
    std::ifstream file(romFilePath, std::ios::binary);
    if (!file.is_open()) return {};

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return calculateFromBuffer(buffer);
}

} // namespace Pocket::Core
