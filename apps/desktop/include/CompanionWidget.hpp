#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>

namespace Pocket::App {

class CompanionWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompanionWidget(QWidget *parent = nullptr);

private:
    QLabel *m_companionNameLabel{nullptr};
    QProgressBar *m_hungerBar{nullptr};
    QProgressBar *m_moodBar{nullptr};
    QProgressBar *m_fatigueBar{nullptr};
};

} // namespace Pocket::App
