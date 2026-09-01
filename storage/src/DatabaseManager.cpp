#include "pocketpartner/storage/DatabaseManager.hpp"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QUuid>

namespace PocketPartner::Storage {

DatabaseManager::DatabaseManager(DatabaseConfig config)
    : m_config(std::move(config)) {}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::initialize() {
    if (m_initialized) {
        return true;
    }

    // Generate unique connection name to avoid collisions
    QString connName = QString("PocketPartner_DB_%1").arg(QUuid::createUuid().toString());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(QString::fromStdString(m_config.dbPath));

    if (!db.open()) {
        qWarning() << "Failed to open SQLite database:" << db.lastError().text();
        return false;
    }

    m_initialized = true;
    return executeSchemaMigrations();
}

bool DatabaseManager::executeSchemaMigrations() {
    if (!m_initialized) return false;

    // Base tables for App-Only Companion State and KV settings
    const char* schemaSql = R"(
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

        CREATE TABLE IF NOT EXISTS companion_history_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            companion_id TEXT NOT NULL,
            activity_type TEXT NOT NULL,
            delta_value REAL NOT NULL,
            canonical_sync_applied INTEGER NOT NULL DEFAULT 0,
            timestamp INTEGER NOT NULL
        );
    )";

    return execute(schemaSql);
}

bool DatabaseManager::close() {
    if (!m_initialized) return true;
    m_initialized = false;
    return true;
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!m_initialized) return false;

    // Use current default connection or temporary execution
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    
    // Split SQL by semicolon for multi-statement execution
    QStringList statements = QString::fromStdString(sql).split(';', Qt::SkipEmptyParts);
    for (const QString& stmt : statements) {
        QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty()) continue;
        if (!query.exec(trimmed)) {
            qWarning() << "SQL Execution error:" << query.lastError().text() << "Query:" << trimmed;
            return false;
        }
    }
    return true;
}

bool DatabaseManager::setKV(const std::string& key, const std::string& value) {
    if (!m_initialized) return false;

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO kv_store (key, value, updated_at)
        VALUES (:key, :value, strftime('%s', 'now'))
        ON CONFLICT(key) DO UPDATE SET
            value = excluded.value,
            updated_at = excluded.updated_at
    )");
    query.bindValue(":key", QString::fromStdString(key));
    query.bindValue(":value", QString::fromStdString(value));

    if (!query.exec()) {
        qWarning() << "Failed to set KV:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<std::string> DatabaseManager::getKV(const std::string& key) {
    if (!m_initialized) return std::nullopt;

    QSqlQuery query;
    query.prepare("SELECT value FROM kv_store WHERE key = :key");
    query.bindValue(":key", QString::fromStdString(key));

    if (query.exec() && query.next()) {
        return query.value(0).toString().toStdString();
    }
    return std::nullopt;
}

} // namespace PocketPartner::Storage
