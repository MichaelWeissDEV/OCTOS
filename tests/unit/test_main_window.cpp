// Smoke + theme-regression test of the complete main window (offscreen).
// Writes a screenshot per theme into $OCTOS_SCREENSHOT_DIR when that variable is set.
#include "../TestSupport.h"
#include <QTest>
#include <QApplication>
#include <QImage>
#include <QStandardPaths>
#include <QDir>
#include "MainWindow.h"
#include "core/AppSettings.h"

namespace {
double luminanceOf(QRgb p) {
    auto lin = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * lin(qRed(p) / 255.0) + 0.7152 * lin(qGreen(p) / 255.0) + 0.0722 * lin(qBlue(p) / 255.0);
}
// Fraction of pixels that are "wrong" for the theme polarity.
double wrongPolarityFraction(const QImage& img, bool lightTheme) {
    qint64 wrong = 0, total = 0;
    for (int y = 0; y < img.height(); y += 2) {
        const QRgb* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); x += 2) {
            const double l = luminanceOf(line[x]);
            if (lightTheme ? l < 0.10 : l > 0.45) ++wrong;
            ++total;
        }
    }
    return total ? double(wrong) / total : 0;
}
}

class TestMainWindow : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void testStartsAndRendersEveryTheme();
    void testCompileResultIsPresented();
    void testArtifactViewerAndDiagnostics();
    void testCompileNowWorksWithAutoCompileOff();
    void testOutputKindSelectorFollowsCompiler();
};

void TestMainWindow::testStartsAndRendersEveryTheme() {
    MainWindow w;
    w.resize(1400, 850);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    const QString shotDir = qEnvironmentVariable("OCTOS_SCREENSHOT_DIR");
    const QStringList names = Theme::getAvailableThemes();
    for (int i = 0; i < names.size(); ++i) {
        QVERIFY(QMetaObject::invokeMethod(&w, "onChangeTheme", Q_ARG(int, i)));
        QTest::qWait(50);
        QImage img = w.grab().toImage();
        QVERIFY(!img.isNull());
        if (!shotDir.isEmpty()) {
            QDir().mkpath(shotDir);
            img.save(QString("%1/main_%2.png").arg(shotDir, QString(names[i]).replace(' ', '_')));
        }
        const bool light = (i == int(ThemeType::Light) || i == int(ThemeType::SolarizedLight));
        const double bad = wrongPolarityFraction(img, light);
        // Hard-coded dark panels in a light theme (or vice versa) would show up here.
        QVERIFY2(bad < 0.12, qPrintable(QString("%1: %2% of the window has the wrong polarity").arg(names[i]).arg(bad * 100)));
    }
}

void TestMainWindow::testCompileResultIsPresented() {
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    // A truncated artifact must reach the user as a visible notice, not just as a field in a struct.
    auto panes = w.findChildren<CompilerOutputPane*>();
    QVERIFY(!panes.isEmpty());
    CompilerOutputPane* pane = panes.first();
    CompilationResult r;
    r.paneId = pane->paneId();
    r.jobId = 77;
    r.revision = 1000;
    r.state = JobState::Completed;
    r.exitCode = 0;
    r.assemblyOutput = "main:\n\tret\n";
    r.artifactTruncated = true;
    r.warnings << "Output artifact was truncated at 1024 bytes; it is incomplete.";
    pane->setCurrentJobId(77);
    QVERIFY(QMetaObject::invokeMethod(&w, "onCompilationResultReceived", Q_ARG(CompilationResult, r)));
    QLabel* notice = nullptr;
    for (QLabel* l : pane->findChildren<QLabel*>())
        if (l->text().contains("truncated")) notice = l;
    QVERIFY2(notice, "truncation warning not shown");
    QVERIFY(notice->isVisible());
    bool incompleteBadge = false;
    for (QLabel* l : pane->findChildren<QLabel*>())
        if (l->text().contains("Incomplete")) incompleteBadge = true;
    QVERIFY(incompleteBadge);

    CompilationResult dockerErr;
    dockerErr.paneId = pane->paneId(); dockerErr.jobId = 78; dockerErr.revision = 1000;
    dockerErr.state = JobState::Failed; dockerErr.exitCode = 125;
    dockerErr.failureOrigin = FailureOrigin::Docker;
    dockerErr.errorOutput = "Cannot connect to the Docker daemon";
    pane->setCurrentJobId(78);
    QVERIFY(QMetaObject::invokeMethod(&w, "onCompilationResultReceived", Q_ARG(CompilationResult, dockerErr)));
    bool dockerBadge = false;
    for (QLabel* l : pane->findChildren<QLabel*>())
        if (l->text().contains("Docker error")) dockerBadge = true;
    QVERIFY(dockerBadge);
}

void TestMainWindow::testCompileNowWorksWithAutoCompileOff() {
    AppSettings::instance().setAutoCompile(false);
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QAction* act = nullptr;
    for (QAction* a : w.findChildren<QAction*>()) if (a->text().contains("Compile") && a->text().contains("Now")) act = a;
    QVERIFY(act);
    QVERIFY(act->shortcuts().contains(QKeySequence(Qt::Key_F5)));
    w.activeEditor()->setPlainText("int main() { return 0; }");
    CompilerOutputPane* pane = w.findChildren<CompilerOutputPane*>().first();
    auto statusText = [&]() { QStringList t; for (QLabel* l : pane->findChildren<QLabel*>()) t << l->text(); return t.join("|"); };
    QVERIFY2(!statusText().contains("No Image") && !statusText().contains("Compiling"), "auto-compile is off: nothing may run on typing");
    act->trigger();
    // Without an installed compiler the pane reports it instead of staying silent.
    QVERIFY2(statusText().contains("No Image") || statusText().contains("Compiling"), qPrintable(statusText()));
    AppSettings::instance().setAutoCompile(true);
}

void TestMainWindow::testOutputKindSelectorFollowsCompiler() {
    MainWindow w;
    w.show();
    CompilerOutputPane* pane = w.findChildren<CompilerOutputPane*>().first();
    // C++ without a Clang image: only assembly
    pane->setOutputKind(OutputKind::LlvmIr);
    QVERIFY(!pane->outputKindSelectable());
    QCOMPARE(pane->getOutputKind(), OutputKind::Assembly);
    // Rust always can
    pane->setLanguage(Language::Rust);
    QVERIFY(pane->outputKindSelectable());
    pane->setOutputKind(OutputKind::LlvmIr);
    QCOMPARE(pane->getOutputKind(), OutputKind::LlvmIr);
    // switching to a language that cannot, forces assembly again (a stale IR choice must not leak)
    pane->setLanguage(Language::Java);
    QVERIFY(!pane->outputKindSelectable());
    QCOMPARE(pane->getOutputKind(), OutputKind::Assembly);
    // the asm syntax choice is meaningless for IR
    pane->setLanguage(Language::Rust);
    pane->setOutputKind(OutputKind::LlvmIr);
    bool syntaxEnabled = true;
    for (QComboBox* c : pane->findChildren<QComboBox*>()) if (c->toolTip().contains("syntax", Qt::CaseInsensitive)) syntaxEnabled = c->isEnabled();
    QVERIFY(!syntaxEnabled);
}

void TestMainWindow::testArtifactViewerAndDiagnostics() {
    MainWindow w;
    w.resize(1400, 850);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    CompilerOutputPane* pane = w.findChildren<CompilerOutputPane*>().first();

    CompilationResult r;
    r.paneId = pane->paneId(); r.jobId = 5; r.revision = 1000;
    r.state = JobState::Completed; r.exitCode = 0;
    r.compilerImage = "clang:17"; r.durationMs = 123;
    CompilationArtifact asmArt; asmArt.type = ArtifactType::Assembly; asmArt.name = "output.s";
    asmArt.content = "main:\n\tmovl\t$0, %eax\n\tret\n"; asmArt.sizeBytes = asmArt.content.size();
    CompilationArtifact ir; ir.type = ArtifactType::LLVM_IR; ir.name = "out.ll";
    ir.content = "define i32 @main() {\nentry:\n  ret i32 0\n}\n"; ir.sizeBytes = ir.content.size();
    r.artifacts << asmArt << ir;
    Diagnostic d1; d1.fileName = "source.cpp"; d1.line = 1; d1.column = 5; d1.severity = DiagnosticSeverity::Warning; d1.message = "unused variable 'x'";
    Diagnostic d2; d2.fileName = "source.cpp"; d2.line = 1; d2.column = 1; d2.severity = DiagnosticSeverity::Note; d2.message = "declared here";
    r.diagnostics << d1 << d2;
    pane->setCurrentJobId(5);
    QVERIFY(QMetaObject::invokeMethod(&w, "onCompilationResultReceived", Q_ARG(CompilationResult, r)));

    QComboBox* sel = pane->artifactSelector();
    QVERIFY(sel->isVisible());
    QCOMPARE(sel->count(), 3);                                   // assembly, LLVM IR, diagnostics
    QVERIFY(sel->itemText(0).contains("Assembly"));
    QVERIFY(sel->itemText(1).contains("LLVM IR"));
    QVERIFY(sel->itemText(2).contains("Diagnostics (2)"));
    QCOMPARE(sel->currentIndex(), 0);                            // a successful job opens its assembly first
    QVERIFY(pane->getEditor()->toPlainText().contains("movl"));
    QVERIFY(!pane->showsDiagnostics());

    sel->setCurrentIndex(1);                                     // LLVM IR: plain text, not run through x86 filters
    QVERIFY(pane->getEditor()->toPlainText().contains("define i32 @main"));
    QVERIFY(pane->getEditor()->toPlainText().contains("ret i32 0"));
    // IR keywords carry the theme's keyword color (ArtifactHighlighter), assembly rules are detached
    QTest::qWait(50);   // QSyntaxHighlighter applies its formats from the event loop
    auto formats = pane->getEditor()->document()->firstBlock().layout()->formats();
    QVERIFY(!formats.isEmpty());
    QCOMPARE(formats.first().format.foreground().color(), Theme::currentTokens().syntaxKeyword.isValid()
             ? Theme::syntaxColor(SyntaxRole::Preprocessor) : QColor());   // `define` is a declaration keyword

    sel->setCurrentIndex(2);                                     // structured diagnostics list
    QVERIFY(pane->showsDiagnostics());
    QCOMPARE(pane->diagnosticsTable()->rowCount(), 2);
    QCOMPARE(pane->diagnosticsTable()->item(0, 0)->text(), QString("Warning"));
    QCOMPARE(pane->diagnosticsTable()->item(0, 4)->text(), QString("unused variable 'x'"));

    // The compiler messages appear as markers in the source editor
    auto editors = w.findChildren<CodeEditor*>();
    CodeEditor* source = nullptr;
    for (CodeEditor* e : editors) if (!e->isReadOnly()) source = e;
    QVERIFY(source);
    QCOMPARE(source->markers().size(), 2);

    // Double-click jumps to the source line
    source->setPlainText("int x;\nint y;\n");
    QMetaObject::invokeMethod(pane->diagnosticsTable(), "cellDoubleClicked", Q_ARG(int, 0), Q_ARG(int, 0));
    QCOMPARE(source->textCursor().blockNumber(), 0);

    const QString shotDir = qEnvironmentVariable("OCTOS_SCREENSHOT_DIR");
    if (!shotDir.isEmpty()) w.grab().save(shotDir + "/artifact_viewer.png");

    // Cancelled results clear the viewer
    CompilationResult c; c.paneId = pane->paneId(); c.jobId = 6; c.revision = 1000; c.state = JobState::Cancelled;
    pane->setCurrentJobId(6);
    QMetaObject::invokeMethod(&w, "onCompilationResultReceived", Q_ARG(CompilationResult, c));
    QCOMPARE(source->markers().size(), 0);
}

OCTOS_TEST_MAIN(TestMainWindow)
#include "test_main_window.moc"
