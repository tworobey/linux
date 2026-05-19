#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <string>

namespace logging {

namespace json = boost::json;

inline void InitLogging() {
    boost::log::add_console_log(
        std::cout,
        boost::log::keywords::auto_flush = true
    );
}

} // namespace logging
