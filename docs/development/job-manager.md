# Job Manager & Scheduler

`JobManager` (`src/backend/JobManager.*`) sits between the GUI (`submitJob()`) and a `QThreadPool`.
Earlier versions handed every job straight to the pool, so cancelled-but-unstarted `QRunnable`s piled up
in the pool's own queue. Now the pool only ever receives jobs that may run right away.

## Queue rules

* **At most `maxParallelJobs()` jobs are in flight** (default from `AppSettings`, changeable at runtime).
* **At most one *waiting* job per pane.** A new request of a pane replaces the waiting one *in place*
  (it keeps its queue position, so a pane that edits constantly neither starves nor jumps the line).
* A job of the same pane that is already running is cancelled (flag + backend `cancel()`).
* **Bounded queue:** at most `maxPendingJobs()` (default 32) jobs wait; when exceeded the oldest waiting
  job is dropped as *Cancelled*.
* Job IDs are unique for the lifetime of the process.
* **Stale results never reach the UI:** only the newest job of a pane whose revision is not superseded
  emits `jobFinished()`. Closing a pane (`invalidatePane`) discards everything for it.

State transitions: `Queued → Cancelled` (removed while waiting) and
`Queued → Starting → Running → Completed | Failed | TimedOut | Cancelled`.
A pending job that is cancelled never touches the backend.

## Synchronisation

All queue data is protected by one mutex that is never held while calling the backend or emitting
signals. Workers report back through a queued call to the GUI thread (`handleTaskFinished`), which frees
the slot and starts the next waiting job. Slot accounting therefore happens on the GUI thread; a blocked
GUI thread stalls the queue, but then nothing could be displayed anyway.

Workers hold only `shared_ptr`s (backend, a small `Shared` bookkeeping block) and a `QPointer` to the
manager. **Worker → GUI hand-off:** state changes and results are appended to a mutex-protected *inbox* in
`Shared`; the worker then pokes the manager with a data-free queued call (`drainInbox`, at most one pending).
The GUI thread drains the inbox in order. Posting is skipped once the manager is `closed`.

## Shutdown

`shutdown(timeoutMs)`:

1. refuse new jobs;
2. drop all waiting jobs;
3. invalidate results and raise the cancel flag of every in-flight job;
4. tell the backend to cancel (kills docker clients; the worker removes containers);
5. **wait until every worker has really returned** (a worker counter, not a fixed sleep), re-sending the
   cancellation every 250 ms;
6. only then release shared state.

It returns `false` if workers did not terminate within the timeout. In that case the destructor *leaks*
the thread pool on purpose (destroying it would block in `waitForDone`) and keeps everything the workers
use alive through their `shared_ptr`s; a late worker finishes against a `closed` manager and delivers
nothing. This path is covered by `testShutdownWithUnresponsiveBackend`.

## Tests

`tests/unit/test_job_manager.cpp`: 1 pane × 100 rapid edits, 5 panes with different profiles, 20 jobs
under a limit of 3, bounded queue, repeated compile with the same revision, cancellation before start /
during run / near completion / after completion, panel closed during a job, clean shutdown, shutdown with
an unresponsive backend. ThreadSanitizer runs of these tests are clean without any suppression
(see [Testing](testing.md#sanitizers)).
