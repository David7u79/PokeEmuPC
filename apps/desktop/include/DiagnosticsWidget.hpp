#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <memory>
#include "pocket/save/Gen3SaveParser.hpp"

namespace Pocket::App {

class DiagnosticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsWidget(QWidget *parent = nullptr);

    void loadAndInspectSave(const QString& saveFilePath);

private slots:
    void onOpenFileClicked();

private:
    QLabel *m_statusLabel{nullptr};
    QLabel *m_trainerLabel{nullptr};
    QTableWidget *m_partyTable{nullptr};
    QPushButton *m_openFileBtn{nullptr};

    Pocket::Save::Gen3SaveParser m_parser;
};

} // namespace Pocket::App
