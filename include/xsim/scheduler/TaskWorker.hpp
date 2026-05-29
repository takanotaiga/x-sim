#pragma once

#include "xsim/scheduler/RuntimeStats.hpp"
#include "xsim/scheduler/TaskCompletion.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/scheduler/WcetViolationPolicy.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <time.h>

namespace xsim {

struct TaskInvocation {
    timespec release_time{};
    timespec deadline_time{};
    uint64_t cycle{};
};

class TaskWorker {
public:
    TaskWorker(Task& task,
               RuntimeStats& stats,
               WcetViolationPolicy wcet_violation_policy,
               std::atomic<bool>& termination_requested);
    ~TaskWorker();

    TaskWorker(const TaskWorker&) = delete;
    TaskWorker& operator=(const TaskWorker&) = delete;

    bool dispatch(const timespec& release_time, const timespec& deadline_time, uint64_t cycle);
    bool wait_for_completion(uint64_t cycle, TaskCompletion& completion);
    timespec wait_until_idle();

private:
    void stop();
    void run();

    Task& task_;
    RuntimeStats& stats_;
    WcetViolationPolicy wcet_violation_policy_;
    std::atomic<bool>& termination_requested_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable completion_cv_;
    TaskInvocation pending_{};
    TaskCompletion last_completion_{};
    bool has_completion_ = false;
    bool has_pending_ = false;
    bool running_ = false;
    bool stopping_ = false;
    std::thread thread_;
};

} // namespace xsim
