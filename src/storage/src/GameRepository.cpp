#include "pocket/storage/GameRepository.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include "pocket/core/RomFingerprint.hpp"
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

namespace Pocket::Storage {

GameRepository::GameRepository(std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager)
    : m_db(std::move(dbManager)) {}

std::string GameRepository::calculateSha256(const std::string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    constexpr qint64 chunkSize = 65536; // 64KB chunk size
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        hasher.addData(chunk);
    }

    return hasher.result().toHex().toStdString();
}

ImportResult GameRepository::importGame(const std::string& romFilePath) {
    ImportResult result;

    QFileInfo fileInfo(QString::fromStdString(romFilePath));
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        result.status = ImportResultStatus::FileNotFound;
        result.errorMessage = "ROM file does not exist or is not a valid file.";
        return result;
    }

    auto systemOpt = Core::GameSystemUtils::detectFromExtension(romFilePath);
    if (!systemOpt.has_value() || systemOpt.value() == Core::GameSystem::Unknown) {
        result.status = ImportResultStatus::InvalidSystem;
        result.errorMessage = "Unsupported file extension (expected .gb, .gbc, .gba, .nds).";
        return result;
    }

    const std::string canonicalPath = fileInfo.canonicalFilePath().toStdString();

    // Check duplicate by path in database
    if (isPathAlreadyImported(canonicalPath)) {
        result.status = ImportResultStatus::DuplicatePath;
        result.errorMessage = "Game path has already been imported into the library.";
        return result;
    }

    // Calculate RomFingerprint ONCE during import
    Core::RomFingerprint fp = Core::RomFingerprint::calculate(canonicalPath);
    return importGame(canonicalPath, fp);
}

ImportResult GameRepository::importGame(const std::string& romFilePath, const Core::RomFingerprint& precomputed) {
    ImportResult result;

    QFileInfo fileInfo(QString::fromStdString(romFilePath));
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        result.status = ImportResultStatus::FileNotFound;
        result.errorMessage = "ROM file does not exist or is not a valid file.";
        return result;
    }

    auto systemOpt = Core::GameSystemUtils::detectFromExtension(romFilePath);
    if (!systemOpt.has_value() || systemOpt.value() == Core::GameSystem::Unknown) {
        result.status = ImportResultStatus::InvalidSystem;
        result.errorMessage = "Unsupported file extension (expected .gb, .gbc, .gba, .nds).";
        return result;
    }

    const std::string canonicalPath = fileInfo.canonicalFilePath().toStdString();
    if (isPathAlreadyImported(canonicalPath)) {
        result.status = ImportResultStatus::DuplicatePath;
        result.errorMessage = "Game path has already been imported into the library.";
        return result;
    }

    const Core::RomFingerprint& fp = precomputed;
    const std::string& sha256 = fp.sha256;

    if (sha256.empty()) {
        result.status = ImportResultStatus::FileNotFound;
        result.errorMessage = "Unable to compute SHA-256 fingerprint for ROM file.";
        return result;
    }

    m_hashCache[canonicalPath] = sha256;

    // Check duplicate by SHA256 in database
    QSqlQuery query;
    query.prepare("SELECT id FROM games WHERE sha256 = :sha256");
    query.bindValue(":sha256", QString::fromStdString(sha256));
    if (query.exec() && query.next()) {
        result.status = ImportResultStatus::DuplicateHash;
        result.errorMessage = "A game with identical SHA-256 fingerprint already exists in library.";
        return result;
    }

    // Resolve Canonical Metadata via 4-tier GameMetadataResolver
    GameMetadata meta = m_metadataResolver.resolve(canonicalPath, fp);

    // Create Game entity
    Core::Game game;
    game.id = Core::GameId::generate();
    game.title = meta.canonicalTitle.empty() ? fileInfo.completeBaseName().toStdString() : meta.canonicalTitle;
    game.system = systemOpt.value();
    game.romPath = canonicalPath;
    game.sha256 = sha256;
    game.fileSizeBytes = static_cast<uint64_t>(fileInfo.size());
    game.source = Core::GameSource::INTERNAL_EMULATOR;
    game.importedAtTs = QDateTime::currentSecsSinceEpoch();

    // Insert into database
    query.prepare(R"(
        INSERT INTO games (id, title, system, rom_path, sha256, file_size_bytes, source, imported_at_ts)
        VALUES (:id, :title, :system, :rom_path, :sha256, :file_size_bytes, :source, :imported_at_ts)
    )");
    query.bindValue(":id", QString::fromStdString(game.id.toString()));
    query.bindValue(":title", QString::fromStdString(game.title));
    query.bindValue(":system", QString::fromStdString(Core::GameSystemUtils::toString(game.system)));
    query.bindValue(":rom_path", QString::fromStdString(game.romPath));
    query.bindValue(":sha256", QString::fromStdString(game.sha256));
    query.bindValue(":file_size_bytes", static_cast<qulonglong>(game.fileSizeBytes));
    query.bindValue(":source", QString::fromStdString(Core::GameSourceUtils::toString(game.source)));
    query.bindValue(":imported_at_ts", static_cast<qlonglong>(game.importedAtTs));

    if (!query.exec()) {
        result.status = ImportResultStatus::DatabaseError;
        result.errorMessage = "Failed to insert game record into database: " + query.lastError().text().toStdString();
        return result;
    }

    result.status = ImportResultStatus::Success;
    result.game = game;
    return result;
}

bool GameRepository::isPathAlreadyImported(const std::string& romFilePath) const {
    QFileInfo fileInfo(QString::fromStdString(romFilePath));
    const QString canonicalPath = fileInfo.canonicalFilePath();
    if (canonicalPath.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare("SELECT id FROM games WHERE rom_path = :path");
    query.bindValue(":path", canonicalPath);
    return query.exec() && query.next();
}

std::vector<Core::Game> GameRepository::getAllGames() const {
    std::vector<Core::Game> games;
    QSqlQuery query("SELECT id, title, system, rom_path, sha256, file_size_bytes, source, imported_at_ts FROM games ORDER BY title ASC");

    while (query.next()) {
        Core::Game g;
        g.id = Core::GameId(query.value(0).toString().toStdString());
        g.title = query.value(1).toString().toStdString();

        g.system = Core::GameSystemUtils::fromString(query.value(2).toString().toStdString());

        g.romPath = query.value(3).toString().toStdString();
        g.sha256 = query.value(4).toString().toStdString();
        g.fileSizeBytes = query.value(5).toULongLong();

        g.source = Core::GameSourceUtils::fromString(query.value(6).toString().toStdString());

        g.importedAtTs = query.value(7).toLongLong();
        games.push_back(g);
    }

    return games;
}

std::optional<Core::Game> GameRepository::getGameById(const Core::GameId& id) const {
    QSqlQuery query;
    query.prepare("SELECT id, title, system, rom_path, sha256, file_size_bytes, source, imported_at_ts FROM games WHERE id = :id");
    query.bindValue(":id", QString::fromStdString(id.toString()));

    if (query.exec() && query.next()) {
        Core::Game g;
        g.id = Core::GameId(query.value(0).toString().toStdString());
        g.title = query.value(1).toString().toStdString();

        g.system = Core::GameSystemUtils::fromString(query.value(2).toString().toStdString());

        g.romPath = query.value(3).toString().toStdString();
        g.sha256 = query.value(4).toString().toStdString();
        g.fileSizeBytes = query.value(5).toULongLong();

        g.source = Core::GameSourceUtils::fromString(query.value(6).toString().toStdString());

        g.importedAtTs = query.value(7).toLongLong();
        return g;
    }
    return std::nullopt;
}

std::optional<Core::Game> GameRepository::getGameBySha256(const std::string& sha256) const {
    QSqlQuery query;
    query.prepare("SELECT id, title, system, rom_path, sha256, file_size_bytes, source, imported_at_ts FROM games WHERE sha256 = :sha256");
    query.bindValue(":sha256", QString::fromStdString(sha256));

    if (query.exec() && query.next()) {
        Core::Game g;
        g.id = Core::GameId(query.value(0).toString().toStdString());
        g.title = query.value(1).toString().toStdString();

        g.system = Core::GameSystemUtils::fromString(query.value(2).toString().toStdString());

        g.romPath = query.value(3).toString().toStdString();
        g.sha256 = query.value(4).toString().toStdString();
        g.fileSizeBytes = query.value(5).toULongLong();

        g.source = Core::GameSourceUtils::fromString(query.value(6).toString().toStdString());

        g.importedAtTs = query.value(7).toLongLong();
        return g;
    }
    return std::nullopt;
}

bool GameRepository::deleteGame(const Core::GameId& id) {
    QSqlQuery query;
    query.prepare("DELETE FROM games WHERE id = :id");
    query.bindValue(":id", QString::fromStdString(id.toString()));
    return query.exec() && query.numRowsAffected() > 0;
}

} // namespace Pocket::Storage
