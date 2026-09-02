#include <QtTest>
#include <QLabel>
#include <QSettings>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include "CoresWidget.hpp"
#include "DevAssets.hpp"
#include "pocket/emulator/LibretroCoreProbe.hpp"

using namespace Pocket;

class CoresWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsUnconfigured();
    void readsSettings();
    void rejectsNonCorePath();
    void refreshHandlesInvalidPaths();
    void showsRealCoreWhenAvailable();

private:
    QTemporaryDir m_settingsDirectory;
};

void CoresWidgetTest::initTestCase()
{
    QVERIFY(m_settingsDirectory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDirectory.path());
}

void CoresWidgetTest::init()
{
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.clear();
    settings.sync();
}

void CoresWidgetTest::startsUnconfigured()
{
    App::CoresWidget widget;
    const auto labels = widget.findChildren<QLabel*>();
    int notConfigured = 0;
    for (const auto* label : labels) {
        if (label->text() == "Not configured")
            ++notConfigured;
    }
    QCOMPARE(notConfigured, 2);
}

void CoresWidgetTest::readsSettings()
{
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/mgbaCorePath", "C:/temporary/mgba.dll");
    settings.setValue("emulator/melonDsCorePath", "C:/temporary/melonds.dll");
    settings.sync();
    App::CoresWidget widget;
    QCOMPARE(widget.corePath(Core::GameSystem::GBA), QString("C:/temporary/mgba.dll"));
    QCOMPARE(widget.corePath(Core::GameSystem::NDS), QString("C:/temporary/melonds.dll"));
}

void CoresWidgetTest::rejectsNonCorePath()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("not a libretro core");
    file.flush();
    App::CoresWidget widget;
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/mgbaCorePath", file.fileName());
    settings.sync();
    widget.refresh();
    QVERIFY(widget.corePath(Core::GameSystem::GBA) == file.fileName());
    QVERIFY(!Emulator::probeLibretroCore(file.fileName().toStdString()).valid);
}

void CoresWidgetTest::refreshHandlesInvalidPaths()
{
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/mgbaCorePath", "C:/temporary/missing.dll");
    settings.setValue("emulator/melonDsCorePath", "C:/temporary/missing.dll");
    settings.sync();
    App::CoresWidget widget;
    widget.refresh();
    QCOMPARE(widget.corePath(Core::GameSystem::GBA), QString("C:/temporary/missing.dll"));
}

void CoresWidgetTest::showsRealCoreWhenAvailable()
{
    const QString core = DevAssets::melonDsCore();
    if (core.isEmpty())
        QSKIP("melonDS core is not configured");
    QSettings settings("PocketPartnerProject", "PocketPartner");
    settings.setValue("emulator/melonDsCorePath", core);
    settings.sync();
    App::CoresWidget widget;
    const auto description = Emulator::probeLibretroCore(core.toStdString());
    QVERIFY(description.valid);
    bool shown = false;
    for (const auto* label : widget.findChildren<QLabel*>()) {
        if (label->text().contains(QString::fromStdString(description.libraryName)) &&
            label->text().contains(QString::fromStdString(description.libraryVersion))) {
            shown = true;
            break;
        }
    }
    QVERIFY(shown);
}

QTEST_MAIN(CoresWidgetTest)
#include "test_cores_widget.moc"
