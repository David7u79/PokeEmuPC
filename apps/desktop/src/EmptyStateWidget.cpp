#include "EmptyStateWidget.hpp"

#include "Theme.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace Pocket::App {

EmptyStateWidget::EmptyStateWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(8);
    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_title->setFont(titleFont);
    m_supportingText = new QLabel(this);
    m_supportingText->setAlignment(Qt::AlignCenter);
    m_supportingText->setWordWrap(true);
    m_button = new QPushButton(this);
    layout->addWidget(m_title);
    layout->addWidget(m_supportingText);
    layout->addWidget(m_button, 0, Qt::AlignHCenter);
    connect(m_button, &QPushButton::clicked, this, &EmptyStateWidget::actionRequested);
    setStyleSheet(QString("QLabel { background: transparent; color: %1; }").arg(Theme::textSecondary().name()));
}

void EmptyStateWidget::setState(const QString& title, const QString& supportingText, const QString& buttonText)
{
    m_title->setText(title);
    m_supportingText->setText(supportingText);
    m_button->setText(buttonText);
    m_button->setVisible(!buttonText.isEmpty());
}

} // namespace Pocket::App
