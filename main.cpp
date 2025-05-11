#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <memory>

namespace Yftp {
std::shared_ptr<spdlog::logger> g_console_logger;
std::shared_ptr<spdlog::logger> g_file_logger;
std::shared_ptr<spdlog::logger> g_async_logger;

void initLoggerSystem() {
    try {
        // Initialize console logger
        g_console_logger = spdlog::stdout_color_mt("console");
        g_console_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        // Initialize file logger
        g_file_logger = spdlog::basic_logger_mt("file_logger", "logs/yftp.log");
        g_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        // Initialize asynchronous logger
        spdlog::init_thread_pool(8192, 1);
        g_async_logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>(
            "async_logger", "logs/async_yftp.log", true);
        g_async_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        std::cout << "Logger system initialization sucess !" << std::endl;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

int main() {
    return 0;
}

};  // namespace Yftp