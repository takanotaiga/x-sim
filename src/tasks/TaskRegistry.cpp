#include "xsim/tasks/TaskRegistry.hpp"

namespace xsim {
namespace sample_apps {

std::unique_ptr<Task> make_task_a(std::mutex& output_mutex);
std::unique_ptr<Task> make_task_b(std::mutex& output_mutex);
std::unique_ptr<Task> make_task_c(std::mutex& output_mutex);
std::unique_ptr<Task> make_task_d(std::mutex& output_mutex);

std::vector<std::unique_ptr<Task>> create_tasks(std::mutex& output_mutex)
{
    std::vector<std::unique_ptr<Task>> tasks;
    tasks.emplace_back(make_task_a(output_mutex));
    tasks.emplace_back(make_task_b(output_mutex));
    tasks.emplace_back(make_task_c(output_mutex));
    tasks.emplace_back(make_task_d(output_mutex));
    return tasks;
}

} // namespace sample_apps
} // namespace xsim
