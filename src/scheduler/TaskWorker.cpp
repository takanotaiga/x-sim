#include "xsim/scheduler/TaskWorker.hpp"

#include "xsim/platform/Clock.hpp"

#include <iostream>

namespace xsim {

TaskWorker::TaskWorker(const Task& task, RuntimeStats& stats, std::mutex& output_mutex)
    : task_(task),
      stats_(stats),
      output_mutex_(output_mutex),
      thread_(&TaskWorker::run, this)
{
}

TaskWorker::~TaskWorker()
{
    stop();
}

bool TaskWorker::dispatch(const timespec& release_time, const timespec& deadline_time, uint64_t cycle)
{
    bool missed = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stopping_) {
            return false;
        }

        if (running_ || has_pending_) {
            missed = true;
        } else {
            pending_ = {release_time, deadline_time, cycle};
            has_pending_ = true;
        }
    }

    if (missed) {
        stats_.dispatch_miss_count++;

        std::lock_guard<std::mutex> output_lock(output_mutex_);
        std::cerr
            << "[DISPATCH MISS] "
            << task_.name
            << " cycle=" << cycle
            << " previous invocation still running\n";
        return false;
    }

    cv_.notify_one();
    return true;
}

void TaskWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }

    cv_.notify_one();

    if (thread_.joinable()) {
        thread_.join();
    }
}

void TaskWorker::run()
{
    while (true) {
        TaskInvocation invocation{};

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return stopping_ || has_pending_;
            });

            if (stopping_ && !has_pending_) {
                return;
            }

            invocation = pending_;
            has_pending_ = false;
            running_ = true;
        }

        timespec actual_start = now_monotonic();
        long start_lateness_ns = diff_ns(actual_start, invocation.release_time);

        if (start_lateness_ns > 0) {
            stats_.late_start_count++;
        }

        task_.func();

        timespec actual_end = now_monotonic();
        long exec_ns = diff_ns(actual_end, actual_start);
        long deadline_lateness_ns = diff_ns(actual_end, invocation.deadline_time);

        if (exec_ns > task_.wcet_ns || deadline_lateness_ns > 0) {
            stats_.overrun_count++;

            std::lock_guard<std::mutex> output_lock(output_mutex_);
            std::cerr
                << "[OVERRUN] "
                << task_.name
                << " cycle=" << invocation.cycle
                << " exec_ns=" << exec_ns
                << " wcet_ns=" << task_.wcet_ns
                << " deadline_lateness_ns=" << deadline_lateness_ns
                << "\n";
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
    }
}

} // namespace xsim
