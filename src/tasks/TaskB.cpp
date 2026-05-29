#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <cstdint>
#include <memory>

namespace xsim {
namespace sample_apps {

class TaskB final : public Task {
public:
    TaskB()
        : Task("TaskB", 100 * NS_PER_MS, 10 * NS_PER_MS)
    {
    }

    void tick() override
    {
        ++counter_;

        logging::cout << "TaskB counter=" << counter_ << logging::endl;
    }

private:
    uint64_t counter_ = 0;
};

std::unique_ptr<Task> make_task_b()
{
    return std::make_unique<TaskB>();
}

} // namespace sample_apps
} // namespace xsim
