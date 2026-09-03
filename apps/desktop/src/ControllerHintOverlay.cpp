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

constexpr int kBadgePadding = 3;
constexpr int kBadgeMinPixelSize = 7;

// Only the key: which button it is, the artwork already says.
QString badgeText(const std::optional<Pocket::Input::InputBinding>& binding)
{
    return binding ? displayLabel(*binding) : QString(QChar(0x2014));
}

bool isDPad(const QString& id)
{
    return id.startsWith(QLatin1String("DPAD_"));
}

// Largest size that fits the box, down to a floor below which the badge is drawn
// outside the button instead.
QFont badgeFont(const QString& text, const QRectF& box)
{
    QFont font;
    font.setBold(true);
    for (int pixelSize = qMax(kBadgeMinPixelSize, qFloor(box.height()) - kBadgePadding * 2);
         pixelSize > kBadgeMinPixelSize; --pixelSize) {
        font.setPixelSize(pixelSize);
        const QFontMetrics metrics(font);
        if (metrics.horizontalAdvance(text) + kBadgePadding * 2 <= box.width()
            && metrics.height() <= box.height())
            return font;
    }
    font.setPixelSize(kBadgeMinPixelSize);
    return font;
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

QFont ControllerHintOverlay::badgeFontFor(const QString& id, const QSize& widgetSize) const
{
    const auto fontFor = [this, &widgetSize](const Pocket::Input::ControllerControl& control) {
        const auto binding = m_mapping ? m_mapping->binding(m_artwork.system(), control.id) : std::nullopt;
        return badgeFont(badgeText(binding), layoutControlRect(control, artworkRect(widgetSize)));
    };
    const auto* control = m_layout ? m_layout->controlById(id) : nullptr;
    if (!control)
        return badgeFont(QString(), {});
    QFont font = fontFor(*control);
    // The four directions read as one control, so they share the smallest size any
    // of them can hold: a bigger "W" over a smaller "S" looks like a mistake.
    if (isDPad(id)) {
        for (const auto& other : m_layout->controls())
            if (isDPad(other.id))
                font.setPixelSize(qMin(font.pixelSize(), fontFor(other).pixelSize()));
    }
    return font;
}

QRectF ControllerHintOverlay::labelRectFor(const QString& id, const QSize& widgetSize) const
{
    if (!m_layout || !isValid())
        return {};
    const auto controls = m_layout->controls();
    const auto it = std::find_if(controls.cbegin(), controls.cend(), [&id](const auto& c) { return c.id == id; });
    if (it == controls.cend() || !it->isBindable())
        return {};
    const QRectF target = artworkRect(widgetSize);
    const QRectF button = layoutControlRect(*it, target);
    const QRect bounds(QPoint(), widgetSize);
    const QString text = badgeText(m_mapping ? m_mapping->binding(m_artwork.system(), id) : std::nullopt);
    const QFontMetrics metrics{badgeFontFor(id, widgetSize)};

    // The badge belongs on the button it names; only a button too small to hold it
    // legibly pushes it outside.
    if (metrics.horizontalAdvance(text) + kBadgePadding * 2 <= button.width()
        && metrics.height() <= button.height())
        return button;

    const qreal width = metrics.horizontalAdvance(text) + kBadgePadding * 2;
    const qreal height = metrics.height() + kBadgePadding * 2;
    const int margin = 3;
    const bool vertical = id == "DPAD_UP" || id == "DPAD_DOWN";
    const bool after = id == "DPAD_DOWN" || id == "DPAD_RIGHT";
    QRectF candidate;
    if (vertical) {
        candidate = {button.center().x() - width / 2.0,
                     after ? button.bottom() + margin : button.top() - margin - height, width, height};
    } else {
        const bool right = id == "DPAD_RIGHT" ? true
                           : id == "DPAD_LEFT" ? false
                                               : bounds.right() - button.right() >= button.left() - bounds.left();
        candidate = {right ? button.right() + margin : button.left() - margin - width,
                     button.center().y() - height / 2.0, width, height};
    }
    candidate.moveLeft(qBound<qreal>(bounds.left(), candidate.left(), bounds.right() - candidate.width() + 1));
    candidate.moveTop(qBound<qreal>(bounds.top(), candidate.top(), bounds.bottom() - candidate.height() + 1));

    // A hint drawn over the running game is worse than no hint: the GB's D-pad sits
    // right under the screen, and its up label used to land on the picture.
    for (const auto& other : m_layout->controls()) {
        if (other.kind != Pocket::Input::ControlKind::Screen
            && other.kind != Pocket::Input::ControlKind::Touchscreen)
            continue;
        if (candidate.intersects(layoutControlRect(other, target)))
            return {};
    }
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
        // Darkening, not lightening: a white wash was invisible on the DS's white
        // buttons, and a pressed button reads as pushed in anyway. Light enough that
        // the button underneath still looks like itself.
        painter.setBrush(QColor(0, 0, 0, 70));
        painter.setPen(QPen(m_captureControls.contains(id) ? QColor(255, 220, 0) : QColor(255, 255, 255, 150), 2));
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
    std::vector<QRect> usedLabelRects;

    for (const auto& control : m_layout->controls()) {
        if (!control.isBindable())
            continue;

        const QString text = badgeText(m_mapping->binding(m_artwork.system(), control.id));
        const QRectF button = layoutControlRect(control, target);
        const QRect labelRect = labelRectFor(control.id, widgetSize).toAlignedRect();
        if (labelRect.isEmpty())
            continue;

        // Inside the button the artwork is the background; outside it needs its own,
        // and it must not land on a neighbour's label.
        const bool inside = button.toAlignedRect().contains(labelRect);
        if (!inside) {
            const bool taken = std::any_of(usedLabelRects.cbegin(), usedLabelRects.cend(),
                                           [&labelRect](const QRect& used) { return labelRect.intersects(used); });
            if (taken)
                continue;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 210));
            painter.drawRoundedRect(labelRect, 3, 3);
            usedLabelRects.push_back(labelRect);
        }

        painter.setFont(badgeFontFor(control.id, widgetSize));
        if (inside) {
            painter.setPen(QColor(0, 0, 0, 200));
            painter.drawText(labelRect.translated(1, 1), Qt::AlignCenter, text);
        }
        painter.setPen(Qt::white);
        painter.drawText(labelRect, Qt::AlignCenter, text);
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
