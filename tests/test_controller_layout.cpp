#include <QtTest>
#include <QFileInfo>
#include <QSvgRenderer>
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
    // Screens are where the picture goes; offering them as bindable buttons in the
    // mapper would be nonsense, and the aspect must match the hardware because the
    // emulated frame is scaled into the rect.
    struct ScreenExpectation { const char* system; const char* id; double aspect; };
    for (const auto& expectation : {ScreenExpectation{"GB", "SCREEN", 160.0 / 144.0},
                                    ScreenExpectation{"GBC", "SCREEN", 160.0 / 144.0},
                                    ScreenExpectation{"GBA", "SCREEN", 240.0 / 160.0},
                                    ScreenExpectation{"NDS", "SCREEN_TOP", 256.0 / 192.0},
                                    ScreenExpectation{"NDS", "TOUCHSCREEN", 256.0 / 192.0}}) {
        const auto layout = Pocket::Input::ControllerLayout::forSystem(expectation.system);
        QVERIFY2(layout.has_value(), expectation.system);
        const auto* screen = layout->controlById(expectation.id);
        QVERIFY2(screen != nullptr, expectation.id);
        QVERIFY2(!screen->isBindable(), "a screen must never be offered as a button");

        QSvgRenderer renderer(layout->artworkFile());
        QVERIFY(renderer.isValid());
        const QSizeF viewBox = renderer.viewBoxF().size();
        const double aspect = (screen->width * viewBox.width()) / (screen->height * viewBox.height());
        QVERIFY2(qAbs(aspect - expectation.aspect) / expectation.aspect < 0.02,
                 qPrintable(QString("%1 %2 aspect %3, expected %4")
                                .arg(expectation.system, expectation.id)
                                .arg(aspect)
                                .arg(expectation.aspect)));
    }

    const QMap<QString, QStringList> expected{{"GB", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","START","SELECT","SCREEN"}}, {"GBC", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","START","SELECT","SCREEN"}}, {"GBA", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","L","R","START","SELECT","SCREEN"}}, {"NDS", {"DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT","A","B","X","Y","L","R","START","SELECT","SCREEN_TOP","TOUCHSCREEN","MICROPHONE","LID"}}};
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) { QString error; const auto layout = ControllerLayout::forSystem(it.key(), &error); QVERIFY2(layout.has_value(), qPrintable(error)); QCOMPARE(layout->system(), it.key()); QVERIFY(QFileInfo::exists(layout->artworkFile())); for (const QString& id : it.value()) QVERIFY2(layout->controlById(id), qPrintable(id)); }
}

QTEST_APPLESS_MAIN(ControllerLayoutTest)
#include "test_controller_layout.moc"
