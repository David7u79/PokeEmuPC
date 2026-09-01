#include "DesktopWidget.hpp"
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <algorithm>

namespace Pocket::Companion {

CompanionVisualCanvas::CompanionVisualCanvas(QWidget *parent)
    : QWidget(parent) {
    setFixedSize(64, 64);
}

void CompanionVisualCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    int offsetY = (m_animFrame % 2 == 0) ? 0 : -3; // Gentle bounce animation

    if (!m_pixmap.isNull()) {
        // Nearest-neighbor sharpness maintained by QPixmap from SpriteCache
        painter.drawPixmap(0, offsetY, m_pixmap);
        return;
    }

    // Vector drawing fallback
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(16, 20 + offsetY, 32, 32);
    path.addEllipse(20, 10 + offsetY, 24, 24);
    path.addEllipse(14, 6 + offsetY,  10, 14);
    path.addEllipse(40, 6 + offsetY,  10, 14);

    painter.setBrush(QColor(136, 192, 208));
    painter.setPen(QPen(QColor(46, 52, 64), 2));
    painter.drawPath(path);

    painter.setBrush(Qt::white);
    painter.drawEllipse(26, 18 + offsetY, 4, 6);
    painter.drawEllipse(34, 18 + offsetY, 4, 6);

    painter.setBrush(Qt::black);
    painter.drawEllipse(27, 20 + offsetY, 2, 3);
    painter.drawEllipse(35, 20 + offsetY, 2, 3);
}

DesktopWidget::DesktopWidget(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_clock(std::make_shared<SystemClock>())
    , m_simulator(m_clock)
    , m_ipcClient("PocketPartnerIPC") {

    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(260, 250);

    // Register sprite providers in order: PokeSprite -> Pkhex -> Placeholder
    QString appDir = QCoreApplication::applicationDirPath();
    auto pokeProv = std::make_shared<PokeSpriteProvider>((appDir + "/assets/pokemon/pokesprite").toStdString());
    auto pkhexProv = std::make_shared<PkhexSpriteProvider>((appDir + "/assets/pokemon/pkhex").toStdString());
    auto placeProv = std::make_shared<PlaceholderSpriteProvider>();

    m_spriteProvider.addProvider(pokeProv);
    m_spriteProvider.addProvider(pkhexProv);
    m_spriteProvider.addProvider(placeProv);

    m_state.companionId = "default";
    m_state.displayName = "Pikachu";

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QWidget *card = new QWidget(this);
    card->setObjectName("cardWidget");
    card->setStyleSheet(
        "#cardWidget {"
        "  background-color: rgba(30, 34, 45, 240);"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 12px;"
        "}"
        "QLabel { color: #ECEFF4; font-family: 'Segoe UI', sans-serif; }"
        "QProgressBar {"
        "  border: 1px solid rgba(255, 255, 255, 0.2);"
        "  border-radius: 4px;"
        "  text-align: center;"
        "  background: rgba(0, 0, 0, 0.3);"
        "  color: #ECEFF4;"
        "  font-size: 9px;"
        "}"
        "QProgressBar::chunk { background-color: #88C0D0; border-radius: 3px; }"
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.08);"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 6px;"
        "  color: #ECEFF4;"
        "  font-size: 11px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover { background-color: rgba(255, 255, 255, 0.18); }"
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);

    QHBoxLayout *topLayout = new QHBoxLayout();
    m_canvas = new CompanionVisualCanvas(card);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    m_nicknameLabel = new QLabel(QString::fromStdString(m_state.displayName), card);
    m_nicknameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #88C0D0;");

    m_canonicalMetaLabel = new QLabel("Gen III Partner | Lvl 5", card);
    m_canonicalMetaLabel->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    m_linkStatusLabel = new QLabel("Save Link: Linked", card);
    m_linkStatusLabel->setStyleSheet("font-size: 10px; color: #A3BE8C; font-weight: bold;");

    nameLayout->addWidget(m_nicknameLabel);
    nameLayout->addWidget(m_canonicalMetaLabel);
    nameLayout->addWidget(m_linkStatusLabel);

    topLayout->addWidget(m_canvas);
    topLayout->addLayout(nameLayout);

    QHBoxLayout *barLayout = new QHBoxLayout();
    m_bondBar = new QProgressBar(card);
    m_bondBar->setRange(0, 100);
    m_bondBar->setValue(10);
    m_bondBar->setFormat("Bond: %p%");

    m_energyBar = new QProgressBar(card);
    m_energyBar->setRange(0, 100);
    m_energyBar->setValue(80);
    m_energyBar->setFormat("Energy: %p%");
    m_energyBar->setStyleSheet("QProgressBar::chunk { background-color: #A3BE8C; }");

    barLayout->addWidget(m_bondBar);
    barLayout->addWidget(m_energyBar);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_feedBtn  = new QPushButton("Feed", card);
    m_petBtn   = new QPushButton("Pet", card);
    m_playBtn  = new QPushButton("Train", card);
    m_restBtn  = new QPushButton("Rest", card);

    actionLayout->addWidget(m_feedBtn);
    actionLayout->addWidget(m_petBtn);
    actionLayout->addWidget(m_playBtn);
    actionLayout->addWidget(m_restBtn);

    m_expandedContainer = new QWidget(card);
    QVBoxLayout *expandedLayout = new QVBoxLayout(m_expandedContainer);
    expandedLayout->setContentsMargins(0, 4, 0, 0);

    m_hungerLabel       = new QLabel("Hunger: --", m_expandedContainer);
    m_moodLabel         = new QLabel("Mood: --", m_expandedContainer);
    m_energyDetailLabel = new QLabel("Energy: --", m_expandedContainer);
    m_bondDetailLabel   = new QLabel("Bond: Level 1 (0 XP)", m_expandedContainer);
    m_openGameBtn       = new QPushButton("🎮 Open Game in PocketPartner", m_expandedContainer);

    expandedLayout->addWidget(m_hungerLabel);
    expandedLayout->addWidget(m_moodLabel);
    expandedLayout->addWidget(m_energyDetailLabel);
    expandedLayout->addWidget(m_bondDetailLabel);
    expandedLayout->addWidget(m_openGameBtn);
    m_expandedContainer->setVisible(false);

    cardLayout->addLayout(topLayout);
    cardLayout->addLayout(barLayout);
    cardLayout->addLayout(actionLayout);
    cardLayout->addWidget(m_expandedContainer);

    mainLayout->addWidget(card);

    connect(m_feedBtn, &QPushButton::clicked, this, &DesktopWidget::onFeedClicked);
    connect(m_petBtn, &QPushButton::clicked, this, &DesktopWidget::onPetClicked);
    connect(m_playBtn, &QPushButton::clicked, this, &DesktopWidget::onTrainClicked);
    connect(m_restBtn, &QPushButton::clicked, this, &DesktopWidget::onRestClicked);

    connect(&m_animController, &CompanionApp::CompanionAnimationController::frameTick, this, &DesktopWidget::onFrameTick);

    connect(&m_ipcClient, &Pocket::Core::IpcClient::messageReceived, this, &DesktopWidget::onIpcMessageReceived);
    m_ipcClient.connectToServer();

    updateCreatureSprite(25); // Initial default Pikachu
    loadPositionSettings();
    refreshUi();
}

DesktopWidget::~DesktopWidget() {
    savePositionSettings();
}

uint16_t DesktopWidget::speciesNameToId(const QString& speciesName) const {
    QString name = speciesName.trimmed().toLower();
    if (name.contains("bulbasaur")) return 1;
    if (name.contains("charmander")) return 4;
    if (name.contains("squirtle")) return 7;
    if (name.contains("pikachu")) return 25;
    if (name.contains("eevee")) return 133;
    if (name.contains("mewtwo")) return 150;
    if (name.contains("mew")) return 151;
    if (name.contains("chikorita")) return 152;
    if (name.contains("cyndaquil")) return 155;
    if (name.contains("totodile")) return 158;
    if (name.contains("umbreon")) return 197;
    if (name.contains("treecko")) return 252;
    if (name.contains("torchic")) return 255;
    if (name.contains("mudkip")) return 258;
    if (name.contains("rayquaza")) return 384;
    if (name.contains("turtwig")) return 387;
    if (name.contains("snivy")) return 495;

    return 25; // Default fallback to Pikachu
}

void DesktopWidget::updateCreatureSprite(uint16_t speciesId, bool shiny, uint8_t formId, Gender gender) {
    m_currentKey = SpriteKey{speciesId, shiny, formId, gender};
    QPixmap pixmap = m_spriteCache.get(m_currentKey, 64, 64, m_spriteProvider);
    if (m_canvas) {
        m_canvas->setPixmap(pixmap);
    }
}

void DesktopWidget::updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus) {
    m_state.displayName = nickname.toStdString();
    m_gameFriendship = friendship;

    if (m_nicknameLabel) {
        m_nicknameLabel->setText(nickname);
    }
    if (m_canonicalMetaLabel) {
        m_canonicalMetaLabel->setText(QString("%1 | Lvl %2").arg(species).arg(level));
    }
    if (m_linkStatusLabel) {
        m_linkStatusLabel->setText(QString("Save Link: %1").arg(linkStatus));
    }

    uint16_t specId = speciesNameToId(species);
    updateCreatureSprite(specId);
}

void DesktopWidget::onIpcMessageReceived(const Pocket::Core::IpcMessage& message) {
    if (message.command == Pocket::Core::IpcCommandType::CompanionStatusChanged) {
        QString nickname = message.payload.contains("nickname") ? message.payload.value("nickname").toString() : "Partner";
        QString species = message.payload.contains("species") ? message.payload.value("species").toString() : "Pikachu";
        int level = message.payload.contains("level") ? message.payload.value("level").toInt() : 1;
        int gameFriendship = message.payload.contains("gameFriendship") ? message.payload.value("gameFriendship").toInt() : 70;
        QString linkStatus = message.payload.contains("linkStatus") ? message.payload.value("linkStatus").toString() : "Linked";

        updateCanonicalInfo(nickname, species, level, gameFriendship, linkStatus);
    }
}

void DesktopWidget::onFrameTick() {
    m_animTickCount++;
    if (m_canvas) {
        m_canvas->setAnimationFrame(m_animTickCount);
    }
}

void DesktopWidget::onFeedClicked() {
    m_animController.triggerInteraction(1000);
    m_state = m_simulator.executeFeed(m_state, FeedCompanionCommand{});
    refreshUi();
}

void DesktopWidget::onPetClicked() {
    m_animController.triggerInteraction(1000);
    m_state = m_simulator.executePet(m_state, PetCompanionCommand{});
    refreshUi();
}

void DesktopWidget::onTrainClicked() {
    m_animController.triggerInteraction(1000);
    m_state = m_simulator.executePlay(m_state, PlayWithCompanionCommand{});
    refreshUi();
}

void DesktopWidget::onRestClicked() {
    m_state = m_simulator.executeRest(m_state, RestCompanionCommand{});
    refreshUi();
}

void DesktopWidget::onOpenGameClicked() {}

void DesktopWidget::toggleExpandedView() {
    m_isExpanded = !m_isExpanded;
    m_expandedContainer->setVisible(m_isExpanded);
    setFixedSize(260, m_isExpanded ? 340 : 250);
}

void DesktopWidget::refreshUi() {
    if (m_bondBar) m_bondBar->setValue(static_cast<int>(m_state.bond.xp % 100));
    if (m_energyBar) m_energyBar->setValue(static_cast<int>(m_state.energy));

    if (m_hungerLabel) m_hungerLabel->setText(QString("Hunger: %1/100").arg(static_cast<int>(m_state.hunger)));
    if (m_moodLabel) m_moodLabel->setText(QString("Mood: %1/100").arg(static_cast<int>(m_state.mood)));
    if (m_energyDetailLabel) m_energyDetailLabel->setText(QString("Energy: %1/100").arg(static_cast<int>(m_state.energy)));
    if (m_bondDetailLabel) m_bondDetailLabel->setText(QString("Bond: Level %1 (%2 XP)").arg(m_state.bond.level).arg(m_state.bond.xp));
}

void DesktopWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void DesktopWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void DesktopWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        savePositionSettings();
        event->accept();
    }
}

void DesktopWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    m_animController.setState(CompanionApp::AnimationState::SlowIdle);
}

void DesktopWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    m_animController.setState(CompanionApp::AnimationState::Hidden);
}

void DesktopWidget::loadPositionSettings() {
    QSettings settings("PocketPartner", "PocketCompanion");
    QPoint pos = settings.value("windowPosition", QPoint(-1, -1)).toPoint();
    if (pos != QPoint(-1, -1)) {
        move(pos);
    } else {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect geom = screen->availableGeometry();
            move(geom.right() - width() - 20, geom.bottom() - height() - 40);
        }
    }
}

void DesktopWidget::savePositionSettings() {
    QSettings settings("PocketPartner", "PocketCompanion");
    settings.setValue("windowPosition", pos());
}

} // namespace Pocket::Companion
