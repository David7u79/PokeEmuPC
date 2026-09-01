#pragma once

#include <QSqlDatabase>
#include <string>

namespace Pocket::Storage {

class SchemaMigration {
public:
    static constexpr int CURRENT_SCHEMA_VERSION = 1;

    static bool runMigrations(QSqlDatabase& db);
    static int getCurrentVersion(QSqlDatabase& db);

private:
    static bool applyMigrationV1(QSqlDatabase& db);
};

} // namespace Pocket::Storage
