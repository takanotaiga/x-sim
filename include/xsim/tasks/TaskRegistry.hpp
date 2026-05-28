#pragma once

#include "xsim/scheduler/Task.hpp"

#include <mutex>
#include <vector>

namespace xsim {

std::vector<Task> create_tasks(std::mutex& output_mutex);

} // namespace xsim
