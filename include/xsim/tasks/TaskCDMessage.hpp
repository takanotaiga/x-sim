#pragma once

#include "xsim/shared/SharedValue.hpp"

#include <cstdint>

namespace xsim {
namespace sample_apps {

struct TaskCDMessage {
    uint64_t produced_cycle{};
    uint64_t value{};
    long long realtime_ms{};
};

using TaskCDChannel = SharedValue<TaskCDMessage>;
using TaskCDReader = TaskCDChannel::Reader;
using TaskCDWriter = TaskCDChannel::Writer;

} // namespace sample_apps
} // namespace xsim
