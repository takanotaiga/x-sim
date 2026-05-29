#pragma once

#include <cstdint>
#include <time.h>

namespace xsim {

struct TaskCompletion {
    uint64_t cycle{};
    timespec start_time{};
    timespec end_time{};
    long exec_ns{};
    long deadline_lateness_ns{};
    bool overran = false;
};

} // namespace xsim
