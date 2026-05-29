#pragma once

namespace xsim {

enum class WcetViolationPolicy {
    Log,
    Terminate,
};

constexpr const char* to_string(WcetViolationPolicy policy)
{
    switch (policy) {
    case WcetViolationPolicy::Log:
        return "log";
    case WcetViolationPolicy::Terminate:
        return "terminate";
    }

    return "unknown";
}

} // namespace xsim
