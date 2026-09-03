#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace Pocket::App {

class EmptyStateWidget : public QWidget {
    Q_OBJECT

public:
    explicit EmptyStateWidget(QWidget* parent = nullptr);

    void setState(const QString& title, const QString& supportingText,
                  const QString& buttonText = {});

signals:
    void actionRequested();

private:
    QLabel* m_title{nullptr};
    QLabel* m_supportingText{nullptr};
    QPushButton* m_button{nullptr};
};

} // namespace Pocket::App
