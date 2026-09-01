#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <memory>
#include "pocket/save/Gen3SaveParser.hpp"
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/core/IpcServer.hpp"

namespace Pocket::App {

class DiagnosticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsWidget(QWidget *parent = nullptr);

    void loadAndInspectSave(const QString& saveFilePath);

private slots:
    void onOpenFileClicked();
    void onSelectCompanionClicked();

private:
    QLabel *m_statusLabel{nullptr};
    QLabel *m_trainerLabel{nullptr};
    QLabel *m_activeCompanionLabel{nullptr};
    QTableWidget *m_partyTable{nullptr};
    QPushButton *m_openFileBtn{nullptr};
    QPushButton *m_selectCompanionBtn{nullptr};

    Pocket::Save::Gen3SaveParser m_parser;
    Pocket::Save::SaveParseResult m_lastParseResult;
    Pocket::Companion::CompanionLink m_currentLink;

    Pocket::Core::IpcServer m_ipcServer;
};

} // namespace Pocket::App
