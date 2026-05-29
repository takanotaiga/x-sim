#include "xsim/scheduler/CyclicScheduler.hpp"

#include "xsim/logging/Logging.hpp"

#include <algorithm>
#include <unordered_map>

namespace xsim {

CyclicScheduler::CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                                 long major_cycle_ns,
                                 WcetViolationPolicy wcet_violation_policy)
    : CyclicScheduler(std::move(tasks), {}, major_cycle_ns, wcet_violation_policy)
{
}

CyclicScheduler::CyclicScheduler(std::vector<std::unique_ptr<Task>> tasks,
                                 std::vector<TaskDependency> dependencies,
                                 long major_cycle_ns,
                                 WcetViolationPolicy wcet_violation_policy)
    : tasks_(std::move(tasks)),
      dependencies_(std::move(dependencies)),
      major_cycle_ns_(major_cycle_ns),
      wcet_violation_policy_(wcet_violation_policy)
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
    DependencyIndices dependency_indices;

    if (!resolve_dependencies(dependency_indices)) {
        return false;
    }

    for (const auto& task : tasks_) {
        if (!task) {
            logging::cerr << "invalid task: null task" << logging::endl;
            return false;
        }

        if (task->offset_ns() < 0 || task->offset_ns() >= major_cycle_ns_) {
            logging::cerr << "invalid offset: " << task->name() << logging::endl;
            return false;
        }

        if (task->wcet_ns() <= 0) {
            logging::cerr << "invalid WCET: " << task->name() << logging::endl;
            return false;
        }

        if (task->offset_ns() + task->wcet_ns() > major_cycle_ns_) {
            logging::cerr << "task exceeds major cycle: " << task->name() << logging::endl;
            return false;
        }
    }

    if (has_dependency_cycle(dependency_indices)) {
        logging::cerr << "dependency cycle detected" << logging::endl;
        return false;
    }

    for (size_t dependent_index = 0; dependent_index < dependency_indices.size(); ++dependent_index) {
        const Task& dependent = *tasks_[dependent_index];

        for (size_t prerequisite_index : dependency_indices[dependent_index]) {
            const Task& prerequisite = *tasks_[prerequisite_index];
            const long prerequisite_deadline_ns = prerequisite.offset_ns() + prerequisite.wcet_ns();

            if (prerequisite_deadline_ns > dependent.offset_ns()) {
                logging::cerr
                    << "dependency crosses fixed release window: "
                    << dependent.name()
                    << " depends_on " << prerequisite.name()
                    << logging::endl;
                return false;
            }
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
    DependencyIndices dependency_indices;

    if (!validate_schedule() || !resolve_dependencies(dependency_indices)) {
        logging::shutdown();
        return;
    }

    termination_requested_.store(false);

    initialize_tasks();

    if (stop_requested() || termination_requested_.load()) {
        finalize_tasks();
        logging::shutdown();
        return;
    }

    start_workers();

    timespec cycle_start = now_monotonic();

    while (!stop_requested() && !termination_requested_.load()) {
        const uint64_t current_cycle = stats_.cycle_count.load();
        const timespec current_cycle_start = cycle_start;
        const timespec current_cycle_end = add_ns(current_cycle_start, major_cycle_ns_);

        for (size_t i = 0; i < tasks_.size() && !termination_requested_.load(); ++i) {
            const Task& task = *tasks_[i];
            timespec release_time = dependency_release_time(
                dependency_indices,
                i,
                current_cycle,
                add_ns(current_cycle_start, task.offset_ns()));
            timespec deadline_time = add_ns(release_time, task.wcet_ns());

            sleep_until(release_time);

            if (stop_requested() || termination_requested_.load()) {
                break;
            }

            workers_[i]->dispatch(release_time, deadline_time, current_cycle);
        }

        if (stop_requested() || termination_requested_.load()) {
            break;
        }

        sleep_until(current_cycle_end);

        if (stop_requested() || termination_requested_.load()) {
            break;
        }

        const uint64_t completed_cycle = ++stats_.cycle_count;

        if (completed_cycle % 10 == 0) {
            logging::cout
                << "cycle=" << completed_cycle
                << " overrun=" << stats_.overrun_count.load()
                << " late_start=" << stats_.late_start_count.load()
                << " cycle_skip=" << stats_.cycle_skip_count.load()
                << " dispatch_miss=" << stats_.dispatch_miss_count.load()
                << " dependency_delay=" << stats_.dependency_delay_count.load()
                << logging::endl;
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
    logging::shutdown();
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
        workers_.emplace_back(
            std::make_unique<TaskWorker>(*task, stats_, wcet_violation_policy_, termination_requested_));
    }
}

void CyclicScheduler::stop_workers()
{
    workers_.clear();
}

bool CyclicScheduler::resolve_dependencies(DependencyIndices& dependency_indices) const
{
    dependency_indices.assign(tasks_.size(), {});

    if (dependencies_.empty()) {
        return true;
    }

    std::unordered_map<std::string, size_t> task_indices;

    for (size_t i = 0; i < tasks_.size(); ++i) {
        if (!tasks_[i]) {
            continue;
        }

        const std::string& name = tasks_[i]->name();

        if (!task_indices.emplace(name, i).second) {
            logging::cerr << "duplicate task name in dependency graph: " << name << logging::endl;
            return false;
        }
    }

    for (const TaskDependency& dependency : dependencies_) {
        const auto dependent = task_indices.find(dependency.dependent);

        if (dependent == task_indices.end()) {
            logging::cerr << "dependency references missing task: " << dependency.dependent << logging::endl;
            return false;
        }

        const auto prerequisite = task_indices.find(dependency.prerequisite);

        if (prerequisite == task_indices.end()) {
            logging::cerr << "dependency references missing task: " << dependency.prerequisite << logging::endl;
            return false;
        }

        dependency_indices[dependent->second].push_back(prerequisite->second);
    }

    return true;
}

bool CyclicScheduler::has_dependency_cycle(const DependencyIndices& dependency_indices) const
{
    enum class VisitState {
        Unvisited,
        Visiting,
        Visited,
    };

    std::vector<VisitState> states(tasks_.size(), VisitState::Unvisited);

    const auto visit = [&](const auto& self, size_t task_index) -> bool {
        if (states[task_index] == VisitState::Visiting) {
            return true;
        }

        if (states[task_index] == VisitState::Visited) {
            return false;
        }

        states[task_index] = VisitState::Visiting;

        for (size_t prerequisite_index : dependency_indices[task_index]) {
            if (self(self, prerequisite_index)) {
                return true;
            }
        }

        states[task_index] = VisitState::Visited;
        return false;
    };

    for (size_t i = 0; i < tasks_.size(); ++i) {
        if (visit(visit, i)) {
            return true;
        }
    }

    return false;
}

timespec CyclicScheduler::dependency_release_time(const DependencyIndices& dependency_indices,
                                                  size_t task_index,
                                                  uint64_t cycle,
                                                  const timespec& nominal_release_time)
{
    timespec release_time = nominal_release_time;

    for (size_t prerequisite_index : dependency_indices[task_index]) {
        const Task& task = *tasks_[task_index];
        const Task& prerequisite = *tasks_[prerequisite_index];
        TaskCompletion completion{};
        timespec prerequisite_ready_time{};
        bool has_completion = workers_[prerequisite_index]->wait_for_completion(cycle, completion);
        long prerequisite_lateness_ns = 0;

        if (has_completion) {
            prerequisite_ready_time = completion.end_time;
            prerequisite_lateness_ns = completion.deadline_lateness_ns;
        } else {
            prerequisite_ready_time = workers_[prerequisite_index]->wait_until_idle();
        }

        const long delay_ns = diff_ns(prerequisite_ready_time, nominal_release_time);

        if (delay_ns <= 0) {
            continue;
        }

        if (diff_ns(prerequisite_ready_time, release_time) > 0) {
            release_time = prerequisite_ready_time;
        }

        stats_.dependency_delay_count++;
        stats_.dependency_delay_ns.fetch_add(static_cast<uint64_t>(delay_ns));

        logging::cerr
            << "[DEPENDENCY DELAY] "
            << task.name()
            << " cycle=" << cycle
            << " depends_on=" << prerequisite.name()
            << " delay_ns=" << delay_ns
            << " prerequisite_deadline_lateness_ns=" << prerequisite_lateness_ns
            << logging::endl;
    }

    return release_time;
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
