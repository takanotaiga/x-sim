#pragma once

#include <functional>
#include <string>

namespace xsim {

struct Task {
    std::string name;
    long offset_ns;
    long wcet_ns;
    std::function<void()> func;
};

} // namespace xsim
