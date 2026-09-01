#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace PocketPartner::Core {

enum class GameGeneration {
    Gen1_GB,   // Red / Blue / Yellow
    Gen2_GBC,  // Gold / Silver / Crystal
    Gen3_GBA,  // Ruby / Sapphire / Emerald / FireRed / LeafGreen
    Gen4_NDS,  // Diamond / Pearl / Platinum / HeartGold / SoulSilver
    Gen5_NDS   // Black / White / Black 2 / White 2
};

class CompanionLink {
public:
    CompanionLink() = default;

    CompanionLink(GameGeneration gen,
                  uint32_t personalityValue,
                  uint16_t trainerId,
                  uint16_t secretId,
                  uint16_t speciesId,
                  std::string nickname,
                  std::string trainerName,
                  uint64_t initialChecksum);

    GameGeneration generation() const { return m_gen; }
    uint32_t personalityValue() const { return m_pv; }
    uint16_t trainerId() const { return m_tid; }
    uint16_t secretId() const { return m_sid; }
    uint16_t speciesId() const { return m_speciesId; }
    const std::string& nickname() const { return m_nickname; }
    const std::string& trainerName() const { return m_trainerName; }
    uint64_t initialChecksum() const { return m_initialChecksum; }

    // Computes unique cryptographic hash string for identity verification
    std::string identityHash() const;

    // Checks if identity is clear and unambiguous
    bool isValid() const;

    // Compares identity with another link signature
    bool matches(const CompanionLink& other) const;

private:
    GameGeneration m_gen{GameGeneration::Gen3_GBA};
    uint32_t m_pv{0};
    uint16_t m_tid{0};
    uint16_t m_sid{0};
    uint16_t m_speciesId{0};
    std::string m_nickname;
    std::string m_trainerName;
    uint64_t m_initialChecksum{0};
};

} // namespace PocketPartner::Core
