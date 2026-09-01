#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QFile>
#include "pocket/companion/SpriteKey.hpp"
#include "pocket/companion/PokeSpriteProvider.hpp"
#include "pocket/companion/PkhexSpriteProvider.hpp"
#include "pocket/companion/PlaceholderSpriteProvider.hpp"
#include "pocket/companion/CompositeSpriteProvider.hpp"
#include "pocket/companion/SpriteCache.hpp"

class TestSpriteProvider : public QObject {
    Q_OBJECT

private:
    std::string m_testAssetDir;

    void createSamplePng(const QString& filePath, const QColor& color) {
        QFileInfo info(filePath);
        QDir().mkpath(info.absolutePath());

        QImage img(32, 32, QImage::Format_ARGB32);
        img.fill(color);
        img.save(filePath, "PNG");
    }

private slots:
    void initTestCase() {
        m_testAssetDir = (QDir::tempPath() + "/pocket_sprite_test").toStdString();
        QDir().mkpath(QString::fromStdString(m_testAssetDir));

        // Create sample test assets
        createSamplePng(QString::fromStdString(m_testAssetDir) + "/pokesprite/regular/pikachu.png", Qt::yellow);
        createSamplePng(QString::fromStdString(m_testAssetDir) + "/pokesprite/shiny/pikachu.png", Qt::darkYellow);
        createSamplePng(QString::fromStdString(m_testAssetDir) + "/pokesprite/regular/umbreon.png", Qt::black);

        createSamplePng(QString::fromStdString(m_testAssetDir) + "/pkhex/regular/384.png", Qt::green); // Rayquaza
    }

    void cleanupTestCase() {
        QDir(QString::fromStdString(m_testAssetDir)).removeRecursively();
    }

    void testSpeciesLookup() {
        QCOMPARE(QString::fromStdString(Pocket::Companion::PokeSpriteProvider::speciesIdToSlug(1)), QString("bulbasaur"));
        QCOMPARE(QString::fromStdString(Pocket::Companion::PokeSpriteProvider::speciesIdToSlug(25)), QString("pikachu"));
        QCOMPARE(QString::fromStdString(Pocket::Companion::PokeSpriteProvider::speciesIdToSlug(197)), QString("umbreon"));
        QCOMPARE(QString::fromStdString(Pocket::Companion::PokeSpriteProvider::speciesIdToSlug(384)), QString("rayquaza"));
    }

    void testNormalSpriteResolution() {
        Pocket::Companion::PokeSpriteProvider provider(m_testAssetDir + "/pokesprite");
        Pocket::Companion::SpriteKey key{25, false, 0, Pocket::Companion::Gender::Male};

        Pocket::Companion::SpriteResult result = provider.resolve(key);
        QVERIFY(result.success);
        QVERIFY(!result.isShiny);
        QVERIFY(!result.isFallback);
        QVERIFY(QFile::exists(QString::fromStdString(result.imagePath)));
    }

    void testShinySpriteResolutionAndFallback() {
        Pocket::Companion::PokeSpriteProvider provider(m_testAssetDir + "/pokesprite");

        // Pikachu has shiny variant
        Pocket::Companion::SpriteKey shinyPikachu{25, true, 0, Pocket::Companion::Gender::Male};
        Pocket::Companion::SpriteResult res1 = provider.resolve(shinyPikachu);
        QVERIFY(res1.success);
        QVERIFY(res1.isShiny);

        // Umbreon lacks shiny in test dir -> falls back to regular
        Pocket::Companion::SpriteKey shinyUmbreon{197, true, 0, Pocket::Companion::Gender::Male};
        Pocket::Companion::SpriteResult res2 = provider.resolve(shinyUmbreon);
        QVERIFY(res2.success);
        QVERIFY(!res2.isShiny);
        QVERIFY(res2.isFallback);
    }

    void testMissingSpeciesFallback() {
        Pocket::Companion::CompositeSpriteProvider composite;
        auto pokesprite = std::make_shared<Pocket::Companion::PokeSpriteProvider>(m_testAssetDir + "/pokesprite");
        auto pkhex = std::make_shared<Pocket::Companion::PkhexSpriteProvider>(m_testAssetDir + "/pkhex");
        auto placeholder = std::make_shared<Pocket::Companion::PlaceholderSpriteProvider>();

        composite.addProvider(pokesprite);
        composite.addProvider(pkhex);
        composite.addProvider(placeholder);

        // Rayquaza (384) exists in PKHeX fallback directory
        Pocket::Companion::SpriteKey rayquazaKey{384, false, 0, Pocket::Companion::Gender::Genderless};
        Pocket::Companion::SpriteResult rayRes = composite.resolve(rayquazaKey);
        QVERIFY(rayRes.success);
        QCOMPARE(QString::fromStdString(rayRes.providerName), QString("PkhexSpriteProvider"));

        // Unknown species (999) -> falls back to PlaceholderSpriteProvider
        Pocket::Companion::SpriteKey unknownKey{999, false, 0, Pocket::Companion::Gender::Unknown};
        Pocket::Companion::SpriteResult unkRes = composite.resolve(unknownKey);
        QVERIFY(unkRes.success);
        QCOMPARE(QString::fromStdString(unkRes.providerName), QString("PlaceholderSpriteProvider"));
    }

    void testLruSpriteCacheHitsAndEviction() {
        Pocket::Companion::CompositeSpriteProvider composite;
        composite.addProvider(std::make_shared<Pocket::Companion::PlaceholderSpriteProvider>());

        Pocket::Companion::SpriteCache cache(3); // Small capacity 3 for testing eviction

        Pocket::Companion::SpriteKey k1{1, false, 0, Pocket::Companion::Gender::Unknown};
        Pocket::Companion::SpriteKey k2{2, false, 0, Pocket::Companion::Gender::Unknown};
        Pocket::Companion::SpriteKey k3{3, false, 0, Pocket::Companion::Gender::Unknown};
        Pocket::Companion::SpriteKey k4{4, false, 0, Pocket::Companion::Gender::Unknown};

        cache.get(k1, 64, 64, composite);
        cache.get(k2, 64, 64, composite);
        cache.get(k3, 64, 64, composite);

        QCOMPARE(cache.size(), static_cast<size_t>(3));
        QCOMPARE(cache.missCount(), static_cast<size_t>(3));
        QCOMPARE(cache.hitCount(), static_cast<size_t>(0));

        // Fetch k1 again -> cache hit
        cache.get(k1, 64, 64, composite);
        QCOMPARE(cache.hitCount(), static_cast<size_t>(1));

        // Add 4th item -> evicts oldest item
        cache.get(k4, 64, 64, composite);
        QCOMPARE(cache.size(), static_cast<size_t>(3));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestSpriteProvider tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_sprite_provider.moc"
