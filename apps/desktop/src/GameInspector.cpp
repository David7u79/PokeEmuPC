#include "GameInspector.hpp"

#include "EmptyStateWidget.hpp"
#include "Theme.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QStackedLayout>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString initials(const QString& title)
{
    QString result;
    for (const QString& word : title.split(' ', Qt::SkipEmptyParts)) {
        result += word.left(1).toUpper();
        if (result.size() == 2) {
            break;
        }
    }
    return result.isEmpty() ? QStringLiteral("?") : result;
}

QColor systemColor(const QString& system)
{
    if (system == "GB") {
        return QColor("#6b7a5a");
    }
    if (system == "GBC") {
        return QColor("#5b4b8a");
    }
    if (system == "GBA") {
        return QColor("#3c438f");
    }
    return QColor("#4e5964");
}

} // namespace

namespace Pocket::App {

class InspectorCoverWidget final : public QWidget {
public:
    explicit InspectorCoverWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }

    void setCover(const QString& title, const QString& system, const QString& artworkPath)
    {
        m_title = title;
        m_system = system;
        m_pixmap = QPixmap(artworkPath);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRect cover = rect();
        QLinearGradient background(cover.topLeft(), cover.bottomRight());
        const QColor color = systemColor(m_system);
        background.setColorAt(0, color.lighter(m_pixmap.isNull() ? 120 : 100));
        background.setColorAt(1, color.darker(m_pixmap.isNull() ? 115 : 150));
        painter.fillRect(cover, background);
        if (!m_pixmap.isNull()) {
            const QSize scaled = m_pixmap.size().scaled(cover.size() - QSize(12, 12), Qt::KeepAspectRatio);
            const QRect target(cover.center() - QPoint(scaled.width() / 2, scaled.height() / 2), scaled);
            painter.drawPixmap(target, m_pixmap);
            return;
        }
        QFont font = painter.font();
        font.setPixelSize(qMax(16, cover.width() / 3));
        font.setBold(true);
        painter.setFont(font);
        QColor textColor(Qt::white);
        textColor.setAlphaF(0.70);
        painter.setPen(textColor);
        painter.drawText(cover, Qt::AlignCenter, initials(m_title));
    }

private:
    QString m_title;
    QString m_system;
    QPixmap m_pixmap;
};

} // namespace Pocket::App

namespace {

QLabel* dataLabel(const QString& label, QWidget* parent)
{
    auto* widget = new QLabel(label, parent);
    widget->setStyleSheet(QString("color: %1;").arg(Pocket::App::Theme::textSecondary().name()));
    return widget;
}

QWidget* dataRow(const QString& label, QLabel** value, QWidget* parent)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(dataLabel(label, row));
    *value = new QLabel(row);
    (*value)->setStyleSheet(QString("color: %1;").arg(Pocket::App::Theme::textPrimary().name()));
    layout->addWidget(*value, 1, Qt::AlignRight);
    return row;
}

} // namespace

namespace Pocket::App {

GameInspector::GameInspector(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(300);
    setMaximumWidth(420);
    setStyleSheet(QString("GameInspector { background: %1; border-left: 1px solid %2; }")
                      .arg(Theme::surface().name(), Theme::border().name()));

    m_stack = new QStackedLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    auto* empty = new EmptyStateWidget(this);
    empty->setState("Ningún juego seleccionado", "Elige un juego de tu biblioteca para ver sus detalles.");
    m_stack->addWidget(empty);

    m_content = new QWidget(this);
    auto* layout = new QVBoxLayout(m_content);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    m_cover = new InspectorCoverWidget(m_content);
    m_cover->setObjectName("detailCover");
    m_cover->setMaximumHeight(300);
    m_cover->setMinimumHeight(180);
    layout->addWidget(m_cover);

    m_title = new QLabel(m_content);
    m_title->setObjectName("detailTitle");
    m_title->setWordWrap(true);
    QFont titleFont = m_title->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setStyleSheet(QString("color: %1;").arg(Theme::textPrimary().name()));
    layout->addWidget(m_title);

    m_platform = new QLabel(m_content);
    m_platform->setStyleSheet(QString("color: %1;").arg(Theme::textSecondary().name()));
    layout->addWidget(m_platform);

    m_playButton = new QPushButton("▶  JUGAR", m_content);
    m_playButton->setObjectName("playButton");
    m_playButton->setDefault(true);
    m_playButton->setMinimumHeight(46);
    m_playButton->setStyleSheet(QString("QPushButton#playButton { background: %1; color: white; border-color: %1; font-weight: bold; }"
                                         "QPushButton#playButton:hover { background: %2; border-color: %2; }")
                                      .arg(Theme::accent().name(), Theme::accentPressed().name()));
    layout->addWidget(m_playButton);

    layout->addWidget(dataRow("Partida guardada:", &m_saveValue, m_content));
    layout->addWidget(dataRow("Añadido:", &m_addedValue, m_content));
    layout->addWidget(dataRow("Tamaño:", &m_sizeValue, m_content));

    m_detailsButton = new QToolButton(m_content);
    m_detailsButton->setText("Detalles");
    m_detailsButton->setCheckable(true);
    m_detailsButton->setArrowType(Qt::RightArrow);
    m_detailsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    layout->addWidget(m_detailsButton, 0, Qt::AlignLeft);

    auto* details = new QWidget(m_content);
    auto* detailsLayout = new QVBoxLayout(details);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(4);
    m_romPath = new QLabel(details);
    m_romPath->setWordWrap(false);
    m_romPath->setStyleSheet(QString("color: %1;").arg(Theme::textSecondary().name()));
    m_sha256 = new QLabel(details);
    m_sha256->setStyleSheet(QString("color: %1;").arg(Theme::textSecondary().name()));
    detailsLayout->addWidget(m_romPath);
    detailsLayout->addWidget(m_sha256);
    details->setVisible(false);
    layout->addWidget(details);
    connect(m_detailsButton, &QToolButton::toggled, details, [this, details](bool expanded) {
        m_detailsButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        details->setVisible(expanded);
    });

    auto* actionsRow = new QHBoxLayout;
    actionsRow->addStretch();
    m_moreActionsButton = new QPushButton("•••", m_content);
    m_moreActionsButton->setObjectName("moreActionsButton");
    m_moreActionsButton->setToolTip("Más acciones");
    actionsRow->addWidget(m_moreActionsButton);
    layout->addLayout(actionsRow);
    layout->addStretch();
    m_stack->addWidget(m_content);

    auto* menu = new QMenu(m_moreActionsButton);
    QAction* changeArtwork = menu->addAction("Cambiar carátula");
    QAction* chooseImage = menu->addAction("Elegir imagen…");
    QAction* openLocation = menu->addAction("Abrir ubicación del ROM");
    menu->addSeparator();
    QAction* remove = menu->addAction("Quitar de la biblioteca");
    m_moreActionsButton->setMenu(menu);
    connect(m_playButton, &QPushButton::clicked, this, &GameInspector::playRequested);
    connect(changeArtwork, &QAction::triggered, this, &GameInspector::changeArtworkRequested);
    connect(chooseImage, &QAction::triggered, this, &GameInspector::chooseImageRequested);
    connect(openLocation, &QAction::triggered, this, &GameInspector::openLocationRequested);
    connect(remove, &QAction::triggered, this, &GameInspector::removeRequested);

    clear();
}

void GameInspector::setGame(const Core::Game& game, const QString& artworkPath)
{
    const QString title = QString::fromStdString(game.title);
    const QString system = QString::fromStdString(Core::GameSystemUtils::toString(game.system));
    const QString romPath = QString::fromStdString(game.romPath);
    const QFileInfo romFile(romPath);
    const QString savePath = romFile.absolutePath() + "/" + romFile.completeBaseName() + ".sav";
    m_cover->setCover(title, system, artworkPath);
    m_title->setText(title);
    m_platform->setText(system);
    m_saveValue->setText(QFileInfo::exists(savePath) ? "Detectada" : "No detectada");
    m_addedValue->setText(QDateTime::fromSecsSinceEpoch(game.importedAtTs).date().toString("dd/MM/yyyy"));
    m_sizeValue->setText(QString::number(game.fileSizeBytes / (1024.0 * 1024.0), 'f', 1) + " MB");
    m_romPath->setText(fontMetrics().elidedText(romPath, Qt::ElideMiddle, width() - 36));
    m_romPath->setToolTip(romPath);
    m_sha256->setText("SHA-256: " + QString::fromStdString(game.sha256).left(12));
    m_stack->setCurrentWidget(m_content);
}

void GameInspector::clear()
{
    m_detailsButton->setChecked(false);
    m_stack->setCurrentIndex(0);
}

} // namespace Pocket::App
