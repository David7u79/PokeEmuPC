#include <QtTest/QtTest>
#include <QUuid>
#include "pocket/core/IpcServer.hpp"
#include "pocket/core/IpcClient.hpp"

class TestIpcConnection : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qRegisterMetaType<Pocket::Core::IpcMessage>("Pocket::Core::IpcMessage");
    }

    void testServerClientHandshakeAndMessaging() {
        QString pipeName = "TestPipe_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

        Pocket::Core::IpcServer server(pipeName);
        QVERIFY(server.start());

        Pocket::Core::IpcClient client(pipeName);
        client.setAutoReconnect(false);

        bool connectedReceived = false;
        connect(&client, &Pocket::Core::IpcClient::connected, [&connectedReceived]() {
            connectedReceived = true;
        });

        bool msgReceived = false;
        Pocket::Core::IpcMessage receivedMsg;
        connect(&server, &Pocket::Core::IpcServer::messageReceived, [&msgReceived, &receivedMsg](QLocalSocket*, const Pocket::Core::IpcMessage& msg) {
            msgReceived = true;
            receivedMsg = msg;
        });

        client.connectToServer();
        
        int waitCount = 0;
        while (!connectedReceived && waitCount < 30) {
            QTest::qWait(50);
            waitCount++;
        }
        QVERIFY(connectedReceived);

        Pocket::Core::IpcMessage sendMsg;
        sendMsg.command = Pocket::Core::IpcCommandType::Ping;
        sendMsg.sequenceNumber = 42;
        sendMsg.payload["testKey"] = "testValue";

        QVERIFY(client.sendMessage(sendMsg));

        waitCount = 0;
        while (!msgReceived && waitCount < 30) {
            QTest::qWait(50);
            waitCount++;
        }
        QVERIFY(msgReceived);

        QCOMPARE(receivedMsg.command, Pocket::Core::IpcCommandType::Ping);
        QCOMPARE(receivedMsg.sequenceNumber, static_cast<uint64_t>(42));
        QCOMPARE(receivedMsg.payload["testKey"].toString(), QString("testValue"));

        client.disconnectFromServer();
        server.stop();
    }
};

QTEST_MAIN(TestIpcConnection)
#include "test_ipc_connection.moc"
