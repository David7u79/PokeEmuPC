#include "DiagnosticsWidget.hpp"
#include "Theme.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QFontDatabase>
#include <chrono>
#include <QFileInfo>
#include "pocket/save/CompanionReidentifier.hpp"
#include "pocket/save/Gen1SaveParser.hpp"
#include "pocket/save/Gen2SaveParser.hpp"
#include "pocket/save/Gen4SaveParser.hpp"
#include "pocket/save/Gen5SaveParser.hpp"

namespace Pocket::App {

namespace {

QLabel* createBlockTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text.toUpper(), parent);
    label->setStyleSheet(QStringLiteral(
        "font-size: 11px;"
        "font-weight: 700;"
        "letter-spacing: 1px;"
        "color: %1;"
        "background: transparent;"
        "padding-bottom: 2px;"
    ).arg(Theme::textSecondary().name()));
    return label;
}

} // namespace

DiagnosticsWidget::DiagnosticsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ipcServer("PocketPartnerIPC", this) {

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    auto* contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    const QString tableStyle = QStringLiteral(
        "QTableWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 6px;"
        "  gridline-color: %3;"
        "}"
        "QHeaderView::section {"
        "  background-color: %4;"
        "  color: %5;"
        "  border: 1px solid %3;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %6;"
        "  color: %2;"
        "}"
    ).arg(Theme::surfaceRaised().name(),
         Theme::textPrimary().name(),
         Theme::border().name(),
         Theme::surface().name(),
         Theme::textSecondary().name(),
         Theme::border().name());

    // Block 1: Save File Inspector Header
    layout->addWidget(createBlockTitle(QStringLiteral("Inspector de partidas (Gen I - Gen V)"), contentWidget));

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    m_openFileBtn = new QPushButton(QStringLiteral("Inspect Save File (.sav)..."), contentWidget);
    m_statusLabel = new QLabel(QStringLiteral("No save file loaded"), contentWidget);
    m_statusLabel->setFont(monoFont);
    m_statusLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::textSecondary().name()));

    headerLayout->addWidget(m_openFileBtn);
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    // Block 2: Trainer Info & Active Companion Status
    layout->addWidget(createBlockTitle(QStringLiteral("Información del entrenador y compañero"), contentWidget));

    auto* trainerLayout = new QVBoxLayout();
    trainerLayout->setContentsMargins(0, 0, 0, 0);
    trainerLayout->setSpacing(4);

    m_trainerLabel = new QLabel(QStringLiteral("Trainer: -- | Play Time: --"), contentWidget);
    m_trainerLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(Theme::textPrimary().name()));

    m_activeCompanionLabel = new QLabel(QStringLiteral("Active Companion: None selected"), contentWidget);
    m_activeCompanionLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::textSecondary().name()));

    m_bondVsFriendshipLabel = new QLabel(QStringLiteral("App Bond (PocketPartner XP): Lv 1 | Game Friendship: --"), contentWidget);
    m_bondVsFriendshipLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; background: transparent;").arg(Theme::textSecondary().name()));

    m_selectCompanionBtn = new QPushButton(QStringLiteral("★ Set Selected Party Row as Active Desktop Companion"), contentWidget);
    m_selectCompanionBtn->setEnabled(false);

    trainerLayout->addWidget(m_trainerLabel);
    trainerLayout->addWidget(m_activeCompanionLabel);
    trainerLayout->addWidget(m_bondVsFriendshipLabel);
    trainerLayout->addWidget(m_selectCompanionBtn);
    layout->addLayout(trainerLayout);

    // Block 3: Companion Training Mini-Activity
    layout->addWidget(createBlockTitle(QStringLiteral("Mini-actividad de entrenamiento"), contentWidget));
    m_timingBarWidget = new TrainingTimingBarWidget(contentWidget);
    layout->addWidget(m_timingBarWidget);

    // Block 4: Pending Reward Ledger Section
    layout->addWidget(createBlockTitle(QStringLiteral("Registro de recompensas pendientes (sin modificar partida)"), contentWidget));

    m_ledgerTable = new QTableWidget(0, 5, contentWidget);
    m_ledgerTable->setStyleSheet(tableStyle);
    m_ledgerTable->setHorizontalHeaderLabels({"Reward ID", "Category", "Stat Target", "Points Staged", "Source Action"});
    m_ledgerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ledgerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ledgerTable->setMinimumHeight(120);

    layout->addWidget(m_ledgerTable);

    // Block 5: Party Table Section
    layout->addWidget(createBlockTitle(QStringLiteral("Pokémon en el equipo (datos de solo lectura)"), contentWidget));

    m_partyTable = new QTableWidget(0, 8, contentWidget);
    m_partyTable->setStyleSheet(tableStyle);
    m_partyTable->setHorizontalHeaderLabels({"Slot", "Species", "Nickname", "Level", "Nature / Gen", "EVs / StatExp", "IVs / DVs", "PID / ID"});
    m_partyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_partyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partyTable->setMinimumHeight(160);

    layout->addWidget(m_partyTable);

    scrollArea->setWidget(contentWidget);
    rootLayout->addWidget(scrollArea);

    connect(m_openFileBtn, &QPushButton::clicked, this, &DiagnosticsWidget::onOpenFileClicked);
    connect(m_selectCompanionBtn, &QPushButton::clicked, this, &DiagnosticsWidget::onSelectCompanionClicked);
    connect(m_timingBarWidget, &TrainingTimingBarWidget::trainingCompleted, this, &DiagnosticsWidget::onTrainingCompleted);

    m_ipcServer.start();
}

void DiagnosticsWidget::onOpenFileClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Save File", "", "Save Files (*.sav *.sa1 *.bin);;All Files (*)");
    if (!file.isEmpty()) {
        loadAndInspectSave(file);
    }
}

void DiagnosticsWidget::loadAndInspectSave(const QString& saveFilePath) {
    QFileInfo info(saveFilePath);
    qint64 size = info.size();

    if (size == 32768) {
        // Try Gen 1 first, then Gen 2
        Pocket::Save::Gen1SaveParser gen1Parser;
        m_lastParseResult = gen1Parser.parseSaveFile(saveFilePath.toStdString());

        if (m_lastParseResult.status != Pocket::Save::SaveParseStatus::Success) {
            Pocket::Save::Gen2SaveParser gen2Parser;
            m_lastParseResult = gen2Parser.parseSaveFile(saveFilePath.toStdString());
        }
    } else if (size == 524288) {
        // Try Gen 4 first, then Gen 5
        Pocket::Save::Gen4SaveParser gen4Parser;
        m_lastParseResult = gen4Parser.parseSaveFile(saveFilePath.toStdString());

        if (m_lastParseResult.status != Pocket::Save::SaveParseStatus::Success) {
            Pocket::Save::Gen5SaveParser gen5Parser;
            m_lastParseResult = gen5Parser.parseSaveFile(saveFilePath.toStdString());
        }
    } else {
        // Gen III 128KB Flash
        m_lastParseResult = m_parser.parseSaveFile(saveFilePath.toStdString());
    }

    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    if (m_lastParseResult.status != Pocket::Save::SaveParseStatus::Success) {
        m_statusLabel->setText(QString("Failed: %1").arg(QString::fromStdString(m_lastParseResult.errorMessage)));
        m_statusLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::textDisabled().name()));
        m_trainerLabel->setText(QStringLiteral("Trainer: --"));
        m_partyTable->setRowCount(0);
        m_selectCompanionBtn->setEnabled(false);
        return;
    }

    QString slotName = (m_lastParseResult.activeSlotIndex == 0) ? "Slot A / Primary" : "Slot B / Backup";
    m_statusLabel->setText(QString("Active Slot: %1 | Save Counter: %2 | Checksums: VALID")
        .arg(slotName).arg(m_lastParseResult.saveCounter));
    m_statusLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::accent().name()));

    m_trainerLabel->setText(QString("Trainer: %1 (ID: %2) | Saved Party Count: %3")
        .arg(QString::fromStdString(m_lastParseResult.trainerName))
        .arg(m_lastParseResult.trainerId)
        .arg(m_lastParseResult.party.size()));

    m_partyTable->setRowCount(0);
    for (size_t i = 0; i < m_lastParseResult.party.size(); ++i) {
        const auto& pkmn = m_lastParseResult.party[i];
        int row = m_partyTable->rowCount();
        m_partyTable->insertRow(row);

        auto* slotItem = new QTableWidgetItem(QString::number(i + 1));
        slotItem->setFont(monoFont);
        slotItem->setTextAlignment(Qt::AlignCenter);
        m_partyTable->setItem(row, 0, slotItem);

        m_partyTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(pkmn.speciesName)));
        m_partyTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(pkmn.nickname)));

        auto* lvlItem = new QTableWidgetItem(QString::number(pkmn.level));
        lvlItem->setFont(monoFont);
        lvlItem->setTextAlignment(Qt::AlignCenter);
        m_partyTable->setItem(row, 3, lvlItem);

        if (pkmn.generation >= Pocket::Save::GenerationType::Gen3) {
            m_partyTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(Pocket::Save::natureToString(pkmn.nature))));

            auto* evItem = new QTableWidgetItem(QString("EV: %1/%2/%3").arg(pkmn.evs.hp).arg(pkmn.evs.attack).arg(pkmn.evs.defense));
            evItem->setFont(monoFont);
            m_partyTable->setItem(row, 5, evItem);

            auto* ivItem = new QTableWidgetItem(QString("IV: %1/%2/%3").arg(pkmn.ivs.hp).arg(pkmn.ivs.attack).arg(pkmn.ivs.defense));
            ivItem->setFont(monoFont);
            m_partyTable->setItem(row, 6, ivItem);

            auto* pidItem = new QTableWidgetItem(QString("0x%1").arg(pkmn.personalityValue, 8, 16, QChar('0')).toUpper());
            pidItem->setFont(monoFont);
            m_partyTable->setItem(row, 7, pidItem);
        } else {
            m_partyTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(Pocket::Save::generationTypeToString(pkmn.generation))));

            auto* expItem = new QTableWidgetItem(QString("Exp: %1/%2/%3").arg(pkmn.statExp.hp).arg(pkmn.statExp.attack).arg(pkmn.statExp.defense));
            expItem->setFont(monoFont);
            m_partyTable->setItem(row, 5, expItem);

            auto* dvItem = new QTableWidgetItem(QString("DV: %1/%2/%3").arg(pkmn.dvs.hp).arg(pkmn.dvs.attack).arg(pkmn.dvs.defense));
            dvItem->setFont(monoFont);
            m_partyTable->setItem(row, 6, dvItem);

            auto* idItem = new QTableWidgetItem(QString("ID: %1").arg(pkmn.trainer.trainerId));
            idItem->setFont(monoFont);
            m_partyTable->setItem(row, 7, idItem);
        }
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

    m_activeCompanionLabel->setText(QString("Active Companion: %1 (%2) [Level %3] [%4]")
        .arg(QString::fromStdString(m_currentLink.nickname))
        .arg(QString::fromStdString(m_currentLink.speciesName))
        .arg(m_currentLink.level)
        .arg(QString::fromStdString(Pocket::Save::generationTypeToString(selectedPkmn.generation))));
    m_activeCompanionLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: %1; background: transparent;").arg(Theme::accent().name()));

    if (selectedPkmn.hasFriendship) {
        m_bondVsFriendshipLabel->setText(QString("App Bond (PocketPartner XP): Lv 1 | Game Friendship (Canonical): %1")
            .arg(m_currentLink.gameFriendship));
    } else {
        m_bondVsFriendshipLabel->setText("App Bond (PocketPartner XP): Lv 1 | Game Friendship: N/A (Gen I)");
    }
    m_bondVsFriendshipLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; background: transparent;").arg(Theme::textSecondary().name()));

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

    if (!m_lastParseResult.party.empty() && m_lastParseResult.party[0].generation != Pocket::Save::GenerationType::Gen3) {
        QMessageBox::information(
            this,
            "Game-Save Training Notice",
            "Game-save training is not yet supported for this generation (Read-Only Mode active).\nApp-only companion bond and XP progression remain fully active!"
        );
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
    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    auto rewards = m_ledger.getPendingRewards(m_currentLink.gameId);
    m_ledgerTable->setRowCount(0);

    for (const auto& r : rewards) {
        int row = m_ledgerTable->rowCount();
        m_ledgerTable->insertRow(row);

        auto* idItem = new QTableWidgetItem(QString::number(r.rewardId));
        idItem->setFont(monoFont);
        idItem->setTextAlignment(Qt::AlignCenter);
        m_ledgerTable->setItem(row, 0, idItem);

        m_ledgerTable->setItem(row, 1, new QTableWidgetItem("EV Reward"));
        m_ledgerTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(Pocket::Save::evTypeToString(r.evStat))));

        auto* amountItem = new QTableWidgetItem(QString("+%1 EV").arg(r.amount));
        amountItem->setFont(monoFont);
        amountItem->setTextAlignment(Qt::AlignCenter);
        m_ledgerTable->setItem(row, 3, amountItem);

        m_ledgerTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(r.sourceAction)));
    }
}

} // namespace Pocket::App
