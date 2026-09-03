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

QString controlDisplayName(const QString& id)
{
    static const QHash<QString, QString> names{{"DPAD_UP", "\u2191"}, {"DPAD_DOWN", "\u2193"},
                                               {"DPAD_LEFT", "\u2190"}, {"DPAD_RIGHT", "\u2192"},
                                               {"A", "A"}, {"B", "B"}, {"X", "X"}, {"Y", "Y"},
                                               {"L", "L"}, {"R", "R"}, {"START", "START"}, {"SELECT", "SELECT"}};
    return names.value(id, id);
}

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

QRectF ControllerHintOverlay::labelRectFor(const QString& id, const QSize& widgetSize) const
{
    if (!m_layout || !isValid())
        return {};
    const auto controls = m_layout->controls();
    const auto it = std::find_if(controls.cbegin(), controls.cend(), [&id](const auto& c) { return c.id == id; });
    if (it == controls.cend() || !it->isBindable())
        return {};
    const QRectF button = layoutControlRect(*it, artworkRect(widgetSize));
    const QRect bounds(QPoint(), widgetSize);
    const int padding = 3;
    QFont font;
    font.setBold(true);
    font.setPointSizeF(qMax(7.0, qMin(12.0, artworkRect(widgetSize).height() / 24.0)));
    const QFontMetrics topMetrics(font);
    QFont bottomFont(font);
    bottomFont.setBold(false);
    bottomFont.setPointSizeF(font.pointSizeF() * .75);
    const QFontMetrics bottomMetrics(bottomFont);
    const auto binding = m_mapping ? m_mapping->binding(m_artwork.system(), id) : std::nullopt;
    // A badge needs a small, stable minimum width; this also keeps the narrow
    // individual D-pad direction targets from pretending a two-line legend fits.
    const int width = qMax(48, qMax(topMetrics.horizontalAdvance(binding ? displayLabel(*binding) : QString(QChar(0x2014))),
                                    bottomMetrics.horizontalAdvance(controlDisplayName(id))) + padding * 2);
    const int height = topMetrics.height() + bottomMetrics.height() + padding * 2;
    if (width <= button.width() && height <= button.height())
        return QRectF(button.center().x() - width / 2.0, button.center().y() - height / 2.0, width, height);
    const int margin = 3;
    const bool vertical = id == "DPAD_UP" || id == "DPAD_DOWN";
    const bool after = id == "DPAD_DOWN" || id == "DPAD_RIGHT";
    QRectF candidate;
    if (vertical) {
        const qreal y = after ? button.bottom() + margin : button.top() - margin - height;
        candidate = {button.center().x() - width / 2.0, y, qreal(width), qreal(height)};
    } else {
        const int roomRight = bounds.right() - qCeil(button.right());
        const int roomLeft = qFloor(button.left()) - bounds.left();
        const bool right = id == "DPAD_RIGHT" ? true : id == "DPAD_LEFT" ? false : roomRight >= roomLeft;
        candidate = {right ? button.right() + margin : button.left() - margin - width,
                     button.center().y() - height / 2.0, qreal(width), qreal(height)};
    }
    candidate.moveLeft(qBound<qreal>(bounds.left(), candidate.left(), bounds.right() - candidate.width() + 1));
    candidate.moveTop(qBound<qreal>(bounds.top(), candidate.top(), bounds.bottom() - candidate.height() + 1));
    return candidate;
}

void ControllerHintOverlay::setPressed(const QString& controlId, bool pressed)
{
    if (pressed) m_pressedControls.insert(controlId); else m_pressedControls.remove(controlId);
}

void ControllerHintOverlay::clearPressed() { m_pressedControls.clear(); }
bool ControllerHintOverlay::isPressed(const QString& controlId) const { return m_pressedControls.contains(controlId); }
void ControllerHintOverlay::setCaptureHighlight(const QString& controlId, bool active)
{
    if (active) m_captureControls.insert(controlId); else m_captureControls.remove(controlId);
}

void ControllerHintOverlay::paintPressed(QPainter& painter, const QSize& widgetSize) const
{
    painter.save(); painter.setRenderHint(QPainter::Antialiasing, true);
    for (const QString& id : m_pressedControls) {
        const QRectF rect = controlRect(id, widgetSize);
        if (rect.isEmpty()) continue;
        const qreal radius = qMin(rect.width(), rect.height()) * .15;
        painter.setBrush(QColor(255, 255, 255, 77));
        painter.setPen(QPen(m_captureControls.contains(id) ? QColor(255, 220, 0) : QColor(255, 255, 255, 179), 1));
        painter.drawRoundedRect(rect, radius, radius);
    }
    painter.restore();
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
    constexpr int padding = 3;
    std::vector<QRect> usedLabelRects;

    for (const auto& control : m_layout->controls()) {
        if (!control.isBindable())
            continue;

        const auto binding = m_mapping->binding(m_artwork.system(), control.id);
        const QString label = binding ? displayLabel(*binding) : QString(QChar(0x2014));
        const QString originalName = controlDisplayName(control.id);

        const QRectF button = layoutControlRect(control, target);
        QFont insideFont = painter.font(); insideFont.setBold(true);
        insideFont.setPixelSize(qMax(7, qFloor(button.height() * 0.42)));
        const QFontMetrics insideMetrics(insideFont);
        QFont insideBottomFont(insideFont); insideBottomFont.setBold(false); insideBottomFont.setPixelSize(qMax(6, qRound(insideFont.pixelSize() * .75)));
        const QFontMetrics insideBottomMetrics(insideBottomFont);
        const QRect insideRect = button.toAlignedRect();
        const bool fitsInside = qMax(insideMetrics.horizontalAdvance(label), insideBottomMetrics.horizontalAdvance(originalName)) + padding * 2 <= insideRect.width()
                                && insideMetrics.height() + insideBottomMetrics.height() + padding * 2 <= insideRect.height();
        if (fitsInside) {
            const int topHeight = insideMetrics.height();
            const QRect topRect(insideRect.x(), insideRect.center().y() - (topHeight + insideBottomMetrics.height()) / 2, insideRect.width(), topHeight);
            painter.setFont(insideFont); painter.setPen(Qt::white); painter.drawText(topRect, Qt::AlignCenter, label);
            painter.setFont(insideBottomFont); painter.setPen(QColor(192, 192, 192)); painter.drawText(QRect(insideRect.x(), topRect.bottom(), insideRect.width(), insideBottomMetrics.height()), Qt::AlignCenter, originalName);
            painter.setBrush(Qt::NoBrush); painter.setPen(QPen(QColor(255, 255, 255, 130), 1)); painter.drawRoundedRect(insideRect.adjusted(1, 1, -1, -1), 3, 3);
            continue;
        }
        const QRect labelRect = labelRectFor(control.id, widgetSize).toAlignedRect();
        const bool overlapsLabel = std::any_of(usedLabelRects.cbegin(), usedLabelRects.cend(), [&labelRect](const QRect& used) { return labelRect.intersects(used); });
        if (labelRect.isEmpty() || overlapsLabel)
            continue;
        QFont outsideFont = painter.font(); outsideFont.setBold(true); outsideFont.setPointSizeF(qMax(7.0, qMin(12.0, target.height() / 24.0)));
        QFont outsideBottomFont(outsideFont); outsideBottomFont.setBold(false); outsideBottomFont.setPointSizeF(outsideFont.pointSizeF() * .75);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 210));
        painter.drawRoundedRect(labelRect, 3, 3);
        const int topHeight = QFontMetrics(outsideFont).height();
        const int bottomHeight = QFontMetrics(outsideBottomFont).height();
        const QRect topRect(labelRect.x(), labelRect.center().y() - (topHeight + bottomHeight) / 2, labelRect.width(), topHeight);
        painter.setFont(outsideFont); painter.setPen(Qt::white); painter.drawText(topRect, Qt::AlignCenter, label);
        painter.setFont(outsideBottomFont); painter.setPen(QColor(192, 192, 192)); painter.drawText(QRect(labelRect.x(), topRect.bottom(), labelRect.width(), bottomHeight), Qt::AlignCenter, originalName);
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
