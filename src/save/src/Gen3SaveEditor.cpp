#include "pocket/save/Gen3SaveEditor.hpp"
#include "pocket/save/CompanionReidentifier.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <QCryptographicHash>
#include <QFile>

namespace Pocket::Save {

static const int kSubstructureOrders[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}
};

Gen3SaveEditor::Gen3SaveEditor(
    std::shared_ptr<SaveSessionCoordinator> coordinator,
    std::shared_ptr<SaveBackupRepository> backupRepo
) : m_coordinator(std::move(coordinator)), m_backupRepo(std::move(backupRepo)) {}

std::string Gen3SaveEditor::calculateSha256(const std::vector<uint8_t>& buffer) {
    QByteArray hash = QCryptographicHash::hash(
        QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(buffer.size())),
        QCryptographicHash::Sha256
    );
    return QString(hash.toHex()).toStdString();
}

MutationResult Gen3SaveEditor::mutateFriendship(
    const std::string& saveFilePath,
    const Pocket::Companion::CompanionLink& targetLink,
    uint8_t newFriendshipValue
) {
    MutationResult result;

    if (!FileStabilityVerifier::isFileStable(saveFilePath)) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Save file is currently being modified by another process.";
        return result;
    }

    if (!m_coordinator->acquireMutationLock(saveFilePath)) {
        result.status = EditorStatus::SaveLockedByEmulator;
        result.errorMessage = "Save file is locked by an active emulator or external process.";
        return result;
    }

    struct LockGuard {
        std::shared_ptr<SaveSessionCoordinator> coord;
        std::string path;
        ~LockGuard() { coord->releaseMutationLock(path); }
    } guard{m_coordinator, saveFilePath};

    SaveParseResult origParse = m_parser.parseSaveFile(saveFilePath);
    if (origParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Failed to parse original save file: " + origParse.errorMessage;
        return result;
    }

    std::ifstream origFile(saveFilePath, std::ios::binary);
    std::vector<uint8_t> originalBuffer(131072);
    origFile.read(reinterpret_cast<char*>(originalBuffer.data()), 131072);
    origFile.close();

    std::string origHash = calculateSha256(originalBuffer);

    Pocket::Companion::CompanionLink reidentifiedLink = CompanionReidentifier::reidentify(targetLink, origParse, origHash);
    if (reidentifiedLink.status != Pocket::Companion::LinkStatus::Linked) {
        result.status = (reidentifiedLink.status == Pocket::Companion::LinkStatus::AmbiguousMatch)
            ? EditorStatus::CreatureAmbiguous : EditorStatus::CreatureNotFound;
        result.errorMessage = "Target creature could not be uniquely re-identified in save file.";
        return result;
    }

    SaveBackup backup = m_backupRepo->createBackup(std::to_string(targetLink.gameId), saveFilePath, "Pre-mutation backup");
    if (backup.path.empty()) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Failed to create pre-mutation backup.";
        return result;
    }
    result.audit.backupFilePath = backup.path;

    std::vector<uint8_t> modifiedBuffer = originalBuffer;

    int activeSlotIdx = origParse.activeSlotIndex;
    uint8_t* slotPtr = modifiedBuffer.data() + (activeSlotIdx == 0 ? 0x00000 : 0x0E000);

    uint8_t* pkmnPtr = nullptr;
    int targetSectionIdx = 0;

    if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Party) {
        targetSectionIdx = 1;
        uint8_t* sec1Ptr = slotPtr + (1 * 4096);
        int slotIdx = reidentifiedLink.locator.partySlot - 1;
        pkmnPtr = sec1Ptr + 0x238 + (slotIdx * 100);
    } else if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Box) {
        int boxIdx = reidentifiedLink.locator.boxNumber - 1;
        int slotIdx = reidentifiedLink.locator.boxSlot - 1;
        int globalSlot = (boxIdx * 30) + slotIdx;

        targetSectionIdx = 5 + (globalSlot / 60);
        uint8_t* secPtr = slotPtr + (targetSectionIdx * 4096);
        int offsetInSec = (globalSlot % 60) * 80 + 0x04;
        pkmnPtr = secPtr + offsetInSec;
    }

    if (!pkmnPtr) {
        result.status = EditorStatus::CreatureNotFound;
        result.errorMessage = "Could not map creature memory address.";
        return result;
    }

    uint32_t pid = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x00);
    uint32_t otId = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x04);
    uint32_t key = pid ^ otId;

    int orderIdx = static_cast<int>(pid % 24);
    const int* order = kSubstructureOrders[orderIdx];

    int blockGPos = 0;
    for (int p = 0; p < 4; ++p) {
        if (order[p] == 0) {
            blockGPos = p;
            break;
        }
    }

    uint32_t* dwords = reinterpret_cast<uint32_t*>(pkmnPtr + 0x20);
    int dwordIndex = (blockGPos * 3) + 2;

    uint32_t origDword = dwords[dwordIndex] ^ key;
    uint8_t oldFriendship = static_cast<uint8_t>((origDword >> 8) & 0xFF);

    uint8_t clampedNewFriendship = static_cast<uint8_t>(std::min<int>(255, newFriendshipValue));

    uint32_t modifiedDword = (origDword & 0xFFFF00FF) | (static_cast<uint32_t>(clampedNewFriendship) << 8);
    dwords[dwordIndex] = modifiedDword ^ key;

    uint8_t* secPtr = slotPtr + (targetSectionIdx * 4096);
    uint16_t newChecksum = Gen3SaveParser::calculateSectionChecksum(secPtr);
    *reinterpret_cast<uint16_t*>(secPtr + 0xFF6) = newChecksum;

    SaveParseResult modParse = m_parser.parseSaveBuffer(modifiedBuffer);
    if (modParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Modified buffer failed section checksum verification.";
        return result;
    }

    size_t bytesModified = 0;
    for (size_t i = 0; i < 131072; ++i) {
        if (originalBuffer[i] != modifiedBuffer[i]) {
            bytesModified++;
        }
    }

    if (bytesModified < 2 || bytesModified > 3) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Semantic diff failed: Expected 2 or 3 modified bytes, but found " + std::to_string(bytesModified);
        return result;
    }

    result.audit.creatureNickname = reidentifiedLink.nickname;
    result.audit.speciesName = reidentifiedLink.speciesName;
    result.audit.oldFriendship = oldFriendship;
    result.audit.newFriendship = clampedNewFriendship;
    result.audit.bytesModified = bytesModified;
    result.audit.unrelatedFieldsChanged = 0;
    result.audit.isVerified = true;

    std::string tmpFilePath = saveFilePath + ".tmp";
    std::ofstream tmpFile(tmpFilePath, std::ios::binary);
    if (!tmpFile.is_open()) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Could not open temporary file for atomic write.";
        return result;
    }
    tmpFile.write(reinterpret_cast<const char*>(modifiedBuffer.data()), 131072);
    tmpFile.close();

    QFile::remove(QString::fromStdString(saveFilePath));
    if (!QFile::rename(QString::fromStdString(tmpFilePath), QString::fromStdString(saveFilePath))) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Atomic rename failed.";
        return result;
    }

    SaveParseResult finalParse = m_parser.parseSaveFile(saveFilePath);
    if (finalParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Final persisted save file failed verification.";
        return result;
    }

    result.status = EditorStatus::Success;
    result.newSaveHash = calculateSha256(modifiedBuffer);
    return result;
}

MutationResult Gen3SaveEditor::mutateEV(
    const std::string& saveFilePath,
    const Pocket::Companion::CompanionLink& targetLink,
    EVType evStat,
    int requestedEvAmount
) {
    MutationResult result;

    if (!FileStabilityVerifier::isFileStable(saveFilePath)) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Save file is currently being modified by another process.";
        return result;
    }

    if (!m_coordinator->acquireMutationLock(saveFilePath)) {
        result.status = EditorStatus::SaveLockedByEmulator;
        result.errorMessage = "Save file is locked by an active emulator or external process.";
        return result;
    }

    struct LockGuard {
        std::shared_ptr<SaveSessionCoordinator> coord;
        std::string path;
        ~LockGuard() { coord->releaseMutationLock(path); }
    } guard{m_coordinator, saveFilePath};

    SaveParseResult origParse = m_parser.parseSaveFile(saveFilePath);
    if (origParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Failed to parse original save file: " + origParse.errorMessage;
        return result;
    }

    std::ifstream origFile(saveFilePath, std::ios::binary);
    std::vector<uint8_t> originalBuffer(131072);
    origFile.read(reinterpret_cast<char*>(originalBuffer.data()), 131072);
    origFile.close();

    std::string origHash = calculateSha256(originalBuffer);

    Pocket::Companion::CompanionLink reidentifiedLink = CompanionReidentifier::reidentify(targetLink, origParse, origHash);
    if (reidentifiedLink.status != Pocket::Companion::LinkStatus::Linked) {
        result.status = (reidentifiedLink.status == Pocket::Companion::LinkStatus::AmbiguousMatch)
            ? EditorStatus::CreatureAmbiguous : EditorStatus::CreatureNotFound;
        result.errorMessage = "Target creature could not be uniquely re-identified in save file.";
        return result;
    }

    SaveBackup backup = m_backupRepo->createBackup(std::to_string(targetLink.gameId), saveFilePath, "Pre-EV-mutation backup");
    if (backup.path.empty()) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Failed to create pre-mutation backup.";
        return result;
    }
    result.audit.backupFilePath = backup.path;

    std::vector<uint8_t> modifiedBuffer = originalBuffer;

    int activeSlotIdx = origParse.activeSlotIndex;
    uint8_t* slotPtr = modifiedBuffer.data() + (activeSlotIdx == 0 ? 0x00000 : 0x0E000);

    uint8_t* pkmnPtr = nullptr;
    int targetSectionIdx = 0;

    if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Party) {
        targetSectionIdx = 1;
        uint8_t* sec1Ptr = slotPtr + (1 * 4096);
        int slotIdx = reidentifiedLink.locator.partySlot - 1;
        pkmnPtr = sec1Ptr + 0x238 + (slotIdx * 100);
    } else if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Box) {
        int boxIdx = reidentifiedLink.locator.boxNumber - 1;
        int slotIdx = reidentifiedLink.locator.boxSlot - 1;
        int globalSlot = (boxIdx * 30) + slotIdx;

        targetSectionIdx = 5 + (globalSlot / 60);
        uint8_t* secPtr = slotPtr + (targetSectionIdx * 4096);
        int offsetInSec = (globalSlot % 60) * 80 + 0x04;
        pkmnPtr = secPtr + offsetInSec;
    }

    if (!pkmnPtr) {
        result.status = EditorStatus::CreatureNotFound;
        result.errorMessage = "Could not map creature memory address.";
        return result;
    }

    uint32_t pid = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x00);
    uint32_t otId = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x04);
    uint32_t key = pid ^ otId;

    int orderIdx = static_cast<int>(pid % 24);
    const int* order = kSubstructureOrders[orderIdx];

    // Block 2 = EVs/Condition
    int blockEPos = 0;
    for (int p = 0; p < 4; ++p) {
        if (order[p] == 2) {
            blockEPos = p;
            break;
        }
    }

    uint32_t* dwords = reinterpret_cast<uint32_t*>(pkmnPtr + 0x20);
    uint32_t dword0 = dwords[(blockEPos * 3) + 0] ^ key;
    uint32_t dword1 = dwords[(blockEPos * 3) + 1] ^ key;

    uint8_t evs[6];
    evs[0] = static_cast<uint8_t>((dword0 >> 0) & 0xFF);  // HP EV
    evs[1] = static_cast<uint8_t>((dword0 >> 8) & 0xFF);  // Attack EV
    evs[2] = static_cast<uint8_t>((dword0 >> 16) & 0xFF); // Defense EV
    evs[3] = static_cast<uint8_t>((dword0 >> 24) & 0xFF); // Speed EV
    evs[4] = static_cast<uint8_t>((dword1 >> 0) & 0xFF);  // Special Attack EV
    evs[5] = static_cast<uint8_t>((dword1 >> 8) & 0xFF);  // Special Defense EV

    int targetIdx = 0;
    switch (evStat) {
        case EVType::HP:             targetIdx = 0; break;
        case EVType::Attack:         targetIdx = 1; break;
        case EVType::Defense:        targetIdx = 2; break;
        case EVType::Speed:          targetIdx = 3; break;
        case EVType::SpecialAttack:  targetIdx = 4; break;
        case EVType::SpecialDefense: targetIdx = 5; break;
    }

    int currentStatEv = evs[targetIdx];
    int currentTotalEvs = 0;
    for (int i = 0; i < 6; ++i) {
        currentTotalEvs += evs[i];
    }

    int statAllowance = std::max(0, 252 - currentStatEv);
    int totalAllowance = std::max(0, 510 - currentTotalEvs);
    int actualGain = std::min({requestedEvAmount, statAllowance, totalAllowance});

    if (actualGain <= 0) {
        result.status = EditorStatus::CapExceededNoGain;
        result.errorMessage = "EV mutation rejected: Target stat or total EV cap (510) reached.";
        result.audit.requestedEvAmount = requestedEvAmount;
        result.audit.appliedEvAmount = 0;
        result.audit.remainingEvAmount = requestedEvAmount;
        return result;
    }

    uint8_t newEvValue = static_cast<uint8_t>(currentStatEv + actualGain);
    evs[targetIdx] = newEvValue;

    // Reconstruct dword0 and dword1
    dword0 = (static_cast<uint32_t>(evs[0]) << 0)  |
             (static_cast<uint32_t>(evs[1]) << 8)  |
             (static_cast<uint32_t>(evs[2]) << 16) |
             (static_cast<uint32_t>(evs[3]) << 24);

    dword1 = (dword1 & 0xFFFF0000) |
             (static_cast<uint32_t>(evs[4]) << 0) |
             (static_cast<uint32_t>(evs[5]) << 8);

    dwords[(blockEPos * 3) + 0] = dword0 ^ key;
    dwords[(blockEPos * 3) + 1] = dword1 ^ key;

    // Recompute section checksum
    uint8_t* secPtr = slotPtr + (targetSectionIdx * 4096);
    uint16_t newChecksum = Gen3SaveParser::calculateSectionChecksum(secPtr);
    *reinterpret_cast<uint16_t*>(secPtr + 0xFF6) = newChecksum;

    SaveParseResult modParse = m_parser.parseSaveBuffer(modifiedBuffer);
    if (modParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Modified EV buffer failed section checksum verification.";
        return result;
    }

    size_t bytesModified = 0;
    for (size_t i = 0; i < 131072; ++i) {
        if (originalBuffer[i] != modifiedBuffer[i]) {
            bytesModified++;
        }
    }

    // Changing 1 byte alters 1 raw byte + 1-2 checksum bytes = 2 or 3 bytes
    if (bytesModified < 2 || bytesModified > 3) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Semantic diff failed: Expected 2 or 3 modified bytes, but found " + std::to_string(bytesModified);
        return result;
    }

    result.audit.creatureNickname = reidentifiedLink.nickname;
    result.audit.speciesName = reidentifiedLink.speciesName;
    result.audit.evStat = evStat;
    result.audit.oldEvValue = static_cast<uint8_t>(currentStatEv);
    result.audit.newEvValue = newEvValue;
    result.audit.requestedEvAmount = requestedEvAmount;
    result.audit.appliedEvAmount = actualGain;
    result.audit.remainingEvAmount = requestedEvAmount - actualGain;
    result.audit.bytesModified = bytesModified;
    result.audit.unrelatedFieldsChanged = 0;
    result.audit.isVerified = true;

    // Atomic File Write
    std::string tmpFilePath = saveFilePath + ".tmp";
    std::ofstream tmpFile(tmpFilePath, std::ios::binary);
    if (!tmpFile.is_open()) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Could not open temporary file for atomic EV write.";
        return result;
    }
    tmpFile.write(reinterpret_cast<const char*>(modifiedBuffer.data()), 131072);
    tmpFile.close();

    QFile::remove(QString::fromStdString(saveFilePath));
    if (!QFile::rename(QString::fromStdString(tmpFilePath), QString::fromStdString(saveFilePath))) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Atomic rename failed during EV mutation.";
        return result;
    }

    SaveParseResult finalParse = m_parser.parseSaveFile(saveFilePath);
    if (finalParse.status != SaveParseStatus::Success) {
        result.status = EditorStatus::SemanticDiffFailed;
        result.errorMessage = "Final persisted EV save file failed verification.";
        return result;
    }

    result.status = EditorStatus::Success;
    result.newSaveHash = calculateSha256(modifiedBuffer);
    return result;
}

} // namespace Pocket::Save
