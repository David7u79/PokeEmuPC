#include "pocket/core/IpcClient.hpp"
#include <QDebug>

namespace Pocket::Core {

IpcClient::IpcClient(const QString& pipeName, QObject *parent)
    : QObject(parent), m_pipeName(pipeName) {

    m_socket = new QLocalSocket(this);

    connect(m_socket, &QLocalSocket::connected, this, &IpcClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::onErrorOccurred);

    connect(&m_reconnectTimer, &QTimer::timeout, this, &IpcClient::onReconnectTimeout);
}

IpcClient::~IpcClient() {
    disconnectFromServer();
}

void IpcClient::connectToServer() {
    if (isConnected()) return;
    m_socket->connectToServer(m_pipeName);
}

void IpcClient::disconnectFromServer() {
    m_reconnectTimer.stop();
    if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnect(this);
        m_socket->abort();
    }
}

bool IpcClient::sendMessage(const IpcMessage& msg) {
    if (!isConnected()) return false;

    QByteArray frame = msg.serialize();
    qint64 bytesWritten = m_socket->write(frame);
    m_socket->flush();
    return bytesWritten == frame.size();
}

void IpcClient::onConnected() {
    m_reconnectTimer.stop();
    m_readBuffer.clear();
    emit connected();
}

void IpcClient::onDisconnected() {
    emit disconnected();
    if (m_autoReconnect) {
        m_reconnectTimer.start(m_reconnectIntervalMs);
    }
}

void IpcClient::onErrorOccurred(QLocalSocket::LocalSocketError socketError) {
    emit connectionError(m_socket->errorString());
    if (m_autoReconnect && socketError != QLocalSocket::PeerClosedError) {
        if (!m_reconnectTimer.isActive()) {
            m_reconnectTimer.start(m_reconnectIntervalMs);
        }
    }
}

void IpcClient::onReconnectTimeout() {
    if (!isConnected()) {
        connectToServer();
    }
}

void IpcClient::onReadyRead() {
    m_readBuffer.append(m_socket->readAll());

    while (m_readBuffer.size() >= 4) {
        const char* data = m_readBuffer.constData();
        uint32_t payloadLen = (static_cast<uint8_t>(data[0]) << 24) |
                             (static_cast<uint8_t>(data[1]) << 16) |
                             (static_cast<uint8_t>(data[2]) << 8)  |
                              static_cast<uint8_t>(data[3]);

        if (m_readBuffer.size() < static_cast<int>(4 + payloadLen)) {
            break; // Wait for full frame
        }

        QByteArray payloadData = m_readBuffer.mid(4, static_cast<qsizetype>(payloadLen));
        m_readBuffer.remove(0, static_cast<qsizetype>(4 + payloadLen));

        IpcMessage msg;
        if (IpcMessage::deserialize(payloadData, msg)) {
            emit messageReceived(msg);
        }
    }
}

} // namespace Pocket::Core
