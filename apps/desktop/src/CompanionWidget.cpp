#include "CompanionWidget.hpp"
#include "EmptyStateWidget.hpp"
#include "Theme.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>

namespace Pocket::App {

CompanionWidget::CompanionWidget(QWidget *parent)
    : QWidget(parent) {

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(14);

    // Page header
    auto *headerWidget = new QWidget(this);
    auto *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto *titleLabel = new QLabel(QStringLiteral("Compañero"), headerWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(Theme::textPrimary().name()));

    auto *descLabel = new QLabel(QStringLiteral("Interactúa con tu Pokémon acompañante y supervisa su estado."), headerWidget);
    descLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(Theme::textSecondary().name()));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(descLabel);
    mainLayout->addWidget(headerWidget);

    // Card Container
    auto *container = new QFrame(this);
    container->setObjectName("companionContainer");
    container->setStyleSheet(QStringLiteral(
        "QFrame#companionContainer {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "}"
    ).arg(Theme::surface().name(), Theme::border().name()));

    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(16, 16, 16, 16);
    containerLayout->setSpacing(12);

    m_stack = new QStackedWidget(container);
    containerLayout->addWidget(m_stack);
    mainLayout->addWidget(container);

    // View 0: Empty state when waiting or no companion active
    m_emptyState = new EmptyStateWidget(m_stack);
    m_emptyState->setState(
        QStringLiteral("Ningún compañero activo"),
        QStringLiteral("Selecciona un Pokémon de tu partida guardada en Ajustes > Diagnóstico o inicia una partida para interactuar con tu compañero."),
        QStringLiteral("Activar compañero de demostración")
    );
    connect(m_emptyState, &EmptyStateWidget::actionRequested, this, [this] {
        setCompanionActive(true);
    });
    m_stack->addWidget(m_emptyState);

    // View 1: Active Companion Status & Interactions Card
    m_contentCard = new QWidget(m_stack);
    auto *contentLayout = new QVBoxLayout(m_contentCard);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    auto *statusTitle = new QLabel(QStringLiteral("ESTADO DEL COMPAÑERO ACTIVO"), m_contentCard);
    statusTitle->setStyleSheet(QStringLiteral(
        "font-size: 11px; font-weight: 700; letter-spacing: 1px; color: %1; background: transparent; padding-bottom: 2px;"
    ).arg(Theme::textSecondary().name()));
    contentLayout->addWidget(statusTitle);

    m_nameLabel = new QLabel(QStringLiteral("Partner (Lv. 5)"), m_contentCard);
    m_nameLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: %1; background: transparent;").arg(Theme::textPrimary().name()));

    m_bondLabel = new QLabel(QStringLiteral("Companion Bond: Level 1 (0 XP)"), m_contentCard);
    m_bondLabel->setStyleSheet(QStringLiteral("font-weight: 600; font-size: 12px; color: %1; background: transparent;").arg(Theme::accent().name()));

    contentLayout->addWidget(m_nameLabel);
    contentLayout->addWidget(m_bondLabel);

    const QString barStyle = QStringLiteral(
        "QProgressBar {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 4px;"
        "  text-align: center;"
        "  color: %3;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  min-height: 18px;"
        "  max-height: 18px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %4;"
        "  border-radius: 3px;"
        "}"
    ).arg(Theme::surfaceRaised().name(),
         Theme::border().name(),
         Theme::textPrimary().name(),
         Theme::accent().name());

    auto createBar = [this, &barStyle](const QString& format) {
        auto *bar = new QProgressBar(m_contentCard);
        bar->setRange(0, 100);
        bar->setFormat(format);
        bar->setStyleSheet(barStyle);
        return bar;
    };

    m_hungerBar = createBar(QStringLiteral("Hunger: %p%"));
    m_moodBar = createBar(QStringLiteral("Mood: %p%"));
    m_energyBar = createBar(QStringLiteral("Energy: %p%"));
    m_fatigueBar = createBar(QStringLiteral("Fatigue: %p%"));

    auto *barsGrid = new QGridLayout();
    barsGrid->setContentsMargins(0, 4, 0, 4);
    barsGrid->setHorizontalSpacing(16);
    barsGrid->setVerticalSpacing(8);
    barsGrid->addWidget(m_hungerBar, 0, 0);
    barsGrid->addWidget(m_moodBar, 0, 1);
    barsGrid->addWidget(m_energyBar, 1, 0);
    barsGrid->addWidget(m_fatigueBar, 1, 1);
    contentLayout->addLayout(barsGrid);

    auto *sep = new QFrame(m_contentCard);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("background-color: %1; max-height: 1px; border: none;").arg(Theme::border().name()));
    contentLayout->addWidget(sep);

    auto *actionsTitle = new QLabel(QStringLiteral("INTERACCIONES CON EL COMPAÑERO"), m_contentCard);
    actionsTitle->setStyleSheet(QStringLiteral(
        "font-size: 11px; font-weight: 700; letter-spacing: 1px; color: %1; background: transparent; padding-bottom: 2px;"
    ).arg(Theme::textSecondary().name()));
    contentLayout->addWidget(actionsTitle);

    auto *actionsGrid = new QGridLayout();
    actionsGrid->setContentsMargins(0, 4, 0, 4);
    actionsGrid->setHorizontalSpacing(12);
    actionsGrid->setVerticalSpacing(8);

    m_feedBtn = new QPushButton(QStringLiteral("Feed Companion"), m_contentCard);
    m_petBtn  = new QPushButton(QStringLiteral("Pet Companion"), m_contentCard);
    m_playBtn = new QPushButton(QStringLiteral("Play with Companion"), m_contentCard);
    m_restBtn = new QPushButton(QStringLiteral("Rest Companion"), m_contentCard);

    actionsGrid->addWidget(m_feedBtn, 0, 0);
    actionsGrid->addWidget(m_petBtn, 0, 1);
    actionsGrid->addWidget(m_playBtn, 1, 0);
    actionsGrid->addWidget(m_restBtn, 1, 1);
    contentLayout->addLayout(actionsGrid);

    contentLayout->addStretch();
    m_stack->addWidget(m_contentCard);

    connect(m_feedBtn, &QPushButton::clicked, this, &CompanionWidget::onFeedClicked);
    connect(m_petBtn,  &QPushButton::clicked, this, &CompanionWidget::onPetClicked);
    connect(m_playBtn, &QPushButton::clicked, this, &CompanionWidget::onPlayClicked);
    connect(m_restBtn, &QPushButton::clicked, this, &CompanionWidget::onRestClicked);

    // Initial state: empty state waiting for active companion
    m_stack->setCurrentWidget(m_emptyState);
}

void CompanionWidget::setCompanionActive(bool active)
{
    m_hasActiveCompanion = active;
    if (active) {
        m_stack->setCurrentWidget(m_contentCard);
        refreshDisplay();
    } else {
        m_stack->setCurrentWidget(m_emptyState);
    }
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
