#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <mutex>

namespace xsim {

Task make_task_c(std::mutex&)
{
    return Task{
        "TaskC",
        500 * NS_PER_MS,
        20 * NS_PER_MS,
        [] {
        },
    };
}

} // namespace xsim
