#include "pocket/core/IpcServer.hpp"
#include <QDebug>
#include <algorithm>
#include <QMetaType>

namespace Pocket::Core {

IpcServer::IpcServer(const QString& pipeName, QObject *parent)
    : QObject(parent), m_pipeName(pipeName) {
    qRegisterMetaType<Pocket::Core::IpcMessage>("Pocket::Core::IpcMessage");
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start() {
    // Remove stale pipe if it exists
    QLocalServer::removeServer(m_pipeName);

    if (!m_server->listen(m_pipeName)) {
        qWarning() << "IpcServer failed to listen on pipe:" << m_pipeName << "Error:" << m_server->errorString();
        return false;
    }
    return true;
}

void IpcServer::stop() {
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
    for (auto* client : m_clients) {
        client->disconnect(this);
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    m_readBuffers.clear();
}

void IpcServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QLocalSocket* socket = m_server->nextPendingConnection();
        if (!socket) continue;

        m_clients.push_back(socket);
        m_readBuffers[socket] = QByteArray();

        connect(socket, &QLocalSocket::readyRead, this, &IpcServer::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, this, &IpcServer::onClientDisconnected);

        emit clientConnected(socket);
    }
}

void IpcServer::onReadyRead() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    QByteArray& buffer = m_readBuffers[socket];
    buffer.append(socket->readAll());

    // Process length-prefixed frames
    while (buffer.size() >= 4) {
        const char* data = buffer.constData();
        uint32_t payloadLen = (static_cast<uint8_t>(data[0]) << 24) |
                             (static_cast<uint8_t>(data[1]) << 16) |
                             (static_cast<uint8_t>(data[2]) << 8)  |
                              static_cast<uint8_t>(data[3]);

        if (buffer.size() < static_cast<int>(4 + payloadLen)) {
            break; // Wait for rest of frame
        }

        QByteArray payloadData = buffer.mid(4, static_cast<qsizetype>(payloadLen));
        buffer.remove(0, static_cast<qsizetype>(4 + payloadLen));

        IpcMessage msg;
        if (IpcMessage::deserialize(payloadData, msg)) {
            emit messageReceived(socket, msg);
        }
    }
}

void IpcServer::onClientDisconnected() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    auto it = std::find(m_clients.begin(), m_clients.end(), socket);
    if (it != m_clients.end()) {
        m_clients.erase(it);
    }
    m_readBuffers.erase(socket);

    emit clientDisconnected(socket);
    socket->deleteLater();
}

void IpcServer::broadcastMessage(const IpcMessage& msg) {
    QByteArray frame = msg.serialize();
    for (auto* client : m_clients) {
        if (client && client->isOpen()) {
            client->write(frame);
            client->flush();
        }
    }
}

void IpcServer::sendMessage(QLocalSocket* socket, const IpcMessage& msg) {
    if (socket && socket->isOpen()) {
        QByteArray frame = msg.serialize();
        socket->write(frame);
        socket->flush();
    }
}

} // namespace Pocket::Core
