#pragma once

#include <atomic>
#include <cstdint>

namespace xsim {

struct RuntimeStats {
    std::atomic<uint64_t> cycle_count{0};
    std::atomic<uint64_t> overrun_count{0};
    std::atomic<uint64_t> late_start_count{0};
    std::atomic<uint64_t> cycle_skip_count{0};
    std::atomic<uint64_t> dispatch_miss_count{0};
};

} // namespace xsim
