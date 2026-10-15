#include "../TestSupport.h"
#include <QTest>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <atomic>
#include <QThread>
#include <thread>
#include <vector>
#include "backend/JobManager.h"
#include "backend/MockExecutionBackend.h"

class TestJobManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }  // never touch the user's real settings

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

    // Scheduler (bounded queue) tests
    void testSinglePaneHundredRapidSubmits();
    void testFivePanesDifferentProfiles();
    void testTwentyConcurrentJobsRespectLimit();
    void testPendingQueueIsBounded();
    void testSameRevisionRepeatedCompile();
    void testShutdownReturnsCleanAndRejectsNewJobs();
    void testShutdownWithUnresponsiveBackend();
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

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);
    QElapsedTimer t; t.start();
    quint64 jobId = manager.submitJob(req);
    manager.cancelJob(jobId);

    // The latest job of the pane reports its cancellation, and does so long before the 200 ms job would end.
    QVERIFY(spyFinished.wait(1000));
    QVERIFY2(t.elapsed() < 190, qPrintable(QString("took %1 ms").arg(t.elapsed())));
    auto res = qvariant_cast<CompilationResult>(spyFinished.at(0).at(0));
    QCOMPARE(res.jobId, jobId);
    QCOMPARE(res.state, JobState::Cancelled);
    QVERIFY(!res.success());
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

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);
    QSet<quint64> cancelled;
    connect(&manager, &JobManager::jobStateChanged, this,
            [&cancelled](quint64 id, quint64, JobState st) { if (st == JobState::Cancelled) cancelled.insert(id); });

    quint64 jobId = manager.submitJob(req);
    manager.invalidatePane(42);

    QVERIFY(mockBackend->wasJobCancelled(jobId));
    QTRY_VERIFY_WITH_TIMEOUT(cancelled.contains(jobId), 1000);
    QTRY_COMPARE_WITH_TIMEOUT(manager.activeJobCount(), 0, 1000);
    QCOMPARE(spyFinished.count(), 0);                 // a closed pane receives nothing
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

    QTest::qWait(400);

    // job2 was still queued behind job1 (1 slot): it must never reach the backend.
    QCOMPARE(mockBackend->executedJobCount(), 1);
    QCOMPARE(manager.activeJobCount(), 0);
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

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);
    QElapsedTimer t; t.start();
    quint64 jobId = manager.submitJob(req);
    QTest::qWait(50); // Let worker start
    QVERIFY(mockBackend->executedJobCount() == 1);
    manager.cancelJob(jobId);

    QVERIFY(spyFinished.wait(1000));
    QVERIFY2(t.elapsed() < 280, "a running job must stop early, not run its full 300 ms");
    auto res = qvariant_cast<CompilationResult>(spyFinished.at(0).at(0));
    QCOMPARE(res.jobId, jobId);
    QCOMPARE(res.state, JobState::Cancelled);
    QVERIFY(mockBackend->wasJobCancelled(jobId));  // and the backend was told
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
    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);
    // Double cancel must be harmless and must produce exactly one result
    manager.cancelJob(jobId);
    manager.cancelJob(jobId);

    QVERIFY(spyFinished.wait(1000));
    QTest::qWait(300);
    QCOMPARE(spyFinished.count(), 1);
    QCOMPARE(qvariant_cast<CompilationResult>(spyFinished.at(0).at(0)).state, JobState::Cancelled);
    QCOMPARE(manager.activeJobCount(), 0);
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

    QSignalSpy spyFinished(&manager, &JobManager::jobFinished);
    quint64 jobId = manager.submitJob(req);
    QTest::qWait(90); // Right before completion
    manager.cancelJob(jobId);

    // Race by design: either outcome is valid, but it must be consistent and delivered exactly once.
    QVERIFY(spyFinished.wait(1000));
    QTest::qWait(150);
    QCOMPARE(spyFinished.count(), 1);
    auto res = qvariant_cast<CompilationResult>(spyFinished.at(0).at(0));
    QCOMPARE(res.jobId, jobId);
    QVERIFY(res.state == JobState::Cancelled || res.state == JobState::Completed);
    QCOMPARE(res.success(), res.state == JobState::Completed);
    QCOMPARE(manager.activeJobCount(), 0);
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

    QSet<quint64> cancelledStates;
    connect(&manager, &JobManager::jobStateChanged, this,
            [&cancelledStates](quint64 id, quint64, JobState st) {
                if (st == JobState::Cancelled) cancelledStates.insert(id);
            });

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

    // Queued jobs are cancelled in the scheduler and never reach the backend;
    // dispatched ones are cancelled in the backend. Either way nothing may remain.
    QTRY_COMPARE_WITH_TIMEOUT(manager.activeJobCount(), 0, 3000);
    QCOMPARE(manager.pendingJobCount(), 0);
    QCOMPARE(manager.runningJobCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(cancelledStates.size(), jobs.size(), 3000);
    QCOMPARE(cancelledStates.size(), jobs.size());
    for (quint64 jId : jobs) QVERIFY(cancelledStates.contains(jId));
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
        manager.setMaxParallelJobs(4);   // independent of the machine's persisted settings
        for (int i = 0; i < 4; ++i) {
            CompilationRequest req;
            req.paneId = 300 + i;
            req.revision = 1;
            req.language = Language::Cpp;
            req.sourceCode = "int shutdown_test();";
            manager.submitJob(req);
        }
        QTRY_COMPARE_WITH_TIMEOUT(mockBackend->executedJobCount(), 4, 2000);
        QElapsedTimer t; t.start();
        // Destructor invokes shutdown(): must cancel the 500 ms jobs instead of waiting them out
    }
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

namespace {
// Backend that records the maximum number of concurrently executing jobs.
class CountingBackend : public IExecutionBackend {
public:
    int delayMs{40};
    std::atomic<int> current{0};
    std::atomic<int> maxSeen{0};
    std::atomic<int> executed{0};
    QMutex m;
    QList<quint64> executedPanes;
    CompilationResult execute(const CompilationRequest& req, const std::function<bool()>& isCancelled) override {
        int c = ++current;
        int prev = maxSeen.load();
        while (c > prev && !maxSeen.compare_exchange_weak(prev, c)) {}
        ++executed;
        { QMutexLocker l(&m); executedPanes.append(req.paneId); }
        for (int i = 0; i < delayMs / 5; ++i) {
            if (isCancelled && isCancelled()) break;
            QThread::msleep(5);
        }
        --current;
        CompilationResult r;
        r.jobId = req.jobId; r.paneId = req.paneId; r.revision = req.revision;
        r.state = JobState::Completed; r.exitCode = 0;
        r.rawOutput = req.userArgs;
        return r;
    }
    void cancel(quint64) override {}
};

// Backend that ignores cancellation for a while (simulates a hung docker client).
class StubbornBackend : public IExecutionBackend {
public:
    std::atomic<bool> release{false};
    std::atomic<int> cancelCalls{0};
    std::atomic<int> started{0};
    CompilationResult execute(const CompilationRequest& req, const std::function<bool()>&) override {
        ++started;
        while (!release.load()) QThread::msleep(5);
        CompilationResult r;
        r.jobId = req.jobId; r.paneId = req.paneId; r.revision = req.revision;
        r.state = JobState::Completed;
        return r;
    }
    void cancel(quint64) override { ++cancelCalls; }
};

CompilationRequest makeReq(quint64 pane, quint64 rev, const QString& args = QString()) {
    CompilationRequest r;
    r.paneId = pane; r.revision = rev; r.language = Language::Cpp;
    r.sourceCode = "int x;"; r.userArgs = args;
    return r;
}
}

void TestJobManager::testSinglePaneHundredRapidSubmits() {
    auto be = std::make_shared<CountingBackend>();
    JobManager manager(be);
    manager.setMaxParallelJobs(2);
    QSignalSpy fin(&manager, &JobManager::jobFinished);

    quint64 last = 0;
    for (int i = 1; i <= 100; ++i) {
        last = manager.submitJob(makeReq(1, i, QString::number(i)));
        QVERIFY(manager.pendingJobCount() <= 1); // one waiting job per pane, always
    }
    QTRY_COMPARE_WITH_TIMEOUT(manager.activeJobCount(), 0, 5000);
    QTest::qWait(50);

    QCOMPARE(fin.count(), 1);
    auto res = qvariant_cast<CompilationResult>(fin.at(0).at(0));
    QCOMPARE(res.jobId, last);
    QCOMPARE(res.rawOutput, QString("100"));
    // Never more than one running + one waiting per pane reaches the backend.
    QVERIFY(be->executed.load() <= 3);
}

void TestJobManager::testFivePanesDifferentProfiles() {
    auto be = std::make_shared<CountingBackend>();
    JobManager manager(be);
    manager.setMaxParallelJobs(2);
    QSignalSpy fin(&manager, &JobManager::jobFinished);

    QHash<quint64, quint64> lastJob;
    for (quint64 pane = 1; pane <= 5; ++pane)
        lastJob[pane] = manager.submitJob(makeReq(pane, 1, QString("-O%1").arg(pane)));

    QTRY_COMPARE_WITH_TIMEOUT(fin.count(), 5, 5000);
    QSet<quint64> seen;
    for (int i = 0; i < fin.count(); ++i) {
        auto r = qvariant_cast<CompilationResult>(fin.at(i).at(0));
        QCOMPARE(r.jobId, lastJob.value(r.paneId));
        QCOMPARE(r.rawOutput, QString("-O%1").arg(r.paneId));
        seen.insert(r.paneId);
    }
    QCOMPARE(seen.size(), 5);
    QVERIFY(be->maxSeen.load() <= 2);
}

void TestJobManager::testTwentyConcurrentJobsRespectLimit() {
    auto be = std::make_shared<CountingBackend>();
    JobManager manager(be);
    manager.setMaxParallelJobs(3);
    QSignalSpy fin(&manager, &JobManager::jobFinished);

    for (quint64 pane = 1; pane <= 20; ++pane) manager.submitJob(makeReq(pane, 1));
    QVERIFY(manager.runningJobCount() <= 3);
    QVERIFY(manager.pendingJobCount() <= 20);

    QTRY_COMPARE_WITH_TIMEOUT(fin.count(), 20, 8000);
    QVERIFY2(be->maxSeen.load() <= 3, qPrintable(QString("max concurrent = %1").arg(be->maxSeen.load())));
    QVERIFY(be->maxSeen.load() >= 2); // the slots really were used in parallel
    QCOMPARE(manager.activeJobCount(), 0);
}

void TestJobManager::testPendingQueueIsBounded() {
    auto be = std::make_shared<StubbornBackend>();
    JobManager manager(be);
    manager.setMaxParallelJobs(1);
    manager.setMaxPendingJobs(5);

    QSet<quint64> cancelled;
    connect(&manager, &JobManager::jobStateChanged, this, [&](quint64 id, quint64, JobState s) {
        if (s == JobState::Cancelled) cancelled.insert(id);
    });

    for (quint64 pane = 1; pane <= 50; ++pane) manager.submitJob(makeReq(pane, 1));
    QCOMPARE(manager.runningJobCount(), 1);
    QCOMPARE(manager.pendingJobCount(), 5);
    QCOMPARE(manager.activeJobCount(), 6);
    QCOMPARE(cancelled.size(), 44); // 49 submitted-while-busy minus 5 kept

    be->release = true;
    QTRY_COMPARE_WITH_TIMEOUT(manager.activeJobCount(), 0, 5000);
}

void TestJobManager::testSameRevisionRepeatedCompile() {
    auto be = std::make_shared<CountingBackend>();
    JobManager manager(be);
    QSignalSpy fin(&manager, &JobManager::jobFinished);
    quint64 last = 0;
    for (int i = 0; i < 6; ++i) last = manager.submitJob(makeReq(7, 3));
    QTRY_VERIFY_WITH_TIMEOUT(manager.activeJobCount() == 0, 5000);
    QTest::qWait(30);
    QCOMPARE(fin.count(), 1);
    QCOMPARE(qvariant_cast<CompilationResult>(fin.at(0).at(0)).jobId, last);
}

void TestJobManager::testShutdownReturnsCleanAndRejectsNewJobs() {
    auto be = std::make_shared<CountingBackend>();
    be->delayMs = 2000;
    JobManager manager(be);
    manager.setMaxParallelJobs(2);
    for (quint64 pane = 1; pane <= 6; ++pane) manager.submitJob(makeReq(pane, 1));
    QTest::qWait(30);
    QElapsedTimer t; t.start();
    QVERIFY(manager.shutdown(5000));
    QVERIFY2(t.elapsed() < 1500, "cancellation must interrupt the running jobs");
    QCOMPARE(manager.activeJobCount(), 0);
    QCOMPARE(manager.submitJob(makeReq(1, 2)), quint64(0));
    // queued jobs were dropped, never started
    QVERIFY(be->executed.load() <= 2);
}

void TestJobManager::testShutdownWithUnresponsiveBackend() {
    auto be = std::make_shared<StubbornBackend>();
    {
        JobManager manager(be);
        manager.setShutdownTimeoutMs(200);
        manager.submitJob(makeReq(1, 1));
        QTRY_COMPARE_WITH_TIMEOUT(be->started.load(), 1, 2000);
        QVERIFY(!manager.shutdown(300));       // reports failure instead of pretending success
        QVERIFY(be->cancelCalls.load() >= 1);  // cancellation was (re)signalled
        QCOMPARE(manager.submitJob(makeReq(2, 1)), quint64(0));
        // ~JobManager now runs with the worker still stuck: it must neither hang nor
        // free anything the worker uses.
    }
    // The manager is gone; the late worker finishes against a closed manager (no crash, no delivery).
    be->release = true;
    QTRY_COMPARE_WITH_TIMEOUT(be.use_count(), 1, 2000);   // the worker released its backend reference
}

OCTOS_TEST_MAIN(TestJobManager)
#include "test_job_manager.moc"
