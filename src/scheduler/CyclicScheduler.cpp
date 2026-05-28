#include "xsim/scheduler/CyclicScheduler.hpp"

#include <algorithm>
#include <iostream>

namespace xsim {

CyclicScheduler::CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                                 std::mutex& output_mutex,
                                 long major_cycle_ns)
    : tasks_(std::move(tasks)),
      output_mutex_(output_mutex),
      major_cycle_ns_(major_cycle_ns)
{
    std::sort(tasks_.begin(), tasks_.end(), [](const auto& a, const auto& b) {
        if (!a) {
            return false;
        }

        if (!b) {
            return true;
        }

        return a->offset_ns() < b->offset_ns();
    });
}

CyclicScheduler::~CyclicScheduler()
{
    stop_workers();
    finalize_tasks();
}

bool CyclicScheduler::validate_schedule() const
{
    for (const auto& task : tasks_) {
        if (!task) {
            std::cerr << "invalid task: null task\n";
            return false;
        }

        if (task->offset_ns() < 0 || task->offset_ns() >= major_cycle_ns_) {
            std::cerr << "invalid offset: " << task->name() << "\n";
            return false;
        }

        if (task->wcet_ns() <= 0) {
            std::cerr << "invalid WCET: " << task->name() << "\n";
            return false;
        }

        if (task->offset_ns() + task->wcet_ns() > major_cycle_ns_) {
            std::cerr << "task exceeds major cycle: " << task->name() << "\n";
            return false;
        }
    }

    return true;
}

void CyclicScheduler::run_forever()
{
    run_until([] {
        return false;
    });
}

void CyclicScheduler::run_until(const StopRequested& stop_requested)
{
    initialize_tasks();

    if (stop_requested()) {
        finalize_tasks();
        return;
    }

    start_workers();

    timespec cycle_start = now_monotonic();

    while (!stop_requested()) {
        const uint64_t current_cycle = stats_.cycle_count.load();
        const timespec current_cycle_start = cycle_start;
        const timespec current_cycle_end = add_ns(current_cycle_start, major_cycle_ns_);

        for (size_t i = 0; i < tasks_.size(); ++i) {
            const Task& task = *tasks_[i];
            timespec release_time = add_ns(current_cycle_start, task.offset_ns());
            timespec deadline_time = add_ns(release_time, task.wcet_ns());

            sleep_until(release_time);

            if (stop_requested()) {
                break;
            }

            workers_[i]->dispatch(release_time, deadline_time, current_cycle);
        }

        if (stop_requested()) {
            break;
        }

        sleep_until(current_cycle_end);

        const uint64_t completed_cycle = ++stats_.cycle_count;

        if (completed_cycle % 10 == 0) {
            std::lock_guard<std::mutex> output_lock(output_mutex_);
            std::cout
                << "cycle=" << completed_cycle
                << " overrun=" << stats_.overrun_count.load()
                << " late_start=" << stats_.late_start_count.load()
                << " cycle_skip=" << stats_.cycle_skip_count.load()
                << " dispatch_miss=" << stats_.dispatch_miss_count.load()
                << "\n";
        }

        cycle_start = add_ns(cycle_start, major_cycle_ns_);

        timespec now = now_monotonic();

        while (diff_ns(now, cycle_start) >= major_cycle_ns_) {
            cycle_start = add_ns(cycle_start, major_cycle_ns_);
            stats_.cycle_skip_count++;
        }
    }

    stop_workers();
    finalize_tasks();
}

const RuntimeStats& CyclicScheduler::stats() const
{
    return stats_;
}

void CyclicScheduler::start_workers()
{
    if (!workers_.empty()) {
        return;
    }

    workers_.reserve(tasks_.size());

    for (const auto& task : tasks_) {
        workers_.emplace_back(std::make_unique<TaskWorker>(*task, stats_, output_mutex_));
    }
}

void CyclicScheduler::stop_workers()
{
    workers_.clear();
}

void CyclicScheduler::initialize_tasks()
{
    if (tasks_initialized_) {
        return;
    }

    for (const auto& task : tasks_) {
        task->initialize();
    }

    tasks_initialized_ = true;
    tasks_finalized_ = false;
}

void CyclicScheduler::finalize_tasks()
{
    if (!tasks_initialized_ || tasks_finalized_) {
        return;
    }

    for (auto task = tasks_.rbegin(); task != tasks_.rend(); ++task) {
        (*task)->finalize();
    }

    tasks_finalized_ = true;
    tasks_initialized_ = false;
}

} // namespace xsim
