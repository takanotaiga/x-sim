#pragma once

#include <string>

namespace xsim {

struct TaskDependency {
    std::string dependent;
    std::string prerequisite;
};

} // namespace xsim
