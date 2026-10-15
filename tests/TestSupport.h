// Shared helpers for all test executables.
#ifndef OCTOS_TEST_SUPPORT_H
#define OCTOS_TEST_SUPPORT_H

#include <QByteArray>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <cstdlib>

// Redirects every AppSettings access of this test process into a throw-away directory. It runs as a
// static initialiser, i.e. before QApplication and before the first AppSettings::instance(), so a test run
// can never read or write the user's real OCTOS preferences (on macOS the native CFPreferences store is NOT
// redirected by QStandardPaths::setTestModeEnabled).
inline int octosIsolateSettings() {
    static QTemporaryDir dir;
    if (dir.isValid()) qputenv("OCTOS_SETTINGS_DIR", dir.path().toUtf8());
    return 0;
}

#define OCTOS_TEST_MAIN(TestClass) \
    static const int octosSettingsIsolation_##TestClass = octosIsolateSettings(); \
    QTEST_MAIN(TestClass)

#endif  // OCTOS_TEST_SUPPORT_H
