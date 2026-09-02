#include <QtTest/QtTest>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include "pocket/input/ControllerLayout.hpp"

// Qt's SVG renderer only understands SVG 1.2 Tiny, so artwork that a browser
// draws happily can still come out empty in the app. Render it for real.
class TestControllerArtwork : public QObject {
    Q_OBJECT
private slots:
    void eachSystemRendersSomething_data() {
        QTest::addColumn<QString>("system");
        for (const QString& s : {"GB", "GBC", "GBA", "NDS"}) QTest::newRow(qPrintable(s)) << s;
    }

    void eachSystemRendersSomething() {
        QFETCH(QString, system);

        QString error;
        const auto layout = Pocket::Input::ControllerLayout::forSystem(system, &error);
        QVERIFY2(layout.has_value(), qPrintable(error));

        QSvgRenderer renderer(layout->artworkFile());
        QVERIFY2(renderer.isValid(), qPrintable("Qt cannot render " + layout->artworkFile()));
        QVERIFY(!renderer.viewBoxF().isEmpty());

        QImage canvas(400, 300, QImage::Format_ARGB32);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        renderer.render(&painter, QRectF(0, 0, 400, 300));
        painter.end();

        // Set POCKET_ARTWORK_DUMP_DIR to eyeball what these assertions cannot judge:
        // whether the console is recognisable and the screen looks the right size.
        const QString dumpDir = qEnvironmentVariable("POCKET_ARTWORK_DUMP_DIR");
        if (!dumpDir.isEmpty()) {
            // The assertions above render into a fixed 400x300, which stretches the
            // drawing. Judging that by eye would judge the distortion, so the dump
            // gets its own canvas at the artwork's real aspect.
            const QSizeF viewBox = renderer.viewBoxF().size();
            const int dumpHeight = 900;
            const int dumpWidth = qRound(dumpHeight * viewBox.width() / viewBox.height());
            QImage annotated(dumpWidth, dumpHeight, QImage::Format_ARGB32);
            annotated.fill(Qt::transparent);
            QPainter shot(&annotated);
            renderer.render(&shot, QRectF(0, 0, dumpWidth, dumpHeight));
            shot.end();
            QDir().mkpath(dumpDir);
            annotated.save(dumpDir + "/" + system + ".png");

            QPainter marker(&annotated);
            marker.setPen(QPen(QColor(255, 0, 0, 220), 2));
            marker.setBrush(QColor(255, 0, 0, 60));
            for (const auto& control : layout->controls()) {
                marker.drawRect(QRectF(control.x * annotated.width(), control.y * annotated.height(),
                                       control.width * annotated.width(), control.height * annotated.height()));
            }
            marker.end();
            annotated.save(dumpDir + "/" + system + "-rects.png");
        }

        // A blank canvas means the file parsed but drew nothing.
        int painted = 0;
        for (int y = 0; y < canvas.height(); ++y) {
            for (int x = 0; x < canvas.width(); ++x) {
                if (qAlpha(canvas.pixel(x, y)) > 0) ++painted;
            }
        }
        QVERIFY2(painted > canvas.width() * canvas.height() / 10,
                 qPrintable(QString("%1 only painted %2 pixels").arg(system).arg(painted)));
    }

    void everyControlSitsOverPaintedArtwork_data() { eachSystemRendersSomething_data(); }

    void everyControlSitsOverPaintedArtwork() {
        QFETCH(QString, system);

        const auto layout = Pocket::Input::ControllerLayout::forSystem(system);
        QVERIFY(layout.has_value());
        QSvgRenderer renderer(layout->artworkFile());
        QVERIFY(renderer.isValid());

        const QSize size(800, 600);
        QImage canvas(size, QImage::Format_ARGB32);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        renderer.render(&painter, QRectF(QPointF(), QSizeF(size)));
        painter.end();

        // Each declared control must land on drawn chassis, not on empty space:
        // that is what keeps layout.json and the artwork from drifting apart.
        for (const auto& control : layout->controls()) {
            const int cx = static_cast<int>((control.x + control.width / 2.0) * size.width());
            const int cy = static_cast<int>((control.y + control.height / 2.0) * size.height());
            QVERIFY2(canvas.rect().contains(cx, cy),
                     qPrintable(system + " control " + control.id + " is outside the canvas"));
            QVERIFY2(qAlpha(canvas.pixel(cx, cy)) > 0,
                     qPrintable(system + " control " + control.id + " sits over blank artwork"));
        }
    }
};

QTEST_MAIN(TestControllerArtwork)
#include "test_controller_artwork.moc"
