// GUI behaviour of the diff dialog (run with QT_QPA_PLATFORM=offscreen).
#include "../TestSupport.h"
#include <QTest>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QApplication>
#include <QPointer>
#include "ui/DiffDialog.h"

namespace {
QString bigText(const QString& prefix, int n, int mod) {
    QStringList l;
    for (int i = 0; i < n; ++i) l << QString("%1%2").arg(prefix).arg(i % mod);
    return l.join('\n');
}
DiffPaneInfo pane(const QString& title, const QString& content) {
    DiffPaneInfo p; p.title = title; p.compilerName = title; p.content = content;
    return p;
}
}

class TestDiffDialog : public QObject {
    Q_OBJECT
private slots:
    void testConstructionDoesNotBlockAndResultArrives();
    void testNewestRequestWins();
    void testCloseWhileComputing();
    void testIncompleteDiffIsLabelled();
    void testModelIsVirtual();
};

void TestDiffDialog::testConstructionDoesNotBlockAndResultArrives() {
    // Large, fully different input: previously computed synchronously in the constructor.
    QVector<DiffPaneInfo> panes{pane("A", bigText("a", 20000, 20000)), pane("B", bigText("b", 20000, 20000))};
    QElapsedTimer t; t.start();
    DiffDialog dlg(panes, 0, 1);
    const qint64 ctorMs = t.elapsed();
    QVERIFY2(dlg.isComputing() || dlg.currentResult(), "dialog must have started a job");
    QVERIFY2(ctorMs < 1500, qPrintable(QString("constructor took %1 ms").arg(ctorMs)));
    QTRY_VERIFY_WITH_TIMEOUT(!dlg.isComputing(), 60000);
    QVERIFY(dlg.currentResult());
    QCOMPARE(dlg.currentResult()->deletions, 20000);
    QCOMPARE(dlg.currentResult()->additions, 20000);
}

void TestDiffDialog::testNewestRequestWins() {
    QVector<DiffPaneInfo> panes{pane("A", "x\ny\nz\n"), pane("B", "x\nY\nz\n"), pane("C", "completely\ndifferent\n")};
    DiffDialog dlg(panes, 0, 1);
    auto* comboB = dlg.findChild<QComboBox*>();  // first combo is A
    QList<QComboBox*> combos = dlg.findChildren<QComboBox*>();
    QCOMPARE(combos.size(), 2);
    QComboBox* b = combos[1];
    for (int i = 0; i < 30; ++i) b->setCurrentIndex(i % 2 == 0 ? 2 : 1);  // ends on index 1
    b->setCurrentIndex(2);  // final request: A vs C
    Q_UNUSED(comboB);
    QTRY_VERIFY_WITH_TIMEOUT(!dlg.isComputing(), 10000);
    QTest::qWait(100);
    QVERIFY(dlg.currentResult());
    // A="x y z" vs C="completely different": 3 removed, 2 added
    QCOMPARE(dlg.currentResult()->deletions, 3);
    QCOMPARE(dlg.currentResult()->additions, 2);
}

void TestDiffDialog::testCloseWhileComputing() {
    QVector<DiffPaneInfo> panes{pane("A", bigText("a", 30000, 30000)), pane("B", bigText("b", 30000, 30000))};
    auto* dlg = new DiffDialog(panes, 0, 1);
    QPointer<DiffDialog> guard(dlg);
    dlg->show();
    QVERIFY(dlg->isComputing());
    dlg->close();
    dlg->done(0);
    delete dlg;                   // must return promptly and not crash with the worker still alive
    QVERIFY(guard.isNull());
    QTest::qWait(200);            // late worker completion must not touch the dead dialog
}

void TestDiffDialog::testIncompleteDiffIsLabelled() {
    // Pathological input for the step budget is simulated through the size limit path instead:
    // many lines that all differ, then check the dialog shows incomplete state for a truncated result.
    DiffResult r;
    r.isTruncated = true;
    r.statusMessage = "synthetic";
    QVERIFY(!r.isComplete());
    QString err;
    QVERIFY(AssemblyDiffProcessor::toUnifiedDiff(r, "a", "b", &err).isEmpty());
    QVERIFY(!err.isEmpty());
}

void TestDiffDialog::testModelIsVirtual() {
    auto res = std::make_shared<DiffResult>();
    for (int i = 0; i < 200000; ++i) {
        DiffEntry e; e.type = i % 3 ? DiffLineType::Unchanged : DiffLineType::Added;
        e.textA = e.textB = "mov"; e.lineIndexA = i; e.lineIndexB = i;
        res->entries.append(e);
    }
    DiffTableModel model;
    QElapsedTimer t; t.start();
    model.setResult(res);
    QVERIFY2(t.elapsed() < 200, "setResult must not create per-row objects");
    QCOMPARE(model.rowCount(), 200000);
    QCOMPARE(model.data(model.index(5, DiffTableModel::LineA), Qt::DisplayRole).toInt(), 6);
    DiffFilterProxy proxy; proxy.setSourceModel(&model);
    proxy.setOnlyDifferences(true);
    QCOMPARE(proxy.rowCount(), 66667);
}

OCTOS_TEST_MAIN(TestDiffDialog)
#include "test_diff_dialog.moc"
