#include "pocketpartner/save/SaveFileParser.hpp"
#include <QCryptographicHash>
#include <QByteArray>
#include <cstring>

namespace PocketPartner::Save {

uint16_t Gen3SaveParser::calculateBlockChecksum(const uint8_t* data, size_t length) const {
    uint32_t sum = 0;
    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(data);
    size_t count = length / 4;

    for (size_t i = 0; i < count; ++i) {
        sum += ptr[i];
    }

    uint16_t upper = static_cast<uint16_t>(sum >> 16);
    uint16_t lower = static_cast<uint16_t>(sum & 0xFFFF);
    return lower + upper;
}

bool Gen3SaveParser::repairChecksums(std::vector<uint8_t>& buffer) const {
    if (buffer.size() < 131072) return false; // Gen3 saves are typically 128KB (131072 bytes)

    // GBA Save file consists of 14 sections per save slot (active and backup slot)
    // Section header / footer contains section ID, checksum, and save index
    // For each active section of 4096 bytes:
    for (size_t slot = 0; slot < 2; ++slot) {
        size_t slotOffset = slot * 57344; // 14 sections * 4096 bytes
        for (size_t sec = 0; sec < 14; ++sec) {
            size_t secOffset = slotOffset + (sec * 4096);
            if (secOffset + 4096 > buffer.size()) break;

            // Footer metadata location in section: checksum at 0x0FF4 (2 bytes)
            uint16_t checksum = calculateBlockChecksum(buffer.data() + secOffset, 3968);
            std::memcpy(buffer.data() + secOffset + 4084, &checksum, sizeof(checksum));
        }
    }
    return true;
}

SaveParseResult Gen3SaveParser::parseSaveBuffer(const std::vector<uint8_t>& buffer) const {
    SaveParseResult result;
    result.generation = Core::GameGeneration::Gen3_GBA;

    if (buffer.empty()) {
        result.errorMessage = "Empty save buffer.";
        return result;
    }

    // Compute SHA256 hash of buffer
    QByteArray hash = QCryptographicHash::hash(
        QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(buffer.size())),
        QCryptographicHash::Sha256
    );
    std::memcpy(&result.fileHash, hash.constData(), sizeof(uint64_t));

    // Validate size (Standard GBA save size is 128KB or 64KB)
    if (buffer.size() != 131072 && buffer.size() != 65536) {
        result.errorMessage = "Invalid GBA save size (expected 64KB or 128KB).";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace PocketPartner::Save
