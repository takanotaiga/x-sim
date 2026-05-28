#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <memory>
#include <mutex>

namespace xsim {
namespace sample_apps {

class TaskC final : public Task {
public:
    TaskC()
        : Task("TaskC", 500 * NS_PER_MS, 20 * NS_PER_MS)
    {
    }

    void tick() override
    {
    }
};

std::unique_ptr<Task> make_task_c(std::mutex&)
{
    return std::make_unique<TaskC>();
}

} // namespace sample_apps
} // namespace xsim
