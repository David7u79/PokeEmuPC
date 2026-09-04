#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QStackedWidget>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <memory>

#include "Theme.hpp"
#include "AppNavigation.hpp"
#include "MainWindow.hpp"
#include "pocketpartner/storage/DatabaseManager.hpp"
#include "pocket/storage/SchemaMigration.hpp"
#include "pocket/storage/GameRepository.hpp"

namespace {

struct TestContext {
    QTemporaryFile database;
    QTemporaryDir roms;
    std::shared_ptr<PocketPartner::Storage::DatabaseManager> dbManager;
    std::shared_ptr<Pocket::Storage::GameRepository> repo;
};

std::unique_ptr<TestContext> createTestContext() {
    auto ctx = std::make_unique<TestContext>();
    if (!ctx->database.open()) {
        return nullptr;
    }
    ctx->database.close();

    PocketPartner::Storage::DatabaseConfig config;
    config.dbPath = ctx->database.fileName().toStdString();
    ctx->dbManager = std::make_shared<PocketPartner::Storage::DatabaseManager>(config);
    if (!ctx->dbManager->initialize()) {
        return nullptr;
    }

    QSqlDatabase sqlDatabase = QSqlDatabase::database();
    if (!Pocket::Storage::SchemaMigration::runMigrations(sqlDatabase)) {
        return nullptr;
    }

    ctx->repo = std::make_shared<Pocket::Storage::GameRepository>(ctx->dbManager);
    for (const auto& name : {"Zelda.gba", "Alpha.gb"}) {
        QFile file(ctx->roms.filePath(name));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(name);
            file.close();
            ctx->repo->importGame(file.fileName().toStdString());
        }
    }
    return ctx;
}

} // namespace

class TestAppNavigation : public QObject
{
    Q_OBJECT

private slots:
    void themeLeavesPaletteDark() {
        auto* app = static_cast<QApplication*>(QCoreApplication::instance());
        QVERIFY(app != nullptr);
        Pocket::App::Theme::applyTheme(*app);

        const QPalette pal = app->palette();
        const QColor windowCol = pal.color(QPalette::Window);
        const QColor windowTextCol = pal.color(QPalette::WindowText);

        // Dark palette requirement: window color is darker than window text
        QVERIFY(windowCol.value() < windowTextCol.value());
        QVERIFY(windowCol.lightness() < windowTextCol.lightness());

        // Tokens are checked by the relationships that must hold, not by exact
        // hexes: pinning the literals made every palette tweak a test edit.
        QCOMPARE(Pocket::App::Theme::accent(), QColor("#4f8cff"));
        QVERIFY(Pocket::App::Theme::surface().lightness() < 40);
        QVERIFY(Pocket::App::Theme::surfaceRaised().lightness() > Pocket::App::Theme::surface().lightness());
        QVERIFY(Pocket::App::Theme::textPrimary().lightness() > 200);
        QVERIFY(Pocket::App::Theme::textSecondary().lightness() < Pocket::App::Theme::textPrimary().lightness());
        QVERIFY(Pocket::App::Theme::textDisabled().lightness() < Pocket::App::Theme::textSecondary().lightness());
        // Translucent layers must actually be translucent, and lighter than the app.
        QVERIFY(Pocket::App::Theme::surfacePanel().alpha() < 255);
        QVERIFY(Pocket::App::Theme::surfaceControl().alpha() < 255);
        QVERIFY(Pocket::App::Theme::borderSubtle().alpha() < Pocket::App::Theme::borderHover().alpha());
        QCOMPARE(Pocket::App::Theme::rgba(QColor(20, 23, 28, 184)), QString("rgba(20, 23, 28, 0.722)"));
    }

    void activeSectionReflectedInBar() {
        Pocket::App::AppNavigation nav;
        nav.show();

        auto* libBtn = nav.libraryButton();
        auto* compBtn = nav.companionButton();
        auto* setBtn = nav.settingsButton();
        QVERIFY(libBtn);
        QVERIFY(compBtn);
        QVERIFY(setBtn);

        // Initial state: Library is active
        QCOMPARE(nav.activeSection(), Pocket::App::AppNavigation::Section::Library);
        QVERIFY(libBtn->isChecked());
        QVERIFY(libBtn->property("active").toBool());
        QVERIFY(!compBtn->isChecked());
        QVERIFY(!compBtn->property("active").toBool());

        // Switch to Companion
        nav.setActiveSection(Pocket::App::AppNavigation::Section::Companion);
        QCOMPARE(nav.activeSection(), Pocket::App::AppNavigation::Section::Companion);
        QVERIFY(!libBtn->isChecked());
        QVERIFY(!libBtn->property("active").toBool());
        QVERIFY(compBtn->isChecked());
        QVERIFY(compBtn->property("active").toBool());
        QCOMPARE(nav.property("activeSection").toInt(), static_cast<int>(Pocket::App::AppNavigation::Section::Companion));

        // Switch to Settings
        nav.setActiveSection(Pocket::App::AppNavigation::Section::Settings);
        QCOMPARE(nav.activeSection(), Pocket::App::AppNavigation::Section::Settings);
        QVERIFY(!compBtn->isChecked());
        QVERIFY(!compBtn->property("active").toBool());
        QVERIFY(setBtn->isChecked());
        QVERIFY(setBtn->property("active").toBool());

        // Switch back to Library
        nav.setActiveSection(Pocket::App::AppNavigation::Section::Library);
        QCOMPARE(nav.activeSection(), Pocket::App::AppNavigation::Section::Library);
        QVERIFY(libBtn->isChecked());
        QVERIFY(libBtn->property("active").toBool());
    }

    void clickCompanionAndLibraryEmitsAndChangesPage() {
        auto ctx = createTestContext();
        QVERIFY(ctx != nullptr);

        Pocket::App::MainWindow window(ctx->dbManager, ctx->repo);
        window.resize(900, 600);
        window.show();
        QTest::qWait(20);

        auto* nav = window.navigation();
        QVERIFY(nav);
        auto* pages = window.pages();
        QVERIFY(pages);
        QCOMPARE(pages->objectName(), QStringLiteral("mainPages"));

        auto* btnLibrary = window.findChild<QPushButton*>("navLibrary");
        auto* btnCompanion = window.findChild<QPushButton*>("navCompanion");
        auto* btnSettings = window.findChild<QPushButton*>("navSettings");
        QVERIFY(btnLibrary);
        QVERIFY(btnCompanion);
        QVERIFY(btnSettings);

        QSignalSpy spy(nav, &Pocket::App::AppNavigation::sectionSelected);

        // Initial page is Library
        QCOMPARE(pages->currentWidget(), window.libraryWidget());
        QCOMPARE(nav->activeSection(), Pocket::App::AppNavigation::Section::Library);

        // Click Companion button
        QTest::mouseClick(btnCompanion, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Pocket::App::AppNavigation::Section>(),
                 Pocket::App::AppNavigation::Section::Companion);
        QCOMPARE(pages->currentWidget(), window.companionWidget());
        QVERIFY(window.companionWidget()->isVisible());
        QCOMPARE(nav->activeSection(), Pocket::App::AppNavigation::Section::Companion);
        QVERIFY(btnCompanion->isChecked());
        QVERIFY(btnCompanion->property("active").toBool());
        QVERIFY(!btnLibrary->isChecked());

        // Click Library button
        QTest::mouseClick(btnLibrary, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Pocket::App::AppNavigation::Section>(),
                 Pocket::App::AppNavigation::Section::Library);
        QCOMPARE(pages->currentWidget(), window.libraryWidget());
        QVERIFY(window.libraryWidget()->isVisible());
        QCOMPARE(nav->activeSection(), Pocket::App::AppNavigation::Section::Library);
        QVERIFY(btnLibrary->isChecked());
        QVERIFY(btnLibrary->property("active").toBool());
        QVERIFY(!btnCompanion->isChecked());

        // Click Settings button
        QTest::mouseClick(btnSettings, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Pocket::App::AppNavigation::Section>(),
                 Pocket::App::AppNavigation::Section::Settings);
        QCOMPARE(pages->currentWidget(), window.settingsWidget());
        QVERIFY(window.settingsWidget()->isVisible());
        QCOMPARE(nav->activeSection(), Pocket::App::AppNavigation::Section::Settings);

        // Diagnostics is hosted inside SettingsWidget
        QVERIFY(window.settingsWidget()->diagnosticsWidget() != nullptr);
    }

    void emulatorPageBackToLibrary() {
        auto ctx = createTestContext();
        QVERIFY(ctx != nullptr);

        Pocket::App::MainWindow window(ctx->dbManager, ctx->repo);
        window.resize(900, 600);
        window.show();
        QTest::qWait(20);

        auto* pages = window.pages();
        QVERIFY(pages);

        // Switch to emulator page
        pages->setCurrentWidget(window.emulatorPage());
        QCOMPARE(pages->currentWidget(), window.emulatorPage());

        auto* backBtn = window.findChild<QPushButton*>("backToLibraryButton");
        QVERIFY(backBtn);

        QTest::mouseClick(backBtn, Qt::LeftButton);
        QCOMPARE(pages->currentWidget(), window.libraryWidget());
        QCOMPARE(window.navigation()->activeSection(), Pocket::App::AppNavigation::Section::Library);
    }
};

QTEST_MAIN(TestAppNavigation)
#include "test_app_navigation.moc"
