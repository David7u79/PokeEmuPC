#include <QtTest/QtTest>
#include <QUuid>
#include "pocket/core/IpcServer.hpp"

class TestCompanionAbsent : public QObject {
    Q_OBJECT
private slots:
    void testMainAppBehaviorWhenCompanionAbsent() {
        QString pipeName = "TestAbsentPipe_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

        Pocket::Core::IpcServer server(pipeName);
        QVERIFY(server.start());
        QCOMPARE(server.clientCount(), 0);

        // Broadcasting when no companion clients are connected operates safely without throwing
        Pocket::Core::IpcMessage msg;
        msg.command = Pocket::Core::IpcCommandType::CompanionStatusChanged;
        server.broadcastMessage(msg);

        QCOMPARE(server.clientCount(), 0);
        server.stop();
    }
};

QTEST_MAIN(TestCompanionAbsent)
#include "test_companion_absent.moc"
