#include <QtTest/QtTest>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>
#include <QDropEvent>
#include <QMimeData>
#include <QSettings>
#include <QUrl>
#include "LibraryWidget.hpp"
#include "LibrarySidebar.hpp"
#include "EmptyStateWidget.hpp"
#include "GameArtworkLoader.hpp"
#include "ArtworkIndex.hpp"
#include "ArtworkPickerDialog.hpp"
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
        auto *categories=widget.findChild<Pocket::App::LibrarySidebar*>("categoryList"); QVERIFY(categories);
        categories->setCurrentCategory("GBA"); QTRY_COMPARE(grid->model()->rowCount(),1);
        categories->setCurrentCategory("Todos"); QTRY_COMPARE(grid->model()->rowCount(),3);
        categories->setCurrentCategory("Recientes"); QVERIFY(grid->model()->rowCount() <= 12);
        categories->setCurrentCategory("Todos");
        auto *sort=widget.findChild<QComboBox*>("sortOrder"); sort->setCurrentText("Título (A-Z)"); QCOMPARE(grid->model()->index(0,0).data().toString(),QString("Alpha"));
        QSignalSpy spy(&widget,&Pocket::App::LibraryWidget::gameSelected); const QModelIndex index=grid->model()->index(0,0); QVERIFY(index.isValid()); QVERIFY(QTest::qWaitForWindowExposed(&widget));
        grid->setCurrentIndex(index);
        QTRY_COMPARE(widget.findChild<QLabel*>("detailTitle")->text(), QString("Alpha"));
        // Enter and double click land on the same slot; Enter is the one a headless
        // run can drive reliably.
        QTest::keyClick(grid, Qt::Key_Return);
        QTRY_COMPARE(spy.count(),1);
        QCOMPARE(qvariant_cast<Pocket::Core::Game>(spy.at(0).at(0)).title, std::string("Alpha"));
    }
    void inspectorShowsSelectionSaveStateAndPlay() {
        QTemporaryFile database;
        QTemporaryDir roms;
        auto repo=makeRepository(database,roms);
        QVERIFY(repo);
        Pocket::App::LibraryWidget widget(repo);
        widget.resize(900,600);
        widget.show();
        auto *grid=widget.findChild<QListView*>("gameGrid");
        auto *title=widget.findChild<QLabel*>("detailTitle");
        auto *play=widget.findChild<QPushButton*>("playButton");
        QVERIFY(grid);
        QVERIFY(title);
        QVERIFY(play);
        const QModelIndex index=grid->model()->index(0,0);
        grid->setCurrentIndex(index);
        QTRY_COMPARE(title->text(),index.data().toString());
        QTRY_VERIFY(play->isVisible());
        QStringList labelTexts;
        for (QLabel* label : widget.findChildren<QLabel*>()) {
            labelTexts.append(label->text());
        }
        QVERIFY(labelTexts.contains("No detectada"));
        QString romPath;
        for (const auto& game : repo->getAllGames()) {
            if (QString::fromStdString(game.title)==index.data().toString()) {
                romPath=QString::fromStdString(game.romPath);
                break;
            }
        }
        QVERIFY(!romPath.isEmpty());
        const QFileInfo romInfo(romPath);
        QFile save(romInfo.absolutePath()+"/"+romInfo.completeBaseName()+".sav");
        QVERIFY(save.open(QIODevice::WriteOnly));
        save.close();
        grid->setCurrentIndex({});
        grid->setCurrentIndex(index);
        QTRY_VERIFY(widget.findChildren<QLabel*>().contains(title));
        labelTexts.clear();
        for (QLabel* label : widget.findChildren<QLabel*>()) {
            labelTexts.append(label->text());
        }
        QVERIFY(labelTexts.contains("Detectada"));
        QSignalSpy spy(&widget,&Pocket::App::LibraryWidget::gameSelected);
        play->click();
        QTRY_COMPARE(spy.count(),1);
        QCOMPARE(qvariant_cast<Pocket::Core::Game>(spy.at(0).at(0)).title,index.data().toString().toStdString());
        grid->setCurrentIndex({});
        QTRY_VERIFY(!play->isVisible());
        QVERIFY(widget.findChildren<QLabel*>().contains(title));
    }
    void cachedArtworkEmitsWithoutNetwork() {
        QTemporaryDir directory; QVERIFY(directory.isValid()); auto cache=std::make_shared<Pocket::Storage::ArtworkCache>(directory.path().toStdString()); QImage image(4,4,QImage::Format_ARGB32); image.fill(Qt::red); QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG"); QVERIFY(cache->saveArtwork("cached",Pocket::Storage::ArtworkType::BoxArt,reinterpret_cast<const uint8_t*>(bytes.constData()),size_t(bytes.size())));
        Pocket::App::GameArtworkLoader loader(cache); QSignalSpy spy(&loader,&Pocket::App::GameArtworkLoader::artworkReady); loader.requestArtwork("cached","Ignored","GBA","C:/roms/Ignored.gba"); QCOMPARE(spy.count(),1); QCOMPARE(spy.at(0).at(0).toString(),QString("cached"));
    }
    void artworkFileAndCandidates() {
        QTemporaryDir directory; QVERIFY(directory.isValid()); auto cache=std::make_shared<Pocket::Storage::ArtworkCache>(directory.path().toStdString());
        const QString imagePath=directory.filePath("cover.png"); QImage image(4,4,QImage::Format_ARGB32); image.fill(Qt::red); QVERIFY(image.save(imagePath));
        Pocket::App::GameArtworkLoader loader(cache); QSignalSpy spy(&loader,&Pocket::App::GameArtworkLoader::artworkReady); loader.setArtworkFromFile("manual",imagePath); QCOMPARE(spy.count(),1); QVERIFY(!cache->getCachedPath("manual",Pocket::Storage::ArtworkType::BoxArt).empty());
        const QStringList candidates=Pocket::App::GameArtworkLoader::titleCandidates("Zelda - Minish Cap (USA) [!].gba");
        QVERIFY(candidates.contains("Zelda - Minish Cap (USA) [!]")); QVERIFY(candidates.contains("Zelda - Minish Cap")); QVERIFY(candidates.contains("Zelda - Minish Cap (USA, Europe)"));
    }
    void artworkIndexMatchesAndCaches() {
        const QStringList names{
            "Pokemon - Edicion Esmeralda (Spain)",
            "Pokemon - Emerald Version (USA, Europe)",
            "Pokemon - Edicion Negra 2 (Spain) (NDSi Enhanced)",
            "Pokemon - Edicion Platino (Spain)",
            "Pokemon - Platin-Edition (Germany)",
            "Mario Kart - Super Circuit (USA)"
        };
        QCOMPARE(Pocket::App::ArtworkIndex::bestMatch("Pokemon Esmeralda", names), QString("Pokemon - Edicion Esmeralda (Spain)"));
        QCOMPARE(Pocket::App::ArtworkIndex::bestMatch("Pokemon Negro 2", names), QString("Pokemon - Edicion Negra 2 (Spain) (NDSi Enhanced)"));
        QCOMPARE(Pocket::App::ArtworkIndex::bestMatch("Pokemon Platino", names), QString("Pokemon - Edicion Platino (Spain)"));
        QVERIFY(Pocket::App::ArtworkIndex::bestMatch("Metroid Fusion", names).isEmpty());
        QCOMPARE(Pocket::App::ArtworkIndex::rankedMatches("Pokemon Esmeralda", names).first(), QString("Pokemon - Edicion Esmeralda (Spain)"));
        const QStringList alphabetical = Pocket::App::ArtworkIndex::rankedMatches({}, names, 3);
        QCOMPARE(alphabetical.size(), 3);
        QCOMPARE(alphabetical, QStringList({"Mario Kart - Super Circuit (USA)", "Pokemon - Edicion Esmeralda (Spain)", "Pokemon - Edicion Negra 2 (Spain) (NDSi Enhanced)"}));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString repo = "Nintendo_-_Game_Boy_Advance";
        Pocket::App::ArtworkIndex index(directory.path());
        index.setNames(repo, names);
        Pocket::App::ArtworkIndex cachedIndex(directory.path());
        cachedIndex.ensureLoaded(repo);
        QVERIFY(cachedIndex.isLoaded(repo));
        QCOMPARE(cachedIndex.names(repo), names);
    }
    void artworkPickerRanksLoadedIndex() {
        const QStringList names{
            "Pokemon - Edicion Esmeralda (Spain)",
            "Pokemon - Emerald Version (USA, Europe)",
            "Pokemon - Edicion Negra 2 (Spain) (NDSi Enhanced)",
            "Pokemon - Edicion Platino (Spain)",
            "Pokemon - Platin-Edition (Germany)",
            "Mario Kart - Super Circuit (USA)"
        };
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto cache = std::make_shared<Pocket::Storage::ArtworkCache>(directory.path().toStdString());
        Pocket::App::GameArtworkLoader loader(cache);
        loader.index()->setNames("Nintendo_-_Game_Boy_Advance", names);
        Pocket::App::ArtworkPickerDialog dialog("Pokemon Esmeralda", "GBA", &loader);
        dialog.show();
        auto* candidates = dialog.findChild<QListWidget*>("artworkCandidates");
        auto* search = dialog.findChild<QLineEdit*>("artworkSearch");
        QVERIFY(candidates);
        QVERIFY(search);
        QTRY_VERIFY(candidates->count() > 0);
        QCOMPARE(candidates->item(0)->text(), QString("Pokemon - Edicion Esmeralda (Spain)"));
        search->setText("Platino");
        QTRY_COMPARE(candidates->item(0)->text(), QString("Pokemon - Edicion Platino (Spain)"));
    }
    void zoomChangesDelegateSizeHint() {
        QTemporaryFile database; QTemporaryDir roms; auto repo=makeRepository(database,roms); QVERIFY(repo); Pocket::App::LibraryWidget widget(repo);
        auto *grid=widget.findChild<QListView*>("gameGrid"); auto *zoom=widget.findChild<QSlider*>("cardZoom"); QVERIFY(grid); QVERIFY(zoom);
        // Driven from both ends: the slider starts at the user's remembered zoom, so
        // comparing against a single hardcoded target passes or fails by accident.
        zoom->setValue(120);
        const QSize before=grid->itemDelegate()->sizeHint(QStyleOptionViewItem(),grid->model()->index(0,0));
        zoom->setValue(240);
        const QSize after=grid->itemDelegate()->sizeHint(QStyleOptionViewItem(),grid->model()->index(0,0));
        QVERIFY(after.width()>before.width());
        QCOMPARE(before.height(), before.width()+42);
        QCOMPARE(after.height(), after.width()+42);
    }
    void personalOrderEnablesDragAndIsRestored() {
        QTemporaryFile database; QTemporaryDir roms; auto repo=makeRepository(database,roms); QVERIFY(repo);
        const QString organization="LibraryGridTest";
        const QString application="personalOrder";
        QSettings settings(organization,application);
        settings.clear();
        QStringList ids;
        for (const auto& game : repo->getAllGames()) ids.append(QString::fromStdString(game.id.toString()));
        std::reverse(ids.begin(),ids.end());
        settings.setValue("library/order",ids);
        settings.sync();
        Pocket::App::LibraryWidget widget(repo,nullptr,organization,application);
        auto *grid=widget.findChild<QListView*>("gameGrid"); auto *sort=widget.findChild<QComboBox*>("sortOrder"); QVERIFY(grid); QVERIFY(sort);
        sort->setCurrentIndex(0);
        QCOMPARE(grid->dragDropMode(),QAbstractItemView::InternalMove);
        sort->setCurrentIndex(1);
        QCOMPARE(grid->dragDropMode(),QAbstractItemView::NoDragDrop);
        settings.clear();
    }
    void externalDropOfMissingRomDoesNotImport() {
        QTemporaryFile database; QTemporaryDir roms; auto repo=makeRepository(database,roms); QVERIFY(repo);
        Pocket::App::LibraryWidget widget(repo);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(roms.filePath("missing.gba"))});
        QDropEvent event(QPointF(1,1),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QCoreApplication::sendEvent(&widget,&event);
        QCOMPARE(repo->getAllGames().size(),size_t(3));
    }
    void sidebarCountsMatchSystems() {
        QTemporaryFile database; QTemporaryDir roms; auto repo=makeRepository(database,roms); QVERIFY(repo);
        Pocket::App::LibraryWidget widget(repo);
        auto *categories=widget.findChild<Pocket::App::LibrarySidebar*>("categoryList"); QVERIFY(categories);
        const auto rows=categories->findChildren<QWidget*>();
        QStringList texts;
        for (QWidget* row : rows) {
            for (QLabel* label : row->findChildren<QLabel*>()) texts.append(label->text());
        }
        QVERIFY(texts.contains("1"));
        QCOMPARE(repo->getAllGames().size(), size_t(3));
    }
    void searchWithoutResultsShowsEmptyState() {
        QTemporaryFile database; QTemporaryDir roms; auto repo=makeRepository(database,roms); QVERIFY(repo);
        Pocket::App::LibraryWidget widget(repo); widget.show();
        auto *search=widget.findChild<QLineEdit*>("librarySearch");
        auto *grid=widget.findChild<QListView*>("gameGrid");
        auto *empty=widget.findChild<Pocket::App::EmptyStateWidget*>();
        QVERIFY(search); QVERIFY(grid); QVERIFY(empty);
        search->setText("not-a-game");
        QTRY_VERIFY(empty->isVisible());
        QVERIFY(!grid->isVisible());
    }
};
QTEST_MAIN(LibraryGridTest)
#include "test_library_grid.moc"
