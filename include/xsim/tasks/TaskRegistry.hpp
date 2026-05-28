#pragma once

#include "xsim/scheduler/Task.hpp"

#include <mutex>
#include <vector>

namespace xsim {
namespace sample_apps {

std::vector<Task> create_tasks(std::mutex& output_mutex);

} // namespace sample_apps
} // namespace xsim
