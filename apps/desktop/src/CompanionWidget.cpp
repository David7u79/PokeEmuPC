#include "CompanionWidget.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

namespace Pocket::App {

CompanionWidget::CompanionWidget(QWidget *parent)
    : QWidget(parent) {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_companionNameLabel = new QLabel("<h2>Active Companion: None Selected</h2>", this);
    m_companionNameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_companionNameLabel);

    QGroupBox *statusGroup = new QGroupBox("Companion Status", this);
    QFormLayout *formLayout = new QFormLayout(statusGroup);

    m_hungerBar = new QProgressBar(statusGroup);
    m_hungerBar->setRange(0, 100);
    m_hungerBar->setValue(100);

    m_moodBar = new QProgressBar(statusGroup);
    m_moodBar->setRange(0, 100);
    m_moodBar->setValue(100);

    m_fatigueBar = new QProgressBar(statusGroup);
    m_fatigueBar->setRange(0, 100);
    m_fatigueBar->setValue(0);

    formLayout->addRow("Hunger:", m_hungerBar);
    formLayout->addRow("Mood:", m_moodBar);
    formLayout->addRow("Fatigue:", m_fatigueBar);

    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();
}

} // namespace Pocket::App
