#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <mutex>

namespace xsim {

Task make_task_d(std::mutex&)
{
    return Task{
        "TaskD",
        900 * NS_PER_MS,
        30 * NS_PER_MS,
        [] {
        },
    };
}

} // namespace xsim
