#pragma once

#include <cstdint>

namespace xsim {

bool has_current_task_cycle();
uint64_t current_task_cycle();

class TaskCycleScope {
public:
    explicit TaskCycleScope(uint64_t cycle);
    ~TaskCycleScope();

    TaskCycleScope(const TaskCycleScope&) = delete;
    TaskCycleScope& operator=(const TaskCycleScope&) = delete;

private:
    bool previous_active_;
    uint64_t previous_cycle_;
};

} // namespace xsim
