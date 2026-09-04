#include "ControllerMapperWidget.hpp"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>

namespace Pocket::App {
using namespace Pocket::Input;

ControllerMapperWidget::ControllerMapperWidget(std::shared_ptr<ControllerMapping> mapping, QWidget* parent)
    : QWidget(parent), m_mapping(std::move(mapping))
{
    if (!m_mapping) m_mapping = std::make_shared<ControllerMapping>();
    // Idle until a capture starts: no polling while the user just looks at the page.
    m_gamepad = new GamepadReader(this);
    m_gamepad->setActive(false);
    connect(m_gamepad, &GamepadReader::buttonChanged, this, [this](int index, bool pressed) {
        if (pressed && !m_capturingId.isEmpty()) applyBinding({InputDevice::Gamepad, index});
    });
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    auto* header = new QVBoxLayout;
    m_header = header;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(6);
    auto* firstRow = new QHBoxLayout;
    auto* secondRow = new QHBoxLayout;
    m_systemSelector = new QComboBox(this); m_systemSelector->addItems({"GB", "GBC", "GBA", "NDS"}); m_systemSelector->setCurrentText(m_system);
    m_scopeSelector = new QComboBox(this); m_scopeSelector->addItems({"Global", "Per system"});
    m_scopeSelector->setCurrentIndex(m_mapping->scope() == MappingScope::Global ? 0 : 1);
    m_keyboardPreset = new QPushButton("Teclado", this); m_gamepadPreset = new QPushButton("Mando genérico", this);
    m_reset = new QPushButton("Restablecer", this); m_clearAll = new QPushButton("Borrar todo", this); m_save = new QPushButton("Guardar", this);
    m_save->setDefault(true);
    auto addTip = [](QWidget* widget, const QString& tip) { widget->setToolTip(tip); };
    addTip(m_systemSelector, "Elige el sistema"); addTip(m_scopeSelector, "Elige el ámbito de las asignaciones");
    addTip(m_keyboardPreset, "Carga asignaciones de teclado"); addTip(m_gamepadPreset, "Carga asignaciones de mando");
    addTip(m_reset, "Restaura las asignaciones predeterminadas"); addTip(m_clearAll, "Elimina todas las asignaciones"); addTip(m_save, "Guarda las asignaciones");
    firstRow->addWidget(new QLabel("Sistema", this)); firstRow->addWidget(m_systemSelector);
    firstRow->addWidget(new QLabel("Ámbito", this)); firstRow->addWidget(m_scopeSelector); firstRow->addStretch(); firstRow->addWidget(m_save);
    secondRow->addWidget(new QLabel("Presets:", this)); secondRow->addWidget(m_keyboardPreset); secondRow->addWidget(m_gamepadPreset);
    auto* separator = new QFrame(this); separator->setFrameShape(QFrame::VLine); separator->setFrameShadow(QFrame::Sunken); secondRow->addWidget(separator);
    secondRow->addWidget(m_reset); secondRow->addWidget(m_clearAll); secondRow->addStretch();
    header->addLayout(firstRow); header->addLayout(secondRow);
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(8, 8, 8, 8); layout->setSpacing(6); layout->addLayout(header);
    m_content = new QHBoxLayout; m_content->setContentsMargins(0, 0, 0, 0); m_content->setSpacing(6);
    // An empty widget rather than a stretch: the artwork is painted on this widget,
    // and a real child is what gives the layout a geometry to read back.
    m_canvas = new QWidget(this);
    m_canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_canvas->setAttribute(Qt::WA_NoSystemBackground);
    m_content->addWidget(m_canvas, 3);
    auto* panel = new QWidget(this); panel->setFixedWidth(320); auto* panelLayout = new QVBoxLayout(panel); panelLayout->setContentsMargins(0, 0, 0, 0); panelLayout->setSpacing(6);
    auto* title = new QLabel("Controles", panel); QFont titleFont = title->font(); titleFont.setBold(true); title->setFont(titleFont); panelLayout->addWidget(title);
    m_captureBanner = new QLabel(panel); m_captureBanner->setStyleSheet("QLabel { background: #fff3cd; color: #3a2f00; padding: 5px 8px; border-radius: 4px; }"); m_captureBanner->setVisible(false); panelLayout->addWidget(m_captureBanner);
    m_controlsTable = new QTableWidget(panel); m_controlsTable->setObjectName("controlsTable"); m_controlsTable->setColumnCount(2); m_controlsTable->setHorizontalHeaderLabels({"Botón", "Asignación"}); m_controlsTable->verticalHeader()->hide(); m_controlsTable->setShowGrid(false); m_controlsTable->setAlternatingRowColors(true); m_controlsTable->setSelectionBehavior(QAbstractItemView::SelectRows); m_controlsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); m_controlsTable->horizontalHeader()->setStretchLastSection(true); m_controlsTable->installEventFilter(this); panelLayout->addWidget(m_controlsTable);
    auto* actions = new QHBoxLayout; m_changeKey = new QPushButton("Cambiar tecla", panel); m_changeKey->setObjectName("changeKeyButton"); m_remove = new QPushButton("Quitar", panel); m_remove->setObjectName("removeBindingButton"); actions->addWidget(m_changeKey); actions->addWidget(m_remove); panelLayout->addLayout(actions);
    auto* legend = new QLabel("<span style='color:#4caf50'>● asignado</span>  <span style='color:#9e9e9e'>● sin asignar</span>  <span style='color:#dc3545'>● conflicto</span>", panel); panelLayout->addWidget(legend);
    m_content->addWidget(panel); layout->addLayout(m_content, 1);
    connect(m_systemSelector, &QComboBox::currentTextChanged, this, &ControllerMapperWidget::setSystem);
    connect(m_scopeSelector, &QComboBox::currentIndexChanged, this, [this](int index) { m_mapping->setScope(index == 0 ? MappingScope::Global : MappingScope::PerSystem); rebuildTable(); update(); });
    connect(m_keyboardPreset, &QPushButton::clicked, this, [this] { loadPreset(false); });
    connect(m_gamepadPreset, &QPushButton::clicked, this, [this] { loadPreset(true); });
    connect(m_reset, &QPushButton::clicked, this, [this] { m_mapping->resetToDefaults(m_system); emit mappingChanged(); rebuildTable(); update(); });
    connect(m_clearAll, &QPushButton::clicked, this, [this] { m_mapping->clearAll(m_system); emit mappingChanged(); rebuildTable(); update(); });
    connect(m_save, &QPushButton::clicked, this, [this] { QSettings settings("PocketPartnerProject", "PocketPartner"); m_mapping->save(settings); });
    connect(m_controlsTable, &QTableWidget::itemSelectionChanged, this, [this] { const int row = m_controlsTable->currentRow(); selectControl(row >= 0 ? m_controlsTable->item(row, 0)->data(Qt::UserRole).toString() : QString()); m_changeKey->setEnabled(row >= 0); m_remove->setEnabled(row >= 0 && m_mapping->binding(m_system, m_controlsTable->item(row, 0)->data(Qt::UserRole).toString()).has_value()); });
    connect(m_controlsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) { startCapture(m_controlsTable->item(row, 0)->data(Qt::UserRole).toString()); });
    connect(m_changeKey, &QPushButton::clicked, this, [this] { if (m_controlsTable->currentRow() >= 0) startCapture(m_controlsTable->item(m_controlsTable->currentRow(), 0)->data(Qt::UserRole).toString()); });
    connect(m_remove, &QPushButton::clicked, this, [this] { if (m_controlsTable->currentRow() >= 0) clearBinding(m_controlsTable->item(m_controlsTable->currentRow(), 0)->data(Qt::UserRole).toString()); });
    setSystem(m_system);
}

void ControllerMapperWidget::setSystem(const QString& system)
{
    const auto layout = ControllerLayout::forSystem(system);
    m_system = system; m_hoverId.clear(); m_selectedId.clear(); m_capturingId.clear(); m_conflictIds.clear();
    if (layout) m_interactive.setLayout(*layout);
    m_artwork.setSystem(system);
    if (m_systemSelector && m_systemSelector->currentText() != system) m_systemSelector->setCurrentText(system);
    rebuildTable(); update();
}

QRect ControllerMapperWidget::canvasRect() const
{
    // The area left of the bindings panel and below the toolbar rows: using the full
    // widget rect would slide the controller under both of them.
    return m_canvas ? m_canvas->geometry() : QRect();
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
    m_interactive.paint(painter, target, canvasRect(), states(), labels());
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
    if (event->button() == Qt::LeftButton) { selectControl(control ? control->id : QString()); m_conflictIds.clear(); if (control && control->isBindable()) startCapture(control->id); else update(); }
}
bool ControllerMapperWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_controlsTable && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete && m_controlsTable->currentRow() >= 0) {
            clearBinding(m_controlsTable->item(m_controlsTable->currentRow(), 0)->data(Qt::UserRole).toString());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
void ControllerMapperWidget::startCapture(const QString& id) { m_capturingId = id; selectControl(id); m_conflictIds.clear(); m_captureBanner->setText("Pulsa una tecla o un botón del mando para " + id + " · Esc cancela"); m_captureBanner->setVisible(true); m_gamepad->setActive(true); setFocus(); rebuildTable(); update(); }
void ControllerMapperWidget::clearBinding(const QString& id) { m_mapping->clear(m_system, id); m_conflictIds.clear(); emit mappingChanged(); rebuildTable(); update(); }
void ControllerMapperWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_capturingId.isEmpty()) { QWidget::keyPressEvent(event); return; }
    if (event->key() == Qt::Key_Escape) { m_capturingId.clear(); m_conflictIds.clear(); m_captureBanner->setVisible(false); m_gamepad->setActive(false); rebuildTable(); update(); return; }
    if (event->isAutoRepeat()) return;
    applyBinding({InputDevice::Keyboard, event->key()});
}
void ControllerMapperWidget::applyBinding(const InputBinding& binding)
{
    m_gamepad->setActive(false);
    const QString id = m_capturingId; const QStringList conflicts = m_mapping->conflicts(m_system, id, binding);
    if (!conflicts.isEmpty()) {
        m_conflictIds = QSet<QString>(conflicts.cbegin(), conflicts.cend()); m_conflictIds.insert(id); update();
        const auto answer = QMessageBox::question(this, "Replace binding", binding.label() + " is already mapped to " + conflicts.join(", ") + ". Replace it?");
        if (answer != QMessageBox::Yes) { m_capturingId.clear(); m_captureBanner->setVisible(false); rebuildTable(); update(); return; }
        for (const QString& conflict : conflicts) m_mapping->clear(m_system, conflict);
    }
    m_mapping->bind(m_system, id, binding); m_capturingId.clear(); m_conflictIds.clear(); m_captureBanner->setVisible(false); emit mappingChanged(); rebuildTable(); update();
}
void ControllerMapperWidget::loadPreset(bool gamepad)
{
    const ControllerMapping preset = gamepad ? ControllerMapping::genericGamepadPreset() : ControllerMapping::keyboardPreset();
    const auto layout = ControllerLayout::forSystem(m_system); if (!layout) return;
    for (const auto& control : layout->controls()) { if (const auto binding = preset.binding(m_system, control.id)) m_mapping->bind(m_system, control.id, *binding); else m_mapping->clear(m_system, control.id); }
    emit mappingChanged(); rebuildTable(); update();
}

void ControllerMapperWidget::selectControl(const QString& id)
{
    if (id == m_selectedId) return;
    m_selectedId = id; m_conflictIds.clear(); update();
}

void ControllerMapperWidget::rebuildTable()
{
    if (!m_controlsTable) return;
    const QSignalBlocker blocker(m_controlsTable);
    m_controlsTable->setRowCount(0);
    const auto layout = ControllerLayout::forSystem(m_system);
    if (layout) for (const auto& control : layout->controls()) {
        if (!control.isBindable()) continue;
        const int row = m_controlsTable->rowCount(); m_controlsTable->insertRow(row);
        auto* name = new QTableWidgetItem(control.id); name->setData(Qt::UserRole, control.id);
        auto* key = new QTableWidgetItem;
        if (const auto binding = m_mapping->binding(m_system, control.id)) key->setText(binding->label());
        else { key->setText("sin asignar"); QFont font = key->font(); font.setItalic(true); key->setFont(font); key->setForeground(Qt::gray); }
        if (m_conflictIds.contains(control.id)) { name->setForeground(QColor(220, 53, 69)); key->setForeground(QColor(220, 53, 69)); }
        if (control.id == m_capturingId) { name->setBackground(QColor(255, 243, 205)); key->setBackground(QColor(255, 243, 205)); }
        m_controlsTable->setItem(row, 0, name); m_controlsTable->setItem(row, 1, key);
        if (control.id == m_selectedId) m_controlsTable->selectRow(row);
    }
    const int row = m_controlsTable->currentRow();
    m_changeKey->setEnabled(row >= 0); m_remove->setEnabled(row >= 0 && m_mapping->binding(m_system, m_controlsTable->item(row, 0)->data(Qt::UserRole).toString()).has_value());
}

} // namespace Pocket::App
