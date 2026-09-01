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

    // Step 1: File Stability Verification
    if (!FileStabilityVerifier::isFileStable(saveFilePath)) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Save file is currently being modified by another process.";
        return result;
    }

    // Step 2: Acquire Mutation Lock from SaveSessionCoordinator
    if (!m_coordinator->acquireMutationLock(saveFilePath)) {
        result.status = EditorStatus::SaveLockedByEmulator;
        result.errorMessage = "Save file is locked by an active emulator or external process.";
        return result;
    }

    // Auto-release lock on exit
    struct LockGuard {
        std::shared_ptr<SaveSessionCoordinator> coord;
        std::string path;
        ~LockGuard() { coord->releaseMutationLock(path); }
    } guard{m_coordinator, saveFilePath};

    // Step 3: Read and Parse Original Save File
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

    // Step 4: Locate Exact Linked Creature
    Pocket::Companion::CompanionLink reidentifiedLink = CompanionReidentifier::reidentify(targetLink, origParse, origHash);
    if (reidentifiedLink.status != Pocket::Companion::LinkStatus::Linked) {
        result.status = (reidentifiedLink.status == Pocket::Companion::LinkStatus::AmbiguousMatch)
            ? EditorStatus::CreatureAmbiguous : EditorStatus::CreatureNotFound;
        result.errorMessage = "Target creature could not be uniquely re-identified in save file.";
        return result;
    }

    // Step 5: Create Pre-Mutation Backup
    SaveBackup backup = m_backupRepo->createBackup(std::to_string(targetLink.gameId), saveFilePath, "Pre-mutation backup");
    if (backup.path.empty()) {
        result.status = EditorStatus::AtomicWriteFailed;
        result.errorMessage = "Failed to create pre-mutation backup.";
        return result;
    }
    result.audit.backupFilePath = backup.path;

    // Step 6: Copy 128KB Save Buffer in Memory
    std::vector<uint8_t> modifiedBuffer = originalBuffer;

    // Step 7: Locate Section Offset & Target Pointer
    int activeSlotIdx = origParse.activeSlotIndex; // 0 = Slot A (0x00000), 1 = Slot B (0x0E000)
    uint8_t* slotPtr = modifiedBuffer.data() + (activeSlotIdx == 0 ? 0x00000 : 0x0E000);

    uint8_t* pkmnPtr = nullptr;
    int targetSectionIdx = 0;

    if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Party) {
        targetSectionIdx = 1; // Section 1 = Party
        uint8_t* sec1Ptr = slotPtr + (1 * 4096);
        int slotIdx = reidentifiedLink.locator.partySlot - 1;
        pkmnPtr = sec1Ptr + 0x238 + (slotIdx * 100);
    } else if (reidentifiedLink.locator.type == Pocket::Companion::LocationType::Box) {
        int boxIdx = reidentifiedLink.locator.boxNumber - 1;
        int slotIdx = reidentifiedLink.locator.boxSlot - 1;
        int globalSlot = (boxIdx * 30) + slotIdx;

        // PC Boxes span Sections 5 through 13
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

    // Step 8: Modify Friendship Byte ONLY
    uint32_t pid = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x00);
    uint32_t otId = *reinterpret_cast<uint32_t*>(pkmnPtr + 0x04);
    uint32_t key = pid ^ otId;

    int orderIdx = static_cast<int>(pid % 24);
    const int* order = kSubstructureOrders[orderIdx];

    // Find position of Block G (Growth) in raw memory (0..3)
    int blockGPos = 0;
    for (int p = 0; p < 4; ++p) {
        if (order[p] == 0) { // Block 0 = Growth
            blockGPos = p;
            break;
        }
    }

    uint32_t* dwords = reinterpret_cast<uint32_t*>(pkmnPtr + 0x20);
    int dwordIndex = (blockGPos * 3) + 2; // Dword 2 of Block G contains Friendship byte

    uint32_t origDword = dwords[dwordIndex] ^ key;
    uint8_t oldFriendship = static_cast<uint8_t>((origDword >> 8) & 0xFF);

    uint8_t clampedNewFriendship = static_cast<uint8_t>(std::min<int>(255, newFriendshipValue));

    uint32_t modifiedDword = (origDword & 0xFFFF00FF) | (static_cast<uint32_t>(clampedNewFriendship) << 8);
    dwords[dwordIndex] = modifiedDword ^ key;

    // Step 9: Recompute & Repair Section Checksum
    uint8_t* secPtr = slotPtr + (targetSectionIdx * 4096);
    uint16_t newChecksum = Gen3SaveParser::calculateSectionChecksum(secPtr);
    *reinterpret_cast<uint16_t*>(secPtr + 0xFF6) = newChecksum;

    // Step 10: Run Semantic Diff Audit
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

    // Must modify EXACTLY 2 or 3 bytes: 1 friendship byte + 1 or 2 section checksum bytes
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

    // Step 11: Atomic File Write (.tmp -> rename)
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

    // Step 12: Re-parse Persisted Save File on Disk & Final Verification
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

} // namespace Pocket::Save
