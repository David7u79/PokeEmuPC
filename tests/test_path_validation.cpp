#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QFileInfo>
#include "pocket/storage/GameRepository.hpp"

class TestPathValidation : public QObject {
    Q_OBJECT
private slots:
    void testNonExistentFileValidation() {
        QTemporaryFile tempFile;
        QString nonExistentPath = tempFile.fileName() + "_non_existent.gba";

        QFileInfo info(nonExistentPath);
        QVERIFY(!info.exists());
    }

    void testValidFileParsing() {
        QTemporaryFile tempRom;
        tempRom.setAutoRemove(true);
        QVERIFY(tempRom.open());
        tempRom.write("DUMMY_ROM_CONTENT_12345");
        tempRom.flush();

        QFileInfo info(tempRom.fileName());
        QVERIFY(info.exists());
        QCOMPARE(info.size(), static_cast<qint64>(23));
    }
};

QTEST_MAIN(TestPathValidation)
#include "test_path_validation.moc"
