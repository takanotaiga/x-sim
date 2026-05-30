#include "xsim/tasks/TaskRegistry.hpp"

#include "xsim/tasks/TaskCDMessage.hpp"

#include <utility>

namespace xsim {
namespace sample_apps {

std::unique_ptr<Task> make_task_a();
std::unique_ptr<Task> make_task_b();
std::unique_ptr<Task> make_task_c(TaskCDWriter writer);
std::unique_ptr<Task> make_task_d(TaskCDReader reader);

std::vector<std::unique_ptr<Task>> create_tasks()
{
    TaskCDChannel task_cd_channel;
    auto task_cd_writer = task_cd_channel.writer();
    auto task_cd_reader = task_cd_channel.reader();

    std::vector<std::unique_ptr<Task>> tasks;
    tasks.emplace_back(make_task_a());
    tasks.emplace_back(make_task_b());
    tasks.emplace_back(make_task_c(std::move(task_cd_writer)));
    tasks.emplace_back(make_task_d(std::move(task_cd_reader)));
    return tasks;
}

} // namespace sample_apps
} // namespace xsim
