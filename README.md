# x-sim

`x-sim` is a cyclic scheduler prototype for running fixed-offset tasks on a 1000 ms major cycle. Each task is dispatched to its own pre-created worker thread so one task does not block dispatch of the next task.

## Requirements

- Linux
- CMake 3.16 or newer
- C++20 compiler
- pthread

## Build

```bash
cmake -S . -B build
cmake --build build
```

The executable is created at:

```bash
./build/x-sim
```

## Run

```bash
./build/x-sim
```

For a short timing check:

```bash
timeout 10s ./build/x-sim
```

The current `TaskA` prints the wall-clock Unix time in milliseconds once per 1000 ms cycle. `SIGINT` and `SIGTERM` request a graceful stop so task finalization hooks can run.

## Memory Locking

The program calls:

- `mlockall(MCL_CURRENT | MCL_FUTURE)`

This may fail without sufficient privileges or a high enough `RLIMIT_MEMLOCK`. If it fails, the program continues and prints:

```text
mlockall failed: ...
```

For production, configure memory locking appropriately, for example with `CAP_IPC_LOCK` or system resource limits.

## Project Layout

```text
include/xsim/platform/     Clock and realtime setup APIs
include/xsim/scheduler/    Task, worker, and cyclic scheduler APIs
include/xsim/shared/       Shared value API for task-to-task state
include/xsim/tasks/        Task registry API

src/logging/               Async logging implementation
src/platform/              Linux-specific clock and realtime implementation
src/scheduler/             Cyclic dispatch and per-task worker threads
src/tasks/                 TaskA-D implementations and registry

tests/                     Unit/integration test placeholders
```

## Current Schedule

```text
TaskA: cycle +   0 ms, WCET  5 ms
TaskB: cycle + 100 ms, WCET 10 ms
TaskC: cycle + 500 ms, WCET 20 ms
TaskD: cycle + 900 ms, WCET 30 ms
```

Task definitions live in `src/tasks/`. Define a task by deriving from `xsim::Task`, passing its name, offset, and WCET to the base constructor, and overriding `tick()`. Tasks may also override `initialize()` and `finalize()` for explicit setup and teardown.

```cpp
class TaskA final : public xsim::Task {
public:
    TaskA()
        : Task("TaskA", 0 * xsim::NS_PER_MS, 5 * xsim::NS_PER_MS)
    {
    }

    void initialize() override
    {
        // Runs before the first tick and is not counted against WCET.
    }

    void tick() override
    {
        // Runs once per scheduled invocation and is measured against WCET.
    }

    void finalize() override
    {
        // Runs after worker threads stop during graceful shutdown.
    }
};
```

The scheduler validates that each task starts inside the major cycle and that each task's offset plus WCET fits inside the cycle. All task `initialize()` calls complete before worker threads are started and before the first scheduled `tick()`. Only `tick()` execution is counted against WCET. Member variables stored inside each task object persist across ticks for that task instance and are released when the task object is destroyed after finalization.

WCET violations are controlled by `xsim::WcetViolationPolicy`. The default policy is `Log`, which records the overrun and keeps running. `Terminate` records the overrun, logs the active policy, then asks the scheduler loop to stop so workers are joined and task `finalize()` hooks still run where possible.

```cpp
xsim::CyclicScheduler scheduler(
    std::move(tasks),
    xsim::DEFAULT_MAJOR_CYCLE_NS,
    xsim::WcetViolationPolicy::Terminate);
```

Task dependencies can be registered by name. A dependency is scoped to the same major cycle and must fit inside the fixed release window, so the prerequisite task's deadline must be at or before the dependent task's offset.

```cpp
xsim::CyclicScheduler scheduler(
    std::move(tasks),
    {xsim::TaskDependency{"TaskB", "TaskA"}});
```

If `TaskA` completes by its deadline, `TaskB` keeps its fixed offset. If `TaskA` is still running at `TaskB`'s release time, `TaskB` waits for `TaskA` to complete and the scheduler records `dependency_delay_count` / `dependency_delay_ns` plus a `[DEPENDENCY DELAY]` log. This delay is tracked separately from `late_start`, which remains focused on worker dispatch timing after a task is released.

## Shared Task State

Task-to-task values can be passed through `xsim::SharedValue<T>`. A shared value exposes one move-only writer handle and any number of reader handles. This keeps each variable single-writer / multi-reader: read access can be shared freely, while a second writer request for the same value fails at setup time.

```cpp
xsim::SharedValue<MyMessage> channel;
auto writer = channel.writer();
auto reader = channel.reader();
```

Each task can read its current scheduler cycle through `Task::current_cycle()`. `SharedValue::Writer::write()` records that current cycle automatically, so task implementations do not need to maintain their own cycle counters just to tag shared data.

```cpp
writer.write(message);
```

Readers get an atomic snapshot of the latest value, writer cycle, and update sequence. Internally the value is protected by `std::shared_mutex`, so concurrent readers are allowed and reads do not race with writes.

`TaskRegistry` demonstrates explicit lifetime and dependency injection by creating one `TaskCDChannel`, passing its writer to `TaskC`, and passing its reader to `TaskD`. The channel state is owned by the handles shared with the tasks, so it remains valid for the scheduler/task lifetime without a global variable.

Current C-D sample flow:

```text
TaskC: cycle + 500 ms writes TaskCDMessage
TaskD: cycle + 900 ms reads the latest TaskCDMessage
```

## Notes

- Scheduler timing uses `CLOCK_MONOTONIC`.
- Display timestamps use `CLOCK_REALTIME`.
- Waiting is not busy wait: the scheduler uses `clock_nanosleep` with absolute time, and workers wait on `std::condition_variable`.
- If a task is still running when the next invocation of the same task is dispatched, that invocation is dropped and counted as `dispatch_miss`.
