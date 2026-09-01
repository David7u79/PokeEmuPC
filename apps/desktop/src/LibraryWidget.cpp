#include "LibraryWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>

namespace Pocket::App {

LibraryWidget::LibraryWidget(std::shared_ptr<Storage::GameRepository> repo, QWidget *parent)
    : QWidget(parent), m_repo(std::move(repo)) {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Header bar
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("<h2>Game Library</h2>", this);
    m_addButton = new QPushButton("Add Game", this);
    m_addButton->setStyleSheet("background-color: #2b5c8f; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_addButton);
    mainLayout->addLayout(headerLayout);

    // Table widget
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Title", "System", "Source", "SHA-256", "Path"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    mainLayout->addWidget(m_table);

    m_statusLabel = new QLabel("0 games in library.", this);
    mainLayout->addWidget(m_statusLabel);

    connect(m_addButton, &QPushButton::clicked, this, &LibraryWidget::onAddGameClicked);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* item) {
        if (!item || !m_repo) return;
        const auto games = m_repo->getAllGames();
        const int row = item->row();
        if (row >= 0 && row < static_cast<int>(games.size())) emit gameSelected(games[static_cast<size_t>(row)]);
    });

    refreshLibrary();
}

void LibraryWidget::refreshLibrary() {
    if (!m_repo) return;

    auto games = m_repo->getAllGames();
    m_table->setRowCount(static_cast<int>(games.size()));

    for (int i = 0; i < static_cast<int>(games.size()); ++i) {
        const auto& g = games[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(g.title)));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(Core::GameSystemUtils::toString(g.system))));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(Core::GameSourceUtils::toString(g.source))));

        QString shortHash = QString::fromStdString(g.sha256).left(12) + "...";
        m_table->setItem(i, 3, new QTableWidgetItem(shortHash));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(g.romPath)));
    }

    m_statusLabel->setText(QString("%1 game(s) imported in library.").arg(games.size()));
}

void LibraryWidget::onAddGameClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Import Game ROM",
        "",
        "Supported ROMs (*.gb *.gbc *.gba *.nds);;Game Boy (*.gb);;Game Boy Color (*.gbc);;Game Boy Advance (*.gba);;Nintendo DS (*.nds);;All Files (*.*)"
    );

    if (filePath.isEmpty()) return;

    auto result = m_repo->importGame(filePath.toStdString());
    if (result.status == Storage::ImportResultStatus::Success) {
        refreshLibrary();
        QMessageBox::information(this, "Import Successful", QString("Successfully imported %1.").arg(QString::fromStdString(result.game->title)));
    } else {
        QMessageBox::warning(this, "Import Failed", QString::fromStdString(result.errorMessage));
    }
}

} // namespace Pocket::App
