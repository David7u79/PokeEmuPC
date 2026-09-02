#include "ControllerHintOverlay.hpp"

#include "pocket/input/ControllerMapping.hpp"

#include <QFontMetrics>
#include <QPainter>

#include <algorithm>
#include <vector>

namespace Pocket::App {
namespace {
QString displayLabel(const Pocket::Input::InputBinding& binding)
{
    QString label = binding.label();
    if (binding.device == Pocket::Input::InputDevice::Keyboard && label.startsWith("Keyboard "))
        label.remove(0, QStringLiteral("Keyboard ").size());
    return label;
}

QRectF layoutControlRect(const Pocket::Input::ControllerControl& control, const QRectF& target)
{
    return {target.left() + control.x * target.width(), target.top() + control.y * target.height(),
            control.width * target.width(), control.height * target.height()};
}
} // namespace

void ControllerHintOverlay::setSystem(const QString& system)
{
    m_layout = Pocket::Input::ControllerLayout::forSystem(system);
    m_artwork.setSystem(system);
    m_framePixmap = {};
}

void ControllerHintOverlay::setMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping)
{
    m_mapping = std::move(mapping);
}

bool ControllerHintOverlay::isValid() const
{
    return m_layout.has_value() && m_artwork.isValid();
}

QSize ControllerHintOverlay::preferredSize(const QSize& available) const
{
    if (!isValid() || available.isEmpty())
        return {};
    return m_artwork.targetRect(available).size().toSize();
}

QRectF ControllerHintOverlay::artworkRect(const QSize& widgetSize) const
{
    if (!isValid())
        return {};
    return m_artwork.targetRect(widgetSize);
}

QRectF ControllerHintOverlay::controlRect(const QString& id, const QSize& widgetSize) const
{
    if (!m_layout)
        return {};
    const auto controls = m_layout->controls();
    const auto control = std::find_if(controls.cbegin(), controls.cend(), [&id](const auto& candidate) {
        return candidate.id == id;
    });
    if (control == controls.cend())
        return {};
    return layoutControlRect(*control, artworkRect(widgetSize));
}

void ControllerHintOverlay::paintFrame(QPainter& painter, const QSize& widgetSize) const
{
    if (!isValid() || widgetSize.isEmpty())
        return;
    const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : 1.0;
    if (m_framePixmap.isNull() || m_cachedSize != widgetSize || !qFuzzyCompare(m_cachedDevicePixelRatio, dpr)
        || m_cachedSystem != m_artwork.system()) {
        QPixmap pixmap(qRound(widgetSize.width() * dpr), qRound(widgetSize.height() * dpr));
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter cachePainter(&pixmap);
        m_artwork.render(cachePainter, artworkRect(widgetSize));
        m_framePixmap = std::move(pixmap);
        m_cachedSize = widgetSize;
        m_cachedDevicePixelRatio = dpr;
        m_cachedSystem = m_artwork.system();
        ++m_rasterizationCount;
    }
    painter.drawPixmap(QPointF(), m_framePixmap);
}

void ControllerHintOverlay::paintKeyLabels(QPainter& painter, const QSize& widgetSize) const
{
    if (!isValid() || !m_mapping)
        return;
    const QRect bounds(QPoint(), widgetSize);
    const QRectF target = artworkRect(widgetSize);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont font = painter.font();
    font.setPointSizeF(qMax(7.0, qMin(12.0, target.height() / 24.0)));
    font.setBold(true);
    painter.setFont(font);
    const QFontMetrics metrics(font);
    constexpr int margin = 3;
    constexpr int padding = 3;
    std::vector<QRect> usedLabelRects;

    for (const auto& control : m_layout->controls()) {
        if (!control.isBindable())
            continue;

        const auto binding = m_mapping->binding(m_artwork.system(), control.id);
        const QString label = binding ? displayLabel(*binding) : QStringLiteral("—");
        if (label.isEmpty())
            continue;

        const QRectF button = layoutControlRect(control, target);
        const int roomRight = bounds.right() - qCeil(button.right());
        const int roomLeft = qFloor(button.left()) - bounds.left();
        const bool preferredRight = roomRight >= roomLeft;
        QString text;
        QRect labelRect;
        for (const bool placeRight : {preferredRight, !preferredRight}) {
            const int available = qMax(0, (placeRight ? roomRight : roomLeft) - margin - padding * 2);
            const QString candidateText = metrics.elidedText(label, Qt::ElideRight, available);
            if (candidateText.isEmpty())
                continue;

            const int boxWidth = metrics.horizontalAdvance(candidateText) + padding * 2;
            const int boxHeight = metrics.height() + padding * 2;
            const int x = placeRight ? qCeil(button.right()) + margin
                                     : qFloor(button.left()) - margin - boxWidth;
            const int y = qBound(bounds.top(), qRound(button.center().y()) - boxHeight / 2,
                                 bounds.bottom() - boxHeight + 1);
            const QRect candidateRect(x, y, boxWidth, boxHeight);
            const bool overlapsLabel = std::any_of(usedLabelRects.cbegin(), usedLabelRects.cend(),
                                                   [&candidateRect](const QRect& usedRect) {
                                                       return candidateRect.intersects(usedRect);
                                                   });
            if (!overlapsLabel) {
                text = candidateText;
                labelRect = candidateRect;
                break;
            }
        }
        if (text.isEmpty())
            continue;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 210));
        painter.drawRoundedRect(labelRect, 3, 3);
        painter.setPen(binding ? Qt::white : QColor(185, 185, 185));
        painter.drawText(labelRect, Qt::AlignCenter, text);
        usedLabelRects.push_back(labelRect);
    }
    painter.restore();
}

void ControllerHintOverlay::paint(QPainter& painter, const QRect& bounds) const
{
    if (!isValid() || bounds.isEmpty())
        return;
    painter.save();
    painter.translate(bounds.topLeft());
    paintFrame(painter, bounds.size());
    paintKeyLabels(painter, bounds.size());
    painter.restore();
}

} // namespace Pocket::App
