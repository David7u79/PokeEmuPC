#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

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

    QSqlDatabase db;
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", QSqlDatabase::defaultConnection);
    }

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
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    return Pocket::Storage::SchemaMigration::runMigrations(db);
}

bool DatabaseManager::close() {
    if (!m_initialized) return true;
    m_initialized = false;
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::database(QSqlDatabase::defaultConnection).close();
    }
    return true;
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!m_initialized) return false;

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    QSqlQuery query(db);
    
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

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    QSqlQuery query(db);
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

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    QSqlQuery query(db);
    query.prepare("SELECT value FROM kv_store WHERE key = :key");
    query.bindValue(":key", QString::fromStdString(key));

    if (query.exec() && query.next()) {
        return query.value(0).toString().toStdString();
    }
    return std::nullopt;
}

} // namespace PocketPartner::Storage
