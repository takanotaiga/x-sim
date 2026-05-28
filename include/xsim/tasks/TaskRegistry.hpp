#pragma once

#include "xsim/scheduler/Task.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace xsim {
namespace sample_apps {

std::vector<std::unique_ptr<Task>> create_tasks(std::mutex& output_mutex);

} // namespace sample_apps
} // namespace xsim
