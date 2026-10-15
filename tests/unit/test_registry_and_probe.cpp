// CompilerRegistry verification states, toolchain probe parsing, image ownership and the
// asynchronous image management. Docker-dependent tests use only locally present images
// (alpine:latest / bash:latest), only create and delete throw-away tags, and SKIP otherwise.
#include "../TestSupport.h"
#include <QTest>
#include <QSignalSpy>
#include <QProcess>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>
#include <QElapsedTimer>
#include "backend/DockerCompilerManager.h"
#include "core/CompilerRegistry.h"
#include "core/AppSettings.h"

Q_DECLARE_METATYPE(ToolchainProbeResult)
Q_DECLARE_METATYPE(QVector<DockerImage>)

namespace {
bool dockerUp() {
    QProcess p; p.start("docker", {"info"});
    return p.waitForStarted(2000) && p.waitForFinished(8000) && p.exitCode() == 0;
}
bool haveImage(const QString& img) {
    QProcess p; p.start("docker", {"images", "-q", img});
    return p.waitForStarted(2000) && p.waitForFinished(8000) && !p.readAllStandardOutput().trimmed().isEmpty();
}
bool imageExists(const QString& img) { return haveImage(img); }
}

class TestRegistryAndProbe : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);   // never touch the user's real settings
        qRegisterMetaType<ToolchainProbeResult>();
        qRegisterMetaType<QVector<DockerImage>>();
    }
    void testKnownNameIsNotVerified();
    void testProbeParsing();
    void testVerifiedStateRequiresProbeAndMatchingDigest();
    void testProbeJsonRoundTrip();
    void testImageNameMatchingIsNotOwnership();
    void testRefreshFailureIsReportedNotHidden();
    void testRealProbeVerifiedAndFailed();
    void testOwnedAndForeignDeletion();
    void testListingNeverBlocks();
};

static CompilerDefinition gccDef() {
    return CompilerRegistry::instance().inferOrMatchDefinition("gcc:13");
}

void TestRegistryAndProbe::testKnownNameIsNotVerified() {
    // The core of the old bug: "!isExperimental" was displayed as Verified.
    CompilerDefinition d = gccDef();
    QVERIFY(!d.isExperimental);
    QCOMPARE(d.verificationState(), VerificationState::ToolchainPlausible);
    QVERIFY(d.verificationState() != VerificationState::Verified);
    QVERIFY(d.actualVersion().isEmpty());
    QCOMPARE(CompilerRegistry::instance().inferOrMatchDefinition("somevendor/mystery:1").verificationState(),
             VerificationState::ImageRecognized);
    QCOMPARE(CompilerRegistry::instance().inferOrMatchDefinition("mono:6").verificationState(),
             VerificationState::ImageRecognized);   // experimental family: not even plausible
}

void TestRegistryAndProbe::testProbeParsing() {
    CompilerDefinition d = gccDef();   // requires gcc + g++
    auto ok = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nFOUND:g++\nARCH:aarch64\nVERSION:gcc (GCC) 13.2.0\n", "", "sha256:abc");
    QVERIFY(ok.success);
    QCOMPARE(ok.version, QString("gcc (GCC) 13.2.0"));
    QCOMPARE(ok.architecture, QString("aarch64"));
    QCOMPARE(ok.imageDigest, QString("sha256:abc"));
    QVERIFY(ok.checkedAt.isValid());

    auto missing = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nMISSING:g++\nARCH:x86_64\nVERSION:gcc 13\n", "", "id");
    QVERIFY(!missing.success);
    QCOMPARE(missing.missingBinaries, QStringList({"g++"}));
    QVERIFY(missing.message.contains("g++"));

    // A tool that was never reported is NOT assumed present
    auto silent = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nVERSION:gcc 13\n", "", "id");
    QVERIFY(!silent.success);
    QVERIFY(silent.missingBinaries.contains("g++"));

    auto noVersion = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nFOUND:g++\nARCH:x\n", "", "id");
    QVERIFY(!noVersion.success);

    auto crashed = ToolchainProbe::parseOutput(d, 126, "", "exec: \"sh\": executable file not found", "id");
    QVERIFY(!crashed.success);
    QVERIFY(crashed.performed);
    QVERIFY(crashed.message.contains("126"));
}

void TestRegistryAndProbe::testVerifiedStateRequiresProbeAndMatchingDigest() {
    CompilerDefinition d = gccDef();
    d.probe = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nFOUND:g++\nARCH:x86_64\nVERSION:gcc 13.2\n", "", "sha256:one");
    d.currentImageId = "sha256:one";
    QCOMPARE(d.verificationState(), VerificationState::Verified);
    QCOMPARE(d.actualVersion(), QString("gcc 13.2"));
    // The image behind the tag changed since the probe: the old result no longer counts
    d.currentImageId = "sha256:two";
    QCOMPARE(d.verificationState(), VerificationState::ToolchainPlausible);
    QVERIFY(d.actualVersion().isEmpty());
    // A failed probe is reported as failed, not hidden
    d.currentImageId = "sha256:one";
    d.probe.success = false; d.probe.missingBinaries = {"g++"};
    QCOMPARE(d.verificationState(), VerificationState::VerificationFailed);

    // Through the registry
    auto& reg = CompilerRegistry::instance();
    ToolchainProbeResult good = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nFOUND:g++\nARCH:aarch64\nVERSION:gcc 14\n", "", "sha256:xyz");
    reg.recordProbeResult("gcc:99", good);
    reg.setInstalledImageId("gcc:99", "sha256:xyz");
    auto fromReg = reg.compilerForImage("gcc:99");
    QCOMPARE(fromReg.verificationState(), VerificationState::Verified);
    QCOMPARE(fromReg.architecture, QString("aarch64"));       // actual architecture from the probe
    QCOMPARE(fromReg.imageDigest, QString("sha256:xyz"));
    reg.setInstalledImageId("gcc:99", "sha256:other");        // re-pulled under the same tag
    QCOMPARE(reg.compilerForImage("gcc:99").verificationState(), VerificationState::ToolchainPlausible);
}

void TestRegistryAndProbe::testProbeJsonRoundTrip() {
    CompilerDefinition d = gccDef();
    auto r = ToolchainProbe::parseOutput(d, 0, "FOUND:gcc\nFOUND:g++\nARCH:x86_64\nVERSION:gcc 13\n", "", "sha256:q");
    auto back = ToolchainProbeResult::fromJson(r.toJson());
    QCOMPARE(back.success, r.success);
    QCOMPARE(back.version, r.version);
    QCOMPARE(back.imageDigest, r.imageDigest);
    QCOMPARE(back.foundBinaries, r.foundBinaries);
    QVERIFY(back.checkedAt.isValid());
}

void TestRegistryAndProbe::testImageNameMatchingIsNotOwnership() {
    // A `gcc:*` image on the machine is recognisable as a compiler image, but OCTOS does not own it.
    QVERIFY(CompilerRegistry::instance().isKnownCompilerImageName("gcc:13"));
    DockerCompilerManager mgr;
    QVERIFY(!mgr.isOwnedByOctos("gcc:13"));
    AppSettings::instance().addOctosPulledImage("gcc:13");
    QVERIFY(mgr.isOwnedByOctos("gcc:13"));
    AppSettings::instance().removeOctosPulledImage("gcc:13");
    QVERIFY(!mgr.isOwnedByOctos("gcc:13"));
    // Non-compiler images are refused outright, confirmed or not
    QSignalSpy spy(&mgr, &DockerCompilerManager::imageRemoved);
    mgr.removeImage("postgres:18.6", true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toBool(), false);
    // A foreign compiler image needs explicit confirmation
    mgr.removeImage("gcc:13", false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(1).toBool(), false);
    QVERIFY(spy.at(1).at(2).toString().contains("not downloaded by OCTOS"));
}

void TestRegistryAndProbe::testRefreshFailureIsReportedNotHidden() {
    // A fake `docker` that fails like an unreachable daemon.
    QTemporaryDir bin;
    QFile f(bin.path() + "/docker");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("#!/bin/sh\necho 'Cannot connect to the Docker daemon' >&2\nexit 1\n");
    f.close();
    f.setPermissions(f.permissions() | QFile::ExeOwner | QFile::ExeUser);
    const QByteArray oldPath = qgetenv("PATH");
    qputenv("PATH", bin.path().toUtf8());

    DockerCompilerManager mgr;     // constructor already queries asynchronously
    QSignalSpy failed(&mgr, &DockerCompilerManager::installedImagesRefreshFailed);
    QSignalSpy ok(&mgr, &DockerCompilerManager::installedImagesRefreshed);
    mgr.refreshInstalledImagesAsync();
    QTRY_VERIFY_WITH_TIMEOUT(failed.count() >= 1, 5000);
    QCOMPARE(ok.count(), 0);       // an error must never look like "zero images installed"
    QVERIFY(failed.at(0).at(0).toString().contains("Cannot connect"));
    qputenv("PATH", oldPath);
}

void TestRegistryAndProbe::testRealProbeVerifiedAndFailed() {
    if (!dockerUp()) QSKIP("Docker daemon not reachable", SkipAll);
    if (!haveImage("bash:latest")) QSKIP("bash:latest not present locally (not pulling)", SkipAll);

    auto& reg = CompilerRegistry::instance();
    CompilerDefinition good;
    good.id = "test-bash"; good.containerImage = "bash:latest"; good.displayName = "bash";
    good.family = CompilerFamily::Generic;
    good.requiredBinaries = {"bash"};
    reg.registerCompiler(good);
    CompilerDefinition bad = good;
    bad.id = "test-bash-needs-gcc"; bad.containerImage = "bash:latest";
    // second registration of the same image would collide, so probe it directly through args/parse
    DockerCompilerManager mgr;
    QSignalSpy spy(&mgr, &DockerCompilerManager::toolchainProbed);
    mgr.probeToolchain("bash:latest");
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 60000);
    auto r = qvariant_cast<ToolchainProbeResult>(spy.at(0).at(1));
    QVERIFY2(r.success, qPrintable(r.message));
    QVERIFY2(r.version.contains("bash", Qt::CaseInsensitive), qPrintable(r.version));
    QVERIFY(!r.imageDigest.isEmpty());
    QVERIFY(!r.architecture.isEmpty());
    QCOMPARE(reg.compilerForImage("bash:latest").verificationState(), VerificationState::Verified);

    // Missing tool, using the real container: ask for a compiler that bash:latest does not have
    CompilerDefinition needGcc = good;
    needGcc.requiredBinaries = {"bash", "definitely-not-a-compiler"};
    QProcess p;
    p.start("docker", ToolchainProbe::buildDockerArgs("bash:latest", needGcc));
    QVERIFY(p.waitForFinished(60000));
    auto r2 = ToolchainProbe::parseOutput(needGcc, p.exitCode(), QString::fromUtf8(p.readAllStandardOutput()),
                                          QString::fromUtf8(p.readAllStandardError()), "x");
    QVERIFY(!r2.success);
    QCOMPARE(r2.missingBinaries, QStringList({"definitely-not-a-compiler"}));

    // Image that does not exist locally: reported, never pulled
    QSignalSpy spy2(&mgr, &DockerCompilerManager::toolchainProbed);
    mgr.probeToolchain("octos-nonexistent-image:0");
    QTRY_COMPARE_WITH_TIMEOUT(spy2.count(), 1, 20000);
    auto r3 = qvariant_cast<ToolchainProbeResult>(spy2.at(0).at(1));
    QVERIFY(!r3.success);
    QVERIFY(r3.message.contains("not available locally"));
}

void TestRegistryAndProbe::testOwnedAndForeignDeletion() {
    if (!dockerUp()) QSKIP("Docker daemon not reachable", SkipAll);
    if (!haveImage("alpine:latest")) QSKIP("alpine:latest not present locally", SkipAll);

    // Throw-away tags of an existing image: `rmi` of a tag never removes the image itself.
    const QString owned = "gcc:octos-test-owned", foreign = "gcc:octos-test-foreign";
    QProcess::execute("docker", {"tag", "alpine:latest", owned});
    QProcess::execute("docker", {"tag", "alpine:latest", foreign});
    QVERIFY(imageExists(owned) && imageExists(foreign));

    DockerCompilerManager mgr;
    QSignalSpy refreshed(&mgr, &DockerCompilerManager::installedImagesRefreshed);
    mgr.refreshInstalledImagesAsync();
    QTRY_VERIFY_WITH_TIMEOUT(refreshed.count() >= 1, 15000);
    QVERIFY(mgr.isImageInstalled(owned));
    // ownership is explicit, never inferred from the name
    QVERIFY(!mgr.isOwnedByOctos(owned));
    AppSettings::instance().addOctosPulledImage(owned);

    QSignalSpy removed(&mgr, &DockerCompilerManager::imageRemoved);
    mgr.removeImage(foreign, false);                      // foreign, unconfirmed: refused, image stays
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed.at(0).at(1).toBool(), false);
    QVERIFY(imageExists(foreign));

    mgr.removeImage(owned, false);                        // owned: removed
    QTRY_COMPARE_WITH_TIMEOUT(removed.count(), 2, 20000);
    QCOMPARE(removed.at(1).at(1).toBool(), true);
    QVERIFY(!imageExists(owned));
    QVERIFY(!mgr.isOwnedByOctos(owned));                  // ownership record cleaned up
    QVERIFY(imageExists("alpine:latest"));                // the underlying image was untouched

    mgr.removeImage(foreign, true);                       // explicit confirmation: removed
    QTRY_COMPARE_WITH_TIMEOUT(removed.count(), 3, 20000);
    QCOMPARE(removed.at(2).at(1).toBool(), true);
    QVERIFY(!imageExists(foreign));
}

void TestRegistryAndProbe::testListingNeverBlocks() {
    DockerCompilerManager mgr;
    QElapsedTimer t; t.start();
    for (int i = 0; i < 1000; ++i) (void)mgr.listInstalledImages();
    QVERIFY2(t.elapsed() < 100, "listInstalledImages() must be a cache read, not a docker call");
}

OCTOS_TEST_MAIN(TestRegistryAndProbe)
#include "test_registry_and_probe.moc"
