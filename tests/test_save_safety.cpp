#include <QtTest/QtTest>
#include "pocketpartner/save/SaveMutationPipeline.hpp"
#include "pocketpartner/save/SaveFileParser.hpp"
#include <memory>
#include <QTemporaryFile>
#include <QDir>

class TestSaveSafety : public QObject {
    Q_OBJECT
private slots:
    void testInvalidSaveSizeRejection() {
        auto parser = std::make_shared<PocketPartner::Save::Gen3SaveParser>();
        PocketPartner::Save::SaveMutationPipeline pipeline(parser);

        QTemporaryFile tempSave;
        QVERIFY(tempSave.open());
        tempSave.write("SHORT_INVALID_DATA");
        tempSave.close();

        PocketPartner::Core::CompanionLink targetLink(
            PocketPartner::Core::GameGeneration::Gen3_GBA,
            0x11223344, 1234, 5678, 1, "BULBA", "RED", 0
        );

        PocketPartner::Save::MutationRequest req;
        req.targetLink = targetLink;
        req.type = PocketPartner::Save::MutationType::IncreaseFriendship;
        req.parameterValue = 10;

        auto result = pipeline.executeMutation(tempSave.fileName().toStdString(), req, false);

        QVERIFY(!result.success);
        QCOMPARE(result.stepFailed, 3u); // Step 3 Format validation fails
    }

    void testEmulatorRunningRejection() {
        auto parser = std::make_shared<PocketPartner::Save::Gen3SaveParser>();
        PocketPartner::Save::SaveMutationPipeline pipeline(parser);

        QTemporaryFile tempSave;
        QVERIFY(tempSave.open());
        std::vector<char> dummy(131072, 0x00);
        tempSave.write(dummy.data(), dummy.size());
        tempSave.close();

        PocketPartner::Core::CompanionLink targetLink;
        PocketPartner::Save::MutationRequest req;
        req.targetLink = targetLink;
        req.type = PocketPartner::Save::MutationType::IncreaseFriendship;

        // Pass isEmulatorRunning = true
        auto result = pipeline.executeMutation(tempSave.fileName().toStdString(), req, true);

        QVERIFY(!result.success);
        QCOMPARE(result.stepFailed, 1u); // Step 1 ABORT when emulator is running
    }
};

QTEST_MAIN(TestSaveSafety)
#include "test_save_safety.moc"
