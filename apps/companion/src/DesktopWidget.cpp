#include "DesktopWidget.hpp"
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

namespace Pocket::Companion {

DesktopWidget::DesktopWidget(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_clock(std::make_shared<SystemClock>())
    , m_simulator(m_clock)
    , m_ipcClient("PocketPartnerIPC") {

    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(260, 240);

    // Initial default state
    m_state.companionId = "default";
    m_state.displayName = "Partner";

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QWidget *card = new QWidget(this);
    card->setObjectName("cardWidget");
    card->setStyleSheet(
        "#cardWidget {"
        "  background-color: rgba(30, 34, 45, 235);"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 12px;"
        "}"
        "QLabel { color: #ECEFF4; font-family: 'Segoe UI', sans-serif; }"
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

    // Canonical Game Header
    m_nicknameLabel = new QLabel(QString::fromStdString(m_state.displayName), card);
    m_nicknameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #88C0D0;");

    m_canonicalMetaLabel = new QLabel("Gen III Partner | Lvl --", card);
    m_canonicalMetaLabel->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    m_linkStatusLabel = new QLabel("Save Link: Linked", card);
    m_linkStatusLabel->setStyleSheet("font-size: 10px; color: #A3BE8C; font-weight: bold;");

    // Action buttons row
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_feedBtn = new QPushButton("Feed", card);
    m_petBtn  = new QPushButton("Pet", card);
    m_playBtn = new QPushButton("Play", card);
    m_restBtn = new QPushButton("Rest", card);

    actionLayout->addWidget(m_feedBtn);
    actionLayout->addWidget(m_petBtn);
    actionLayout->addWidget(m_playBtn);
    actionLayout->addWidget(m_restBtn);

    // App-only Stats (Expanded Container)
    m_expandedContainer = new QWidget(card);
    QVBoxLayout *expandedLayout = new QVBoxLayout(m_expandedContainer);
    expandedLayout->setContentsMargins(0, 4, 0, 0);

    m_hungerLabel = new QLabel("Hunger: --", m_expandedContainer);
    m_moodLabel   = new QLabel("Mood: --", m_expandedContainer);
    m_energyLabel = new QLabel("Energy: --", m_expandedContainer);
    m_bondLabel   = new QLabel("Bond: Level 1 (0 XP)", m_expandedContainer);

    expandedLayout->addWidget(m_hungerLabel);
    expandedLayout->addWidget(m_moodLabel);
    expandedLayout->addWidget(m_energyLabel);
    expandedLayout->addWidget(m_bondLabel);

    m_expandedContainer->setVisible(false);

    m_expandBtn = new QPushButton("▼ Expand Stats", card);

    cardLayout->addWidget(m_nicknameLabel);
    cardLayout->addWidget(m_canonicalMetaLabel);
    cardLayout->addWidget(m_linkStatusLabel);
    cardLayout->addLayout(actionLayout);
    cardLayout->addWidget(m_expandBtn);
    cardLayout->addWidget(m_expandedContainer);

    mainLayout->addWidget(card);

    // QGraphicsDropShadowEffect for clean visual elevation
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

    // Event-driven UI timer (1 Hz only when visible)
    m_uiTimer = new QTimer(this);
    connect(m_uiTimer, &QTimer::timeout, this, &DesktopWidget::refreshUi);
    m_uiTimer->start(1000);

    // IPC Client Setup
    connect(&m_ipcClient, &Pocket::Core::IpcClient::messageReceived, this, &DesktopWidget::onIpcMessageReceived);
    m_ipcClient.connectToServer();

    refreshUi();
}

void DesktopWidget::updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus) {
    m_nicknameLabel->setText(nickname);
    m_canonicalMetaLabel->setText(QString("%1 | Lvl %2 | Friendship: %3").arg(species).arg(level).arg(friendship));
    m_linkStatusLabel->setText(QString("Save Link: %1").arg(linkStatus));

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
    refreshUi();
}

void DesktopWidget::onPetClicked() {
    PetCompanionCommand cmd;
    m_state = m_simulator.executePet(m_state, cmd);
    refreshUi();
}

void DesktopWidget::onPlayClicked() {
    PlayWithCompanionCommand cmd;
    m_state = m_simulator.executePlay(m_state, cmd);
    refreshUi();
}

void DesktopWidget::onRestClicked() {
    RestCompanionCommand cmd;
    m_state = m_simulator.executeRest(m_state, cmd);
    refreshUi();
}

void DesktopWidget::toggleExpandedView() {
    m_isExpanded = !m_isExpanded;
    m_expandedContainer->setVisible(m_isExpanded);
    m_expandBtn->setText(m_isExpanded ? "▲ Collapse Stats" : "▼ Expand Stats");
    setFixedSize(260, m_isExpanded ? 320 : 240);
}

void DesktopWidget::refreshUi() {
    m_state = m_simulator.calculateCurrentState(m_state);

    m_hungerLabel->setText(QString("Hunger: %1%").arg(static_cast<int>(m_state.hunger)));
    m_moodLabel->setText(QString("Mood: %1%").arg(static_cast<int>(m_state.mood)));
    m_energyLabel->setText(QString("Energy: %1%").arg(static_cast<int>(m_state.energy)));
    m_bondLabel->setText(QString("Bond: Level %1 (%2 XP)")
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
        event->accept();
    }
}

} // namespace Pocket::Companion
