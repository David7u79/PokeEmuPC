#include <QtTest>
#include <QFileInfo>
#include "pocket/input/ControllerLayout.hpp"

using namespace Pocket::Input;

class ControllerLayoutTest : public QObject {
    Q_OBJECT
private slots:
    void parsesAndHits();
    void rejectsInvalid();
    void normalizedResize();
    void loadsAssets();
};

void ControllerLayoutTest::parsesAndHits()
{
    const QByteArray json = R"({"system":"TEST","artwork":"a.svg","controls":[{"id":"BOTTOM","x":0.1,"y":0.1,"width":0.4,"height":0.4},{"id":"TOP","kind":"DPad","x":0.2,"y":0.2,"width":0.4,"height":0.4}]})";
    QString error;
    const auto layout = ControllerLayout::fromJson(json, &error);
    QVERIFY(layout.has_value()); QCOMPARE(layout->system(), QStringLiteral("TEST")); QCOMPARE(layout->controls().size(), size_t(2));
    QCOMPARE(layout->controlById(QStringLiteral("TOP"))->kind, ControlKind::DPad);
    QCOMPARE(layout->controlAt(.3, .3)->id, QStringLiteral("TOP")); QVERIFY(layout->controlAt(.9, .9) == nullptr);
}

void ControllerLayoutTest::rejectsInvalid()
{
    const QList<QByteArray> invalid{ "not json", "{}", R"({"system":"GB"})", R"({"system":"GB","controls":[{"x":0,"y":0,"width":0,"height":0}]})", R"({"system":"GB","controls":[{"id":"A","x":-0.1,"y":0,"width":.1,"height":.1}]})" };
    for (const QByteArray& json : invalid) { QString error; QVERIFY(!ControllerLayout::fromJson(json, &error)); QVERIFY(!error.isEmpty()); }
}

void ControllerLayoutTest::normalizedResize()
{
    constexpr double x = .25, y = .5, width = .2, height = .1;
    QCOMPARE(x * 800.0, x * 400.0 * 2.0); QCOMPARE(y * 600.0, y * 300.0 * 2.0);
    QCOMPARE(width * 800.0, width * 400.0 * 2.0); QCOMPARE(height * 600.0, height * 300.0 * 2.0);
}

void ControllerLayoutTest::loadsAssets()
{
    const QMap<QString, QStringList> expected{{"GB", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","START","SELECT"}}, {"GBC", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","START","SELECT"}}, {"GBA", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","L","R","START","SELECT"}}, {"NDS", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","X","Y","L","R","START","SELECT","TOUCHSCREEN","MICROPHONE","LID"}}};
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) { QString error; const auto layout = ControllerLayout::forSystem(it.key(), &error); QVERIFY2(layout.has_value(), qPrintable(error)); QCOMPARE(layout->system(), it.key()); QVERIFY(QFileInfo::exists(layout->artworkFile())); for (const QString& id : it.value()) QVERIFY2(layout->controlById(id), qPrintable(id)); }
}

QTEST_APPLESS_MAIN(ControllerLayoutTest)
#include "test_controller_layout.moc"
