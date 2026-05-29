#include "xsim/tasks/TaskRegistry.hpp"

namespace xsim {
namespace sample_apps {

std::unique_ptr<Task> make_task_a();
std::unique_ptr<Task> make_task_b();
std::unique_ptr<Task> make_task_c();
std::unique_ptr<Task> make_task_d();

std::vector<std::unique_ptr<Task>> create_tasks()
{
    std::vector<std::unique_ptr<Task>> tasks;
    tasks.emplace_back(make_task_a());
    tasks.emplace_back(make_task_b());
    tasks.emplace_back(make_task_c());
    tasks.emplace_back(make_task_d());
    return tasks;
}

} // namespace sample_apps
} // namespace xsim
