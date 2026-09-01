#include <QtTest/QtTest>
#include <QUuid>
#include "pocket/core/IpcServer.hpp"
#include "pocket/core/IpcClient.hpp"

class TestGracefulShutdown : public QObject {
    Q_OBJECT
private slots:
    void testCleanShutdown() {
        QString pipeName = "ShutdownPipe_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

        auto server = std::make_unique<Pocket::Core::IpcServer>(pipeName);
        QVERIFY(server->start());

        auto client = std::make_unique<Pocket::Core::IpcClient>(pipeName);
        client->setAutoReconnect(false);
        client->connectToServer();
        QTest::qWait(200);

        QVERIFY(client->isConnected());

        // Graceful shutdown sequence
        client->disconnectFromServer();
        server->stop();

        QVERIFY(!client->isConnected());
        QVERIFY(!server->isListening());
    }
};

QTEST_MAIN(TestGracefulShutdown)
#include "test_graceful_shutdown.moc"
