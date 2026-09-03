#include <QtTest/QtTest>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include "LibraryWidget.hpp"
#include "GameArtworkLoader.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include "pocket/storage/ArtworkCache.hpp"

class LibraryGridTest : public QObject {
    Q_OBJECT
private:
    std::shared_ptr<Pocket::Storage::GameRepository> makeRepository(QTemporaryFile& database, QTemporaryDir& roms) {
        if (!database.open()) return {}; database.close();
        PocketPartner::Storage::DatabaseConfig config; config.dbPath=database.fileName().toStdString();
        auto db=std::make_shared<PocketPartner::Storage::DatabaseManager>(config); if (!db->initialize()) return {};
        QSqlDatabase sqlDatabase=QSqlDatabase::database(); if (!Pocket::Storage::SchemaMigration::runMigrations(sqlDatabase)) return {};
        auto repo=std::make_shared<Pocket::Storage::GameRepository>(db);
        for (const auto& name : {"Zelda.gba", "Alpha.gb", "Mario.nds"}) { QFile file(roms.filePath(name)); if (!file.open(QIODevice::WriteOnly)) return {}; file.write(name); file.close(); if (repo->importGame(file.fileName().toStdString()).status!=Pocket::Storage::ImportResultStatus::Success) return {}; }
        return repo;
    }
private slots:
    void gridFiltersSortsAndSelects() {
        QTemporaryFile database; QTemporaryDir roms; QVERIFY(roms.isValid()); auto repo=makeRepository(database,roms); QVERIFY(repo);
        Pocket::App::LibraryWidget widget(repo); widget.resize(800,600); widget.show(); QTest::qWait(20);
        auto *grid=widget.findChild<QListView*>("gameGrid"); QVERIFY(grid); QCOMPARE(grid->model()->rowCount(),3);
        auto *search=widget.findChild<QLineEdit*>("librarySearch"); search->setText("Alpha"); QTRY_COMPARE(grid->model()->rowCount(),1); search->clear(); QTRY_COMPARE(grid->model()->rowCount(),3);
        auto *systems=widget.findChild<QComboBox*>("systemFilter"); systems->setCurrentText("GBA"); QTRY_COMPARE(grid->model()->rowCount(),1); systems->setCurrentText("Todos"); QTRY_COMPARE(grid->model()->rowCount(),3);
        auto *sort=widget.findChild<QComboBox*>("sortOrder"); sort->setCurrentText("Título (A-Z)"); QCOMPARE(grid->model()->index(0,0).data().toString(),QString("Alpha"));
        QSignalSpy spy(&widget,&Pocket::App::LibraryWidget::gameSelected); const QModelIndex index=grid->model()->index(0,0); QVERIFY(index.isValid()); QVERIFY(QTest::qWaitForWindowExposed(&widget));
        grid->setCurrentIndex(index);
        // Enter and double click land on the same slot; Enter is the one a headless
        // run can drive reliably.
        QTest::keyClick(grid, Qt::Key_Return);
        QTRY_COMPARE(spy.count(),1);
        QCOMPARE(qvariant_cast<Pocket::Core::Game>(spy.at(0).at(0)).title, std::string("Alpha"));
    }
    void cachedArtworkEmitsWithoutNetwork() {
        QTemporaryDir directory; QVERIFY(directory.isValid()); auto cache=std::make_shared<Pocket::Storage::ArtworkCache>(directory.path().toStdString()); QImage image(4,4,QImage::Format_ARGB32); image.fill(Qt::red); QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG"); QVERIFY(cache->saveArtwork("cached",Pocket::Storage::ArtworkType::BoxArt,reinterpret_cast<const uint8_t*>(bytes.constData()),size_t(bytes.size())));
        Pocket::App::GameArtworkLoader loader(cache); QSignalSpy spy(&loader,&Pocket::App::GameArtworkLoader::artworkReady); loader.requestArtwork("cached","Ignored","GBA","C:/roms/Ignored.gba"); QCOMPARE(spy.count(),1); QCOMPARE(spy.at(0).at(0).toString(),QString("cached"));
    }
};
QTEST_MAIN(LibraryGridTest)
#include "test_library_grid.moc"
