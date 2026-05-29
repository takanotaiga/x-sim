#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Clock.hpp"

#include <cerrno>
#include <cstring>

namespace xsim {

timespec add_ns(timespec t, long ns)
{
    t.tv_nsec += ns;

    while (t.tv_nsec >= NS_PER_SEC) {
        t.tv_nsec -= NS_PER_SEC;
        t.tv_sec += 1;
    }

    while (t.tv_nsec < 0) {
        t.tv_nsec += NS_PER_SEC;
        t.tv_sec -= 1;
    }

    return t;
}

long diff_ns(const timespec& a, const timespec& b)
{
    return (a.tv_sec - b.tv_sec) * NS_PER_SEC + (a.tv_nsec - b.tv_nsec);
}

timespec now_monotonic()
{
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t;
}

long long now_realtime_ms()
{
    timespec t{};
    clock_gettime(CLOCK_REALTIME, &t);
    return static_cast<long long>(t.tv_sec) * 1000 + t.tv_nsec / NS_PER_MS;
}

void sleep_until(const timespec& target)
{
    while (true) {
        int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, nullptr);

        if (ret == 0) {
            return;
        }

        if (ret != EINTR) {
            logging::cerr << "clock_nanosleep failed: " << strerror(ret) << logging::endl;
            return;
        }
    }
}

} // namespace xsim
