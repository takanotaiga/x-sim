#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/tasks/TaskCDMessage.hpp"

#include <memory>
#include <utility>

namespace xsim {
namespace sample_apps {

class TaskD final : public Task {
public:
    explicit TaskD(TaskCDReader reader)
        : Task("TaskD", 900 * NS_PER_MS, 30 * NS_PER_MS)
        , reader_(std::move(reader))
    {
    }

    void tick() override
    {
        const auto snapshot = reader_.read();

        if (!snapshot.has_value()) {
            logging::cout << "TaskD read no TaskC value" << logging::endl;
            return;
        }

        const TaskCDMessage& message = *snapshot.value;

        logging::cout
            << "TaskD read value=" << message.value
            << " produced_cycle=" << message.produced_cycle
            << " shared_cycle=" << snapshot.cycle
            << " sequence=" << snapshot.sequence
            << logging::endl;
    }

private:
    TaskCDReader reader_;
};

std::unique_ptr<Task> make_task_d(TaskCDReader reader)
{
    return std::make_unique<TaskD>(std::move(reader));
}

} // namespace sample_apps
} // namespace xsim
