#include "xsim/scheduler/TaskWorker.hpp"

#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/TaskCycleContext.hpp"

namespace xsim {

TaskWorker::TaskWorker(Task& task,
                       RuntimeStats& stats,
                       WcetViolationPolicy wcet_violation_policy,
                       std::atomic<bool>& termination_requested)
    : task_(task),
      stats_(stats),
      wcet_violation_policy_(wcet_violation_policy),
      termination_requested_(termination_requested)
{
    thread_ = std::thread(&TaskWorker::run, this);
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

        logging::cerr
            << "[DISPATCH MISS] "
            << task_.name()
            << " cycle=" << cycle
            << " previous invocation still running"
            << logging::endl;
        return false;
    }

    cv_.notify_one();
    return true;
}

bool TaskWorker::wait_for_completion(uint64_t cycle, TaskCompletion& completion)
{
    std::unique_lock<std::mutex> lock(mutex_);
    completion_cv_.wait(lock, [this, cycle] {
        return stopping_ || (has_completion_ && last_completion_.cycle >= cycle);
    });

    if (!has_completion_ || last_completion_.cycle < cycle) {
        return false;
    }

    completion = last_completion_;
    return true;
}

timespec TaskWorker::wait_until_idle()
{
    std::unique_lock<std::mutex> lock(mutex_);
    completion_cv_.wait(lock, [this] {
        return stopping_ || (!running_ && !has_pending_);
    });

    return now_monotonic();
}

void TaskWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }

    cv_.notify_one();
    completion_cv_.notify_all();

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

        task_.set_current_cycle(invocation.cycle);

        {
            TaskCycleScope cycle_scope(invocation.cycle);
            task_.tick();
        }

        timespec actual_end = now_monotonic();
        long exec_ns = diff_ns(actual_end, actual_start);
        long deadline_lateness_ns = diff_ns(actual_end, invocation.deadline_time);
        const bool overran = exec_ns > task_.wcet_ns() || deadline_lateness_ns > 0;

        if (overran) {
            stats_.overrun_count++;

            logging::cerr
                << "[OVERRUN] "
                << task_.name()
                << " cycle=" << invocation.cycle
                << " exec_ns=" << exec_ns
                << " wcet_ns=" << task_.wcet_ns()
                << " deadline_lateness_ns=" << deadline_lateness_ns
                << " policy=" << to_string(wcet_violation_policy_)
                << logging::endl;

            if (wcet_violation_policy_ == WcetViolationPolicy::Terminate) {
                termination_requested_.store(true);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
            last_completion_ = {
                invocation.cycle,
                actual_start,
                actual_end,
                exec_ns,
                deadline_lateness_ns,
                overran,
            };
            has_completion_ = true;
        }

        completion_cv_.notify_all();
    }
}

} // namespace xsim
