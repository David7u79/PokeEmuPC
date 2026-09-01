#include "DesktopWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

namespace Pocket::CompanionApp {

DesktopWidget::DesktopWidget(std::shared_ptr<Core::IpcClient> ipcClient, QWidget *parent)
    : QWidget(parent), m_ipcClient(std::move(ipcClient)) {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(160, 180);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    m_nameLabel = new QLabel("Partner", this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("color: white; font-weight: bold; font-size: 13px;");

    m_levelLabel = new QLabel("Lv. 5", this);
    m_levelLabel->setAlignment(Qt::AlignCenter);
    m_levelLabel->setStyleSheet("color: #FFD700; font-weight: bold; font-size: 11px;");

    m_hungerBar = new QProgressBar(this);
    m_hungerBar->setRange(0, 100);
    m_hungerBar->setValue(85);
    m_hungerBar->setFixedHeight(8);
    m_hungerBar->setTextVisible(false);
    m_hungerBar->setStyleSheet("QProgressBar { background-color: #333; border-radius: 4px; } QProgressBar::chunk { background-color: #4CAF50; border-radius: 4px; }");

    m_moodBar = new QProgressBar(this);
    m_moodBar->setRange(0, 100);
    m_moodBar->setValue(90);
    m_moodBar->setFixedHeight(8);
    m_moodBar->setTextVisible(false);
    m_moodBar->setStyleSheet("QProgressBar { background-color: #333; border-radius: 4px; } QProgressBar::chunk { background-color: #2196F3; border-radius: 4px; }");

    layout->addSpacing(75); // Reserve space for creature silhouette drawing
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_levelLabel);
    layout->addWidget(m_hungerBar);
    layout->addWidget(m_moodBar);

    m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);

    connect(&m_governor, &PocketPartner::DesktopCompanion::FramerateGovernor::renderTick, this, [this]() {
        update(); // 0 FPS when idle, updates on demand
    });
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

void DesktopWidget::updateStatus(double hunger, double mood, int level) {
    m_hungerBar->setValue(static_cast<int>(hunger));
    m_moodBar->setValue(static_cast<int>(mood));
    m_levelLabel->setText(QString("Lv. %1").arg(level));
    update();
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
    painter.setBrush(QColor(20, 25, 35, 210));
    painter.setPen(QPen(QColor(100, 180, 255, 180), 1.5));
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 12, 12);

    // Neutral placeholder creature silhouette (Asset policy compliance: no proprietary Pokémon art)
    painter.setBrush(QColor(70, 160, 240, 240));
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.drawEllipse(width() / 2 - 25, 15, 50, 50);

    // Cute ears silhouette
    painter.drawEllipse(width() / 2 - 26, 10, 14, 18);
    painter.drawEllipse(width() / 2 + 12, 10, 14, 18);
}

} // namespace Pocket::CompanionApp
