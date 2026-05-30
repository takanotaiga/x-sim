#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"
#include "xsim/tasks/TaskCDMessage.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace xsim {
namespace sample_apps {

class TaskC final : public Task {
public:
    explicit TaskC(TaskCDWriter writer)
        : Task("TaskC", 500 * NS_PER_MS, 20 * NS_PER_MS)
        , writer_(std::move(writer))
    {
    }

    void tick() override
    {
        const uint64_t cycle = current_cycle();

        TaskCDMessage message{
            cycle,
            cycle * 100,
            now_realtime_ms(),
        };

        writer_.write(message);

        logging::cout
            << "TaskC wrote value=" << message.value
            << " produced_cycle=" << message.produced_cycle
            << logging::endl;
    }

private:
    TaskCDWriter writer_;
};

std::unique_ptr<Task> make_task_c(TaskCDWriter writer)
{
    return std::make_unique<TaskC>(std::move(writer));
}

} // namespace sample_apps
} // namespace xsim
