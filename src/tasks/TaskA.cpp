#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <iostream>
#include <memory>
#include <mutex>

namespace xsim {
namespace sample_apps {

class TaskA final : public Task {
public:
    explicit TaskA(std::mutex& output_mutex)
        : Task("TaskA", 0 * NS_PER_MS, 5 * NS_PER_MS),
          output_mutex_(output_mutex)
    {
    }

    void initialize() override
    {
    }

    void tick() override
    {
        std::lock_guard<std::mutex> output_lock(output_mutex_);
        std::cout << "TaskA realtime_ms=" << now_realtime_ms() << std::endl;
    }

    void finalize() override
    {
    }

private:
    std::mutex& output_mutex_;
};

std::unique_ptr<Task> make_task_a(std::mutex& output_mutex)
{
    return std::make_unique<TaskA>(output_mutex);
}

} // namespace sample_apps
} // namespace xsim
