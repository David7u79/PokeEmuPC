#include <QtTest>
#include <QMessageBox>
#include <QTimer>
#include <QTableWidget>
#include <QPushButton>
#include "ControllerMapperWidget.hpp"

using namespace Pocket::App;
using namespace Pocket::Input;

class ControllerMapperWidgetTest : public QObject {
    Q_OBJECT
private slots:
    void resizeAndHitTesting();
    void capturesAndCancels();
    void duplicateAndNonBindable();
    void allSystemsLoad();
    void tableSelectionAndActions();
};

void ControllerMapperWidgetTest::resizeAndHitTesting()
{
    auto mapping = std::make_shared<ControllerMapping>();
    ControllerMapperWidget widget(mapping); widget.resize(400, 300); widget.show(); QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const QRectF first = widget.controlRect("A"); const QRectF artFirst = widget.artworkRect();
    widget.resize(800, 600); QCoreApplication::processEvents(); const QRectF second = widget.controlRect("A"); const QRectF artSecond = widget.artworkRect();
    // The artwork is fit to the canvas preserving aspect, so doubling the widget
    // does not necessarily double the drawing: a control must scale with the
    // artwork rect, whichever axis ends up constraining it.
    const qreal artScale = artSecond.width() / artFirst.width();
    QVERIFY(artScale > 1.0);
    QCOMPARE(second.width() / first.width(), artScale);
    QCOMPARE(second.height() / first.height(), artSecond.height() / artFirst.height());
    // Centred horizontally in the left canvas, and clear of the toolbar row above.
    QCOMPARE(artSecond.center().x(), widget.artworkRect().center().x());
    QVERIFY(artSecond.top() > 0.0);
    QVERIFY(artSecond.bottom() <= 600.0);
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, second.center().toPoint()); QCOMPARE(widget.selectedControlId(), QString("A"));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(1, 599)); QVERIFY(widget.selectedControlId().isEmpty());
}

void ControllerMapperWidgetTest::tableSelectionAndActions()
{
    auto mapping = std::make_shared<ControllerMapping>(); mapping->bind("GBA", "B", {InputDevice::Keyboard, Qt::Key_N});
    ControllerMapperWidget widget(mapping); widget.resize(800, 600); widget.show(); QVERIFY(QTest::qWaitForWindowExposed(&widget));
    auto* table = widget.findChild<QTableWidget*>("controlsTable"); QVERIFY(table);
    int row = -1; for (int i = 0; i < table->rowCount(); ++i) if (table->item(i, 0)->text() == "B") { row = i; break; } QVERIFY(row >= 0);
    table->selectRow(row); QCOMPARE(widget.selectedControlId(), QString("B"));
    QTest::mouseClick(widget.findChild<QPushButton*>("changeKeyButton"), Qt::LeftButton); QCOMPARE(widget.capturingControlId(), QString("B"));
    QTest::keyClick(&widget, Qt::Key_Escape); QTest::mouseClick(widget.findChild<QPushButton*>("removeBindingButton"), Qt::LeftButton); QVERIFY(!mapping->binding("GBA", "B"));
}

void ControllerMapperWidgetTest::capturesAndCancels()
{
    auto mapping = std::make_shared<ControllerMapping>(); ControllerMapperWidget widget(mapping); widget.resize(400, 300); widget.show(); QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, widget.controlRect("A").center().toPoint()); QCOMPARE(widget.capturingControlId(), QString("A"));
    QTest::keyClick(&widget, Qt::Key_M); QCOMPARE(mapping->binding("GBA", "A")->code, int(Qt::Key_M));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, widget.controlRect("B").center().toPoint()); QTest::keyClick(&widget, Qt::Key_Escape);
    QVERIFY(!mapping->binding("GBA", "B")); QVERIFY(widget.capturingControlId().isEmpty());
}

void ControllerMapperWidgetTest::duplicateAndNonBindable()
{
    auto mapping = std::make_shared<ControllerMapping>(); ControllerMapperWidget widget(mapping); widget.resize(400, 300); widget.show(); QVERIFY(QTest::qWaitForWindowExposed(&widget));
    mapping->bind("GBA", "A", {InputDevice::Keyboard, Qt::Key_M});
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, widget.controlRect("B").center().toPoint());
    QTimer::singleShot(0, [] { for (QWidget* top : QApplication::topLevelWidgets()) if (auto* box = qobject_cast<QMessageBox*>(top)) box->done(QMessageBox::No); });
    QTest::keyClick(&widget, Qt::Key_M); QVERIFY(!mapping->binding("GBA", "B")); QCOMPARE(widget.visualStateFor("A"), ControlVisualState::CONFLICT);
    widget.setSystem("NDS"); QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, widget.controlRect("TOUCHSCREEN").center().toPoint()); QVERIFY(widget.capturingControlId().isEmpty());
}

void ControllerMapperWidgetTest::allSystemsLoad()
{
    auto mapping = std::make_shared<ControllerMapping>(); ControllerMapperWidget widget(mapping); widget.resize(400, 300); widget.show();
    for (const QString& system : {QString("GB"), QString("GBC"), QString("GBA"), QString("NDS")}) { widget.setSystem(system); QVERIFY(!widget.artworkRect().isEmpty()); }
}

QTEST_MAIN(ControllerMapperWidgetTest)
#include "test_controller_mapper_widget.moc"
