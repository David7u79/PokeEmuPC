#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QKeyEvent>
#include "pocket/save/PendingGameReward.hpp"

namespace Pocket::App {

class TrainingTimingBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrainingTimingBarWidget(QWidget *parent = nullptr);
    ~TrainingTimingBarWidget() override = default;

    void startTraining();
    void stopTraining();

signals:
    void trainingCompleted(Pocket::Save::EVType stat, int evPoints, double qualityScore);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTick();
    void onHitClicked();

private:
    void evaluateHit();

    QComboBox *m_evSelector{nullptr};
    QProgressBar *m_timingBar{nullptr};
    QPushButton *m_hitBtn{nullptr};
    QLabel *m_resultLabel{nullptr};

    QTimer *m_animTimer{nullptr};
    int m_cursorPosition{0};
    int m_direction{1}; // 1 = right, -1 = left
    bool m_isTrainingActive{false};
};

} // namespace Pocket::App
