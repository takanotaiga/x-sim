#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <memory>

namespace xsim {
namespace sample_apps {

class TaskA final : public Task {
public:
    TaskA()
        : Task("TaskA", 0 * NS_PER_MS, 5 * NS_PER_MS)
    {
    }

    void initialize() override
    {
    }

    void tick() override
    {
        logging::cout << "TaskA realtime_ms=" << now_realtime_ms() << logging::endl;
    }

    void finalize() override
    {
    }

};

std::unique_ptr<Task> make_task_a()
{
    return std::make_unique<TaskA>();
}

} // namespace sample_apps
} // namespace xsim
