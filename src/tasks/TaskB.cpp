#include "xsim/platform/Clock.hpp"
#include "xsim/scheduler/Task.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

namespace xsim {
namespace sample_apps {

class TaskB final : public Task {
public:
    explicit TaskB(std::mutex& output_mutex)
        : Task("TaskB", 100 * NS_PER_MS, 10 * NS_PER_MS),
          output_mutex_(output_mutex)
    {
    }

    void tick() override
    {
        ++counter_;

        std::lock_guard<std::mutex> output_lock(output_mutex_);
        std::cout << "TaskB counter=" << counter_ << std::endl;
    }

private:
    std::mutex& output_mutex_;
    uint64_t counter_ = 0;
};

std::unique_ptr<Task> make_task_b(std::mutex& output_mutex)
{
    return std::make_unique<TaskB>(output_mutex);
}

} // namespace sample_apps
} // namespace xsim
