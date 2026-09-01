#include <QtTest/QtTest>
#include <QTemporaryFile>
#include "pocket/storage/GameRepository.hpp"

class TestGameIdentity : public QObject {
    Q_OBJECT
private slots:
    void testSha256FingerprintCalculation() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("POCKET_PARTNER_ROM_TEST_DATA");
        tempFile.flush();

        std::string hash1 = Pocket::Storage::GameRepository::calculateSha256(tempFile.fileName().toStdString());
        std::string hash2 = Pocket::Storage::GameRepository::calculateSha256(tempFile.fileName().toStdString());

        QVERIFY(!hash1.empty());
        QCOMPARE(hash1, hash2); // Deterministic SHA-256
        QCOMPARE(hash1.length(), 64u); // 64 hex characters
    }
};

QTEST_MAIN(TestGameIdentity)
#include "test_game_identity.moc"
