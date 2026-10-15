#include "../TestSupport.h"
#include <QTest>
#include "core/ResultPresentation.h"

class TestResultPresentation : public QObject {
    Q_OBJECT
private slots:
    void testSuccess();
    void testTruncatedArtifactIsAWarningNotSuccess();
    void testFailureOriginsAreDistinguished();
    void testCleanupWarningIsShown();
    void testTimeoutAndCancel();
};

static CompilationResult ok() {
    CompilationResult r; r.state = JobState::Completed; r.exitCode = 0; return r;
}

void TestResultPresentation::testSuccess() {
    auto p = presentResult(ok());
    QCOMPARE(p.status, QString("Success"));
    QCOMPARE(p.level, PresentationLevel::Ok);
    QVERIFY(p.notice.isEmpty());
}

void TestResultPresentation::testTruncatedArtifactIsAWarningNotSuccess() {
    auto r = ok();
    r.artifactTruncated = true;
    auto p = presentResult(r);
    QCOMPARE(p.status, QString("Incomplete"));
    QCOMPARE(p.level, PresentationLevel::Warning);
    QVERIFY(p.notice.contains("truncated"));
    auto r2 = ok(); r2.stdoutTruncated = true;
    QCOMPARE(presentResult(r2).status, QString("Incomplete"));
}

void TestResultPresentation::testFailureOriginsAreDistinguished() {
    struct Row { FailureOrigin o; const char* text; };
    for (Row row : {Row{FailureOrigin::Compiler, "Compiler error"}, Row{FailureOrigin::Docker, "Docker error"},
                    Row{FailureOrigin::Limits, "Limit exceeded"}, Row{FailureOrigin::Internal, "OCTOS internal error"},
                    Row{FailureOrigin::None, "Failed"}}) {
        CompilationResult r; r.state = JobState::Failed; r.exitCode = 1; r.failureOrigin = row.o;
        auto p = presentResult(r);
        QCOMPARE(p.status, QString(row.text));
        QCOMPARE(p.level, PresentationLevel::Error);
    }
    // A "completed" job with a non-zero exit code is a failure, never "Success"
    CompilationResult bad; bad.state = JobState::Completed; bad.exitCode = 2; bad.failureOrigin = FailureOrigin::Compiler;
    QCOMPARE(presentResult(bad).level, PresentationLevel::Error);
}

void TestResultPresentation::testCleanupWarningIsShown() {
    auto r = ok();
    r.warnings << "Container octos_1_2 could not be removed; it will be retried.";
    auto p = presentResult(r);
    QVERIFY(p.notice.contains("octos_1_2"));
}

void TestResultPresentation::testTimeoutAndCancel() {
    CompilationResult t; t.state = JobState::TimedOut;
    QCOMPARE(presentResult(t).status, QString("Timeout"));
    CompilationResult c; c.state = JobState::Cancelled;
    QCOMPARE(presentResult(c).status, QString("Cancelled"));
    QCOMPARE(presentResult(c).level, PresentationLevel::Ok);
}

OCTOS_TEST_MAIN(TestResultPresentation)
#include "test_result_presentation.moc"
