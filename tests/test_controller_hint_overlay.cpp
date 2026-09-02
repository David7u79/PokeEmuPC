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
