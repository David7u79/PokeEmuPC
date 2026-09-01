#include <QtTest/QtTest>
#include "pocket/save/PendingGameReward.hpp"

class TestTrainingLedger : public QObject {
    Q_OBJECT

private slots:
    void testCooldownEnforcement() {
        Pocket::Save::PendingRewardLedger ledger;
        Pocket::Save::TrainingRewardRules rules;
        rules.cooldownSeconds = 30;
        ledger.setRules(rules);

        uint64_t now = 100000;
        std::string reason;

        Pocket::Save::PendingGameReward r1;
        r1.companionLinkId = 1;
        r1.category = Pocket::Save::RewardCategory::EV;
        r1.evStat = Pocket::Save::EVType::Attack;
        r1.amount = 4;
        r1.timestamp = now;

        QVERIFY(ledger.recordReward(r1, reason));

        // Attempt second reward 10 seconds later (cooldown active -> should fail)
        Pocket::Save::PendingGameReward r2 = r1;
        r2.timestamp = now + 10;
        QVERIFY(!ledger.recordReward(r2, reason));

        // Attempt third reward 35 seconds later (cooldown passed -> should succeed)
        Pocket::Save::PendingGameReward r3 = r1;
        r3.timestamp = now + 35;
        QVERIFY(ledger.recordReward(r3, reason));
    }

    void testDailyEvCapEnforcement() {
        Pocket::Save::PendingRewardLedger ledger;
        Pocket::Save::TrainingRewardRules rules;
        rules.cooldownSeconds = 0;
        rules.maxDailyEvPoints = 10;
        ledger.setRules(rules);

        uint64_t now = 100000;
        std::string reason;

        // Add 8 EV points
        Pocket::Save::PendingGameReward r1;
        r1.companionLinkId = 1;
        r1.category = Pocket::Save::RewardCategory::EV;
        r1.evStat = Pocket::Save::EVType::Attack;
        r1.amount = 8;
        r1.timestamp = now;

        QVERIFY(ledger.recordReward(r1, reason));
        QCOMPARE(ledger.getTotalPendingEV(1, Pocket::Save::EVType::Attack), 8);

        // Attempt adding 5 EV points (8 + 5 = 13 > 10 cap -> rejected)
        Pocket::Save::PendingGameReward r2 = r1;
        r2.amount = 5;
        r2.timestamp = now + 1;
        QVERIFY(!ledger.recordReward(r2, reason));
    }

    void testDiminishingReturns() {
        Pocket::Save::PendingRewardLedger ledger;
        Pocket::Save::TrainingRewardRules rules;
        rules.cooldownSeconds = 0;
        rules.diminishingReturnsThreshold = 3; // After 3 sessions
        rules.maxDailyEvPoints = 100;
        ledger.setRules(rules);

        uint64_t now = 100000;
        std::string reason;

        // Sessions 1..3 get full 4 EV points
        for (int i = 0; i < 3; ++i) {
            Pocket::Save::PendingGameReward r;
            r.companionLinkId = 1;
            r.category = Pocket::Save::RewardCategory::EV;
            r.evStat = Pocket::Save::EVType::Speed;
            r.amount = 4;
            r.timestamp = now + i;
            QVERIFY(ledger.recordReward(r, reason));
        }

        // Session 4 should suffer diminishing returns (4 / 2 = 2 EV points)
        Pocket::Save::PendingGameReward r4;
        r4.companionLinkId = 1;
        r4.category = Pocket::Save::RewardCategory::EV;
        r4.evStat = Pocket::Save::EVType::Speed;
        r4.amount = 4;
        r4.timestamp = now + 4;

        QVERIFY(ledger.recordReward(r4, reason));
        // Total = 3 * 4 + 2 = 14
        QCOMPARE(ledger.getTotalPendingEV(1, Pocket::Save::EVType::Speed), 14);
    }

    void testMultipleDaysClockAdvancement() {
        Pocket::Save::PendingRewardLedger ledger;
        Pocket::Save::TrainingRewardRules rules;
        rules.cooldownSeconds = 0;
        rules.maxDailyEvPoints = 10;
        ledger.setRules(rules);

        uint64_t day1 = 100000; // Day 1
        uint64_t day2 = day1 + 86400; // Day 2 (+24 hours)

        std::string reason;

        // Reach Day 1 cap
        Pocket::Save::PendingGameReward r1;
        r1.companionLinkId = 1;
        r1.category = Pocket::Save::RewardCategory::EV;
        r1.evStat = Pocket::Save::EVType::HP;
        r1.amount = 10;
        r1.timestamp = day1;
        QVERIFY(ledger.recordReward(r1, reason));

        // Day 1 addition rejected (cap reached)
        Pocket::Save::PendingGameReward r2 = r1;
        r2.timestamp = day1 + 100;
        QVERIFY(!ledger.recordReward(r2, reason));

        // Day 2 addition succeeds (new day resets daily cap counter)
        Pocket::Save::PendingGameReward r3 = r1;
        r3.timestamp = day2;
        QVERIFY(ledger.recordReward(r3, reason));
        QCOMPARE(ledger.getTotalPendingEV(1, Pocket::Save::EVType::HP), 20);
    }
};

QTEST_MAIN(TestTrainingLedger)
#include "test_training_ledger.moc"
