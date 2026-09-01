#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <memory>
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/save/PendingGameReward.hpp"
#include "pocket/core/IpcServer.hpp"
#include "TrainingTimingBarWidget.hpp"

namespace Pocket::App {

class DiagnosticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsWidget(QWidget *parent = nullptr);

    void loadAndInspectSave(const QString& saveFilePath);

private slots:
    void onOpenFileClicked();
    void onSelectCompanionClicked();
    void onTrainingCompleted(Pocket::Save::EVType stat, int evPoints, double qualityScore);

private:
    void refreshLedgerDisplay();

    QLabel *m_statusLabel{nullptr};
    QLabel *m_trainerLabel{nullptr};
    QLabel *m_activeCompanionLabel{nullptr};
    QLabel *m_bondVsFriendshipLabel{nullptr};

    QTableWidget *m_partyTable{nullptr};
    QTableWidget *m_ledgerTable{nullptr};

    QPushButton *m_openFileBtn{nullptr};
    QPushButton *m_selectCompanionBtn{nullptr};

    TrainingTimingBarWidget *m_timingBarWidget{nullptr};

    Pocket::Save::Gen3SaveParser m_parser;
    Pocket::Save::SaveParseResult m_lastParseResult;
    Pocket::Companion::CompanionLink m_currentLink;
    Pocket::Save::PendingRewardLedger m_ledger;

    Pocket::Core::IpcServer m_ipcServer;
};

} // namespace Pocket::App
