#pragma once

#include <sstream>
#include <string>

namespace xsim {
namespace logging {

enum class Severity {
    Info,
    Warning,
    Error,
};

struct Endl {
};

class AsyncStream {
public:
    enum class Target {
        Stdout,
        Stderr,
    };

    AsyncStream(Target target, Severity severity);

    template <typename T>
    AsyncStream& operator<<(const T& value)
    {
        buffer() << value;
        return *this;
    }

    AsyncStream& operator<<(Endl);

private:
    std::ostringstream& buffer() const;

    Target target_;
    Severity severity_;
};

extern AsyncStream cout;
extern AsyncStream cerr;
extern const Endl endl;

void log(Severity severity, std::string message);
void flush();
void shutdown();

} // namespace logging
} // namespace xsim
