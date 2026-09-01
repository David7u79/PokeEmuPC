#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <vector>
#include <memory>
#include "pocket/core/IpcMessage.hpp"

namespace Pocket::Core {

class IpcServer : public QObject {
    Q_OBJECT
public:
    explicit IpcServer(const QString& pipeName = "PocketPartner_IPC_Pipe", QObject *parent = nullptr);
    ~IpcServer() override;

    bool start();
    void stop();

    bool isListening() const { return m_server && m_server->isListening(); }
    int clientCount() const { return static_cast<int>(m_clients.size()); }

    void broadcastMessage(const IpcMessage& msg);
    void sendMessage(QLocalSocket* socket, const IpcMessage& msg);

signals:
    void clientConnected(QLocalSocket* socket);
    void clientDisconnected(QLocalSocket* socket);
    void messageReceived(QLocalSocket* socket, const IpcMessage& msg);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QString m_pipeName;
    QLocalServer *m_server{nullptr};
    std::vector<QLocalSocket*> m_clients;
    std::map<QLocalSocket*, QByteArray> m_readBuffers;
};

} // namespace Pocket::Core
