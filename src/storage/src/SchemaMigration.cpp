#include "pocket/storage/SchemaMigration.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace Pocket::Storage {

int SchemaMigration::getCurrentVersion(QSqlDatabase& db) {
    QSqlQuery query(db);
    if (query.exec("PRAGMA user_version") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool SchemaMigration::runMigrations(QSqlDatabase& db) {
    int version = getCurrentVersion(db);
    if (version >= CURRENT_SCHEMA_VERSION) {
        return true;
    }

    if (version < 1) {
        if (!applyMigrationV1(db)) {
            return false;
        }
    }

    // Set updated user_version
    QSqlQuery query(db);
    QString updatePragma = QString("PRAGMA user_version = %1").arg(CURRENT_SCHEMA_VERSION);
    if (!query.exec(updatePragma)) {
        qWarning() << "Failed to update user_version:" << query.lastError().text();
        return false;
    }

    return true;
}

bool SchemaMigration::applyMigrationV1(QSqlDatabase& db) {
    QSqlQuery query(db);

    const char* v1Schema = R"(
        CREATE TABLE IF NOT EXISTS games (
            id TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            system TEXT NOT NULL,
            rom_path TEXT NOT NULL UNIQUE,
            sha256 TEXT NOT NULL,
            file_size_bytes INTEGER NOT NULL,
            source TEXT NOT NULL DEFAULT 'INTERNAL_EMULATOR',
            imported_at_ts INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS kv_store (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            updated_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS companion_app_state (
            companion_id TEXT PRIMARY KEY,
            hunger REAL NOT NULL DEFAULT 100.0,
            mood REAL NOT NULL DEFAULT 100.0,
            fatigue REAL NOT NULL DEFAULT 0.0,
            cleanliness REAL NOT NULL DEFAULT 100.0,
            bond_level INTEGER NOT NULL DEFAULT 1,
            streak INTEGER NOT NULL DEFAULT 0,
            last_interaction_ts INTEGER NOT NULL,
            companion_xp INTEGER NOT NULL DEFAULT 0,
            cosmetic_state TEXT,
            animation_state TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_games_sha256 ON games(sha256);
        CREATE INDEX IF NOT EXISTS idx_games_system ON games(system);
    )";

    QStringList statements = QString(v1Schema).split(';', Qt::SkipEmptyParts);
    for (const QString& stmt : statements) {
        QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty()) continue;
        if (!query.exec(trimmed)) {
            qWarning() << "Migration V1 SQL error:" << query.lastError().text() << "Query:" << trimmed;
            return false;
        }
    }

    return true;
}

} // namespace Pocket::Storage
