#include "pocketpartner/core/CompanionLink.hpp"
#include <QCryptographicHash>
#include <QByteArray>
#include <QString>

namespace PocketPartner::Core {

CompanionLink::CompanionLink(GameGeneration gen,
                             uint32_t personalityValue,
                             uint16_t trainerId,
                             uint16_t secretId,
                             uint16_t speciesId,
                             std::string nickname,
                             std::string trainerName,
                             uint64_t initialChecksum)
    : m_gen(gen),
      m_pv(personalityValue),
      m_tid(trainerId),
      m_sid(secretId),
      m_speciesId(speciesId),
      m_nickname(std::move(nickname)),
      m_trainerName(std::move(trainerName)),
      m_initialChecksum(initialChecksum) {}

std::string CompanionLink::identityHash() const {
    QByteArray data;
    data.append(reinterpret_cast<const char*>(&m_gen), sizeof(m_gen));
    data.append(reinterpret_cast<const char*>(&m_pv), sizeof(m_pv));
    data.append(reinterpret_cast<const char*>(&m_tid), sizeof(m_tid));
    data.append(reinterpret_cast<const char*>(&m_sid), sizeof(m_sid));
    data.append(reinterpret_cast<const char*>(&m_speciesId), sizeof(m_speciesId));
    data.append(m_nickname.c_str(), static_cast<qsizetype>(m_nickname.size()));
    data.append(m_trainerName.c_str(), static_cast<qsizetype>(m_trainerName.size()));
    data.append(reinterpret_cast<const char*>(&m_initialChecksum), sizeof(m_initialChecksum));

    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return hash.toHex().toStdString();
}

bool CompanionLink::isValid() const {
    return m_speciesId != 0 && (m_pv != 0 || m_tid != 0 || m_initialChecksum != 0);
}

bool CompanionLink::matches(const CompanionLink& other) const {
    if (!isValid() || !other.isValid()) return false;
    return m_gen == other.m_gen &&
           m_pv == other.m_pv &&
           m_tid == other.m_tid &&
           m_sid == other.m_sid &&
           m_speciesId == other.m_speciesId &&
           m_nickname == other.m_nickname &&
           m_trainerName == other.m_trainerName;
}

} // namespace PocketPartner::Core
