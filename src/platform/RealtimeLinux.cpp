#include "xsim/platform/Realtime.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
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

} // namespace xsim
