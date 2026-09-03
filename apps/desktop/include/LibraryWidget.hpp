#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "pocket/storage/GameRepository.hpp"

namespace Pocket::App {

class GameArtworkLoader;
class LibraryWidget : public QWidget {
    Q_OBJECT
public:
    explicit LibraryWidget(std::shared_ptr<Storage::GameRepository> repo, QWidget *parent = nullptr);

    void refreshLibrary();

signals:
    void gameSelected(const Core::Game& game);

private slots:
    void onAddGameClicked();
    void playGame(const QModelIndex& index);

private:
    void updateEmptyState();
    void updateStatus();
    void applyFilters();
    std::shared_ptr<Storage::GameRepository> m_repo;
    QStandardItemModel *m_model{nullptr};
    QSortFilterProxyModel *m_proxy{nullptr};
    QListView *m_grid{nullptr};
    QLabel *m_emptyLabel{nullptr};
    QLabel *m_statusLabel{nullptr};
    QPushButton *m_addButton{nullptr};
    QLineEdit *m_search{nullptr};
    QComboBox *m_systemFilter{nullptr};
    QComboBox *m_sortOrder{nullptr};
    GameArtworkLoader *m_artworkLoader{nullptr};
};

} // namespace Pocket::App
