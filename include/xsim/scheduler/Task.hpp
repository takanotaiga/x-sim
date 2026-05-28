#pragma once

#include <string>
#include <utility>

namespace xsim {

class Task {
public:
    virtual ~Task() = default;

    const std::string& name() const
    {
        return name_;
    }

    long offset_ns() const
    {
        return offset_ns_;
    }

    long wcet_ns() const
    {
        return wcet_ns_;
    }

    virtual void initialize()
    {
    }

    virtual void tick() = 0;

    virtual void finalize()
    {
    }

protected:
    Task(std::string name, long offset_ns, long wcet_ns)
        : name_(std::move(name)),
          offset_ns_(offset_ns),
          wcet_ns_(wcet_ns)
    {
    }

private:
    std::string name_;
    long offset_ns_;
    long wcet_ns_;
};

} // namespace xsim
