#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"

class TestDatabaseMigration : public QObject {
    Q_OBJECT
private slots:
    void testVersionedSchemaMigration() {
        QTemporaryFile tempDb;
        QVERIFY(tempDb.open());
        tempDb.close();

        PocketPartner::Storage::DatabaseConfig config;
        config.dbPath = tempDb.fileName().toStdString();
        PocketPartner::Storage::DatabaseManager manager(config);

        QVERIFY(manager.initialize());

        QSqlDatabase db = QSqlDatabase::database();
        int version = Pocket::Storage::SchemaMigration::getCurrentVersion(db);

        QCOMPARE(version, Pocket::Storage::SchemaMigration::CURRENT_SCHEMA_VERSION);

        // Verify games table exists
        QSqlQuery query(db);
        QVERIFY(query.exec("SELECT count(*) FROM games"));
        QVERIFY(query.next());
    }
};

QTEST_MAIN(TestDatabaseMigration)
#include "test_database_migration.moc"
