#include <streaming_protocol/Logging.hpp>
#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/logger.h>

namespace daq::streaming_protocol
{
    LogCallback Logging::logCallback()
    {
        const std::string name = "bbstreaming";
        static auto logger =
            std::make_shared<spdlog::logger>(name, std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        return [](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
            logger->log(location, level, msg);
        };
    }
}
