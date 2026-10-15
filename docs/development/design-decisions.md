# Design Decisions

The reasoning behind choices that are not obvious from the code. Each entry: the decision, the alternative,
and what made the difference.

## Scheduler in front of the thread pool
*Decision:* jobs wait in `JobManager`'s own queue and are only given to the pool when a slot is free.
*Alternative:* `QThreadPool::start()` for everything (the earlier code) and "cancel" old tasks.
*Why:* cancelled-but-unstarted runnables stay in the pool, so a user typing quickly grows an unbounded backlog,
and fairness between panes cannot be expressed. With an own queue replacing a pane's waiting job is O(1) and
the pool never holds more than it can run.

## Replacement keeps the queue position
A pane's new request replaces its waiting one *in place*. Moving it to the back would let other panes
overtake a pane that constantly edits (starving it); moving it to the front would let one pane monopolise the
queue. In place gives each pane exactly one slot in the line.

## Observed shutdown instead of `waitForDone(5000)`
A fixed wait is not evidence that workers ended. `shutdown()` counts workers and waits until the count is
zero, re-sending cancellation, and reports failure honestly. If workers stay stuck, the pool is leaked rather
than destroyed under them: leaking a thread pool at process exit is harmless; destroying shared state that a
running thread uses is not.

## Cancel is a flag, cleanup belongs to the worker
`cancel()` is called from the GUI thread, so it must be instant. The worker owns the process and the container
name, so it is the only one that can *know* the cleanup finished; a cancel that also ran `docker rm` would
block the UI and report success it cannot prove.

## Container name stays registered until cleanup finished
Removing the name first would make a failed cleanup invisible. A failed cleanup is tracked ("leaked") and
retried; the user sees a warning.

## `--pull=never`
`docker run` pulls missing images silently, which can mean gigabytes of download as a side effect of typing a
character. Downloads are an explicit action with a confirmation.

## File-size ulimit above the read limit
`--ulimit fsize` equal to the read limit would kill the compiler before truncation could ever happen, turning
"incomplete" into a confusing failure. It sits at the *total* artifact limit; exceeding it is reported as a
limit violation (exit 153).

## Verified means probed
Name-based "verified" misled users: a `gcc` image without `g++` looked fine. The probe checks the real tools
and records the version, architecture and image ID, and expires when the image behind the tag changes.

## Ownership is explicit
Matching names (`gcc:*`) says nothing about who pulled the image. OCTOS records the images it downloaded *when
they were not installed before*; deleting anything else needs a stronger confirmation.

## Linear-space Myers
The classic implementation stores the V array per edit step — O(D²) memory, hundreds of GB for completely
different 50k-line inputs. The divide-and-conquer variant needs O(N+M) and, with a work budget, a defined
failure mode. The coarse fallback is flagged, never silently substituted.

## Comments depend on the dialect
`#` is a comment on x86, an immediate on ARM64 and a constant-pool reference in javap output. Without a known
dialect only whitespace is touched.

## Label renaming by first appearance
Replacing every `.L…` with a constant makes different jump targets look identical. Numbering by first
appearance keeps structure; the cost (shifting after insertions) is documented.

## The parser never invents
Indirect jumps get an `IndirectUnknown` edge without target; unknown operands carry no flags; sizes are only
known from literal `.size`; a missing source file stays "unknown". Wrong certainty is worse than none.

## Calls do not split blocks
A call returns to the next instruction; for intra-procedural analysis the flow continues. Splitting would
inflate block counts and add edges that do not represent control transfer within the function.

## Project saves are atomic and per file
A project file holds all contents, so "save this file" must write the *saved* state plus this file — writing
the working copy would silently save other files' unfinished edits. `QSaveFile` guarantees the old file
survives any failure.

## Tab switches never edit the project
Compile target selection is user intent. Updating the target on every tab switch made fresh projects dirty
and silently changed the main file when a header tab was clicked.

## Theme tokens instead of stylesheets per widget
Per-widget stylesheets with hex colours only looked right in one theme and could not be tested. Tokens +
roles allow contrast tests and one place to change a colour.

## Inbox for worker → GUI results
See [Threading](threading.md#worker-gui-hand-off): data behind a mutex that TSan understands; only a data-free
call crosses Qt's queue.

## Settings isolation by environment variable
`QStandardPaths::setTestModeEnabled` does not redirect macOS' native preferences; tests that wrote there
changed a developer's real settings. A tiny `OCTOS_SETTINGS_DIR` switch (also useful for portable installs)
fixes it for every test executable at once.

## Docker as convenience boundary
It reliably bounds resources and accidents and removes the need to install toolchains; it is not claimed to
contain hostile code. The threat model says so.
