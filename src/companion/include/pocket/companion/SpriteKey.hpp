#pragma once

#include <cstdint>
#include <string>

namespace Pocket::Companion {

enum class Gender : uint8_t {
    Unknown = 0,
    Male,
    Female,
    Genderless
};

struct SpriteKey {
    uint16_t speciesId{0};
    bool shiny{false};
    uint8_t formId{0};
    Gender gender{Gender::Unknown};

    bool isValid() const { return speciesId > 0; }

    std::string toString() const {
        return "Species:" + std::to_string(speciesId) +
               "_Shiny:" + (shiny ? "1" : "0") +
               "_Form:" + std::to_string(formId) +
               "_Gender:" + std::to_string(static_cast<int>(gender));
    }

    bool operator==(const SpriteKey& other) const {
        return speciesId == other.speciesId &&
               shiny == other.shiny &&
               formId == other.formId &&
               gender == other.gender;
    }

    bool operator<(const SpriteKey& other) const {
        if (speciesId != other.speciesId) return speciesId < other.speciesId;
        if (shiny != other.shiny) return shiny < other.shiny;
        if (formId != other.formId) return formId < other.formId;
        return gender < other.gender;
    }
};

} // namespace Pocket::Companion
