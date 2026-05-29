#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <memory>

namespace xsim {
namespace sample_apps {

class TaskD final : public Task {
public:
    TaskD()
        : Task("TaskD", 900 * NS_PER_MS, 30 * NS_PER_MS)
    {
    }

    void tick() override
    {
    }
};

std::unique_ptr<Task> make_task_d()
{
    return std::make_unique<TaskD>();
}

} // namespace sample_apps
} // namespace xsim
