#include "xsim/platform/Realtime.hpp"
#include "xsim/scheduler/CyclicScheduler.hpp"
#include "xsim/tasks/TaskRegistry.hpp"

#include <csignal>
#include <mutex>

namespace {

volatile std::sig_atomic_t g_stop_requested = 0;

void request_stop(int)
{
    g_stop_requested = 1;
}

} // namespace

int main()
{
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);

    std::mutex output_mutex;

    xsim::lock_memory();

    auto tasks = xsim::sample_apps::create_tasks(output_mutex);
    xsim::CyclicScheduler scheduler(std::move(tasks), output_mutex);

    if (!scheduler.validate_schedule()) {
        return 1;
    }

    scheduler.run_until([] {
        return g_stop_requested != 0;
    });
    return 0;
}
