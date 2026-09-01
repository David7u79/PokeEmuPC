#include "DesktopWidget.hpp"
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>

namespace Pocket::Companion {

CompanionVisualCanvas::CompanionVisualCanvas(QWidget *parent)
    : QWidget(parent) {
    setFixedSize(64, 64);
}

void CompanionVisualCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int offsetY = (m_animFrame % 2 == 0) ? 0 : -3; // Gentle bounce animation

    // Draw neutral creature silhouette (QPainter vector drawing, zero proprietary sprites)
    QPainterPath path;
    path.addEllipse(16, 20 + offsetY, 32, 32); // Body
    path.addEllipse(20, 10 + offsetY, 24, 24); // Head
    path.addEllipse(14, 6 + offsetY,  10, 14); // Left Ear
    path.addEllipse(40, 6 + offsetY,  10, 14); // Right Ear

    painter.setBrush(QColor(136, 192, 208));
    painter.setPen(QPen(QColor(46, 52, 64), 2));
    painter.drawPath(path);

    // Cute eyes
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

    m_state.companionId = "default";
    m_state.displayName = "Partner";

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

    // Top Header: Canvas + Nickname/Meta
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_canvas = new CompanionVisualCanvas(card);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    m_nicknameLabel = new QLabel(QString::fromStdString(m_state.displayName), card);
    m_nicknameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #88C0D0;");

    m_canonicalMetaLabel = new QLabel("Gen III Partner | Lvl --", card);
    m_canonicalMetaLabel->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    m_linkStatusLabel = new QLabel("Save Link: Linked", card);
    m_linkStatusLabel->setStyleSheet("font-size: 10px; color: #A3BE8C; font-weight: bold;");

    nameLayout->addWidget(m_nicknameLabel);
    nameLayout->addWidget(m_canonicalMetaLabel);
    nameLayout->addWidget(m_linkStatusLabel);

    topLayout->addWidget(m_canvas);
    topLayout->addLayout(nameLayout);

    // Compact Status Bars (Bond & Energy)
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

    // Action Row
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_feedBtn  = new QPushButton("Feed", card);
    m_petBtn   = new QPushButton("Pet", card);
    m_playBtn  = new QPushButton("Train", card);
    m_restBtn  = new QPushButton("Rest", card);

    actionLayout->addWidget(m_feedBtn);
    actionLayout->addWidget(m_petBtn);
    actionLayout->addWidget(m_playBtn);
    actionLayout->addWidget(m_restBtn);

    // Expanded Container
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

    m_expandBtn = new QPushButton("▼ Expand Details", card);

    cardLayout->addLayout(topLayout);
    cardLayout->addLayout(barLayout);
    cardLayout->addLayout(actionLayout);
    cardLayout->addWidget(m_expandBtn);
    cardLayout->addWidget(m_expandedContainer);

    mainLayout->addWidget(card);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    connect(m_feedBtn, &QPushButton::clicked, this, &DesktopWidget::onFeedClicked);
    connect(m_petBtn, &QPushButton::clicked, this, &DesktopWidget::onPetClicked);
    connect(m_playBtn, &QPushButton::clicked, this, &DesktopWidget::onPlayClicked);
    connect(m_restBtn, &QPushButton::clicked, this, &DesktopWidget::onRestClicked);
    connect(m_expandBtn, &QPushButton::clicked, this, &DesktopWidget::toggleExpandedView);
    connect(m_openGameBtn, &QPushButton::clicked, this, &DesktopWidget::onOpenGameClicked);

    // Animation Controller setup
    connect(&m_animController, &Pocket::CompanionApp::CompanionAnimationController::frameTick, this, &DesktopWidget::onFrameTick);
    m_animController.setState(Pocket::CompanionApp::AnimationState::SlowIdle);

    // Decay Timer (1 Hz)
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, this, &DesktopWidget::refreshUi);
    m_decayTimer->start(1000);

    // Power Check Timer (every 5 seconds)
    m_powerCheckTimer = new QTimer(this);
    connect(m_powerCheckTimer, &QTimer::timeout, this, [this]() {
        auto pInfo = Pocket::CompanionApp::PowerStatusMonitor::queryPowerStatus();
        m_animController.updatePowerPolicy(pInfo.isBatterySaverActive);
    });
    m_powerCheckTimer->start(5000);

    // IPC Client Setup
    connect(&m_ipcClient, &Pocket::Core::IpcClient::messageReceived, this, &DesktopWidget::onIpcMessageReceived);
    m_ipcClient.connectToServer();

    loadSavedPosition();
    refreshUi();
}

DesktopWidget::~DesktopWidget() {
    saveCurrentPosition();
}

void DesktopWidget::loadSavedPosition() {
    QSettings settings("PocketPartner", "PocketCompanion");
    if (settings.contains("windowPosition")) {
        QPoint pos = settings.value("windowPosition").toPoint();
        QScreen *screen = QGuiApplication::screenAt(pos);
        if (screen) {
            move(pos); // Multi-monitor safe
        }
    }
}

void DesktopWidget::saveCurrentPosition() {
    QSettings settings("PocketPartner", "PocketCompanion");
    settings.setValue("windowPosition", pos());
}

void DesktopWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    m_animController.setState(Pocket::CompanionApp::AnimationState::SlowIdle);
}

void DesktopWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    m_animController.setState(Pocket::CompanionApp::AnimationState::Hidden);
}

void DesktopWidget::onFrameTick() {
    m_animStep++;
    m_canvas->setAnimationFrame(m_animStep);
}

void DesktopWidget::updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus) {
    m_nicknameLabel->setText(nickname);
    m_canonicalMetaLabel->setText(QString("%1 | Lvl %2").arg(species).arg(level));
    m_linkStatusLabel->setText(QString("Save Link: %1 | Friendship: %2").arg(linkStatus).arg(friendship));

    if (linkStatus == "Linked") {
        m_linkStatusLabel->setStyleSheet("font-size: 10px; color: #A3BE8C; font-weight: bold;");
    } else {
        m_linkStatusLabel->setStyleSheet("font-size: 10px; color: #BF616A; font-weight: bold;");
    }
}

void DesktopWidget::onIpcMessageReceived(const Pocket::Core::IpcMessage& message) {
    if (message.command == Pocket::Core::IpcCommandType::CompanionStatusChanged) {
        QString nickname = message.payload["nickname"].toString("Partner");
        QString species  = message.payload["species"].toString("Pokémon");
        int level        = message.payload["level"].toInt(5);
        int friendship   = message.payload["gameFriendship"].toInt(70);
        QString status   = message.payload["linkStatus"].toString("Linked");

        updateCanonicalInfo(nickname, species, level, friendship, status);
    }
}

void DesktopWidget::onFeedClicked() {
    FeedCompanionCommand cmd;
    m_state = m_simulator.executeFeed(m_state, cmd);
    m_animController.triggerInteraction(1500); // 25 FPS temporary boost
    refreshUi();
}

void DesktopWidget::onPetClicked() {
    PetCompanionCommand cmd;
    m_state = m_simulator.executePet(m_state, cmd);
    m_animController.triggerInteraction(1500);
    refreshUi();
}

void DesktopWidget::onPlayClicked() {
    PlayWithCompanionCommand cmd;
    m_state = m_simulator.executePlay(m_state, cmd);
    m_animController.triggerInteraction(1500);
    refreshUi();
}

void DesktopWidget::onRestClicked() {
    RestCompanionCommand cmd;
    m_state = m_simulator.executeRest(m_state, cmd);
    m_animController.triggerInteraction(1500);
    refreshUi();
}

void DesktopWidget::onTrainClicked() {
    PlayWithCompanionCommand cmd;
    m_state = m_simulator.executePlay(m_state, cmd);
    m_animController.triggerInteraction(1500);
    refreshUi();
}

void DesktopWidget::onOpenGameClicked() {
    Pocket::Core::IpcMessage msg;
    msg.command = Pocket::Core::IpcCommandType::OpenMainApplication;
    m_ipcClient.sendMessage(msg);
}

void DesktopWidget::toggleExpandedView() {
    m_isExpanded = !m_isExpanded;
    m_expandedContainer->setVisible(m_isExpanded);
    m_expandBtn->setText(m_isExpanded ? "▲ Hide Details" : "▼ Expand Details");
    setFixedSize(260, m_isExpanded ? 350 : 250);
}

void DesktopWidget::refreshUi() {
    m_state = m_simulator.calculateCurrentState(m_state);

    m_energyBar->setValue(static_cast<int>(m_state.energy));
    m_bondBar->setValue(static_cast<int>(m_state.bond.level * 10));

    m_hungerLabel->setText(QString("Hunger: %1%").arg(static_cast<int>(m_state.hunger)));
    m_moodLabel->setText(QString("Mood: %1%").arg(static_cast<int>(m_state.mood)));
    m_energyDetailLabel->setText(QString("Energy: %1%").arg(static_cast<int>(m_state.energy)));
    m_bondDetailLabel->setText(QString("Bond: Level %1 (%2 XP)")
        .arg(m_state.bond.level)
        .arg(m_state.bond.xp));
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
        saveCurrentPosition();
        event->accept();
    }
}

} // namespace Pocket::Companion
