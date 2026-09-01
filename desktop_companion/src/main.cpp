#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

class CompanionWidget : public QWidget {
public:
    CompanionWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(120, 120);

        m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);

        QObject::connect(&m_governor, &PocketPartner::DesktopCompanion::FramerateGovernor::renderTick, this, [this]() {
            update(); // Event-driven paint update
        });
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::InteractiveAnimation);
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPosition().toPoint() - m_dragPosition);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);
            event->accept();
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Neutral placeholder desktop companion silhouette (Asset policy compliance)
        painter.setBrush(QColor(60, 140, 230, 220));
        painter.setPen(QPen(QColor(255, 255, 255), 2));
        painter.drawEllipse(10, 10, 100, 100);

        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Companion\n(Idle)");
    }

private:
    QPoint m_dragPosition;
    PocketPartner::DesktopCompanion::FramerateGovernor m_governor;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PocketCompanion");

    CompanionWidget widget;
    widget.show();

    return app.exec();
}
