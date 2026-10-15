# Threading Model

## Threads

| Thread | Runs | Rules |
| --- | --- | --- |
| **GUI thread** | all widgets, `JobManager` slot logic, `DockerCompilerManager` (asynchronous `QProcess` objects driven by the event loop), result presentation | never blocks on Docker; never waits for a worker except in `shutdown()` (bounded, with an observed worker counter) |
| **Compile workers** (`QThreadPool` in `JobManager`) | `IExecutionBackend::execute()` — one job per thread | own their `QProcess` and temporary directories; talk to shared state only through the backend registry and the manager's inbox |
| **Diff worker** (`QThreadPool` with one thread per `DiffDialog`) | `AssemblyDiffProcessor::computeDiff` | works on copies of the strings; polls `QPromise::isCanceled` |
| **Analysis worker** (global pool) | `AssemblyParser::parse` | works on a copy; result is a `shared_ptr<AssemblyModule>` |

## Shared state and the locks that guard it

| State | Lock | Held while |
| --- | --- | --- |
| `JobManager` queue, records, pane maps | `JobManager::m_mutex` | bookkeeping only; **never** while calling the backend or emitting signals |
| worker counter, `closed`, inbox | `Shared::mutex`, `Shared::inboxMutex` (std mutexes) | a few instructions |
| `DockerExecutionBackend::m_jobs`, `m_leaked`, config | `m_mutex` | bookkeeping only; **never** while a docker process runs |
| `AppSettings` | `QSettings` is thread-safe for access; the object is used from the GUI thread | |

## Worker → GUI hand-off

1. The worker builds a message (`state change` or `result`) and appends it to the **inbox**
   (mutex-protected deque in `JobManager::Shared`).
2. It posts a data-free `drainInbox` call to the manager with `Qt::QueuedConnection` (at most one pending).
3. The GUI thread drains the inbox in order: state changes become `jobStateChanged`, results go through
   `handleTaskFinished` (frees the slot, starts the next waiting job, decides staleness, emits `jobFinished`).

Why not post lambdas that carry the result? Qt's event queue uses library-internal atomics that
ThreadSanitizer cannot see; the inbox puts the data behind a mutex that it *does* see. Runnables handed to
a `QThreadPool` are annotated explicitly (`OCTOS_TSAN_RELEASE/ACQUIRE`, `src/utils/TsanAnnotations.h`), there
are no suppressions.

## Cancellation

* Every job has a `std::atomic_bool` cancel flag (shared with the worker).
* `cancelJob`/`cancelPaneJobs`/`invalidatePane`/`submitJob` (superseding) raise the flag for dispatched jobs,
  remove waiting jobs, and call `backend->cancel(jobId)`.
* `DockerExecutionBackend::cancel()` only marks the registry entry. The worker loop checks both the flag and
  the registry entry at least every 100 ms, kills its docker client and removes the container itself.
* Diff and analysis workers use `QPromise::isCanceled()`; the Myers loop polls every 4096 work units.

## Lifetime rules

* A worker holds `shared_ptr`s (backend, `Shared`) and a `QPointer` to the manager. It never dereferences the
  manager without holding `Shared::mutex` and seeing `closed == false`.
* The manager's destructor sets `closed` under that mutex *before* members are destroyed. If workers did not
  terminate in time, the thread pool is **leaked on purpose** (its destructor would block) and the workers
  finish against a closed manager.
* Dialogs cancel their futures in `done()`/destructor; their pools are drained in the destructor
  (cancellation is prompt).
* `QProcess` objects of the Docker manager are children of the manager and are killed in its destructor.

## Common pitfalls (all of them happened during development)

* A `QMutex` that is held across a docker call makes `cancel()` from the GUI thread freeze the UI.
* Capturing a `QPointer` in a lambda posted from another thread is a data race on the weak pointer.
* `Q_ASSERT(foo())` is compiled out in Release — never put side effects in it.
* `QSettings("Org", "App")` always uses the native store; pass the format explicitly (see `AppSettings`).
