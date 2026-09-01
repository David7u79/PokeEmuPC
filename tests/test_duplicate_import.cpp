#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/GameRepository.hpp"
#include "pocket/storage/SchemaMigration.hpp"

class TestDuplicateImport : public QObject {
    Q_OBJECT
private slots:
    void testDuplicateImportDetection() {
        QTemporaryFile tempDb;
        QVERIFY(tempDb.open());
        tempDb.close();

        PocketPartner::Storage::DatabaseConfig config;
        config.dbPath = tempDb.fileName().toStdString();
        auto dbManager = std::make_shared<PocketPartner::Storage::DatabaseManager>(config);
        QVERIFY(dbManager->initialize());

        QSqlDatabase db = QSqlDatabase::database();
        QVERIFY(Pocket::Storage::SchemaMigration::runMigrations(db));

        Pocket::Storage::GameRepository repo(dbManager);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString rom1Path = tempDir.filePath("test_game1.gba");
        QFile rom1(rom1Path);
        QVERIFY(rom1.open(QIODevice::WriteOnly));
        rom1.write("UNIQUE_ROM_CONTENT_GBA_123");
        rom1.close();

        // Initial import -> Success
        auto res1 = repo.importGame(rom1Path.toStdString());
        QCOMPARE(res1.status, Pocket::Storage::ImportResultStatus::Success);

        // Duplicate path import -> Rejection
        auto res2 = repo.importGame(rom1Path.toStdString());
        QCOMPARE(res2.status, Pocket::Storage::ImportResultStatus::DuplicatePath);

        // Duplicate content (different path, identical SHA-256) -> Rejection
        QString rom2Path = tempDir.filePath("test_game1_copy.gba");
        QFile rom2(rom2Path);
        QVERIFY(rom2.open(QIODevice::WriteOnly));
        rom2.write("UNIQUE_ROM_CONTENT_GBA_123");
        rom2.close();

        auto res3 = repo.importGame(rom2Path.toStdString());
        QCOMPARE(res3.status, Pocket::Storage::ImportResultStatus::DuplicateHash);
    }
};

QTEST_MAIN(TestDuplicateImport)
#include "test_duplicate_import.moc"
