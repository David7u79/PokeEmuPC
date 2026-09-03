#include <QtTest/QtTest>

#include "ControllerHintOverlay.hpp"
#include "EmulatorWidget.hpp"
#include "NdsDisplayWidget.hpp"
#include "pocket/input/ControllerMapping.hpp"

#include <QImage>
#include <QFile>
#include <QPainter>
#include <QStandardPaths>

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
    overlay.paintFrame(painter, image.size());
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

    void screenControlsFitArtworkAndScale()
    {
        for (const QString& system : {QStringLiteral("GB"), QStringLiteral("GBC"), QStringLiteral("GBA"),
                                      QStringLiteral("NDS")}) {
            Pocket::App::ControllerHintOverlay overlay;
            overlay.setSystem(system);
            const QString screen = system == QStringLiteral("NDS") ? QStringLiteral("SCREEN_TOP")
                                                                     : QStringLiteral("SCREEN");
            const QRectF artwork = overlay.artworkRect(QSize(800, 600));
            const QRectF screenRect = overlay.controlRect(screen, QSize(800, 600));
            QVERIFY(artwork.contains(screenRect));
            const auto scaledScreenRect = overlay.controlRect(screen, QSize(400, 300));
            QVERIFY(qAbs(screenRect.width() / scaledScreenRect.width() - 2.0) < 0.02);
            QVERIFY(qAbs(screenRect.height() / scaledScreenRect.height() - 2.0) < 0.02);
            const double expectedAspect = system == QStringLiteral("GBA") ? 1.5 : system == QStringLiteral("NDS") ? 1.333 : 1.111;
            QVERIFY(qAbs(screenRect.width() / screenRect.height() - expectedAspect) / expectedAspect < 0.02);
        }
    }

    void frameCachingAndViewModeShortcut()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        QImage image(800, 600, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        for (int i = 0; i < 100; ++i)
            overlay.paintFrame(painter, image.size());
        QCOMPARE(overlay.rasterizationCount(), 1);
        QVERIFY(paintedPixels(image) > 0);

        Pocket::App::EmulatorWidget widget;
        const Pocket::App::EmulatorViewMode initial = widget.viewMode();
        QTest::keyClick(&widget, Qt::Key_F2);
        QCOMPARE(widget.viewMode(), initial == Pocket::App::EmulatorViewMode::ConsoleFrame
                                        ? Pocket::App::EmulatorViewMode::FullScreen
                                        : Pocket::App::EmulatorViewMode::ConsoleFrame);
    }

    void dpadLabelsPreferVerticalSides()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        overlay.setMapping(std::make_shared<Pocket::Input::ControllerMapping>());
        // Roomy: the badge belongs on the button it names.
        const QSize roomy(800, 600);
        QCOMPARE(overlay.labelRectFor(QStringLiteral("DPAD_UP"), roomy),
                 overlay.controlRect(QStringLiteral("DPAD_UP"), roomy));

        // Cramped: it goes above and below, never to one side, which is what used to
        // leave the up label floating next to the pad.
        const QSize cramped(240, 180);
        const QRectF up = overlay.controlRect(QStringLiteral("DPAD_UP"), cramped);
        const QRectF down = overlay.controlRect(QStringLiteral("DPAD_DOWN"), cramped);
        const QRectF upLabel = overlay.labelRectFor(QStringLiteral("DPAD_UP"), cramped);
        const QRectF downLabel = overlay.labelRectFor(QStringLiteral("DPAD_DOWN"), cramped);
        QVERIFY(upLabel.center().y() < up.center().y());
        QVERIFY(downLabel.center().y() > down.center().y());
        QVERIFY(qAbs(upLabel.center().x() - up.center().x()) < 1.0);
        QVERIFY(qAbs(downLabel.center().x() - down.center().x()) < 1.0);
    }

    void labelsNeverCoverTheScreen()
    {
        // The GB's D-pad sits right under the screen: its up label used to land on
        // the running picture.
        for (const QString& system : {QStringLiteral("GB"), QStringLiteral("GBC"), QStringLiteral("GBA")}) {
            Pocket::App::ControllerHintOverlay overlay;
            overlay.setSystem(system);
            overlay.setMapping(std::make_shared<Pocket::Input::ControllerMapping>());
            const QSize size(800, 600);
            const QRectF screen = overlay.controlRect(QStringLiteral("SCREEN"), size);
            QVERIFY(!screen.isEmpty());
            for (const QString& id : {QStringLiteral("DPAD_UP"), QStringLiteral("DPAD_DOWN"),
                                      QStringLiteral("DPAD_LEFT"), QStringLiteral("DPAD_RIGHT"),
                                      QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("START"),
                                      QStringLiteral("SELECT")}) {
                const QRectF label = overlay.labelRectFor(id, size);
                QVERIFY2(!label.intersects(screen), qPrintable(system + " label " + id + " covers the screen"));
            }
        }
    }

    void gbaControlDisplayNames()
    {
        QCOMPARE(Pocket::App::controlDisplayName(QStringLiteral("DPAD_UP")), QStringLiteral("↑"));
        QCOMPARE(Pocket::App::controlDisplayName(QStringLiteral("DPAD_DOWN")), QStringLiteral("↓"));
        QCOMPARE(Pocket::App::controlDisplayName(QStringLiteral("DPAD_LEFT")), QStringLiteral("←"));
        QCOMPARE(Pocket::App::controlDisplayName(QStringLiteral("DPAD_RIGHT")), QStringLiteral("→"));
        for (const QString& id : {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("L"),
                                  QStringLiteral("R"), QStringLiteral("START"), QStringLiteral("SELECT")})
            QCOMPARE(Pocket::App::controlDisplayName(id), id);
    }

    void pressedControlsHighlightOnlyTheirControl()
    {
        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("GBA"));
        const QSize size(800, 600);
        const QRect rect = overlay.controlRect(QStringLiteral("A"), size).toAlignedRect();
        QImage before(size, QImage::Format_ARGB32);
        before.fill(Qt::transparent);
        QImage after = before;
        { QPainter painter(&after); overlay.setPressed(QStringLiteral("A"), true); overlay.paintPressed(painter, size); }
        QVERIFY(overlay.isPressed(QStringLiteral("A")));
        QVERIFY(after.pixelColor(rect.center()).alpha() > before.pixelColor(rect.center()).alpha());
        QVERIFY(after.pixelColor(0, 0) == before.pixelColor(0, 0));
        overlay.clearPressed();
        QVERIFY(!overlay.isPressed(QStringLiteral("A")));
    }

    void ndsWidgetHighlightsAndClicksControls()
    {
        Pocket::App::NdsDisplayWidget widget;
        widget.resize(800, 900);
        widget.setViewMode(Pocket::App::EmulatorViewMode::ConsoleFrame);
        auto mapping = std::make_shared<Pocket::Input::ControllerMapping>();
        mapping->bind(QStringLiteral("NDS"), QStringLiteral("A"),
                      {Pocket::Input::InputDevice::Keyboard, Qt::Key_A});
        widget.setControllerMapping(mapping);
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));

        Pocket::App::ControllerHintOverlay overlay;
        overlay.setSystem(QStringLiteral("NDS"));
        const QRect aRect = overlay.controlRect(QStringLiteral("A"), widget.size()).toAlignedRect();
        const QRect topRect = overlay.controlRect(QStringLiteral("SCREEN_TOP"), widget.size()).toAlignedRect();
        const QImage before = widget.grab().toImage();
        widget.setFocus();
        QTest::keyPress(&widget, Qt::Key_A);
        const QImage pressed = widget.grab().toImage();
        // Counted over the whole button, not sampled at its centre: the centre pixel
        // is the artwork's own black "A" glyph, which darkening leaves black.
        const auto differingPixels = [&before, &pressed](const QRect& area) {
            int count = 0;
            for (int y = area.top(); y <= area.bottom(); ++y)
                for (int x = area.left(); x <= area.right(); ++x)
                    if (before.pixelColor(x, y) != pressed.pixelColor(x, y))
                        ++count;
            return count;
        };
        // grab() renders at the screen's device pixel ratio, so the layout's logical
        // rects have to be scaled before they index the image.
        const qreal scale = before.width() / qreal(widget.width());
        const auto scaled = [scale](const QRect& r) {
            return QRectF(r.x() * scale, r.y() * scale, r.width() * scale, r.height() * scale).toAlignedRect();
        };
        QVERIFY(differingPixels(scaled(aRect)) > 0);
        QCOMPARE(differingPixels(scaled(topRect)), 0);
        QTest::keyRelease(&widget, Qt::Key_A);

        QSignalSpy touchSpy(&widget, &Pocket::App::NdsDisplayWidget::touchInputChanged);
        QSignalSpy buttonSpy(&widget, &Pocket::App::NdsDisplayWidget::buttonInputChanged);
        const QRect touchRect = overlay.controlRect(QStringLiteral("TOUCHSCREEN"), widget.size()).toAlignedRect();
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, touchRect.center());
        QVERIFY(touchSpy.count() > 0);
        QCOMPARE(buttonSpy.count(), 0);

        touchSpy.clear();
        QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, aRect.center());
        QCOMPARE(touchSpy.count(), 0);
        QCOMPARE(buttonSpy.count(), 2);
    }

    void testFramesStayInsideScreenHoles()
    {
        const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        for (const QString& system : {QStringLiteral("GBA"), QStringLiteral("NDS")}) {
            Pocket::App::ControllerHintOverlay overlay;
            overlay.setSystem(system);
            QImage image(800, 600, QImage::Format_ARGB32);
            image.fill(Qt::black);
            QPainter painter(&image);
            overlay.paintFrame(painter, image.size());
            const QString topId = system == QStringLiteral("NDS") ? QStringLiteral("SCREEN_TOP")
                                                                   : QStringLiteral("SCREEN");
            const QRect target = overlay.controlRect(topId, image.size()).toAlignedRect();
            QImage frame(target.size(), QImage::Format_RGB32);
            frame.fill(QColor(48, 176, 255));
            painter.drawImage(target, frame);
            if (system == QStringLiteral("NDS")) {
                const QRect bottom = overlay.controlRect(QStringLiteral("TOUCHSCREEN"), image.size()).toAlignedRect();
                painter.drawImage(bottom, frame);
            }
            const QString path = tempDir + QStringLiteral("/console-frame-") + system + QStringLiteral(".png");
            QVERIFY(image.save(path));
            QVERIFY(QFile::remove(path));
        }
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
