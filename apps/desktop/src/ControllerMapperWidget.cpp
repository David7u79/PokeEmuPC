#include "ControllerMapperWidget.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>

namespace Pocket::App {
using namespace Pocket::Input;

ControllerMapperWidget::ControllerMapperWidget(std::shared_ptr<ControllerMapping> mapping, QWidget* parent)
    : QWidget(parent), m_mapping(std::move(mapping))
{
    if (!m_mapping) m_mapping = std::make_shared<ControllerMapping>();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    auto* header = new QHBoxLayout;
    m_header = header;
    m_systemSelector = new QComboBox(this); m_systemSelector->addItems({"GB", "GBC", "GBA", "NDS"}); m_systemSelector->setCurrentText(m_system);
    m_scopeSelector = new QComboBox(this); m_scopeSelector->addItems({"Global", "Per system"});
    m_scopeSelector->setCurrentIndex(m_mapping->scope() == MappingScope::Global ? 0 : 1);
    m_keyboardPreset = new QPushButton("Keyboard preset", this); m_gamepadPreset = new QPushButton("Generic gamepad preset", this);
    m_reset = new QPushButton("Reset to defaults", this); m_clearAll = new QPushButton("Clear all", this); m_save = new QPushButton("Save", this);
    header->addWidget(m_systemSelector); header->addWidget(m_scopeSelector); header->addWidget(m_keyboardPreset); header->addWidget(m_gamepadPreset); header->addWidget(m_reset); header->addWidget(m_clearAll); header->addWidget(m_save); header->addStretch();
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(0, 0, 0, 0); layout->addLayout(header); layout->addStretch();
    connect(m_systemSelector, &QComboBox::currentTextChanged, this, &ControllerMapperWidget::setSystem);
    connect(m_scopeSelector, &QComboBox::currentIndexChanged, this, [this](int index) { m_mapping->setScope(index == 0 ? MappingScope::Global : MappingScope::PerSystem); update(); });
    connect(m_keyboardPreset, &QPushButton::clicked, this, [this] { loadPreset(false); });
    connect(m_gamepadPreset, &QPushButton::clicked, this, [this] { loadPreset(true); });
    connect(m_reset, &QPushButton::clicked, this, [this] { m_mapping->resetToDefaults(m_system); emit mappingChanged(); update(); });
    connect(m_clearAll, &QPushButton::clicked, this, [this] { m_mapping->clearAll(m_system); emit mappingChanged(); update(); });
    connect(m_save, &QPushButton::clicked, this, [this] { QSettings settings("PocketPartnerProject", "PocketPartner"); m_mapping->save(settings); });
    setSystem(m_system);
}

void ControllerMapperWidget::setSystem(const QString& system)
{
    const auto layout = ControllerLayout::forSystem(system);
    m_system = system; m_hoverId.clear(); m_selectedId.clear(); m_capturingId.clear(); m_conflictIds.clear();
    if (layout) m_interactive.setLayout(*layout);
    m_artwork.setSystem(system);
    if (m_systemSelector && m_systemSelector->currentText() != system) m_systemSelector->setCurrentText(system);
    update();
}

QRect ControllerMapperWidget::canvasRect() const
{
    // Everything below the toolbar row. Using the full widget rect would slide the
    // controller under the buttons and hide the capture prompt behind them.
    const int top = m_header ? m_header->geometry().bottom() + 1 : 0;
    return rect().adjusted(0, qMax(0, top), 0, 0);
}
QRectF ControllerMapperWidget::targetRect() const { const QRect canvas = canvasRect(); return m_artwork.targetRect(canvas.size()).translated(canvas.topLeft()); }
QRectF ControllerMapperWidget::artworkRect() const { return targetRect(); }
QRectF ControllerMapperWidget::controlRect(const QString& id) const { if (!m_interactive.hasLayout()) return {}; const auto* control = ControllerLayout::forSystem(m_system)->controlById(id); return control ? m_interactive.rectFor(*control, targetRect()) : QRectF(); }
ControlVisualState ControllerMapperWidget::visualStateFor(const QString& id) const { return states().value(id, ControlVisualState::NORMAL); }

QHash<QString, ControlVisualState> ControllerMapperWidget::states() const
{
    QHash<QString, ControlVisualState> result;
    const auto layout = ControllerLayout::forSystem(m_system);
    if (!layout) return result;
    for (const auto& control : layout->controls()) {
        ControlVisualState state = m_mapping->binding(m_system, control.id) ? ControlVisualState::MAPPED : ControlVisualState::NORMAL;
        if (control.id == m_hoverId) state = ControlVisualState::HOVER;
        if (control.id == m_selectedId) state = ControlVisualState::SELECTED;
        if (m_conflictIds.contains(control.id)) state = ControlVisualState::CONFLICT;
        if (control.id == m_capturingId) state = ControlVisualState::CAPTURING;
        result.insert(control.id, state);
    }
    return result;
}
QHash<QString, QString> ControllerMapperWidget::labels() const
{
    QHash<QString, QString> result;
    const auto layout = ControllerLayout::forSystem(m_system);
    if (!layout) return result;
    for (const auto& control : layout->controls()) if (const auto binding = m_mapping->binding(m_system, control.id)) result.insert(control.id, binding->label());
    return result;
}

void ControllerMapperWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing); const QRectF target = targetRect();
    if (m_artwork.isValid()) m_artwork.render(painter, target);
    else { painter.fillRect(canvasRect(), QColor(55, 55, 55)); painter.setPen(Qt::white); painter.drawText(canvasRect(), Qt::AlignCenter, m_system); }
    m_interactive.paint(painter, target, rect(), states(), labels());
    if (!m_capturingId.isEmpty()) { painter.setPen(Qt::white); painter.drawText(QRect(8, canvasRect().top() + 6, width() - 16, 24), Qt::AlignHCenter, "Press a keyboard key or controller button for " + m_capturingId); }
}
void ControllerMapperWidget::resizeEvent(QResizeEvent* event) { QWidget::resizeEvent(event); update(); }
void ControllerMapperWidget::enterEvent(QEnterEvent* event) { QWidget::enterEvent(event); }
void ControllerMapperWidget::leaveEvent(QEvent* event) { if (!m_hoverId.isEmpty()) { m_hoverId.clear(); update(); } QWidget::leaveEvent(event); }
void ControllerMapperWidget::updateHover(const QPointF& point) { const auto* control = m_interactive.hitTest(point, targetRect()); const QString id = control ? control->id : QString(); if (id != m_hoverId) { m_hoverId = id; update(); } }
void ControllerMapperWidget::mouseMoveEvent(QMouseEvent* event) { updateHover(event->position()); QWidget::mouseMoveEvent(event); }
void ControllerMapperWidget::mousePressEvent(QMouseEvent* event)
{
    const auto* control = m_interactive.hitTest(event->position(), targetRect());
    if (event->button() == Qt::RightButton && control && m_mapping->binding(m_system, control->id)) { clearBinding(control->id); return; }
    if (event->button() == Qt::LeftButton) { m_selectedId = control ? control->id : QString(); m_conflictIds.clear(); if (control && control->isBindable()) startCapture(control->id); else update(); }
}
void ControllerMapperWidget::startCapture(const QString& id) { m_capturingId = id; m_selectedId = id; m_conflictIds.clear(); setFocus(); update(); }
void ControllerMapperWidget::clearBinding(const QString& id) { m_mapping->clear(m_system, id); m_conflictIds.clear(); emit mappingChanged(); update(); }
void ControllerMapperWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_capturingId.isEmpty()) { QWidget::keyPressEvent(event); return; }
    if (event->key() == Qt::Key_Escape) { m_capturingId.clear(); m_conflictIds.clear(); update(); return; }
    if (event->isAutoRepeat()) return;
    applyBinding({InputDevice::Keyboard, event->key()});
}
void ControllerMapperWidget::applyBinding(const InputBinding& binding)
{
    const QString id = m_capturingId; const QStringList conflicts = m_mapping->conflicts(m_system, id, binding);
    if (!conflicts.isEmpty()) {
        m_conflictIds = QSet<QString>(conflicts.cbegin(), conflicts.cend()); m_conflictIds.insert(id); update();
        const auto answer = QMessageBox::question(this, "Replace binding", binding.label() + " is already mapped to " + conflicts.join(", ") + ". Replace it?");
        if (answer != QMessageBox::Yes) { m_capturingId.clear(); update(); return; }
        for (const QString& conflict : conflicts) m_mapping->clear(m_system, conflict);
    }
    m_mapping->bind(m_system, id, binding); m_capturingId.clear(); m_conflictIds.clear(); emit mappingChanged(); update();
}
void ControllerMapperWidget::loadPreset(bool gamepad)
{
    const ControllerMapping preset = gamepad ? ControllerMapping::genericGamepadPreset() : ControllerMapping::keyboardPreset();
    const auto layout = ControllerLayout::forSystem(m_system); if (!layout) return;
    for (const auto& control : layout->controls()) { if (const auto binding = preset.binding(m_system, control.id)) m_mapping->bind(m_system, control.id, *binding); else m_mapping->clear(m_system, control.id); }
    emit mappingChanged(); update();
}

} // namespace Pocket::App
