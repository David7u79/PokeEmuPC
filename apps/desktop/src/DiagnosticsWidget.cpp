#include "DiagnosticsWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <chrono>
#include "pocket/save/CompanionReidentifier.hpp"

namespace Pocket::App {

DiagnosticsWidget::DiagnosticsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ipcServer("PocketPartnerIPC", this) {

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    QGroupBox *headerGroup = new QGroupBox("Developer Save File Inspector (Gen III Read-Only)", this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerGroup);

    m_openFileBtn = new QPushButton("Inspect Save File (.sav)...", headerGroup);
    m_statusLabel = new QLabel("No save file loaded", headerGroup);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #88AACC;");

    headerLayout->addWidget(m_openFileBtn);
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();

    QGroupBox *trainerGroup = new QGroupBox("Trainer Info & Active Companion Status", this);
    QVBoxLayout *trainerLayout = new QVBoxLayout(trainerGroup);

    m_trainerLabel = new QLabel("Trainer: -- | Play Time: --", trainerGroup);
    m_activeCompanionLabel = new QLabel("Active Companion: None selected", trainerGroup);
    m_activeCompanionLabel->setStyleSheet("font-weight: bold; color: #A3BE8C;");

    m_bondVsFriendshipLabel = new QLabel("App Bond (PocketPartner XP): Lv 1 | Game Friendship (Canonical): -- | IVs: Read-Only", trainerGroup);
    m_bondVsFriendshipLabel->setStyleSheet("font-size: 11px; color: #EBCB8B; font-weight: bold;");

    m_selectCompanionBtn = new QPushButton("★ Set Selected Party Row as Active Desktop Companion", trainerGroup);
    m_selectCompanionBtn->setEnabled(false);

    trainerLayout->addWidget(m_trainerLabel);
    trainerLayout->addWidget(m_activeCompanionLabel);
    trainerLayout->addWidget(m_bondVsFriendshipLabel);
    trainerLayout->addWidget(m_selectCompanionBtn);

    // Timing Bar Training Mini-Activity
    QGroupBox *trainingGroup = new QGroupBox("Companion Training Mini-Activity (Pending EV Rewards)", this);
    QVBoxLayout *trainingLayout = new QVBoxLayout(trainingGroup);
    m_timingBarWidget = new TrainingTimingBarWidget(trainingGroup);
    trainingLayout->addWidget(m_timingBarWidget);

    // Pending Reward Ledger Section
    QGroupBox *ledgerGroup = new QGroupBox("Pending Game Reward Ledger (Staged - Save Unmodified)", this);
    QVBoxLayout *ledgerLayout = new QVBoxLayout(ledgerGroup);

    m_ledgerTable = new QTableWidget(0, 5, ledgerGroup);
    m_ledgerTable->setHorizontalHeaderLabels({"Reward ID", "Category", "Stat Target", "Points Staged", "Source Action"});
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ledgerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ledgerLayout->addWidget(m_ledgerTable);

    // Party Table Section
    QGroupBox *partyGroup = new QGroupBox("Parsed Party Pokémon (Read-Only Save Data)", this);
    QVBoxLayout *partyLayout = new QVBoxLayout(partyGroup);

    m_partyTable = new QTableWidget(0, 8, partyGroup);
    m_partyTable->setHorizontalHeaderLabels({"Slot", "Species", "Nickname", "Level", "Nature", "EVs (HP/Atk/Def)", "IVs (HP/Atk/Def)", "PID"});
    m_partyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_partyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partyTable->setSelectionMode(QAbstractItemView::SingleSelection);

    partyLayout->addWidget(m_partyTable);

    layout->addWidget(headerGroup);
    layout->addWidget(trainerGroup);
    layout->addWidget(trainingGroup);
    layout->addWidget(ledgerGroup);
    layout->addWidget(partyGroup);

    connect(m_openFileBtn, &QPushButton::clicked, this, &DiagnosticsWidget::onOpenFileClicked);
    connect(m_selectCompanionBtn, &QPushButton::clicked, this, &DiagnosticsWidget::onSelectCompanionClicked);
    connect(m_timingBarWidget, &TrainingTimingBarWidget::trainingCompleted, this, &DiagnosticsWidget::onTrainingCompleted);

    m_ipcServer.start();
}

void DiagnosticsWidget::onOpenFileClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Gen III Save File", "", "Save Files (*.sav *.sa1 *.bin);;All Files (*)");
    if (!file.isEmpty()) {
        loadAndInspectSave(file);
    }
}

void DiagnosticsWidget::loadAndInspectSave(const QString& saveFilePath) {
    m_lastParseResult = m_parser.parseSaveFile(saveFilePath.toStdString());

    if (m_lastParseResult.status != Pocket::Save::SaveParseStatus::Success) {
        m_statusLabel->setText(QString("Failed: %1").arg(QString::fromStdString(m_lastParseResult.errorMessage)));
        m_statusLabel->setStyleSheet("font-weight: bold; color: #FF5252;");
        m_trainerLabel->setText("Trainer: --");
        m_partyTable->setRowCount(0);
        m_selectCompanionBtn->setEnabled(false);
        return;
    }

    QString slotName = (m_lastParseResult.activeSlotIndex == 0) ? "Slot A (0x00000)" : "Slot B (0x0E000)";
    m_statusLabel->setText(QString("Active Slot: %1 | Save Counter: %2 | Checksums: VALID")
        .arg(slotName).arg(m_lastParseResult.saveCounter));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");

    m_trainerLabel->setText(QString("Trainer: %1 (ID: %2, Secret: %3) | Play Time: %4h %5m")
        .arg(QString::fromStdString(m_lastParseResult.trainerName))
        .arg(m_lastParseResult.trainerId)
        .arg(m_lastParseResult.secretId)
        .arg(m_lastParseResult.playTimeHours)
        .arg(m_lastParseResult.playTimeMinutes));

    m_partyTable->setRowCount(0);
    for (size_t i = 0; i < m_lastParseResult.party.size(); ++i) {
        const auto& pkmn = m_lastParseResult.party[i];
        int row = m_partyTable->rowCount();
        m_partyTable->insertRow(row);

        m_partyTable->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_partyTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(pkmn.speciesName)));
        m_partyTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(pkmn.nickname)));
        m_partyTable->setItem(row, 3, new QTableWidgetItem(QString::number(pkmn.level)));
        m_partyTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(Pocket::Save::natureToString(pkmn.nature))));

        QString evStr = QString("%1/%2/%3").arg(pkmn.evs.hp).arg(pkmn.evs.attack).arg(pkmn.evs.defense);
        QString ivStr = QString("%1/%2/%3").arg(pkmn.ivs.hp).arg(pkmn.ivs.attack).arg(pkmn.ivs.defense);

        m_partyTable->setItem(row, 5, new QTableWidgetItem(evStr));
        m_partyTable->setItem(row, 6, new QTableWidgetItem(ivStr));
        m_partyTable->setItem(row, 7, new QTableWidgetItem(QString("0x%1").arg(pkmn.personalityValue, 8, 16, QChar('0')).toUpper()));
    }

    m_selectCompanionBtn->setEnabled(!m_lastParseResult.party.empty());
}

void DiagnosticsWidget::onSelectCompanionClicked() {
    int row = m_partyTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_lastParseResult.party.size())) {
        QMessageBox::information(this, "Select Companion", "Please click on a row in the Party table to select a companion.");
        return;
    }

    const auto& selectedPkmn = m_lastParseResult.party[row];
    m_currentLink = Pocket::Save::CompanionReidentifier::createLinkFromCreature(selectedPkmn, 1, "synthetic_hash");

    m_activeCompanionLabel->setText(QString("Active Companion: %1 (%2) [Level %3]")
        .arg(QString::fromStdString(m_currentLink.nickname))
        .arg(QString::fromStdString(m_currentLink.speciesName))
        .arg(m_currentLink.level));

    m_bondVsFriendshipLabel->setText(QString("App Bond (PocketPartner XP): Lv 1 | Game Friendship (Canonical): %1 | IVs: Read-Only (HP %2/Atk %3)")
        .arg(m_currentLink.gameFriendship)
        .arg(selectedPkmn.ivs.hp)
        .arg(selectedPkmn.ivs.attack));

    // Send IPC Message to Desktop Companion Widget
    Pocket::Core::IpcMessage msg;
    msg.command = Pocket::Core::IpcCommandType::CompanionStatusChanged;
    msg.payload["nickname"] = QString::fromStdString(m_currentLink.nickname);
    msg.payload["species"] = QString::fromStdString(m_currentLink.speciesName);
    msg.payload["level"] = m_currentLink.level;
    msg.payload["gameFriendship"] = m_currentLink.gameFriendship;
    msg.payload["linkStatus"] = QString::fromStdString(Pocket::Companion::linkStatusToString(m_currentLink.status));

    m_ipcServer.broadcastMessage(msg);
}

void DiagnosticsWidget::onTrainingCompleted(Pocket::Save::EVType stat, int evPoints, double qualityScore) {
    Q_UNUSED(qualityScore);
    if (m_currentLink.gameId == 0) {
        QMessageBox::warning(this, "Training", "Please select an active companion from the party table first!");
        return;
    }

    if (evPoints <= 0) return;

    auto now = std::chrono::system_clock::now();
    uint64_t nowSecs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    Pocket::Save::PendingGameReward reward;
    reward.companionLinkId = m_currentLink.gameId;
    reward.category = Pocket::Save::RewardCategory::EV;
    reward.evStat = stat;
    reward.amount = evPoints;
    reward.timestamp = nowSecs;
    reward.sourceAction = "Train_TimingBar";

    std::string reason;
    if (m_ledger.recordReward(reward, reason)) {
        refreshLedgerDisplay();
    } else {
        QMessageBox::warning(this, "Training Reward Blocked", QString::fromStdString(reason));
    }
}

void DiagnosticsWidget::refreshLedgerDisplay() {
    auto rewards = m_ledger.getPendingRewards(m_currentLink.gameId);
    m_ledgerTable->setRowCount(0);

    for (const auto& r : rewards) {
        int row = m_ledgerTable->rowCount();
        m_ledgerTable->insertRow(row);

        m_ledgerTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.rewardId)));
        m_ledgerTable->setItem(row, 1, new QTableWidgetItem("EV Reward"));
        m_ledgerTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(Pocket::Save::evTypeToString(r.evStat))));
        m_ledgerTable->setItem(row, 3, new QTableWidgetItem(QString("+%1 EV").arg(r.amount)));
        m_ledgerTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(r.sourceAction)));
    }
}

} // namespace Pocket::App
