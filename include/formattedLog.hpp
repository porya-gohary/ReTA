#ifndef FORMATTEDLOG_HPP
#define FORMATTEDLOG_HPP

#include <sstream>
#include <boost/format.hpp>
#include <iostream>
#include <iomanip>
#include <mutex>

enum log_level_t {
    LOG_NOTHING,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};


namespace log_impl {
    const char *levelNames[] = {"", "[CRITICAL]", "[ERROR]", "[WARNING]", "[INFO]", "[DEBUG]"};
    int GLOBAL_LEVEL = 2;
    std::mutex mutex;

    class formattedLog {
    public:
        formattedLog(log_level_t level, const std::string msg) : fmt(msg), level(level) { }

        ~formattedLog() {
            std::lock_guard<std::mutex> guard(mutex);
            if (level <= GLOBAL_LEVEL)
                std::cout << std::left << std::setw(10) << levelNames[level] << " " << fmt << std::endl;
        }

        template<typename T>
        formattedLog &operator%(T value) {
            fmt % value;
            return *this;
        }

    protected:
        log_level_t level;
        boost::format fmt;

    };
}//namespace log_impl
// Helper function. Class formatted_log_t will not be used directly.

template<log_level_t level>
log_impl::formattedLog log(const std::string msg) {
    return log_impl::formattedLog(level, msg);
}

#endif