#include <QtTest>
#include <QMessageBox>
#include <QTimer>
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
};

void ControllerMapperWidgetTest::resizeAndHitTesting()
{
    auto mapping = std::make_shared<ControllerMapping>();
    ControllerMapperWidget widget(mapping); widget.resize(400, 300); widget.show(); QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const QRectF first = widget.controlRect("A"); const QRectF artFirst = widget.artworkRect();
    widget.resize(800, 600); QCoreApplication::processEvents(); const QRectF second = widget.controlRect("A"); const QRectF artSecond = widget.artworkRect();
    QCOMPARE(second.width() / first.width(), 2.0); QCOMPARE(second.height() / first.height(), 2.0);
    QCOMPARE(artSecond.center(), QPointF(400, (600 + artSecond.top()) / 2.0));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, second.center().toPoint()); QCOMPARE(widget.selectedControlId(), QString("A"));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(1, 599)); QVERIFY(widget.selectedControlId().isEmpty());
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
