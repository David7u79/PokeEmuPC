#include <QtTest/QtTest>

#include "ControllerHintOverlay.hpp"
#include "EmulatorWidget.hpp"
#include "NdsDisplayWidget.hpp"
#include "pocket/input/ControllerMapping.hpp"

#include <QImage>
#include <QPainter>

namespace {
int paintedPixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0)
                ++count;
        }
    }
    return count;
}

QImage render(const Pocket::App::ControllerHintOverlay& overlay)
{
    QImage image(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    overlay.paint(painter, image.rect());
    return image;
}

} // namespace

class TestControllerHintOverlay : public QObject {
    Q_OBJECT

private slots:
    void knownSystemsAreValid_data()
    {
        QTest::addColumn<QString>("system");
        for (const QString& system : {"GB", "GBC", "GBA", "NDS"})
            QTest::newRow(qPrintable(system)) << system;
    }

    void knownSystemsAreValid()
    {
        QFETCH(QString, system);
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(system);
        QVERIFY(overlay.isValid());
    }

    void unknownSystemIsSafe()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("NOT_A_SYSTEM"));
        QVERIFY(!overlay.isValid());

        QImage image(800, 600, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        overlay.paint(painter, image.rect());
    }

    void keyboardPresetPaintsArtwork()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        overlay.setMapping(
            std::make_shared<Pocket::Input::ControllerMapping>(Pocket::Input::ControllerMapping::keyboardPreset()));
        QVERIFY(paintedPixels(render(overlay)) > 0);
    }

    void preferredSizePreservesArtworkAspect()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        const QSize available(333, 177);
        const QSize preferred = overlay.preferredSize(available);
        QVERIFY(!preferred.isEmpty());
        QVERIFY(preferred.width() <= available.width());
        QVERIFY(preferred.height() <= available.height());

        const QSize doubled = overlay.preferredSize(available * 2);
        QVERIFY(qAbs(static_cast<double>(preferred.width()) / preferred.height()
                     - static_cast<double>(doubled.width()) / doubled.height())
                 < 0.02);
    }

    void noMappingIsSafe()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        QVERIFY(paintedPixels(render(overlay)) > 0);
    }

    void nonBindableNdsControlsDoNotCreateLabels()
    {
        Pocket::App::ControllerHintOverlay withoutTouchBinding;
        withoutTouchBinding.setSystem(QStringLiteral("NDS"));
        auto withoutMapping = std::make_shared<Pocket::Input::ControllerMapping>();
        withoutTouchBinding.setMapping(withoutMapping);

        Pocket::App::ControllerHintOverlay withTouchBinding;
        withTouchBinding.setSystem(QStringLiteral("NDS"));
        auto withMapping = std::make_shared<Pocket::Input::ControllerMapping>();
        withMapping->bind(QStringLiteral("NDS"), QStringLiteral("TOUCHSCREEN"),
                          {Pocket::Input::InputDevice::Keyboard, Qt::Key_T});
        withTouchBinding.setMapping(withMapping);

        QCOMPARE(render(withTouchBinding), render(withoutTouchBinding));
    }

    void emulatorWidgetTogglesHintsWithF1()
    {
        // Hints stay up until the player turns them off: a legend that vanishes
        // on a timer is gone exactly when someone is still working out the keys.
        Pocket::App::EmulatorWidget widget;
        const bool initial = widget.hintsVisible();

        widget.toggleHints();
        QCOMPARE(widget.hintsVisible(), !initial);
        widget.toggleHints();
        QCOMPARE(widget.hintsVisible(), initial);

        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QTest::keyClick(&widget, Qt::Key_F1);
        QCOMPARE(widget.hintsVisible(), !initial);
        QTest::keyClick(&widget, Qt::Key_F1);
        QCOMPARE(widget.hintsVisible(), initial);

        // Nothing hides them on its own.
        QTest::qWait(250);
        QCOMPARE(widget.hintsVisible(), initial);
    }
};

QTEST_MAIN(TestControllerHintOverlay)
#include "test_controller_hint_overlay.moc"
