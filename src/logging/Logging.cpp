#include "xsim/logging/Logging.hpp"

#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

namespace xsim {
namespace logging {
namespace {

struct LogMessage {
    AsyncStream::Target target;
    Severity severity;
    std::string text;
};

struct ThreadBuffers {
    std::ostringstream stdout_buffer;
    std::ostringstream stderr_buffer;
};

thread_local ThreadBuffers thread_buffers;

class Logger {
public:
    ~Logger()
    {
        shutdown();
    }

    void enqueue(LogMessage message)
    {
        bool write_now = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (stopping_) {
                write_now = true;
            } else {
                start_worker_locked();
                queue_.push_back(std::move(message));
            }
        }

        if (write_now) {
            write_direct(message);
            return;
        }

        cv_.notify_one();
    }

    void flush()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!worker_started_) {
            return;
        }

        flushed_cv_.wait(lock, [this] {
            return queue_.empty() && !processing_;
        });
    }

    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }

        cv_.notify_one();

        if (worker_.joinable()) {
            worker_.join();
        }

        std::cout.flush();
        std::cerr.flush();
    }

private:
    void start_worker_locked()
    {
        if (worker_started_) {
            return;
        }

        worker_started_ = true;
        worker_ = std::thread(&Logger::run, this);
    }

    void run()
    {
        while (true) {
            LogMessage message;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] {
                    return stopping_ || !queue_.empty();
                });

                if (queue_.empty() && stopping_) {
                    flushed_cv_.notify_all();
                    return;
                }

                message = std::move(queue_.front());
                queue_.pop_front();
                processing_ = true;
            }

            write_direct(message);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                processing_ = false;

                if (queue_.empty()) {
                    flushed_cv_.notify_all();
                }
            }
        }
    }

    void write_direct(const LogMessage& message)
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        std::ostream& stream = message.target == AsyncStream::Target::Stdout ? std::cout : std::cerr;
        stream << message.text;
        stream.flush();
    }

    std::mutex mutex_;
    std::mutex write_mutex_;
    std::condition_variable cv_;
    std::condition_variable flushed_cv_;
    std::deque<LogMessage> queue_;
    std::thread worker_;
    bool worker_started_ = false;
    bool stopping_ = false;
    bool processing_ = false;
};

Logger& logger()
{
    static Logger instance;
    return instance;
}

AsyncStream::Target target_for_severity(Severity severity)
{
    return severity == Severity::Info ? AsyncStream::Target::Stdout : AsyncStream::Target::Stderr;
}

} // namespace

AsyncStream cout{AsyncStream::Target::Stdout, Severity::Info};
AsyncStream cerr{AsyncStream::Target::Stderr, Severity::Error};
const Endl endl{};

AsyncStream::AsyncStream(Target target, Severity severity)
    : target_(target),
      severity_(severity)
{
}

AsyncStream& AsyncStream::operator<<(Endl)
{
    std::ostringstream& stream = buffer();
    logger().enqueue(LogMessage{target_, severity_, stream.str() + '\n'});
    stream.str({});
    stream.clear();
    return *this;
}

std::ostringstream& AsyncStream::buffer() const
{
    if (target_ == Target::Stdout) {
        return thread_buffers.stdout_buffer;
    }

    return thread_buffers.stderr_buffer;
}

void log(Severity severity, std::string message)
{
    logger().enqueue(LogMessage{target_for_severity(severity), severity, std::move(message)});
}

void flush()
{
    logger().flush();
}

void shutdown()
{
    logger().shutdown();
}

} // namespace logging
} // namespace xsim
