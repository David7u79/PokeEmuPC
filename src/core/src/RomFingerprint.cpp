#include "pocket/core/RomFingerprint.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include <algorithm>
#include <QCryptographicHash>
#include <QByteArray>

namespace Pocket::Core {

namespace {

const std::array<uint32_t, 256>& crc32Table() {
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> result{};
        for (uint32_t value = 0; value < result.size(); ++value) {
            uint32_t crc = value;
            for (int bit = 0; bit < 8; ++bit) {
                crc = (crc >> 1) ^ ((crc & 1U) != 0U ? 0xEDB88320U : 0U);
            }
            result[value] = crc;
        }
        return result;
    }();
    return table;
}

uint32_t updateCrc32(uint32_t crc, const uint8_t* data, size_t length) {
    const auto& table = crc32Table();
    for (size_t index = 0; index < length; ++index) {
        crc = (crc >> 8) ^ table[(crc ^ data[index]) & 0xFFU];
    }
    return crc;
}

std::string formatCrc32(uint32_t crc) {
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << crc;
    return stream.str();
}

} // namespace

uint32_t RomFingerprint::calculateCrc32(const uint8_t* data, size_t length) {
    uint32_t crc = updateCrc32(0xFFFFFFFFU, data, length);
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
    fp.crc32 = formatCrc32(crc);

    fp.sha256 = calculateSha256(buffer.data(), buffer.size());

    QByteArray md5Hash = QCryptographicHash::hash(
        QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(buffer.size())),
        QCryptographicHash::Md5
    );
    fp.md5 = md5Hash.toHex().toUpper().toStdString();

    return fp;
}

RomFingerprint RomFingerprint::calculate(const std::string& romFilePath) {
    return calculate(romFilePath, {});
}

RomFingerprint RomFingerprint::calculate(const std::string& romFilePath, const ProgressCallback& progress) {
    std::ifstream file(romFilePath, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    file.seekg(0, std::ios::end);
    const std::streamoff end = file.tellg();
    if (end < 0) {
        return {};
    }
    const qint64 total = static_cast<qint64>(end);
    file.seekg(0, std::ios::beg);
    if (!file) {
        return {};
    }

    if (progress && !progress(0, total)) {
        return {};
    }

    constexpr size_t bufferSize = 256 * 1024;
    std::array<uint8_t, bufferSize> buffer{};
    QCryptographicHash sha256(QCryptographicHash::Sha256);
    QCryptographicHash md5(QCryptographicHash::Md5);
    uint32_t crc = 0xFFFFFFFFU;
    qint64 done = 0;
    qint64 nextProgress = total > 0 ? std::max<qint64>(1, total / 100) : 0;

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) {
            break;
        }

        const size_t length = static_cast<size_t>(bytesRead);
        crc = updateCrc32(crc, buffer.data(), length);
        sha256.addData(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(length));
        md5.addData(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(length));
        done += static_cast<qint64>(length);

        if (progress && (done >= nextProgress || done == total)) {
            if (!progress(done, total)) {
                return {};
            }
            nextProgress = done + std::max<qint64>(1, total / 100);
        }
    }

    if (!file.eof() || done != total) {
        return {};
    }

    RomFingerprint fp;
    if (done == 0) {
        return fp;
    }

    fp.fileSize = static_cast<uint64_t>(done);
    fp.crc32 = formatCrc32(~crc);
    fp.sha256 = sha256.result().toHex().toStdString();
    fp.md5 = md5.result().toHex().toUpper().toStdString();
    return fp;
}

} // namespace Pocket::Core
