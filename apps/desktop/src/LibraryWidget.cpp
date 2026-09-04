#include "LibraryWidget.hpp"

#include "GameArtworkLoader.hpp"
#include "GameCardDelegate.hpp"
#include "GameInspector.hpp"
#include "ArtworkPickerDialog.hpp"
#include "EmptyStateWidget.hpp"
#include "Icons.hpp"
#include "LibrarySidebar.hpp"
#include "Theme.hpp"

#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHash>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QShortcut>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <limits>

namespace {

enum GameRoles {
    GameIdRole = Qt::UserRole + 1,
    SystemRole,
    ArtworkRole,
    ImportedRole,
    RomPathRole,
    FileSizeRole,
    OrderRole
};

QString systemForCategory(const QString& category)
{
    if (category == "GB" || category == "GBC" || category == "GBA" || category == "NDS") return category;
    return {};
}

class GameProxyModel final : public QSortFilterProxyModel {
public:
    QString category{"Todos"};
    enum class SortMode { Personal, Title, Recent };
    SortMode sortMode{SortMode::Title};

    void refresh()
    {
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(row, parent)) return false;
        const QModelIndex index = sourceModel()->index(row, 0, parent);
        if (category == "Todos") return true;
        if (category == "Recientes") {
            QList<qint64> dates;
            for (int sourceRow = 0; sourceRow < sourceModel()->rowCount(); ++sourceRow) {
                dates.append(sourceModel()->index(sourceRow, 0).data(ImportedRole).toLongLong());
            }
            std::sort(dates.begin(), dates.end(), std::greater<qint64>());
            return dates.size() <= 12 || index.data(ImportedRole).toLongLong() >= dates.at(11);
        }
        return index.data(SystemRole).toString() == systemForCategory(category);
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        if (sortMode == SortMode::Personal) {
            return sourceModel()->data(left, OrderRole).toLongLong()
                   < sourceModel()->data(right, OrderRole).toLongLong();
        }
        if (sortMode == SortMode::Recent) {
            return sourceModel()->data(left, ImportedRole).toLongLong()
                   > sourceModel()->data(right, ImportedRole).toLongLong();
        }
        return QString::localeAwareCompare(sourceModel()->data(left).toString(), sourceModel()->data(right).toString()) < 0;
    }
};

} // namespace

namespace Pocket::App {

LibraryWidget::LibraryWidget(std::shared_ptr<Storage::GameRepository> repo, QWidget* parent,
                             QString settingsOrganization, QString settingsApplication)
    : QWidget(parent)
    , m_repo(std::move(repo))
    , m_settingsOrganization(std::move(settingsOrganization))
    , m_settingsApplication(std::move(settingsApplication))
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(12);
    auto* header = new QHBoxLayout;
    auto* title = new QLabel("My Games", this);
    QFont titleFont = title->font();
    titleFont.setWeight(QFont::DemiBold);
    titleFont.setPixelSize(15);
    title->setFont(titleFont);
    title->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textPrimary().name()));
    // The count already lives next to "All Games" in the sidebar. Kept as a hidden
    // widget so the status plumbing stays, shown nowhere.
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("libraryStatus");
    m_statusLabel->setVisible(false);
    m_search = new QLineEdit(this);
    m_search->setObjectName("librarySearch");
    m_search->setPlaceholderText("Buscar…");
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(240);
    m_search->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_search->addAction(Icons::icon(Icons::Name::Search, Theme::textSecondary()), QLineEdit::LeadingPosition);
    m_sortOrder = new QComboBox(this);
    m_sortOrder->setObjectName("sortOrder");
    m_sortOrder->setToolTip("Ordenar juegos");
    m_sortOrder->addItems({"Personalizado", "Título (A-Z)", "Añadido recientemente"});
    for (int item = 0; item < m_sortOrder->count(); ++item) {
        m_sortOrder->setItemIcon(item, Icons::icon(Icons::Name::Sort, Theme::textSecondary()));
    }
    if (QSettings(m_settingsOrganization, m_settingsApplication).value("library/order").toStringList().isEmpty()) {
        m_sortOrder->setCurrentIndex(1);
    }
    m_addButton = new QPushButton("Añadir juego", this);
    m_addButton->setObjectName("addGameButton");
    m_addButton->setIcon(Icons::icon(Icons::Name::Plus, Theme::accent(), 15));
    m_addButton->setToolTip("Añadir un juego a la biblioteca");
    // An explicit action, but quieter than Play: accent outline, not a filled slab.
    m_addButton->setStyleSheet(QString("QPushButton#addGameButton { background: %1; color: %2; border: 1px solid %3;"
                                       " border-radius: 6px; padding: 6px 12px; font-weight: 600; }"
                                       "QPushButton#addGameButton:hover { background: %4; color: white; border-color: %4; }")
                                    .arg(Theme::rgba(Theme::surfaceControl()), Theme::accent().name(),
                                         Theme::rgba(Theme::borderHover()), Theme::accent().name()));
    header->addWidget(title);
    header->addWidget(m_statusLabel);
    header->addSpacing(16);
    header->addWidget(m_search, 1);
    header->addWidget(m_sortOrder);

    // The bare slider read as a volume control. Framed by two grid icons and with
    // tooltips it says what it changes without any label.
    m_cardZoom = new QSlider(Qt::Horizontal, this);
    m_cardZoom->setObjectName("cardZoom");
    m_cardZoom->setRange(120, 240);
    m_cardZoom->setValue(QSettings(m_settingsOrganization, m_settingsApplication).value("library/cardWidth", 176).toInt());
    m_cardZoom->setFixedWidth(84);
    m_cardZoom->setToolTip("Tamaño de carátulas");
    auto* smallCovers = new QLabel(this);
    smallCovers->setPixmap(Icons::pixmap(Icons::Name::GridSmall, Theme::textSecondary(), 15));
    smallCovers->setToolTip("Carátulas más pequeñas");
    auto* largeCovers = new QLabel(this);
    largeCovers->setPixmap(Icons::pixmap(Icons::Name::GridLarge, Theme::textSecondary(), 15));
    largeCovers->setToolTip("Carátulas más grandes");
    // The three pieces read as one control instead of a slider floating alone.
    auto* zoomGroup = new QWidget(this);
    zoomGroup->setObjectName("coverSizeGroup");
    zoomGroup->setStyleSheet(QString("QWidget#coverSizeGroup { background: %1; border: 1px solid %2; border-radius: 6px; }"
                                     "QWidget#coverSizeGroup:hover { border: 1px solid %3; }")
                                 .arg(Theme::rgba(Theme::surfaceControl()), Theme::rgba(Theme::borderSubtle()),
                                      Theme::rgba(Theme::borderHover())));
    auto* zoomRow = new QHBoxLayout(zoomGroup);
    zoomRow->setContentsMargins(9, 4, 9, 4);
    zoomRow->setSpacing(8);
    zoomRow->addWidget(smallCovers);
    zoomRow->addWidget(m_cardZoom);
    zoomRow->addWidget(largeCovers);
    header->addWidget(zoomGroup);
    header->addWidget(m_addButton);
    mainLayout->addLayout(header);

    m_model = new QStandardItemModel(this);
    m_proxy = new GameProxyModel;
    m_proxy->setParent(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(0);

    auto* content = new QHBoxLayout;
    content->setSpacing(14);
    m_categories = new LibrarySidebar(this);
    content->addWidget(m_categories);

    auto* center = new QVBoxLayout;
    m_grid = new QListView(this);
    m_grid->setObjectName("gameGrid");
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Snap);
    m_grid->setUniformItemSizes(true);
    m_grid->setSpacing(18);
    m_grid->setContentsMargins(4, 4, 4, 4);
    // No frame. A barely-there surface, one step off the window, so the library
    // reads as a place rather than a hole.
    m_grid->setFrameShape(QFrame::NoFrame);
    m_grid->setStyleSheet("QListView#gameGrid { background: rgba(255, 255, 255, 0.014); border: none; border-radius: 10px; }");
    m_grid->viewport()->setAutoFillBackground(false);
    m_grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    m_grid->viewport()->setCursor(Qt::PointingHandCursor);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setWordWrap(true);
    m_grid->setMouseTracking(true);
    m_grid->viewport()->setMouseTracking(true);
    m_grid->setModel(m_proxy);
    m_grid->setAcceptDrops(false);
    m_grid->setDragEnabled(false);
    m_delegate = new GameCardDelegate(m_grid);
    m_grid->setItemDelegate(m_delegate);
    center->addWidget(m_grid);
    m_emptyState = new EmptyStateWidget(this);
    center->addWidget(m_emptyState);
    content->addLayout(center, 1);

    m_inspector = new GameInspector(this);
    // Contextual, not a permanent column: with nothing selected the library gets
    // the whole width instead of a panel explaining that nothing is selected.
    m_inspector->setVisible(false);
    content->addWidget(m_inspector);
    mainLayout->addLayout(content, 1);

    m_artworkLoader = new GameArtworkLoader(this);
    setAcceptDrops(true);
    connect(m_addButton, &QPushButton::clicked, this, &LibraryWidget::onAddGameClicked);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setFilterFixedString(text);
        updateStatus();
        updateEmptyState();
    });
    connect(m_categories, &LibrarySidebar::categoryChanged, this, [this] { applyFilters(); });
    connect(m_emptyState, &EmptyStateWidget::actionRequested, this, [this] {
        if (m_model->rowCount() == 0) {
            onAddGameClicked();
        } else {
            m_search->clear();
        }
    });
    connect(m_sortOrder, &QComboBox::currentIndexChanged, this, [this] { applyFilters(); });
    connect(m_grid, &QListView::activated, this, &LibraryWidget::playGame);
    connect(m_grid, &QListView::doubleClicked, this, &LibraryWidget::playGame);
    connect(m_grid->selectionModel(), &QItemSelectionModel::currentChanged, this, &LibraryWidget::updateDetail);
    connect(m_inspector, &GameInspector::playRequested, this, [this] { playGame(m_grid->currentIndex()); });
    connect(m_inspector, &GameInspector::changeArtworkRequested, this, &LibraryWidget::changeArtwork);
    connect(m_inspector, &GameInspector::chooseImageRequested, this, &LibraryWidget::chooseArtworkImage);
    connect(m_inspector, &GameInspector::removeRequested, this, &LibraryWidget::removeSelectedGame);
    connect(m_inspector, &GameInspector::openLocationRequested, this, &LibraryWidget::openRomLocation);
    connect(new QShortcut(QKeySequence::Delete, m_grid), &QShortcut::activated, this, &LibraryWidget::removeSelectedGame);
    connect(m_cardZoom, &QSlider::valueChanged, this, [this](int value) {
        applyCardZoom(value);
        QSettings(m_settingsOrganization, m_settingsApplication).setValue("library/cardWidth", value);
    });
    applyCardZoom(m_cardZoom->value());
    connect(m_grid, &QWidget::customContextMenuRequested, this, &LibraryWidget::showCardContextMenu);
    connect(m_artworkLoader, &GameArtworkLoader::artworkReady, this, [this](const QString& id, const QString& path) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const QModelIndex index = m_model->index(row, 0);
            if (index.data(GameIdRole).toString() == id) m_model->setData(index, path, ArtworkRole);
        }
        // QImageReader reads the header only: the cell can follow the real artwork
        // without decoding anything.
        const QSize cover = QImageReader(path).size();
        if (cover.isValid() && cover.width() > 0) {
            const double aspect = double(cover.height()) / cover.width();
            if (aspect > m_tallestCover + 0.005) {
                m_tallestCover = aspect;
                m_delegate->setCoverAspect(aspect);
                m_grid->doItemsLayout();
            }
        }
        updateDetail(m_grid->currentIndex());
    });
    connect(m_proxy, &QAbstractItemModel::rowsMoved, this, [this] { savePersonalOrder(); });
    refreshLibrary();
}

void LibraryWidget::applyFilters()
{
    auto* proxy = static_cast<GameProxyModel*>(m_proxy);
    proxy->category = m_categories->currentCategory();
    proxy->sortMode = m_sortOrder->currentIndex() == 0 ? GameProxyModel::SortMode::Personal
        : m_sortOrder->currentIndex() == 2 ? GameProxyModel::SortMode::Recent
                                           : GameProxyModel::SortMode::Title;
    proxy->refresh();
    m_proxy->sort(0);
    const bool personal = proxy->sortMode == GameProxyModel::SortMode::Personal;
    m_grid->setDragDropMode(personal ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
    m_grid->setDefaultDropAction(Qt::MoveAction);
    m_grid->setDragEnabled(personal);
    m_grid->setAcceptDrops(personal);
    updateStatus();
    updateEmptyState();
}

void LibraryWidget::updateEmptyState()
{
    const bool libraryEmpty = m_model->rowCount() == 0;
    const bool noResults = !libraryEmpty && m_proxy->rowCount() == 0;
    m_grid->setVisible(!libraryEmpty && !noResults);
    m_emptyState->setVisible(libraryEmpty || noResults);
    if (libraryEmpty) {
        m_emptyState->setState("Tu biblioteca está vacía", "Añade una ROM de Pokémon para empezar tu colección.", "+ Añadir juego");
    } else if (noResults) {
        m_emptyState->setState(QString("Sin resultados para «%1»").arg(m_search->text()), "Prueba con otro nombre o cambia de categoría.", "Limpiar búsqueda");
    }
}

void LibraryWidget::updateStatus()
{
    const int total = m_model->rowCount();
    const int shown = m_proxy->rowCount();
    Q_UNUSED(shown);
    m_statusLabel->setText(QString::number(total));
}

void LibraryWidget::updateCategoryCounts()
{
    QHash<QString, int> counts;
    for (const QString& category : {"Todos", "Recientes", "GB", "GBC", "GBA", "NDS"}) {
        int count = 0;
        for (int gameRow = 0; gameRow < m_model->rowCount(); ++gameRow) {
            if (category == "Todos" || category == "Recientes" || m_model->index(gameRow, 0).data(SystemRole).toString() == systemForCategory(category)) ++count;
        }
        if (category == "Recientes") count = qMin(count, 12);
        counts.insert(category, count);
    }
    m_categories->setCounts(counts);
}

void LibraryWidget::refreshLibrary()
{
    if (!m_repo) return;
    m_model->clear();
    m_tallestCover = 0.90;
    const QStringList savedOrder = QSettings(m_settingsOrganization, m_settingsApplication).value("library/order").toStringList();
    const std::vector<Core::Game> games = m_repo->getAllGames();
    QHash<QString, int> savedPositions;
    for (int position = 0; position < savedOrder.size(); ++position) {
        savedPositions.insert(savedOrder.at(position), position);
    }
    QVector<const Core::Game*> orderedGames;
    orderedGames.reserve(static_cast<qsizetype>(games.size()));
    for (const Core::Game& game : games) {
        orderedGames.append(&game);
    }
    std::sort(orderedGames.begin(), orderedGames.end(), [&savedPositions](const Core::Game* left, const Core::Game* right) {
        const QString leftId = QString::fromStdString(left->id.toString());
        const QString rightId = QString::fromStdString(right->id.toString());
        const int leftPosition = savedPositions.value(leftId, std::numeric_limits<int>::max());
        const int rightPosition = savedPositions.value(rightId, std::numeric_limits<int>::max());
        if (leftPosition != rightPosition) return leftPosition < rightPosition;
        return left->importedAtTs > right->importedAtTs;
    });
    int order = 0;
    for (const Core::Game* gamePtr : orderedGames) {
        const Core::Game& game = *gamePtr;
        auto* item = new QStandardItem(QString::fromStdString(game.title));
        const QString id = QString::fromStdString(game.id.toString());
        const QString system = QString::fromStdString(Core::GameSystemUtils::toString(game.system));
        item->setData(id, GameIdRole);
        item->setData(system, SystemRole);
        item->setData(QString(), ArtworkRole);
        item->setData(qint64(game.importedAtTs), ImportedRole);
        item->setData(QString::fromStdString(game.romPath), RomPathRole);
        item->setData(qulonglong(game.fileSizeBytes), FileSizeRole);
        item->setData(order++, OrderRole);
        m_model->appendRow(item);
        m_artworkLoader->requestArtwork(id, item->text(), system, QString::fromStdString(game.romPath));
    }
    updateCategoryCounts();
    applyFilters();
    updateEmptyState();
    updateDetail({});
}

std::optional<Core::Game> LibraryWidget::gameForIndex(const QModelIndex& index) const
{
    if (!index.isValid() || !m_repo) return std::nullopt;
    const QString id = index.data(GameIdRole).toString();
    for (const Core::Game& game : m_repo->getAllGames()) {
        if (QString::fromStdString(game.id.toString()) == id) return game;
    }
    return std::nullopt;
}

void LibraryWidget::updateDetail(const QModelIndex& index)
{
    const auto game = gameForIndex(index);
    if (!game) {
        m_inspector->clear();
        m_inspector->setVisible(false);
        return;
    }
    m_inspector->setGame(*game, index.data(ArtworkRole).toString());
    m_inspector->setVisible(true);
}

void LibraryWidget::playGame(const QModelIndex& index)
{
    const auto game = gameForIndex(index);
    if (game) emit gameSelected(*game);
}

void LibraryWidget::removeSelectedGame()
{
    const auto game = gameForIndex(m_grid->currentIndex());
    if (!game) return;
    if (QMessageBox::question(this, "Quitar de la biblioteca", "¿Quitar este juego de la biblioteca?") == QMessageBox::Yes) {
        m_repo->deleteGame(game->id);
        refreshLibrary();
    }
}

void LibraryWidget::onAddGameClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "Import Game ROM", {}, "Supported ROMs (*.gb *.gbc *.gba *.nds);;All Files (*.*)");
    importGames({filePath});
}

void LibraryWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void LibraryWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    QStringList filePaths;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QFileInfo file(url.toLocalFile());
        if (url.isLocalFile() && QStringList({"gb", "gbc", "gba", "nds"}).contains(file.suffix(), Qt::CaseInsensitive)) {
            filePaths.append(file.absoluteFilePath());
        }
    }
    importGames(filePaths);
    event->acceptProposedAction();
}

void LibraryWidget::importGames(const QStringList& filePaths)
{
    if (!m_repo) {
        return;
    }
    bool imported = false;
    for (const QString& filePath : filePaths) {
        if (filePath.isEmpty() || m_repo->isPathAlreadyImported(filePath.toStdString())) {
            continue;
        }
        imported = m_repo->importGame(filePath.toStdString()).status == Storage::ImportResultStatus::Success || imported;
    }
    if (imported) {
        refreshLibrary();
    }
}

void LibraryWidget::applyCardZoom(int sliderValue)
{
    // Discrete steps: a continuous slider produced a different column count on
    // almost every pixel and the grid never settled into a rhythm.
    struct Step { int threshold; int width; int spacing; };
    // Roughly 25% smaller than the poster-sized first cut: a shelf, not a wall of
    // posters, and four to five games per row at a normal window width.
    static const Step steps[] = {{160, 122, 10}, {205, 150, 12}, {std::numeric_limits<int>::max(), 182, 14}};
    const Step& step = sliderValue < steps[0].threshold ? steps[0]
                     : sliderValue < steps[1].threshold ? steps[1]
                                                        : steps[2];
    m_delegate->setCardWidth(step.width);
    m_grid->setSpacing(step.spacing);
    m_grid->doItemsLayout();
}

void LibraryWidget::changeArtwork()
{
    const auto game = gameForIndex(m_grid->currentIndex());
    if (!game) {
        return;
    }
    const QString system = QString::fromStdString(Core::GameSystemUtils::toString(game->system));
    ArtworkPickerDialog dialog(QString::fromStdString(game->title), system, m_artworkLoader, this);
    if (dialog.exec() == QDialog::Accepted && !dialog.chosenName().isEmpty()) {
        m_artworkLoader->useIndexName(QString::fromStdString(game->id.toString()), system, dialog.chosenName());
    }
}

void LibraryWidget::chooseArtworkImage()
{
    const auto game = gameForIndex(m_grid->currentIndex());
    const QString path = QFileDialog::getOpenFileName(this, "Elegir imagen", {}, "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (game && !path.isEmpty()) m_artworkLoader->setArtworkFromFile(QString::fromStdString(game->id.toString()), path);
}

void LibraryWidget::openRomLocation()
{
    const auto game = gameForIndex(m_grid->currentIndex());
    if (game) {
        const QString path = QFileInfo(QString::fromStdString(game->romPath)).absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void LibraryWidget::showCardContextMenu(const QPoint& viewportPos)
{
    const QModelIndex index = m_grid->indexAt(viewportPos);
    if (!index.isValid()) return;
    m_grid->setCurrentIndex(index);

    // Same actions as the inspector's ••• menu, routed to the same slots.
    QMenu menu(this);
    const QColor menuIcon = Theme::textSecondary();
    QAction* play = menu.addAction(Icons::icon(Icons::Name::Play, menuIcon), "Jugar");
    menu.addSeparator();
    QAction* artwork = menu.addAction(Icons::icon(Icons::Name::Image, menuIcon), "Cambiar carátula");
    QAction* image = menu.addAction(Icons::icon(Icons::Name::Image, menuIcon), "Elegir imagen…");
    QAction* location = menu.addAction(Icons::icon(Icons::Name::Folder, menuIcon), "Abrir ubicación del ROM");
    menu.addSeparator();
    QAction* remove = menu.addAction(Icons::icon(Icons::Name::Trash, menuIcon), "Quitar de la biblioteca");

    const QAction* chosen = menu.exec(m_grid->viewport()->mapToGlobal(viewportPos));
    if (chosen == play) playGame(m_grid->currentIndex());
    else if (chosen == artwork) changeArtwork();
    else if (chosen == image) chooseArtworkImage();
    else if (chosen == location) openRomLocation();
    else if (chosen == remove) removeSelectedGame();
}

void LibraryWidget::savePersonalOrder()
{
    if (m_sortOrder->currentIndex() != 0) {
        return;
    }
    QStringList ids;
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        const QModelIndex proxyIndex = m_proxy->index(row, 0);
        const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
        m_model->setData(sourceIndex, row, OrderRole);
        ids.append(proxyIndex.data(GameIdRole).toString());
    }
    QSettings(m_settingsOrganization, m_settingsApplication).setValue("library/order", ids);
}

} // namespace Pocket::App
