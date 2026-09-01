#pragma once

#include <string>
#include <cstdint>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>

namespace Pocket::Core {

enum class IpcCommandType {
    Unknown = 0,
    Ping = 1,
    Pong = 2,
    CompanionStatusRequest = 10,
    CompanionStatusChanged = 11,
    OpenMainApplication = 20,
    ShowCompanion = 30,
    HideCompanion = 31,
    ShutdownCompanion = 40
};

struct IpcMessage {
    int version{1};
    IpcCommandType command{IpcCommandType::Unknown};
    uint64_t sequenceNumber{0};
    QJsonObject payload;

    static std::string commandToString(IpcCommandType cmd) {
        switch (cmd) {
            case IpcCommandType::Ping:                   return "Ping";
            case IpcCommandType::Pong:                   return "Pong";
            case IpcCommandType::CompanionStatusRequest: return "CompanionStatusRequest";
            case IpcCommandType::CompanionStatusChanged: return "CompanionStatusChanged";
            case IpcCommandType::OpenMainApplication:    return "OpenMainApplication";
            case IpcCommandType::ShowCompanion:          return "ShowCompanion";
            case IpcCommandType::HideCompanion:          return "HideCompanion";
            case IpcCommandType::ShutdownCompanion:      return "ShutdownCompanion";
            default:                                     return "Unknown";
        }
    }

    static IpcCommandType commandFromString(const std::string& str) {
        if (str == "Ping")                   return IpcCommandType::Ping;
        if (str == "Pong")                   return IpcCommandType::Pong;
        if (str == "CompanionStatusRequest") return IpcCommandType::CompanionStatusRequest;
        if (str == "CompanionStatusChanged") return IpcCommandType::CompanionStatusChanged;
        if (str == "OpenMainApplication")    return IpcCommandType::OpenMainApplication;
        if (str == "ShowCompanion")          return IpcCommandType::ShowCompanion;
        if (str == "HideCompanion")          return IpcCommandType::HideCompanion;
        if (str == "ShutdownCompanion")      return IpcCommandType::ShutdownCompanion;
        return IpcCommandType::Unknown;
    }

    QByteArray serialize() const {
        QJsonObject root;
        root["version"] = version;
        root["command"] = QString::fromStdString(commandToString(command));
        root["seq"] = static_cast<qint64>(sequenceNumber);
        root["payload"] = payload;

        QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Compact);
        
        // Framing header: 4-byte big-endian uint32 payload size
        uint32_t length = static_cast<uint32_t>(jsonData.size());
        QByteArray frame;
        frame.append(static_cast<char>((length >> 24) & 0xFF));
        frame.append(static_cast<char>((length >> 16) & 0xFF));
        frame.append(static_cast<char>((length >> 8) & 0xFF));
        frame.append(static_cast<char>(length & 0xFF));
        frame.append(jsonData);
        return frame;
    }

    static bool deserialize(const QByteArray& payloadData, IpcMessage& msg) {
        QJsonDocument doc = QJsonDocument::fromJson(payloadData);
        if (!doc.isObject()) return false;

        QJsonObject root = doc.object();
        msg.version = root["version"].toInt(1);
        msg.command = commandFromString(root["command"].toString().toStdString());
        msg.sequenceNumber = static_cast<uint64_t>(root["seq"].toInteger(0));
        msg.payload = root["payload"].toObject();
        return true;
    }
};

} // namespace Pocket::Core
