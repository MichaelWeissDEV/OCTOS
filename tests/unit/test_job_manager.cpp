#include <QTest>
#include <QSignalSpy>
#include "backend/JobManager.h"
#include "backend/MockExecutionBackend.h"

class TestJobManager : public QObject {
    Q_OBJECT

private slots:
    void testBasicJobExecution();
    void testRevisionInvalidation();
    void testJobCancellation();
    void testJobTimeout();
    void testPaneInvalidation();
};

void TestJobManager::testBasicJobExecution() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    CompilationRequest req;
    req.paneId = 1;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int main() { return 0; }";

    quint64 jobId = manager.submitJob(req);
    QVERIFY(jobId > 0);

    QVERIFY(spyFinished.wait(2000));
    QCOMPARE(spyFinished.count(), 1);

    auto result = qvariant_cast<CompilationResult>(spyFinished.at(0).at(0));
    QCOMPARE(result.jobId, jobId);
    QCOMPARE(result.paneId, static_cast<quint64>(1));
    QCOMPARE(result.state, JobState::Completed);
    QVERIFY(result.success());
}

void TestJobManager::testRevisionInvalidation() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(50);
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    CompilationRequest req1;
    req1.paneId = 1;
    req1.revision = 1;
    req1.language = Language::Cpp;
    req1.sourceCode = "int x = 1;";

    quint64 job1 = manager.submitJob(req1);

    // Fast-typing: submit revision 2 for the same pane immediately
    CompilationRequest req2;
    req2.paneId = 1;
    req2.revision = 2;
    req2.language = Language::Cpp;
    req2.sourceCode = "int x = 2;";

    quint64 job2 = manager.submitJob(req2);
    QVERIFY(job2 > job1);

    // Wait for completion
    QVERIFY(spyFinished.wait(3000));

    // Wait a little more to ensure both tasks finished
    QTest::qWait(200);

    // Only revision 2 should have been delivered as a successful pane update!
    // Outdated revision 1 must NOT overwrite revision 2!
    for (int i = 0; i < spyFinished.count(); ++i) {
        auto res = qvariant_cast<CompilationResult>(spyFinished.at(i).at(0));
        QVERIFY(res.revision >= 2);
    }
}

void TestJobManager::testJobCancellation() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(200);
    JobManager manager(mockBackend);

    CompilationRequest req;
    req.paneId = 2;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int main() { return 0; }";

    quint64 jobId = manager.submitJob(req);
    manager.cancelJob(jobId);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
}

void TestJobManager::testJobTimeout() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    CompilationRequest req;
    req.paneId = 3;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "// TRIGGER_TIMEOUT";

    quint64 jobId = manager.submitJob(req);
    QVERIFY(spyFinished.wait(2000));

    auto result = qvariant_cast<CompilationResult>(spyFinished.at(0).at(0));
    QCOMPARE(result.jobId, jobId);
    QCOMPARE(result.state, JobState::TimedOut);
    QVERIFY(!result.success());
}

void TestJobManager::testPaneInvalidation() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(100);
    JobManager manager(mockBackend);

    CompilationRequest req;
    req.paneId = 42;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int z = 100;";

    quint64 jobId = manager.submitJob(req);
    manager.invalidatePane(42);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
}

QTEST_MAIN(TestJobManager)
#include "test_job_manager.moc"
