#include "CompanionWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>

namespace Pocket::App {

CompanionWidget::CompanionWidget(QWidget *parent)
    : QWidget(parent) {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QGroupBox *statusGroup = new QGroupBox("Active Companion Status", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(statusGroup);

    m_nameLabel = new QLabel("Partner (Lv. 5)", statusGroup);
    m_nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E88E5;");

    m_bondLabel = new QLabel("Companion Bond: Level 1 (0 XP)", statusGroup);
    m_bondLabel->setStyleSheet("font-weight: bold; color: #FF8C00;");

    m_hungerBar = new QProgressBar(statusGroup);
    m_hungerBar->setFormat("Hunger: %p%");
    m_hungerBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");

    m_moodBar = new QProgressBar(statusGroup);
    m_moodBar->setFormat("Mood: %p%");
    m_moodBar->setStyleSheet("QProgressBar::chunk { background-color: #2196F3; }");

    m_energyBar = new QProgressBar(statusGroup);
    m_energyBar->setFormat("Energy: %p%");
    m_energyBar->setStyleSheet("QProgressBar::chunk { background-color: #9C27B0; }");

    m_fatigueBar = new QProgressBar(statusGroup);
    m_fatigueBar->setFormat("Fatigue: %p%");
    m_fatigueBar->setStyleSheet("QProgressBar::chunk { background-color: #FF5722; }");

    groupLayout->addWidget(m_nameLabel);
    groupLayout->addWidget(m_bondLabel);
    groupLayout->addSpacing(10);
    groupLayout->addWidget(m_hungerBar);
    groupLayout->addWidget(m_moodBar);
    groupLayout->addWidget(m_energyBar);
    groupLayout->addWidget(m_fatigueBar);

    QGroupBox *actionsGroup = new QGroupBox("Companion Interactions", this);
    QGridLayout *actionsLayout = new QGridLayout(actionsGroup);

    m_feedBtn = new QPushButton("Feed Companion", actionsGroup);
    m_petBtn  = new QPushButton("Pet Companion", actionsGroup);
    m_playBtn = new QPushButton("Play with Companion", actionsGroup);
    m_restBtn = new QPushButton("Rest Companion", actionsGroup);

    actionsLayout->addWidget(m_feedBtn, 0, 0);
    actionsLayout->addWidget(m_petBtn, 0, 1);
    actionsLayout->addWidget(m_playBtn, 1, 0);
    actionsLayout->addWidget(m_restBtn, 1, 1);

    mainLayout->addWidget(statusGroup);
    mainLayout->addWidget(actionsGroup);
    mainLayout->addStretch();

    connect(m_feedBtn, &QPushButton::clicked, this, &CompanionWidget::onFeedClicked);
    connect(m_petBtn,  &QPushButton::clicked, this, &CompanionWidget::onPetClicked);
    connect(m_playBtn, &QPushButton::clicked, this, &CompanionWidget::onPlayClicked);
    connect(m_restBtn, &QPushButton::clicked, this, &CompanionWidget::onRestClicked);

    refreshDisplay();
}

void CompanionWidget::refreshDisplay() {
    m_state = m_simulator.calculateCurrentState(m_state);

    m_hungerBar->setValue(static_cast<int>(m_state.hunger));
    m_moodBar->setValue(static_cast<int>(m_state.mood));
    m_energyBar->setValue(static_cast<int>(m_state.energy));
    m_fatigueBar->setValue(static_cast<int>(m_state.fatigue));

    m_bondLabel->setText(QString("Companion Bond: Level %1 (%2 XP)").arg(m_state.bond.level).arg(m_state.bond.xp));
}

void CompanionWidget::onFeedClicked() {
    Pocket::Companion::FeedCompanionCommand cmd;
    m_state = m_simulator.executeFeed(m_state, cmd);
    refreshDisplay();
}

void CompanionWidget::onPetClicked() {
    Pocket::Companion::PetCompanionCommand cmd;
    m_state = m_simulator.executePet(m_state, cmd);
    refreshDisplay();
}

void CompanionWidget::onPlayClicked() {
    Pocket::Companion::PlayWithCompanionCommand cmd;
    m_state = m_simulator.executePlay(m_state, cmd);
    refreshDisplay();
}

void CompanionWidget::onRestClicked() {
    Pocket::Companion::RestCompanionCommand cmd;
    m_state = m_simulator.executeRest(m_state, cmd);
    refreshDisplay();
}

} // namespace Pocket::App
