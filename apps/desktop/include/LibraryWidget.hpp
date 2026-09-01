#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <memory>
#include "pocket/storage/GameRepository.hpp"

namespace Pocket::App {

class LibraryWidget : public QWidget {
    Q_OBJECT
public:
    explicit LibraryWidget(std::shared_ptr<Storage::GameRepository> repo, QWidget *parent = nullptr);

    void refreshLibrary();

signals:
    void gameSelected(const Core::Game& game);

private slots:
    void onAddGameClicked();

private:
    std::shared_ptr<Storage::GameRepository> m_repo;
    QTableWidget *m_table{nullptr};
    QLabel *m_statusLabel{nullptr};
    QPushButton *m_addButton{nullptr};
};

} // namespace Pocket::App
