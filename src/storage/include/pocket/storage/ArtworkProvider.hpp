#pragma once

#include <string>
#include <functional>

namespace Pocket::Storage {

enum class ArtworkType {
    BoxArt,
    TitleScreen,
    Screenshot
};

struct ArtworkResult {
    bool success{false};
    std::string cachedFilePath;
    std::string errorMessage;
};

class ArtworkProvider {
public:
    virtual ~ArtworkProvider() = default;
    virtual void fetchArtworkAsync(
        const std::string& platform,
        const std::string& canonicalTitle,
        ArtworkType type,
        std::function<void(const ArtworkResult&)> callback
    ) = 0;
};

} // namespace Pocket::Storage
