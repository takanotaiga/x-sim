#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <mutex>

namespace xsim {
namespace sample_apps {

Task make_task_b(std::mutex&)
{
    return Task{
        "TaskB",
        100 * NS_PER_MS,
        10 * NS_PER_MS,
        [] {
        },
    };
}

} // namespace sample_apps
} // namespace xsim
