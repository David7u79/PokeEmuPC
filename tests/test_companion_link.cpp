#include <QtTest/QtTest>
#include "pocketpartner/core/CompanionLink.hpp"

class TestCompanionLink : public QObject {
    Q_OBJECT
private slots:
    void testIdentityHashUniqueness() {
        PocketPartner::Core::CompanionLink link1(
            PocketPartner::Core::GameGeneration::Gen3_GBA,
            0x12345678, 10001, 20002, 25, "PIKA", "ASH", 0xABCDEF
        );

        PocketPartner::Core::CompanionLink link2(
            PocketPartner::Core::GameGeneration::Gen3_GBA,
            0x87654321, 10001, 20002, 25, "PIKA", "ASH", 0xABCDEF
        );

        QVERIFY(link1.isValid());
        QVERIFY(link2.isValid());
        QVERIFY(link1.identityHash() != link2.identityHash());
        QVERIFY(!link1.matches(link2));
    }

    void testIdentityMatchSuccess() {
        PocketPartner::Core::CompanionLink link1(
            PocketPartner::Core::GameGeneration::Gen3_GBA,
            0x12345678, 10001, 20002, 25, "PIKA", "ASH", 0xABCDEF
        );

        PocketPartner::Core::CompanionLink link2(
            PocketPartner::Core::GameGeneration::Gen3_GBA,
            0x12345678, 10001, 20002, 25, "PIKA", "ASH", 0x999999
        );

        QVERIFY(link1.matches(link2));
    }
};

QTEST_MAIN(TestCompanionLink)
#include "test_companion_link.moc"
