#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

namespace Pocket::Save {

class FileStabilityVerifier {
public:
    static bool isFileStable(const std::string& path, int debounceMs = 50, int maxWaitMs = 1500) {
        namespace fs = std::filesystem;
        if (!fs::exists(path)) return false;

        auto startTime = std::chrono::steady_clock::now();
        uint64_t prevSize = 0;
        fs::file_time_type prevTime;

        try {
            prevSize = fs::file_size(path);
            prevTime = fs::last_write_time(path);
        } catch (...) {
            return false;
        }

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(debounceMs));

            if (!fs::exists(path)) return false;

            try {
                uint64_t currSize = fs::file_size(path);
                auto currTime = fs::last_write_time(path);

                if (currSize == prevSize && currTime == prevTime && currSize > 0) {
                    return true; // Size and write timestamp remain stable
                }

                prevSize = currSize;
                prevTime = currTime;
            } catch (...) {
                return false;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
            if (elapsed >= maxWaitMs) {
                break;
            }
        }
        return false;
    }
};

} // namespace Pocket::Save
