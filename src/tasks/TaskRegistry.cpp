#include "xsim/tasks/TaskRegistry.hpp"

namespace xsim {
namespace sample_apps {

Task make_task_a(std::mutex& output_mutex);
Task make_task_b(std::mutex& output_mutex);
Task make_task_c(std::mutex& output_mutex);
Task make_task_d(std::mutex& output_mutex);

std::vector<Task> create_tasks(std::mutex& output_mutex)
{
    return {
        make_task_a(output_mutex),
        make_task_b(output_mutex),
        make_task_c(output_mutex),
        make_task_d(output_mutex),
    };
}

} // namespace sample_apps
} // namespace xsim
