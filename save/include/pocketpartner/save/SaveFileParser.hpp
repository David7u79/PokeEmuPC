#pragma once

#include "pocketpartner/core/CanonicalPokemonState.hpp"
#include "pocketpartner/core/CompanionLink.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <optional>

namespace PocketPartner::Save {

struct SaveParseResult {
    bool success{false};
    std::string errorMessage;
    Core::GameGeneration generation{Core::GameGeneration::Gen3_GBA};
    uint64_t fileHash{0};
    std::vector<Core::CanonicalPokemonState> party;
    std::vector<Core::CanonicalPokemonState> boxes;
};

class SaveFileParser {
public:
    virtual ~SaveFileParser() = default;

    virtual SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) const = 0;
    virtual Core::GameGeneration supportedGeneration() const = 0;
    virtual uint16_t calculateBlockChecksum(const uint8_t* data, size_t length) const = 0;
};

// GBA Gen3 concrete parser implementation
class Gen3SaveParser : public SaveFileParser {
public:
    SaveParseResult parseSaveBuffer(const std::vector<uint8_t>& buffer) const override;
    Core::GameGeneration supportedGeneration() const override { return Core::GameGeneration::Gen3_GBA; }
    uint16_t calculateBlockChecksum(const uint8_t* data, size_t length) const override;

    // Repairs block checksums in mutated buffer
    bool repairChecksums(std::vector<uint8_t>& buffer) const;
};

} // namespace PocketPartner::Save
