#pragma once

#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/RuntimeStats.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/scheduler/TaskWorker.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace xsim {

class CyclicScheduler {
public:
    CyclicScheduler(std::vector<Task> tasks,
                    std::mutex& output_mutex,
                    long major_cycle_ns = DEFAULT_MAJOR_CYCLE_NS);

    bool validate_schedule() const;
    void run_forever();

    const RuntimeStats& stats() const;

private:
    void start_workers();

    std::vector<Task> tasks_;
    RuntimeStats stats_;
    std::vector<std::unique_ptr<TaskWorker>> workers_;
    std::mutex& output_mutex_;
    long major_cycle_ns_;
};

} // namespace xsim
