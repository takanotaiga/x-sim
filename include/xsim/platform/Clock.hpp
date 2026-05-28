#pragma once

#include <time.h>

namespace xsim {

inline constexpr long NS_PER_MS = 1'000'000L;
inline constexpr long NS_PER_SEC = 1'000'000'000L;
inline constexpr long DEFAULT_MAJOR_CYCLE_NS = 1000 * NS_PER_MS;

timespec add_ns(timespec t, long ns);
long diff_ns(const timespec& a, const timespec& b);
timespec now_monotonic();
long long now_realtime_ms();
void sleep_until(const timespec& target);

} // namespace xsim
