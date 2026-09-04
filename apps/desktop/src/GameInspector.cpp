#include "GameInspector.hpp"

#include "EmptyStateWidget.hpp"
#include "Icons.hpp"
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

// One representative colour from the box art. Read from a 12x12 thumbnail once
// per selection: no filter, no blur, nothing per frame.
QColor dominantColor(const QPixmap& pixmap, const QColor& fallback)
{
    if (pixmap.isNull()) return fallback;
    const QImage sample = pixmap.toImage().scaled(12, 12, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QColor best = fallback;
    int bestScore = -1;
    for (int y = 0; y < sample.height(); ++y) {
        for (int x = 0; x < sample.width(); ++x) {
            const QColor pixel = sample.pixelColor(x, y);
            // Saturated but not near-black or blown out: white borders are common
            // on box art and would win a plain "most saturated" contest.
            const int lightness = pixel.lightness();
            if (lightness < 40 || lightness > 225) continue;
            const int score = pixel.saturation();
            if (score > bestScore) {
                bestScore = score;
                best = pixel;
            }
        }
    }
    return best;
}

QString systemDisplayName(const QString& system)
{
    if (system == "GB") return QStringLiteral("Game Boy");
    if (system == "GBC") return QStringLiteral("Game Boy Color");
    if (system == "GBA") return QStringLiteral("Game Boy Advance");
    if (system == "NDS") return QStringLiteral("Nintendo DS");
    return system;
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
        m_tint = dominantColor(m_pixmap, systemColor(system));
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRect cover = rect();
        // Contextual wash: one colour taken from the artwork, bled behind it at
        // ~7%. Enough to tie the panel to the game, far from a themed background.
        if (m_tint.isValid()) {
            QRadialGradient wash(cover.center(), cover.width() * 0.75);
            QColor centre = m_tint;
            centre.setAlphaF(0.07);
            wash.setColorAt(0.0, centre);
            centre.setAlphaF(0.0);
            wash.setColorAt(1.0, centre);
            painter.fillRect(cover, wash);
        }
        if (!m_pixmap.isNull()) {
            // No panel behind the art: same rule as the grid, the box is the object.
            const QSize scaled = m_pixmap.size().scaled(cover.size() - QSize(8, 8), Qt::KeepAspectRatio);
            const QRect target(cover.center() - QPoint(scaled.width() / 2, scaled.height() / 2), scaled);
            QColor shadow(Qt::black);
            shadow.setAlphaF(0.22);
            painter.setPen(Qt::NoPen);
            painter.setBrush(shadow);
            painter.drawRoundedRect(target.adjusted(-2, 3, 2, 6), 5, 5);
            painter.drawPixmap(target, m_pixmap);
            QColor edge(Qt::white);
            edge.setAlphaF(0.10);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(edge, 1));
            painter.drawRoundedRect(QRectF(target).adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
            return;
        }
        QLinearGradient background(cover.topLeft(), cover.bottomRight());
        const QColor color = systemColor(m_system);
        background.setColorAt(0, color.darker(130));
        background.setColorAt(1, color.darker(175));
        painter.fillRect(cover, background);
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
    QColor m_tint;
};

} // namespace Pocket::App

namespace {

QLabel* dataLabel(const QString& label, QWidget* parent)
{
    auto* widget = new QLabel(label, parent);
    // Label muted, value bright: the eye lands on the answer, not on the question.
    widget->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;")
                              .arg(Pocket::App::Theme::textDisabled().name()));
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
    (*value)->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;")
                                .arg(Pocket::App::Theme::textPrimary().name()));
    layout->addWidget(*value, 1, Qt::AlignRight);
    return row;
}

} // namespace

namespace Pocket::App {

GameInspector::GameInspector(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(300);
    setMaximumWidth(360);
    // Floating layer, not a bolted-on panel: translucent surface, no hard divider.
    setStyleSheet(QString("GameInspector { background: %1; border: none; border-radius: 10px; }")
                      .arg(Theme::rgba(Theme::surfacePanel())));

    m_stack = new QStackedLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    auto* empty = new EmptyStateWidget(this);
    empty->setState("Ningún juego seleccionado", "Elige un juego de tu biblioteca para ver sus detalles.");
    m_stack->addWidget(empty);

    m_content = new QWidget(this);
    auto* layout = new QVBoxLayout(m_content);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    m_cover = new InspectorCoverWidget(m_content);
    m_cover->setObjectName("detailCover");
    m_cover->setMaximumHeight(300);
    m_cover->setMinimumHeight(180);
    layout->addWidget(m_cover);
    layout->addSpacing(6);

    m_title = new QLabel(m_content);
    m_title->setObjectName("detailTitle");
    m_title->setWordWrap(true);
    QFont titleFont = m_title->font();
    titleFont.setPixelSize(16);
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    m_title->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textPrimary().name()));
    layout->addWidget(m_title);

    m_platform = new QLabel(m_content);
    m_platform->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;")
                                  .arg(Theme::textSecondary().name()));
    layout->addWidget(m_platform);
    layout->addSpacing(8);

    m_playButton = new QPushButton("Jugar", m_content);
    m_playButton->setObjectName("playButton");
    m_playButton->setIcon(Icons::icon(Icons::Name::Play, Qt::white, 15));
    m_playButton->setDefault(true);
    m_playButton->setFixedHeight(36);
    m_playButton->setStyleSheet(QString("QPushButton#playButton { background: %1; color: white; border: 1px solid %1;"
                                        " border-radius: 6px; font-weight: 600; font-size: 13px; }"
                                        "QPushButton#playButton:hover { background: %2; border-color: %2; }"
                                        "QPushButton#playButton:pressed { background: %2; }")
                                      .arg(Theme::accent().name(), Theme::accentPressed().name()));
    layout->addWidget(m_playButton);
    layout->addSpacing(6);

    layout->addWidget(dataRow("Partida guardada", &m_saveValue, m_content));
    layout->addWidget(dataRow("Añadido", &m_addedValue, m_content));
    layout->addWidget(dataRow("Tamaño", &m_sizeValue, m_content));

    m_detailsButton = new QToolButton(m_content);
    m_detailsButton->setText("Detalles");
    m_detailsButton->setCheckable(true);
    m_detailsButton->setIcon(Icons::icon(Icons::Name::Info, Theme::textSecondary(), 15));
    m_detailsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // Secondary: no surface of its own next to Play.
    m_detailsButton->setStyleSheet(QString("QToolButton { background: transparent; border: 1px solid transparent;"
                                           " color: %1; padding: 4px 6px; }"
                                           "QToolButton:hover { color: %2; background: %3; }"
                                           "QToolButton:focus { border: 1px solid %4; }")
                                       .arg(Theme::textSecondary().name(), Theme::textPrimary().name(),
                                            Theme::rgba(Theme::surfaceHover()), Theme::accent().name()));
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
    connect(m_detailsButton, &QToolButton::toggled, details, [details](bool expanded) {
        details->setVisible(expanded);
    });

    auto* actionsRow = new QHBoxLayout;
    actionsRow->setSpacing(4);
    actionsRow->addStretch();
    auto* artworkShortcut = new QToolButton(m_content);
    artworkShortcut->setObjectName("changeArtworkButton");
    artworkShortcut->setIcon(Icons::icon(Icons::Name::Image, Theme::textSecondary(), 16));
    artworkShortcut->setToolTip("Cambiar carátula");
    artworkShortcut->setAccessibleName("Cambiar carátula");
    auto* locationShortcut = new QToolButton(m_content);
    locationShortcut->setObjectName("openFolderButton");
    locationShortcut->setIcon(Icons::icon(Icons::Name::Folder, Theme::textSecondary(), 16));
    locationShortcut->setToolTip("Abrir ubicación del ROM");
    locationShortcut->setAccessibleName("Abrir ubicación del ROM");
    m_moreActionsButton = new QPushButton(m_content);
    m_moreActionsButton->setObjectName("moreActionsButton");
    m_moreActionsButton->setIcon(Icons::icon(Icons::Name::More, Theme::textSecondary(), 16));
    m_moreActionsButton->setToolTip("Más acciones");
    m_moreActionsButton->setAccessibleName("Más acciones");
    // The dropdown arrow next to the ellipsis was pure noise.
    m_moreActionsButton->setStyleSheet("QPushButton#moreActionsButton::menu-indicator { image: none; width: 0px; }");
    actionsRow->addWidget(artworkShortcut);
    actionsRow->addWidget(locationShortcut);
    actionsRow->addWidget(m_moreActionsButton);
    // QToolButton defaults to NoFocus: icon-only actions must still be reachable
    // with the keyboard and must show the focus ring.
    for (QToolButton* iconButton : {artworkShortcut, locationShortcut, m_detailsButton}) {
        iconButton->setFocusPolicy(Qt::StrongFocus);
    }
    // One size and one treatment for the three icon actions.
    for (QWidget* iconAction : {static_cast<QWidget*>(artworkShortcut), static_cast<QWidget*>(locationShortcut),
                                static_cast<QWidget*>(m_moreActionsButton)}) {
        iconAction->setFixedSize(34, 30);
    }
    connect(artworkShortcut, &QToolButton::clicked, this, &GameInspector::changeArtworkRequested);
    connect(locationShortcut, &QToolButton::clicked, this, &GameInspector::openLocationRequested);
    layout->addLayout(actionsRow);
    layout->addStretch();
    m_stack->addWidget(m_content);

    auto* menu = new QMenu(m_moreActionsButton);
    const QColor menuIcon = Theme::textSecondary();
    QAction* changeArtwork = menu->addAction(Icons::icon(Icons::Name::Image, menuIcon), "Cambiar carátula");
    QAction* chooseImage = menu->addAction(Icons::icon(Icons::Name::Image, menuIcon), "Elegir imagen…");
    QAction* openLocation = menu->addAction(Icons::icon(Icons::Name::Folder, menuIcon), "Abrir ubicación del ROM");
    menu->addSeparator();
    QAction* remove = menu->addAction(Icons::icon(Icons::Name::Trash, menuIcon), "Quitar de la biblioteca");
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
    m_platform->setText(systemDisplayName(system));
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
