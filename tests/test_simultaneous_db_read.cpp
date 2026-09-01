#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include "pocket/storage/GameRepository.hpp"

class TestSimultaneousDbRead : public QObject {
    Q_OBJECT
private slots:
    void testConcurrentReadAccess() {
        QTemporaryFile tempDb;
        QVERIFY(tempDb.open());
        tempDb.close();

        // Process 1 connection (PocketPartner.exe)
        PocketPartner::Storage::DatabaseConfig config1;
        config1.dbPath = tempDb.fileName().toStdString();
        auto dbManager1 = std::make_shared<PocketPartner::Storage::DatabaseManager>(config1);
        QVERIFY(dbManager1->initialize());

        QSqlDatabase db1 = QSqlDatabase::database(QSqlDatabase::defaultConnection);
        QVERIFY(Pocket::Storage::SchemaMigration::runMigrations(db1));

        Pocket::Storage::GameRepository repo1(dbManager1);

        // Process 2 connection (PocketCompanion.exe)
        QSqlDatabase db2 = QSqlDatabase::addDatabase("QSQLITE", "CompanionConn");
        db2.setDatabaseName(tempDb.fileName());
        QVERIFY(db2.open());

        // Simultaneous queries from both process connections
        auto games1 = repo1.getAllGames();

        QSqlQuery query2(db2);
        QVERIFY(query2.exec("SELECT count(*) FROM games"));
        QVERIFY(query2.next());

        QCOMPARE(games1.size(), 0u);
        QCOMPARE(query2.value(0).toInt(), 0);

        db2.close();
        QSqlDatabase::removeDatabase("CompanionConn");
    }
};

QTEST_MAIN(TestSimultaneousDbRead)
#include "test_simultaneous_db_read.moc"
