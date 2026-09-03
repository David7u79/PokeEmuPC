#pragma once

#include "ControllerArtworkLayer.hpp"
#include "InteractiveControlLayer.hpp"
#include "pocket/input/ControllerMapping.hpp"
#include <QWidget>
#include <QSet>
#include <memory>

class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTableWidget;
class QVBoxLayout;

namespace Pocket::App {

class ControllerMapperWidget : public QWidget {
    Q_OBJECT
public:
    explicit ControllerMapperWidget(std::shared_ptr<Pocket::Input::ControllerMapping> mapping, QWidget* parent = nullptr);
    void setSystem(const QString& system);
    QString system() const { return m_system; }
    QString capturingControlId() const { return m_capturingId; }
    QString selectedControlId() const { return m_selectedId; }
    ControlVisualState visualStateFor(const QString& id) const;
    QRectF controlRect(const QString& id) const;
    QRectF artworkRect() const;

signals:
    void mappingChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QRect canvasRect() const;
    QRectF targetRect() const;
    void rebuildTable();
    void selectControl(const QString& id);
    void updateHover(const QPointF& point);
    void startCapture(const QString& id);
    void clearBinding(const QString& id);
    void applyBinding(const Pocket::Input::InputBinding& binding);
    QHash<QString, ControlVisualState> states() const;
    QHash<QString, QString> labels() const;
    void loadPreset(bool gamepad);

    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
    ControllerArtworkLayer m_artwork;
    InteractiveControlLayer m_interactive;
    QString m_system{"GBA"};
    QString m_hoverId, m_selectedId, m_capturingId;
    QSet<QString> m_conflictIds;
    QVBoxLayout* m_header{};
    QHBoxLayout* m_content{};
    QWidget* m_canvas{};
    QComboBox* m_systemSelector{};
    QComboBox* m_scopeSelector{};
    QPushButton* m_keyboardPreset{};
    QPushButton* m_gamepadPreset{};
    QPushButton* m_reset{};
    QPushButton* m_clearAll{};
    QPushButton* m_save{};
    QTableWidget* m_controlsTable{};
    QLabel* m_captureBanner{};
    QPushButton* m_changeKey{};
    QPushButton* m_remove{};
};

} // namespace Pocket::App
