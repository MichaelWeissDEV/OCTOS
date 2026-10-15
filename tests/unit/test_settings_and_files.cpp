#include "../TestSupport.h"
#include <QTest>
#include <QDirIterator>
#include <QTemporaryFile>
#include "core/AppSettings.h"
#include "managers/RecentFilesManager.h"

class TestSettingsAndFiles : public QObject {
    Q_OBJECT

private slots:
    void testAppSettingsDefaultsAndBounds();
    void testSettingsAreIsolatedFromUserPreferences();
    void testRecentFilesOrderingAndLimit();
};

void TestSettingsAndFiles::testSettingsAreIsolatedFromUserPreferences() {
    // Every test process redirects settings (see tests/TestSupport.h): nothing may reach the native store.
    const QString dir = qEnvironmentVariable("OCTOS_SETTINGS_DIR");
    QVERIFY2(!dir.isEmpty(), "test settings are not isolated");
    QCOMPARE(octosSettingsFormat(), QSettings::IniFormat);
    AppSettings::instance().setLayoutBlob("isolationProbe", "x");
    AppSettings::instance().sync();
    bool foundIni = false;
    QDirIterator it(dir, {"*.ini"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); foundIni = true; }
    QVERIFY2(foundIni, "settings were not written below OCTOS_SETTINGS_DIR");
    RecentFilesManager recent;                           // the second QSettings user in the application
    recent.clearRecentFiles();
    QVERIFY(recent.saveToSettings());                   // writes through the redirected store
}

void TestSettingsAndFiles::testAppSettingsDefaultsAndBounds() {
    auto& settings = AppSettings::instance();

    // Auto compile
    settings.setAutoCompile(true);
    QVERIFY(settings.autoCompile());
    settings.setAutoCompile(false);
    QVERIFY(!settings.autoCompile());
    settings.setAutoCompile(true);

    // Debounce interval bounds
    settings.setDebounceIntervalMs(10); // below minimum 50
    QCOMPARE(settings.debounceIntervalMs(), 50);

    settings.setDebounceIntervalMs(10000); // above maximum 5000
    QCOMPARE(settings.debounceIntervalMs(), 5000);

    settings.setDebounceIntervalMs(300);
    QCOMPARE(settings.debounceIntervalMs(), 300);

    // Max parallel jobs bounds
    settings.setMaxParallelJobs(0); // below minimum 1
    QCOMPARE(settings.maxParallelJobs(), 1);

    settings.setMaxParallelJobs(100); // above maximum 16
    QCOMPARE(settings.maxParallelJobs(), 16);

    settings.setMaxParallelJobs(4);
    QCOMPARE(settings.maxParallelJobs(), 4);

    // Docker limits
    settings.setDockerMemoryLimit("1024m");
    QCOMPARE(settings.dockerMemoryLimit(), QString("1024m"));

    settings.setDockerCpuLimit("2.5");
    QCOMPARE(settings.dockerCpuLimit(), QString("2.5"));
}

void TestSettingsAndFiles::testRecentFilesOrderingAndLimit() {
    RecentFilesManager manager;
    manager.clearRecentFiles();
    manager.setMaxRecentFiles(3);

    // Create 4 temporary files
    QTemporaryFile file1;
    QTemporaryFile file2;
    QTemporaryFile file3;
    QTemporaryFile file4;

    QVERIFY(file1.open());
    QVERIFY(file2.open());
    QVERIFY(file3.open());
    QVERIFY(file4.open());

    QString path1 = file1.fileName();
    QString path2 = file2.fileName();
    QString path3 = file3.fileName();
    QString path4 = file4.fileName();

    manager.addRecentFile(path1);
    manager.addRecentFile(path2);
    manager.addRecentFile(path3);

    QStringList list = manager.getRecentFiles();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0), path3); // most recent first
    QCOMPARE(list.at(1), path2);
    QCOMPARE(list.at(2), path1);

    // Add 4th file -> exceeds limit of 3, path1 should be dropped
    manager.addRecentFile(path4);
    list = manager.getRecentFiles();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0), path4);
    QCOMPARE(list.at(1), path3);
    QCOMPARE(list.at(2), path2);

    // Re-add path2 -> should move to top without duplicates
    manager.addRecentFile(path2);
    list = manager.getRecentFiles();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0), path2);
    QCOMPARE(list.at(1), path4);
    QCOMPARE(list.at(2), path3);

    manager.clearRecentFiles();
    QVERIFY(manager.getRecentFiles().isEmpty());
}

OCTOS_TEST_MAIN(TestSettingsAndFiles)
#include "test_settings_and_files.moc"
