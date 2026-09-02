#include "NdsDisplayTransform.hpp"

#include <QtTest/QtTest>

using namespace Pocket::App;

class NdsDisplayTransformTest : public QObject {
    Q_OBJECT

private slots:
    void layouts();
    void letterboxAndTouch();
    void dpiAndOutside();
};

void NdsDisplayTransformTest::layouts() {
    NdsDisplayTransform transform;
    transform.setViewport(QSize(512, 768));
    transform.setLayout(NdsScreenLayout::Vertical);
    QCOMPARE(transform.topRect(), QRect(0, 0, 512, 384));
    QCOMPARE(transform.bottomRect(), QRect(0, 384, 512, 384));

    transform.setViewport(QSize(1024, 768));
    QCOMPARE(transform.topRect(), QRect(256, 0, 512, 384));
    QCOMPARE(transform.bottomRect(), QRect(256, 384, 512, 384));

    transform.setLayout(NdsScreenLayout::Horizontal);
    QCOMPARE(transform.topRect(), QRect(0, 192, 512, 384));
    QCOMPARE(transform.bottomRect(), QRect(512, 192, 512, 384));

    transform.setLayout(NdsScreenLayout::FocusedTop);
    QVERIFY(!transform.topRect().isEmpty());
    QVERIFY(!transform.bottomRect().isEmpty());
    QVERIFY(transform.topRect().height() > transform.bottomRect().height());
    transform.setLayout(NdsScreenLayout::FocusedBottom);
    QVERIFY(transform.bottomRect().height() > transform.topRect().height());
}

void NdsDisplayTransformTest::letterboxAndTouch() {
    NdsDisplayTransform transform;
    transform.setLayout(NdsScreenLayout::Vertical);
    transform.setViewport(QSize(1000, 768));
    QCOMPARE(transform.screenAt(QPoint(0, 0)), NdsScreen::None);
    QVERIFY(!transform.touchAt(QPoint(0, 0)));
    QCOMPARE(transform.screenAt(transform.topRect().center()), NdsScreen::Top);
    QVERIFY(!transform.touchAt(transform.topRect().center()));

    const QRect bottom = transform.bottomRect();
    QCOMPARE(transform.touchAt(QPoint(bottom.left() + bottom.width() / 2, bottom.top() + bottom.height() / 2)).value(),
             QPoint(128, 96));
    QCOMPARE(transform.touchAt(bottom.topLeft()).value(), QPoint(0, 0));
    QCOMPARE(transform.touchAt(bottom.bottomRight()).value(), QPoint(255, 191));
    QVERIFY(transform.touchAt(QPoint(-1, 10)) == std::nullopt);

    const auto first = transform.touchAt(QPoint(bottom.left() + 10, bottom.top() + 10)).value();
    const auto last = transform.touchAt(QPoint(bottom.right() - 10, bottom.bottom() - 10)).value();
    QVERIFY(first.x() < last.x());
    QVERIFY(first.y() < last.y());
}

void NdsDisplayTransformTest::dpiAndOutside() {
    NdsDisplayTransform transform;
    transform.setLayout(NdsScreenLayout::Vertical);
    transform.setViewport(QSize(512, 768), 1.0);
    const QPoint point = transform.bottomRect().center();
    const auto expected = transform.touchAt(point);
    transform.setViewport(QSize(512, 768), 1.25);
    QCOMPARE(transform.touchAt(point), expected);
    transform.setViewport(QSize(512, 768), 1.5);
    QCOMPARE(transform.touchAt(point), expected);
    QVERIFY(!transform.touchAt(QPoint(512, 767)));
}

QTEST_APPLESS_MAIN(NdsDisplayTransformTest)

#include "test_nds_display_transform.moc"
