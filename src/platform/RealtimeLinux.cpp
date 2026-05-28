#include "xsim/platform/Realtime.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

namespace xsim {

bool lock_memory()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "mlockall failed: " << strerror(errno) << "\n";
        return false;
    }

    return true;
}

bool set_realtime_priority(int priority)
{
    sched_param param{};
    param.sched_priority = priority;

    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        std::cerr << "pthread_setschedparam failed: "
                  << strerror(errno)
                  << "\n";
        return false;
    }

    return true;
}

} // namespace xsim
