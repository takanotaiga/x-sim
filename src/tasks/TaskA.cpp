#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <iostream>
#include <mutex>

namespace xsim {

Task make_task_a(std::mutex& output_mutex)
{
    return Task{
        "TaskA",
        0 * NS_PER_MS,
        5 * NS_PER_MS,
        [&output_mutex] {
            std::lock_guard<std::mutex> output_lock(output_mutex);
            std::cout << now_realtime_ms() << std::endl;
        },
    };
}

} // namespace xsim
