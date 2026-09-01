#include <QtTest/QtTest>
#include <QUuid>
#include "pocket/core/IpcClient.hpp"

class TestMainAppAbsent : public QObject {
    Q_OBJECT
private slots:
    void testCompanionBehaviorWhenMainAppAbsent() {
        QString nonExistentPipe = "NonExistentPipe_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

        Pocket::Core::IpcClient client(nonExistentPipe);
        client.setAutoReconnect(false);

        // Client attempts to connect when server is absent
        client.connectToServer();
        QTest::qWait(100);

        QVERIFY(!client.isConnected());

        // Sending message when disconnected fails gracefully without throwing or crashing
        Pocket::Core::IpcMessage msg;
        msg.command = Pocket::Core::IpcCommandType::Ping;
        QVERIFY(!client.sendMessage(msg));
    }
};

QTEST_MAIN(TestMainAppAbsent)
#include "test_main_app_absent.moc"
