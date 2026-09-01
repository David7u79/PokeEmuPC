#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include "pocket/core/IpcMessage.hpp"

namespace Pocket::Core {

class IpcClient : public QObject {
    Q_OBJECT
public:
    explicit IpcClient(const QString& pipeName = "PocketPartner_IPC_Pipe", QObject *parent = nullptr);
    ~IpcClient() override;

    void connectToServer();
    void disconnectFromServer();

    bool isConnected() const { return m_socket && m_socket->state() == QLocalSocket::ConnectedState; }
    void setAutoReconnect(bool enable) { m_autoReconnect = enable; }

    bool sendMessage(const IpcMessage& msg);

signals:
    void connected();
    void disconnected();
    void messageReceived(const IpcMessage& msg);
    void connectionError(const QString& errorStr);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QLocalSocket::LocalSocketError socketError);
    void onReconnectTimeout();

private:
    QString m_pipeName;
    QLocalSocket *m_socket{nullptr};
    QTimer m_reconnectTimer;
    QByteArray m_readBuffer;
    bool m_autoReconnect{true};
    int m_reconnectIntervalMs{1500};
};

} // namespace Pocket::Core
