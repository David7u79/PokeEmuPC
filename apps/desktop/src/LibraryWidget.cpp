#include "LibraryWidget.hpp"

#include "GameArtworkLoader.hpp"
#include "GameCardDelegate.hpp"
#include "ArtworkPickerDialog.hpp"
#include "EmptyStateWidget.hpp"
#include "LibrarySidebar.hpp"
#include "Theme.hpp"

#include <QComboBox>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHash>
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

QString initials(const QString& title)
{
    QString result;
    for (const QString& word : title.split(' ', Qt::SkipEmptyParts)) {
        result += word.left(1).toUpper();
        if (result.size() == 2) break;
    }
    return result.isEmpty() ? QStringLiteral("?") : result;
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
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);
    auto* header = new QHBoxLayout;
    auto* title = new QLabel("My Games", this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    title->setFont(titleFont);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("libraryStatus");
    m_statusLabel->setStyleSheet(QString("color: %1;").arg(Theme::textSecondary().name()));
    m_search = new QLineEdit(this);
    m_search->setObjectName("librarySearch");
    m_search->setPlaceholderText("Buscar…");
    m_search->setClearButtonEnabled(true);
    m_search->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);
    m_sortOrder = new QComboBox(this);
    m_sortOrder->setObjectName("sortOrder");
    m_sortOrder->addItems({"Personalizado", "Título (A-Z)", "Añadido recientemente"});
    if (QSettings(m_settingsOrganization, m_settingsApplication).value("library/order").toStringList().isEmpty()) {
        m_sortOrder->setCurrentIndex(1);
    }
    m_addButton = new QPushButton("+ Añadir juego", this);
    m_addButton->setObjectName("addGameButton");
    m_addButton->setStyleSheet(QString("QPushButton#addGameButton { background: %1; color: %2; border-color: %1; }"
                                        "QPushButton#addGameButton:hover { background: %3; border-color: %3; }")
                                    .arg(Theme::accent().name(), Theme::surface().name(), Theme::accentPressed().name()));
    header->addWidget(title);
    header->addWidget(m_statusLabel);
    header->addStretch();
    header->addWidget(m_search);
    header->addWidget(m_sortOrder);
    m_cardZoom = new QSlider(Qt::Horizontal, this);
    m_cardZoom->setObjectName("cardZoom");
    m_cardZoom->setRange(120, 240);
    m_cardZoom->setValue(QSettings(m_settingsOrganization, m_settingsApplication).value("library/cardWidth", 176).toInt());
    m_cardZoom->setFixedWidth(100);
    header->addWidget(m_cardZoom);
    header->addWidget(m_addButton);
    mainLayout->addLayout(header);

    m_model = new QStandardItemModel(this);
    m_proxy = new GameProxyModel;
    m_proxy->setParent(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(0);

    auto* content = new QHBoxLayout;
    content->setSpacing(10);
    m_categories = new LibrarySidebar(this);
    content->addWidget(m_categories);

    auto* center = new QVBoxLayout;
    m_grid = new QListView(this);
    m_grid->setObjectName("gameGrid");
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Snap);
    m_grid->setUniformItemSizes(true);
    m_grid->setSpacing(16);
    m_grid->setContentsMargins(4, 4, 4, 4);
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

    auto* detail = new QWidget(this);
    detail->setFixedWidth(300);
    auto* detailLayout = new QVBoxLayout(detail);
    m_detailCover = new QLabel(detail);
    m_detailCover->setObjectName("detailCover");
    m_detailCover->setFixedSize(260, 260);
    m_detailCover->setAlignment(Qt::AlignCenter);
    m_detailTitle = new QLabel("Selecciona un juego", detail);
    m_detailTitle->setObjectName("detailTitle");
    m_detailTitle->setAlignment(Qt::AlignCenter);
    QFont detailTitleFont = m_detailTitle->font();
    detailTitleFont.setBold(true);
    m_detailTitle->setFont(detailTitleFont);
    m_detailInfo = new QLabel(detail);
    m_detailInfo->setWordWrap(true);
    m_detailPath = new QLabel(detail);
    m_detailPath->setWordWrap(false);
    m_playButton = new QPushButton("Jugar", detail);
    m_playButton->setDefault(true);
    m_searchArtworkButton = new QPushButton("Elegir carátula…", detail);
    m_chooseArtworkButton = new QPushButton("Elegir imagen…", detail);
    m_removeButton = new QPushButton("Quitar de la biblioteca", detail);
    detailLayout->setSpacing(6);
    detailLayout->addWidget(m_detailCover, 0, Qt::AlignHCenter);
    detailLayout->addWidget(m_detailTitle);
    detailLayout->addWidget(m_detailInfo);
    detailLayout->addWidget(m_detailPath);
    detailLayout->addWidget(m_playButton);
    detailLayout->addWidget(m_searchArtworkButton);
    detailLayout->addWidget(m_chooseArtworkButton);
    detailLayout->addWidget(m_removeButton);
    detailLayout->addStretch();
    content->addWidget(detail);
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
    connect(m_playButton, &QPushButton::clicked, this, [this] { playGame(m_grid->currentIndex()); });
    connect(m_searchArtworkButton, &QPushButton::clicked, this, [this] {
        const auto game = gameForIndex(m_grid->currentIndex());
        if (!game) {
            return;
        }
        const QString system = QString::fromStdString(Core::GameSystemUtils::toString(game->system));
        ArtworkPickerDialog dialog(QString::fromStdString(game->title), system, m_artworkLoader, this);
        if (dialog.exec() == QDialog::Accepted && !dialog.chosenName().isEmpty()) {
            m_artworkLoader->useIndexName(QString::fromStdString(game->id.toString()), system, dialog.chosenName());
        }
    });
    connect(m_chooseArtworkButton, &QPushButton::clicked, this, [this] {
        const auto game = gameForIndex(m_grid->currentIndex());
        const QString path = QFileDialog::getOpenFileName(this, "Elegir imagen", {}, "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
        if (game && !path.isEmpty()) m_artworkLoader->setArtworkFromFile(QString::fromStdString(game->id.toString()), path);
    });
    connect(m_removeButton, &QPushButton::clicked, this, &LibraryWidget::removeSelectedGame);
    connect(new QShortcut(QKeySequence::Delete, m_grid), &QShortcut::activated, this, &LibraryWidget::removeSelectedGame);
    connect(m_cardZoom, &QSlider::valueChanged, this, [this](int width) {
        m_delegate->setCardWidth(width);
        m_grid->doItemsLayout();
        QSettings(m_settingsOrganization, m_settingsApplication).setValue("library/cardWidth", width);
    });
    connect(m_artworkLoader, &GameArtworkLoader::artworkReady, this, [this](const QString& id, const QString& path) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const QModelIndex index = m_model->index(row, 0);
            if (index.data(GameIdRole).toString() == id) m_model->setData(index, path, ArtworkRole);
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
    const bool hasGame = game.has_value();
    m_playButton->setEnabled(hasGame);
    m_searchArtworkButton->setEnabled(hasGame);
    m_chooseArtworkButton->setEnabled(hasGame);
    m_removeButton->setEnabled(hasGame);
    if (!hasGame) {
        m_detailTitle->setText("Selecciona un juego");
        m_detailTitle->setStyleSheet("color: palette(mid);");
        m_detailInfo->clear();
        m_detailPath->clear();
        m_detailCover->clear();
        return;
    }
    m_detailTitle->setStyleSheet(QString());
    m_detailTitle->setText(QString::fromStdString(game->title));
    const QString imported = QDateTime::fromSecsSinceEpoch(game->importedAtTs).date().toString("dd/MM/yyyy");
    m_detailInfo->setText(QString("%1\n%2 MB\nImportado: %3").arg(QString::fromStdString(Core::GameSystemUtils::toString(game->system))).arg(game->fileSizeBytes / (1024.0 * 1024.0), 0, 'f', 1).arg(imported));
    const QString romPath = QString::fromStdString(game->romPath);
    m_detailPath->setText(fontMetrics().elidedText(romPath, Qt::ElideMiddle, 280));
    m_detailPath->setToolTip(romPath);
    QPixmap pixmap(index.data(ArtworkRole).toString());
    if (!pixmap.isNull()) {
        m_detailCover->setPixmap(pixmap.scaled(260, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_detailCover->setPixmap(QPixmap());
        m_detailCover->setText(initials(QString::fromStdString(game->title)));
    }
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
