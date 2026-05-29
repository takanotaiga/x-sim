#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Realtime.hpp"
#include "xsim/scheduler/CyclicScheduler.hpp"
#include "xsim/tasks/TaskRegistry.hpp"

#include <csignal>

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

    xsim::lock_memory();

    auto tasks = xsim::sample_apps::create_tasks();
    xsim::CyclicScheduler scheduler(std::move(tasks));

    if (!scheduler.validate_schedule()) {
        xsim::logging::shutdown();
        return 1;
    }

    scheduler.run_until([] {
        return g_stop_requested != 0;
    });
    return 0;
}
