#include "xsim/platform/Realtime.hpp"
#include "xsim/scheduler/CyclicScheduler.hpp"
#include "xsim/tasks/TaskRegistry.hpp"

#include <mutex>

int main()
{
    std::mutex output_mutex;

    xsim::lock_memory();

    auto tasks = xsim::sample_apps::create_tasks(output_mutex);
    xsim::CyclicScheduler scheduler(std::move(tasks), output_mutex);

    if (!scheduler.validate_schedule()) {
        return 1;
    }

    scheduler.run_forever();
    return 0;
}
