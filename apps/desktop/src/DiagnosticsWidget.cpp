#include "DiagnosticsWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QHeaderView>

namespace Pocket::App {

DiagnosticsWidget::DiagnosticsWidget(QWidget *parent)
    : QWidget(parent) {
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

    QGroupBox *trainerGroup = new QGroupBox("Trainer Information", this);
    QVBoxLayout *trainerLayout = new QVBoxLayout(trainerGroup);
    m_trainerLabel = new QLabel("Trainer: -- | Play Time: --", trainerGroup);
    trainerLayout->addWidget(m_trainerLabel);

    QGroupBox *partyGroup = new QGroupBox("Parsed Party Pokémon", this);
    QVBoxLayout *partyLayout = new QVBoxLayout(partyGroup);

    m_partyTable = new QTableWidget(0, 8, partyGroup);
    m_partyTable->setHorizontalHeaderLabels({"Slot", "Species", "Nickname", "Level", "Nature", "EVs (HP/Atk/Def)", "IVs (HP/Atk/Def)", "PID"});
    m_partyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_partyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    partyLayout->addWidget(m_partyTable);

    layout->addWidget(headerGroup);
    layout->addWidget(trainerGroup);
    layout->addWidget(partyGroup);

    connect(m_openFileBtn, &QPushButton::clicked, this, &DiagnosticsWidget::onOpenFileClicked);
}

void DiagnosticsWidget::onOpenFileClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Gen III Save File", "", "Save Files (*.sav *.sa1 *.bin);;All Files (*)");
    if (!file.isEmpty()) {
        loadAndInspectSave(file);
    }
}

void DiagnosticsWidget::loadAndInspectSave(const QString& saveFilePath) {
    Pocket::Save::SaveParseResult result = m_parser.parseSaveFile(saveFilePath.toStdString());

    if (result.status != Pocket::Save::SaveParseStatus::Success) {
        m_statusLabel->setText(QString("Failed: %1").arg(QString::fromStdString(result.errorMessage)));
        m_statusLabel->setStyleSheet("font-weight: bold; color: #FF5252;");
        m_trainerLabel->setText("Trainer: --");
        m_partyTable->setRowCount(0);
        return;
    }

    QString slotName = (result.activeSlotIndex == 0) ? "Slot A (0x00000)" : "Slot B (0x0E000)";
    m_statusLabel->setText(QString("Active Slot: %1 | Save Counter: %2 | Checksums: VALID")
        .arg(slotName).arg(result.saveCounter));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");

    m_trainerLabel->setText(QString("Trainer: %1 (ID: %2, Secret: %3) | Play Time: %4h %5m")
        .arg(QString::fromStdString(result.trainerName))
        .arg(result.trainerId)
        .arg(result.secretId)
        .arg(result.playTimeHours)
        .arg(result.playTimeMinutes));

    m_partyTable->setRowCount(0);
    for (size_t i = 0; i < result.party.size(); ++i) {
        const auto& pkmn = result.party[i];
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
}

} // namespace Pocket::App
