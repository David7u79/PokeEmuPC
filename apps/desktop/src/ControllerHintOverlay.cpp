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

QRectF controlRect(const Pocket::Input::ControllerControl& control, const QRectF& target)
{
    return {target.left() + control.x * target.width(), target.top() + control.y * target.height(),
            control.width * target.width(), control.height * target.height()};
}
} // namespace

void ControllerHintOverlay::setSystem(const QString& system)
{
    m_layout = Pocket::Input::ControllerLayout::forSystem(system);
    m_artwork.setSystem(system);
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

void ControllerHintOverlay::paint(QPainter& painter, const QRect& bounds) const
{
    if (!isValid() || bounds.isEmpty())
        return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(bounds, QColor(0, 0, 0, 175));

    const QRectF target = m_artwork.targetRect(bounds.size()).translated(bounds.topLeft());
    m_artwork.render(painter, target);

    if (!m_mapping) {
        painter.restore();
        return;
    }

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

        const QRectF button = controlRect(control, target);
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

} // namespace Pocket::App
