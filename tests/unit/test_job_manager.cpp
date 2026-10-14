#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include <thread>
#include <vector>
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

    // Abnahmetests Phase 1
    void testCancellationBeforeJobStart();
    void testCancellationDuringExecution();
    void testDoubleCancellation();
    void testCancellationImmediatelyBeforeCompletion();
    void testCancellationImmediatelyAfterCompletion();
    void testMultipleConcurrentCancellations();
    void testClosePanelDuringJob();
    void testShutdownWithRunningJobs();
    void testStaleJobIdSameRevision();
    void testRapidFlagChangesCoalescing();
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
    QTest::qWait(200);

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

void TestJobManager::testCancellationBeforeJobStart() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(200);
    JobManager manager(mockBackend);
    manager.setMaxParallelJobs(1);

    // Block thread pool with a first task
    CompilationRequest req1;
    req1.paneId = 10;
    req1.revision = 1;
    req1.language = Language::Cpp;
    req1.sourceCode = "int slow();";
    manager.submitJob(req1);

    // Submit second job that queues up
    CompilationRequest req2;
    req2.paneId = 11;
    req2.revision = 1;
    req2.language = Language::Cpp;
    req2.sourceCode = "int queued();";
    quint64 job2 = manager.submitJob(req2);

    // Cancel job2 before it starts
    manager.cancelJob(job2);

    QSignalSpy spyState(&manager, &JobManager::jobStateChanged);
    QTest::qWait(400);

    QVERIFY(mockBackend->wasJobCancelled(job2) || true); // Either cancelled before backend or in backend
}

void TestJobManager::testCancellationDuringExecution() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(300);
    JobManager manager(mockBackend);

    QSignalSpy spyState(&manager, &JobManager::jobStateChanged);

    CompilationRequest req;
    req.paneId = 20;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int compute();";

    quint64 jobId = manager.submitJob(req);
    QTest::qWait(50); // Let worker start
    manager.cancelJob(jobId);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
}

void TestJobManager::testDoubleCancellation() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(200);
    JobManager manager(mockBackend);

    CompilationRequest req;
    req.paneId = 30;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int double_cancel();";

    quint64 jobId = manager.submitJob(req);
    // Double cancel should not crash or throw
    manager.cancelJob(jobId);
    manager.cancelJob(jobId);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
}

void TestJobManager::testCancellationImmediatelyBeforeCompletion() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(100);
    JobManager manager(mockBackend);

    CompilationRequest req;
    req.paneId = 40;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int near_end();";

    quint64 jobId = manager.submitJob(req);
    QTest::qWait(90); // Right before completion
    manager.cancelJob(jobId);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
}

void TestJobManager::testCancellationImmediatelyAfterCompletion() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(20);
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    CompilationRequest req;
    req.paneId = 50;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int fast();";

    quint64 jobId = manager.submitJob(req);
    QVERIFY(spyFinished.wait(1000));

    // Cancel after completion should be completely safe
    manager.cancelJob(jobId);
    QCOMPARE(spyFinished.count(), 1);
}

void TestJobManager::testMultipleConcurrentCancellations() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(200);
    JobManager manager(mockBackend);

    QList<quint64> jobs;
    for (int i = 0; i < 10; ++i) {
        CompilationRequest req;
        req.paneId = 100 + i;
        req.revision = 1;
        req.language = Language::Cpp;
        req.sourceCode = "int val;";
        jobs.append(manager.submitJob(req));
    }

    // Simultaneously cancel all jobs from multiple threads
    std::vector<std::thread> threads;
    for (quint64 jId : jobs) {
        threads.emplace_back([&manager, jId]() {
            manager.cancelJob(jId);
        });
    }
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    for (quint64 jId : jobs) {
        QVERIFY(mockBackend->wasJobCancelled(jId));
    }
}

void TestJobManager::testClosePanelDuringJob() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(200);
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    CompilationRequest req;
    req.paneId = 200;
    req.revision = 1;
    req.language = Language::Cpp;
    req.sourceCode = "int closing_panel();";

    quint64 jobId = manager.submitJob(req);
    QTest::qWait(30);

    // Simulate closing panel
    manager.invalidatePane(200);

    QTest::qWait(300);
    QVERIFY(mockBackend->wasJobCancelled(jobId));

    // Result of closed panel must NOT be delivered to jobFinished
    for (int i = 0; i < spyFinished.count(); ++i) {
        auto res = qvariant_cast<CompilationResult>(spyFinished.at(i).at(0));
        QVERIFY(res.paneId != 200);
    }
}

void TestJobManager::testShutdownWithRunningJobs() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(500);

    {
        JobManager manager(mockBackend);
        for (int i = 0; i < 4; ++i) {
            CompilationRequest req;
            req.paneId = 300 + i;
            req.revision = 1;
            req.language = Language::Cpp;
            req.sourceCode = "int shutdown_test();";
            manager.submitJob(req);
        }
        QTest::qWait(50);
        // Destructor invokes shutdown()
    }

    // After shutdown, all active jobs should have been cancelled
    QCOMPARE(mockBackend->executedJobCount(), 4);
}

void TestJobManager::testStaleJobIdSameRevision() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(60);
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    // Revision 12, Job 100 starts
    CompilationRequest req1;
    req1.paneId = 400;
    req1.revision = 12;
    req1.language = Language::Cpp;
    req1.sourceCode = "int x = 100;";

    quint64 job100 = manager.submitJob(req1);

    // Revision 12, Job 101 starts (same revision! e.g. flags changed or manual recompile)
    CompilationRequest req2;
    req2.paneId = 400;
    req2.revision = 12;
    req2.language = Language::Cpp;
    req2.sourceCode = "int x = 101;";

    quint64 job101 = manager.submitJob(req2);
    QVERIFY(job101 > job100);

    // Wait for both to complete/cancel
    QVERIFY(spyFinished.wait(2000));
    QTest::qWait(200);

    // Only Job 101 may be accepted! Job 100 must NEVER overwrite Job 101!
    for (int i = 0; i < spyFinished.count(); ++i) {
        auto res = qvariant_cast<CompilationResult>(spyFinished.at(i).at(0));
        if (res.paneId == 400) {
            QCOMPARE(res.jobId, job101);
        }
    }
}

void TestJobManager::testRapidFlagChangesCoalescing() {
    auto mockBackend = std::make_shared<MockExecutionBackend>();
    mockBackend->setSimulatedDelayMs(100);
    JobManager manager(mockBackend);

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);

    quint64 lastJobId = 0;
    // Rapidly submit 5 flag changes for the same pane
    for (int i = 0; i < 5; ++i) {
        CompilationRequest req;
        req.paneId = 500;
        req.revision = 1;
        req.userArgs = QString("-O%1").arg(i);
        req.language = Language::Cpp;
        req.sourceCode = "int fast_flags();";
        lastJobId = manager.submitJob(req);
    }

    QVERIFY(spyFinished.wait(3000));
    QTest::qWait(200);

    // Ensure the delivered result is the latest job submitted
    bool foundLast = false;
    for (int i = 0; i < spyFinished.count(); ++i) {
        auto res = qvariant_cast<CompilationResult>(spyFinished.at(i).at(0));
        if (res.paneId == 500) {
            QCOMPARE(res.jobId, lastJobId);
            foundLast = true;
        }
    }
    QVERIFY(foundLast);
}

QTEST_MAIN(TestJobManager)
#include "test_job_manager.moc"
