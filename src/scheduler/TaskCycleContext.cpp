#include "xsim/scheduler/TaskCycleContext.hpp"

#include <stdexcept>

namespace xsim {
namespace {

thread_local bool current_task_cycle_active = false;
thread_local uint64_t current_task_cycle_value = 0;

} // namespace

bool has_current_task_cycle()
{
    return current_task_cycle_active;
}

uint64_t current_task_cycle()
{
    if (!current_task_cycle_active) {
        throw std::logic_error("current task cycle is not available outside task execution");
    }

    return current_task_cycle_value;
}

TaskCycleScope::TaskCycleScope(uint64_t cycle)
    : previous_active_(current_task_cycle_active)
    , previous_cycle_(current_task_cycle_value)
{
    current_task_cycle_active = true;
    current_task_cycle_value = cycle;
}

TaskCycleScope::~TaskCycleScope()
{
    current_task_cycle_active = previous_active_;
    current_task_cycle_value = previous_cycle_;
}

} // namespace xsim
