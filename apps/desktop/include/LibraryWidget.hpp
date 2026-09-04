#pragma once

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSlider>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QWidget>

#include <memory>

#include "pocket/storage/GameRepository.hpp"

namespace Pocket::App {

class GameArtworkLoader;
class GameCardDelegate;
class LibrarySidebar;
class EmptyStateWidget;
class GameInspector;

class LibraryWidget : public QWidget {
    Q_OBJECT

public:
    explicit LibraryWidget(std::shared_ptr<Storage::GameRepository> repo, QWidget* parent = nullptr,
                           QString settingsOrganization = "PocketPartnerProject",
                           QString settingsApplication = "PocketPartner");

    void refreshLibrary();

signals:
    void gameSelected(const Core::Game& game);

private slots:
    void onAddGameClicked();
    void playGame(const QModelIndex& index);
    void updateDetail(const QModelIndex& index);
    void removeSelectedGame();

private:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void updateEmptyState();
    void updateStatus();
    void updateCategoryCounts();
    void applyFilters();
    void importGames(const QStringList& filePaths);
    void applyCardZoom(int sliderValue);
    void changeArtwork();
    void chooseArtworkImage();
    void openRomLocation();
    void showCardContextMenu(const QPoint& viewportPos);
    void savePersonalOrder();
    std::optional<Core::Game> gameForIndex(const QModelIndex& index) const;

    std::shared_ptr<Storage::GameRepository> m_repo;
    QStandardItemModel* m_model{nullptr};
    QSortFilterProxyModel* m_proxy{nullptr};
    QListView* m_grid{nullptr};
    LibrarySidebar* m_categories{nullptr};
    EmptyStateWidget* m_emptyState{nullptr};
    QLabel* m_statusLabel{nullptr};
    QPushButton* m_addButton{nullptr};
    GameInspector* m_inspector{nullptr};
    QLineEdit* m_search{nullptr};
    QComboBox* m_sortOrder{nullptr};
    QSlider* m_cardZoom{nullptr};
    GameCardDelegate* m_delegate{nullptr};
    GameArtworkLoader* m_artworkLoader{nullptr};
    double m_tallestCover{0.90};
    QString m_settingsOrganization;
    QString m_settingsApplication;
};

} // namespace Pocket::App
