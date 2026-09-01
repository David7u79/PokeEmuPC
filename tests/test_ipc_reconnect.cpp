#include <QtTest/QtTest>
#include <QUuid>
#include "pocket/core/IpcServer.hpp"
#include "pocket/core/IpcClient.hpp"

class TestIpcReconnect : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qRegisterMetaType<Pocket::Core::IpcMessage>("Pocket::Core::IpcMessage");
    }

    void testClientAutoReconnect() {
        QString pipeName = "TestReconnectPipe_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

        auto server = std::make_unique<Pocket::Core::IpcServer>(pipeName);
        QVERIFY(server->start());

        Pocket::Core::IpcClient client(pipeName);
        client.setAutoReconnect(true);

        int connectCount = 0;
        int disconnectCount = 0;

        connect(&client, &Pocket::Core::IpcClient::connected, [&connectCount]() {
            connectCount++;
        });

        connect(&client, &Pocket::Core::IpcClient::disconnected, [&disconnectCount]() {
            disconnectCount++;
        });

        client.connectToServer();

        int waitCount = 0;
        while (connectCount < 1 && waitCount < 30) {
            QTest::qWait(50);
            waitCount++;
        }
        QCOMPARE(connectCount, 1);

        // Stop server -> disconnects client
        server->stop();

        waitCount = 0;
        while (disconnectCount < 1 && waitCount < 30) {
            QTest::qWait(50);
            waitCount++;
        }
        QCOMPARE(disconnectCount, 1);

        // Restart server -> client reconnects automatically
        QVERIFY(server->start());

        waitCount = 0;
        while (connectCount < 2 && waitCount < 50) {
            QTest::qWait(50);
            waitCount++;
        }
        QCOMPARE(connectCount, 2);

        client.disconnectFromServer();
        server->stop();
    }
};

QTEST_MAIN(TestIpcReconnect)
#include "test_ipc_reconnect.moc"
