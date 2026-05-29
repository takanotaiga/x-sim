#include "xsim/logging/Logging.hpp"
#include "xsim/platform/Realtime.hpp"

#include <cerrno>
#include <cstring>
#include <sys/mman.h>

namespace xsim {

bool lock_memory()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        logging::cerr << "mlockall failed: " << strerror(errno) << logging::endl;
        return false;
    }

    return true;
}

} // namespace xsim
