#include "TrainingTimingBarWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace Pocket::App {

TrainingTimingBarWidget::TrainingTimingBarWidget(QWidget *parent)
    : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *selectorLayout = new QHBoxLayout();
    QLabel *statLabel = new QLabel("Select EV Target Stat:", this);
    m_evSelector = new QComboBox(this);
    m_evSelector->addItem("Attack", static_cast<int>(Pocket::Save::EVType::Attack));
    m_evSelector->addItem("Defense", static_cast<int>(Pocket::Save::EVType::Defense));
    m_evSelector->addItem("Speed", static_cast<int>(Pocket::Save::EVType::Speed));
    m_evSelector->addItem("HP", static_cast<int>(Pocket::Save::EVType::HP));
    m_evSelector->addItem("Special Attack", static_cast<int>(Pocket::Save::EVType::SpecialAttack));
    m_evSelector->addItem("Special Defense", static_cast<int>(Pocket::Save::EVType::SpecialDefense));

    selectorLayout->addWidget(statLabel);
    selectorLayout->addWidget(m_evSelector);

    m_timingBar = new QProgressBar(this);
    m_timingBar->setRange(0, 100);
    m_timingBar->setValue(0);
    m_timingBar->setTextVisible(false);
    m_timingBar->setStyleSheet(
        "QProgressBar { border: 2px solid #5E81AC; border-radius: 6px; background: #2E3440; height: 24px; }"
        "QProgressBar::chunk { background-color: #88C0D0; border-radius: 4px; }"
    );

    m_hitBtn = new QPushButton("HIT! (or Press SPACE)", this);
    m_hitBtn->setStyleSheet("font-weight: bold; background-color: #A3BE8C; color: #2E3440; padding: 6px;");

    m_resultLabel = new QLabel("Press Start Training to begin.", this);
    m_resultLabel->setStyleSheet("font-size: 11px; color: #D8DEE9;");

    layout->addLayout(selectorLayout);
    layout->addWidget(m_timingBar);
    layout->addWidget(m_hitBtn);
    layout->addWidget(m_resultLabel);

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &TrainingTimingBarWidget::onTick);
    connect(m_hitBtn, &QPushButton::clicked, this, &TrainingTimingBarWidget::onHitClicked);

    setFocusPolicy(Qt::StrongFocus);
}

void TrainingTimingBarWidget::startTraining() {
    m_isTrainingActive = true;
    m_cursorPosition = 0;
    m_direction = 1;
    m_resultLabel->setText("Timing bar moving... Hit SPACE in the center zone!");
    m_resultLabel->setStyleSheet("font-weight: bold; color: #EBCB8B;");
    m_animTimer->start(16); // ~60 FPS animation tick during active mini-game
}

void TrainingTimingBarWidget::stopTraining() {
    m_isTrainingActive = false;
    m_animTimer->stop();
}

void TrainingTimingBarWidget::onTick() {
    if (!m_isTrainingActive) return;

    m_cursorPosition += (m_direction * 2);
    if (m_cursorPosition >= 100) {
        m_cursorPosition = 100;
        m_direction = -1;
    } else if (m_cursorPosition <= 0) {
        m_cursorPosition = 0;
        m_direction = 1;
    }

    m_timingBar->setValue(m_cursorPosition);
}

void TrainingTimingBarWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && m_isTrainingActive) {
        evaluateHit();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void TrainingTimingBarWidget::onHitClicked() {
    if (m_isTrainingActive) {
        evaluateHit();
    } else {
        startTraining();
    }
}

void TrainingTimingBarWidget::evaluateHit() {
    stopTraining();

    int pos = m_cursorPosition;
    Pocket::Save::EVType stat = static_cast<Pocket::Save::EVType>(m_evSelector->currentData().toInt());

    int evPoints = 0;
    double score = 0.0;
    QString resultText;

    if (pos >= 40 && pos <= 60) {
        // Perfect Center Zone
        evPoints = 4;
        score = 1.0;
        resultText = "★ PERFECT HIT! +4 EV Points staged in Pending Ledger.";
        m_resultLabel->setStyleSheet("font-weight: bold; color: #A3BE8C;");
    } else if (pos >= 25 && pos <= 75) {
        // Good Zone
        evPoints = 2;
        score = 0.7;
        resultText = "✓ GOOD HIT! +2 EV Points staged in Pending Ledger.";
        m_resultLabel->setStyleSheet("font-weight: bold; color: #EBCB8B;");
    } else {
        // Miss
        evPoints = 0;
        score = 0.0;
        resultText = "✗ MISS! Timing outside target zone (+0 EV).";
        m_resultLabel->setStyleSheet("font-weight: bold; color: #BF616A;");
    }

    m_resultLabel->setText(resultText);
    emit trainingCompleted(stat, evPoints, score);
}

} // namespace Pocket::App
