#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include "pocketpartner/desktop_companion/FramerateGovernor.hpp"

class DesktopCompanionWidget : public QWidget {
public:
    DesktopCompanionWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(120, 120);

        m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::FullyStatic);

        QObject::connect(&m_governor, &PocketPartner::DesktopCompanion::FramerateGovernor::renderTick, this, [this]() {
            update();
        });
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_governor.setRenderState(PocketPartner::DesktopCompanion::RenderState::InteractiveAnimation);
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPosition().toPoint() - m_dragPos);
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

        // Neutral silhouette asset (Asset policy compliance)
        painter.setBrush(QColor(40, 160, 220, 230));
        painter.setPen(QPen(QColor(255, 255, 255), 2));
        painter.drawEllipse(10, 10, 100, 100);

        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Companion");
    }

private:
    QPoint m_dragPos;
    PocketPartner::DesktopCompanion::FramerateGovernor m_governor;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PocketCompanion");

    DesktopCompanionWidget widget;
    widget.show();

    return app.exec();
}
