#include <QtTest/QtTest>
#include <QApplication>
#include <vector>
#include <fstream>
#include <QDir>
#include <QImage>
#include "pocket/core/RomFingerprint.hpp"
#include "pocket/storage/GameMetadataResolver.hpp"
#include "pocket/storage/ArtworkCache.hpp"
#include "pocket/storage/LibretroArtworkProvider.hpp"

class TestGameMetadataArtwork : public QObject {
    Q_OBJECT

private slots:
    void testRomFingerprint() {
        std::vector<uint8_t> dummyData = {'P', 'O', 'K', 'E', 'M', 'O', 'N'};
        Pocket::Core::RomFingerprint fp = Pocket::Core::RomFingerprint::calculateFromBuffer(dummyData);

        QVERIFY(fp.isValid());
        QCOMPARE(fp.fileSize, static_cast<uint64_t>(7));
        QVERIFY(!fp.crc32.empty());
        QVERIFY(!fp.sha256.empty());
        QVERIFY(!fp.md5.empty());
    }

    void testMetadataResolutionHashPriority() {
        Pocket::Storage::GameMetadataResolver resolver;

        // Custom database entry with CRC32 = 12345678
        Pocket::Storage::GameMetadata customMeta;
        customMeta.canonicalTitle = "Pokemon Emerald";
        customMeta.platform = "Nintendo - Game Boy Advance";
        customMeta.matchedBy = "ExactHash";

        resolver.addDatabaseEntry("12345678", customMeta);

        Pocket::Core::RomFingerprint fp;
        fp.crc32 = "12345678";
        fp.sha256 = "abc";

        // Even with wrong filename, hash match MUST take priority!
        Pocket::Storage::GameMetadata resolved = resolver.resolve("C:/Games/WrongFilename.gba", fp);

        QCOMPARE(QString::fromStdString(resolved.canonicalTitle), QString("Pokemon Emerald"));
        QCOMPARE(QString::fromStdString(resolved.matchedBy), QString("ExactHash"));
    }

    void testMetadataResolutionFilenameFallback() {
        std::string raw = "Pokemon - FireRed Version (USA, Europe) [!].gba";
        std::string cleaned = Pocket::Storage::GameMetadataResolver::normalizeFilename(raw);

        QCOMPARE(QString::fromStdString(cleaned), QString("Pokemon - FireRed Version"));
    }

    void testMetadataResolutionHeaderFallback() {
        std::vector<uint8_t> gbaHeader(512, 0x00);
        // GBA Title at 0xA0
        std::string title = "POKEMON RUBY";
        std::memcpy(gbaHeader.data() + 0xA0, title.c_str(), title.size());

        Pocket::Storage::GameMetadataResolver resolver;
        Pocket::Storage::GameMetadata meta = resolver.resolveFromHeader(gbaHeader, ".gba");

        QCOMPARE(QString::fromStdString(meta.canonicalTitle), QString("POKEMON RUBY"));
        QCOMPARE(QString::fromStdString(meta.platform), QString("Nintendo - Game Boy Advance"));
        QCOMPARE(QString::fromStdString(meta.matchedBy), QString("HeaderMetadata"));
    }

    void testArtworkCacheSaveAndRetrieve() {
        QString tempDir = QDir::tempPath() + "/pocket_art_test";
        Pocket::Storage::ArtworkCache cache(tempDir.toStdString());

        // Create a 64x64 test QImage
        QImage testImg(64, 64, QImage::Format_RGB32);
        testImg.fill(Qt::blue);
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        testImg.save(&buf, "PNG");

        bool saved = cache.saveArtwork("game123", Pocket::Storage::ArtworkType::BoxArt, reinterpret_cast<const uint8_t*>(ba.constData()), ba.size());
        QVERIFY(saved);

        std::string cachedPath = cache.getCachedPath("game123", Pocket::Storage::ArtworkType::BoxArt);
        QVERIFY(!cachedPath.empty());
        QVERIFY(QFile::exists(QString::fromStdString(cachedPath)));

        QDir(tempDir).removeRecursively();
    }

    void testArtworkNegativeCache() {
        QString tempDir = QDir::tempPath() + "/pocket_neg_test";
        Pocket::Storage::ArtworkCache cache(tempDir.toStdString());

        QVERIFY(!cache.isNegativeCached("game456", Pocket::Storage::ArtworkType::BoxArt));
        cache.addNegativeCache("game456", Pocket::Storage::ArtworkType::BoxArt);
        QVERIFY(cache.isNegativeCached("game456", Pocket::Storage::ArtworkType::BoxArt));

        QDir(tempDir).removeRecursively();
    }

    void testLibretroArtworkUrlBuilder() {
        std::string url = Pocket::Storage::LibretroArtworkProvider::buildUrl(
            "Nintendo - Game Boy Advance",
            "Pokemon - Emerald Version",
            Pocket::Storage::ArtworkType::BoxArt
        );

        std::string expected = "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/Pokemon - Emerald Version.png";
        QCOMPARE(QString::fromStdString(url), QString::fromStdString(expected));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestGameMetadataArtwork tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_game_metadata_artwork.moc"
