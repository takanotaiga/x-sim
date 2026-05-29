#pragma once

#include "xsim/scheduler/Task.hpp"

#include <memory>
#include <vector>

namespace xsim {
namespace sample_apps {

std::vector<std::unique_ptr<Task>> create_tasks();

} // namespace sample_apps
} // namespace xsim
