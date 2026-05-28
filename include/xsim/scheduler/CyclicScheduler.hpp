#pragma once

#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/RuntimeStats.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/scheduler/TaskWorker.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace xsim {

class CyclicScheduler {
public:
    using StopRequested = std::function<bool()>;

    CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                    std::mutex& output_mutex,
                    long major_cycle_ns = DEFAULT_MAJOR_CYCLE_NS);
    ~CyclicScheduler();

    bool validate_schedule() const;
    void run_forever();
    void run_until(const StopRequested& stop_requested);

    const RuntimeStats& stats() const;

private:
    void initialize_tasks();
    void finalize_tasks();
    void start_workers();
    void stop_workers();

    std::vector<std::unique_ptr<Task>> tasks_;
    RuntimeStats stats_;
    std::vector<std::unique_ptr<TaskWorker>> workers_;
    std::mutex& output_mutex_;
    long major_cycle_ns_;
    bool tasks_initialized_ = false;
    bool tasks_finalized_ = false;
};

} // namespace xsim
