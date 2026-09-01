#include "DesktopWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>

namespace Pocket::CompanionApp {

DesktopWidget::DesktopWidget(std::shared_ptr<Core::IpcClient> ipcClient, QWidget *parent)
    : QWidget(parent), m_ipcClient(std::move(ipcClient)) {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(180, 260);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);

    m_nameLabel = new QLabel("Partner", this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");

    m_levelLabel = new QLabel("Lv. 5", this);
    m_levelLabel->setAlignment(Qt::AlignCenter);
    m_levelLabel->setStyleSheet("color: #FFD700; font-weight: bold; font-size: 11px;");

    m_bondLabel = new QLabel("Bond Lv. 1 (0 XP)", this);
    m_bondLabel->setAlignment(Qt::AlignCenter);
    m_bondLabel->setStyleSheet("color: #FF8C00; font-weight: bold; font-size: 11px;");

    m_hungerBar = new QProgressBar(this);
    m_hungerBar->setRange(0, 100);
    m_hungerBar->setFixedHeight(8);
    m_hungerBar->setTextVisible(false);
    m_hungerBar->setToolTip("Hunger");
    m_hungerBar->setStyleSheet("QProgressBar { background-color: #333; border-radius: 4px; } QProgressBar::chunk { background-color: #4CAF50; border-radius: 4px; }");

    m_moodBar = new QProgressBar(this);
    m_moodBar->setRange(0, 100);
    m_moodBar->setFixedHeight(8);
    m_moodBar->setTextVisible(false);
    m_moodBar->setToolTip("Mood");
    m_moodBar->setStyleSheet("QProgressBar { background-color: #333; border-radius: 4px; } QProgressBar::chunk { background-color: #2196F3; border-radius: 4px; }");

    m_energyBar = new QProgressBar(this);
    m_energyBar->setRange(0, 100);
    m_energyBar->setFixedHeight(8);
    m_energyBar->setTextVisible(false);
    m_energyBar->setToolTip("Energy");
    m_energyBar->setStyleSheet("QProgressBar { background-color: #333; border-radius: 4px; } QProgressBar::chunk { background-color: #9C27B0; border-radius: 4px; }");

    // Action Buttons
    m_buttonContainer = new QWidget(this);
    QGridLayout *btnLayout = new QGridLayout(m_buttonContainer);
    btnLayout->setContentsMargins(0, 4, 0, 4);
    btnLayout->setSpacing(4);

    m_feedBtn = new QPushButton("Feed", m_buttonContainer);
    m_petBtn  = new QPushButton("Pet", m_buttonContainer);
    m_playBtn = new QPushButton("Play", m_buttonContainer);
    m_restBtn = new QPushButton("Rest", m_buttonContainer);

    QString btnStyle = "QPushButton { background-color: #2A364F; color: white; border: 1px solid #4A5B7D; border-radius: 4px; padding: 4px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #3A4B6E; }";
    m_feedBtn->setStyleSheet(btnStyle);
    m_petBtn->setStyleSheet(btnStyle);
    m_playBtn->setStyleSheet(btnStyle);
    m_restBtn->setStyleSheet(btnStyle);

    btnLayout->addWidget(m_feedBtn, 0, 0);
    btnLayout->addWidget(m_petBtn, 0, 1);
    btnLayout->addWidget(m_playBtn, 1, 0);
    btnLayout->addWidget(m_restBtn, 1, 1);

    m_detailsBtn = new QPushButton("Details ▼", this);
    m_detailsBtn->setStyleSheet("QPushButton { background-color: transparent; color: #88AACC; border: none; font-size: 10px; } QPushButton:hover { color: white; }");

    m_detailsLabel = new QLabel(this);
    m_detailsLabel->setAlignment(Qt::AlignCenter);
    m_detailsLabel->setStyleSheet("color: #CCCCCC; font-size: 10px;");
    m_detailsLabel->setVisible(false);

    layout->addSpacing(65); // Reserve top space for creature silhouette
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_levelLabel);
    layout->addWidget(m_bondLabel);
    layout->addWidget(m_hungerBar);
    layout->addWidget(m_moodBar);
    layout->addWidget(m_energyBar);
    layout->addWidget(m_buttonContainer);
    layout->addWidget(m_detailsBtn);
    layout->addWidget(m_detailsLabel);

    connect(m_feedBtn, &QPushButton::clicked, this, &DesktopWidget::onFeedClicked);
    connect(m_petBtn,  &QPushButton::clicked, this, &DesktopWidget::onPetClicked);
    connect(m_playBtn, &QPushButton::clicked, this, &DesktopWidget::onPlayClicked);
    connect(m_restBtn, &QPushButton::clicked, this, &DesktopWidget::onRestClicked);
    connect(m_detailsBtn, &QPushButton::clicked, this, &DesktopWidget::onToggleDetails);

    m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);

    connect(&m_governor, &PocketPartner::DesktopCompanion::FramerateGovernor::renderTick, this, [this]() {
        update();
    });

    refreshStateDisplay();
}

void DesktopWidget::setAlwaysOnTop(bool onTop) {
    m_alwaysOnTop = onTop;
    Qt::WindowFlags flags = windowFlags();
    if (onTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    show();
}

void DesktopWidget::refreshStateDisplay() {
    m_state = m_simulator.calculateCurrentState(m_state);

    m_hungerBar->setValue(static_cast<int>(m_state.hunger));
    m_moodBar->setValue(static_cast<int>(m_state.mood));
    m_energyBar->setValue(static_cast<int>(m_state.energy));

    m_bondLabel->setText(QString("Bond Lv. %1 (%2 XP)").arg(m_state.bond.level).arg(m_state.bond.xp));

    if (m_detailsExpanded) {
        m_detailsLabel->setText(QString("Hunger: %1%\nMood: %2%\nEnergy: %3%\nFatigue: %4%")
            .arg(static_cast<int>(m_state.hunger))
            .arg(static_cast<int>(m_state.mood))
            .arg(static_cast<int>(m_state.energy))
            .arg(static_cast<int>(m_state.fatigue)));
    }
    update();
}

void DesktopWidget::onFeedClicked() {
    Pocket::Companion::FeedCompanionCommand cmd;
    m_state = m_simulator.executeFeed(m_state, cmd);
    refreshStateDisplay();
}

void DesktopWidget::onPetClicked() {
    Pocket::Companion::PetCompanionCommand cmd;
    m_state = m_simulator.executePet(m_state, cmd);
    refreshStateDisplay();
}

void DesktopWidget::onPlayClicked() {
    Pocket::Companion::PlayWithCompanionCommand cmd;
    m_state = m_simulator.executePlay(m_state, cmd);
    refreshStateDisplay();
}

void DesktopWidget::onRestClicked() {
    Pocket::Companion::RestCompanionCommand cmd;
    m_state = m_simulator.executeRest(m_state, cmd);
    refreshStateDisplay();
}

void DesktopWidget::onToggleDetails() {
    m_detailsExpanded = !m_detailsExpanded;
    m_detailsLabel->setVisible(m_detailsExpanded);
    m_detailsBtn->setText(m_detailsExpanded ? "Details ▲" : "Details ▼");
    adjustSize();
    refreshStateDisplay();
}

void DesktopWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::InteractiveAnimation);
        event->accept();
    }
}

void DesktopWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void DesktopWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);
        event->accept();
    }
}

void DesktopWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background card container
    painter.setBrush(QColor(20, 25, 35, 220));
    painter.setPen(QPen(QColor(100, 180, 255, 180), 1.5));
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 12, 12);

    // Neutral placeholder creature silhouette (Asset policy compliance: no proprietary Pokémon art)
    painter.setBrush(QColor(70, 160, 240, 240));
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.drawEllipse(width() / 2 - 22, 12, 44, 44);

    // Cute ears silhouette
    painter.drawEllipse(width() / 2 - 23, 8, 12, 16);
    painter.drawEllipse(width() / 2 + 11, 8, 12, 16);
}

} // namespace Pocket::CompanionApp
