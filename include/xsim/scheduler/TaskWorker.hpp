#pragma once

#include "xsim/scheduler/RuntimeStats.hpp"
#include "xsim/scheduler/Task.hpp"

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
    TaskWorker(const Task& task, RuntimeStats& stats, std::mutex& output_mutex);
    ~TaskWorker();

    TaskWorker(const TaskWorker&) = delete;
    TaskWorker& operator=(const TaskWorker&) = delete;

    bool dispatch(const timespec& release_time, const timespec& deadline_time, uint64_t cycle);

private:
    void stop();
    void run();

    const Task& task_;
    RuntimeStats& stats_;
    std::mutex& output_mutex_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    TaskInvocation pending_{};
    bool has_pending_ = false;
    bool running_ = false;
    bool stopping_ = false;
};

} // namespace xsim
