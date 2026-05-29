#pragma once

#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/RuntimeStats.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/scheduler/TaskDependency.hpp"
#include "xsim/scheduler/TaskWorker.hpp"
#include "xsim/scheduler/WcetViolationPolicy.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace xsim {

class CyclicScheduler {
public:
    using StopRequested = std::function<bool()>;

    explicit CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                             long major_cycle_ns = DEFAULT_MAJOR_CYCLE_NS,
                             WcetViolationPolicy wcet_violation_policy = WcetViolationPolicy::Log);
    CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                    std::vector<TaskDependency> dependencies,
                    long major_cycle_ns = DEFAULT_MAJOR_CYCLE_NS,
                    WcetViolationPolicy wcet_violation_policy = WcetViolationPolicy::Log);
    ~CyclicScheduler();

    bool validate_schedule() const;
    void run_forever();
    void run_until(const StopRequested& stop_requested);

    const RuntimeStats& stats() const;

private:
    using DependencyIndices = std::vector<std::vector<size_t>>;

    void initialize_tasks();
    void finalize_tasks();
    void start_workers();
    void stop_workers();
    bool resolve_dependencies(DependencyIndices& dependency_indices) const;
    bool has_dependency_cycle(const DependencyIndices& dependency_indices) const;
    timespec dependency_release_time(const DependencyIndices& dependency_indices,
                                     size_t task_index,
                                     uint64_t cycle,
                                     const timespec& nominal_release_time);

    std::vector<std::unique_ptr<Task>> tasks_;
    std::vector<TaskDependency> dependencies_;
    RuntimeStats stats_;
    std::vector<std::unique_ptr<TaskWorker>> workers_;
    long major_cycle_ns_;
    WcetViolationPolicy wcet_violation_policy_;
    std::atomic<bool> termination_requested_{false};
    bool tasks_initialized_ = false;
    bool tasks_finalized_ = false;
};

} // namespace xsim
