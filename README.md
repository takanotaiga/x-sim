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
include/xsim/tasks/        Task registry API

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

## Notes

- Scheduler timing uses `CLOCK_MONOTONIC`.
- Display timestamps use `CLOCK_REALTIME`.
- Waiting is not busy wait: the scheduler uses `clock_nanosleep` with absolute time, and workers wait on `std::condition_variable`.
- If a task is still running when the next invocation of the same task is dispatched, that invocation is dropped and counted as `dispatch_miss`.
