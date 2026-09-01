#pragma once

#include <QWidget>
#include <QImage>
#include <QRect>
#include <vector>
#include <cstdint>

namespace Pocket::App {

enum class NdsScreenLayout {
    Vertical,      // Top above Bottom
    Horizontal,    // Top left, Bottom right
    FocusedTop,    // Top screen enlarged
    FocusedBottom  // Bottom screen enlarged
};

class NdsDisplayWidget : public QWidget {
    Q_OBJECT
public:
    explicit NdsDisplayWidget(QWidget *parent = nullptr);
    ~NdsDisplayWidget() override = default;

    void setLayoutMode(NdsScreenLayout mode);
    NdsScreenLayout layoutMode() const { return m_layoutMode; }

    void updateFramebuffers(const uint8_t* topRgba, const uint8_t* bottomRgba);

    // Calculate bounding rects for current widget size
    void calculateScreenRects(const QRect& totalBounds, QRect& outTopRect, QRect& outBottomRect) const;

signals:
    void touchInputChanged(int touchX, int touchY, bool isPressed);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void processTouchEvent(const QPoint& mousePos, bool isPressed);

    NdsScreenLayout m_layoutMode{NdsScreenLayout::Vertical};

    QImage m_topImage;
    QImage m_bottomImage;

    QRect m_currentTopRect;
    QRect m_currentBottomRect;
};

} // namespace Pocket::App
