#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include "pocket/companion/CompanionSimulator.hpp"

namespace Pocket::App {

class EmptyStateWidget;

class CompanionWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompanionWidget(QWidget *parent = nullptr);

    void refreshDisplay();
    void setCompanionActive(bool active);
    bool hasActiveCompanion() const { return m_hasActiveCompanion; }

private slots:
    void onFeedClicked();
    void onPetClicked();
    void onPlayClicked();
    void onRestClicked();

private:
    Pocket::Companion::CompanionSimulator m_simulator;
    Pocket::Companion::CompanionState m_state;
    bool m_hasActiveCompanion{false};

    QStackedWidget *m_stack{nullptr};
    EmptyStateWidget *m_emptyState{nullptr};
    QWidget *m_contentCard{nullptr};

    QLabel *m_nameLabel{nullptr};
    QLabel *m_bondLabel{nullptr};
    QProgressBar *m_hungerBar{nullptr};
    QProgressBar *m_moodBar{nullptr};
    QProgressBar *m_energyBar{nullptr};
    QProgressBar *m_fatigueBar{nullptr};

    QPushButton *m_feedBtn{nullptr};
    QPushButton *m_petBtn{nullptr};
    QPushButton *m_playBtn{nullptr};
    QPushButton *m_restBtn{nullptr};
};

} // namespace Pocket::App
