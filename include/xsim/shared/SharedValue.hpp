#pragma once

#include "xsim/scheduler/TaskCycleContext.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <utility>

namespace xsim {

template <typename T>
class SharedValue {
    struct State;

public:
    struct Snapshot {
        std::optional<T> value;
        uint64_t cycle{};
        uint64_t sequence{};

        bool has_value() const
        {
            return value.has_value();
        }
    };

    class Reader {
    public:
        Snapshot read() const
        {
            std::shared_lock<std::shared_mutex> lock(state_->value_mutex);
            return Snapshot{
                state_->value,
                state_->cycle,
                state_->sequence,
            };
        }

    private:
        friend class SharedValue;

        explicit Reader(std::shared_ptr<State> state)
            : state_(std::move(state))
        {
        }

        std::shared_ptr<State> state_;
    };

    class Writer {
    public:
        Writer(const Writer&) = delete;
        Writer& operator=(const Writer&) = delete;
        Writer(Writer&&) noexcept = default;
        Writer& operator=(Writer&&) noexcept = default;

        void write(T value)
        {
            const uint64_t cycle = current_task_cycle();
            std::unique_lock<std::shared_mutex> lock(state_->value_mutex);
            state_->value = std::move(value);
            state_->cycle = cycle;
            ++state_->sequence;
        }

    private:
        friend class SharedValue;

        explicit Writer(std::shared_ptr<State> state)
            : state_(std::move(state))
        {
        }

        std::shared_ptr<State> state_;
    };

    SharedValue()
        : state_(std::make_shared<State>())
    {
    }

    Writer writer()
    {
        std::lock_guard<std::mutex> lock(state_->writer_mutex);

        if (state_->writer_taken) {
            throw std::logic_error("SharedValue writer already taken");
        }

        state_->writer_taken = true;
        return Writer(state_);
    }

    Reader reader() const
    {
        return Reader(state_);
    }

private:
    struct State {
        mutable std::shared_mutex value_mutex;
        std::optional<T> value;
        uint64_t cycle{};
        uint64_t sequence{};
        std::mutex writer_mutex;
        bool writer_taken = false;
    };

    std::shared_ptr<State> state_;
};

} // namespace xsim
