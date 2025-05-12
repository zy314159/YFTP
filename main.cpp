#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <memory>

#include "ConfigManager.hpp"

namespace Yftp {
std::shared_ptr<spdlog::logger> g_console_logger;
std::shared_ptr<spdlog::logger> g_file_logger;
std::shared_ptr<spdlog::logger> g_async_logger;
const char* g_config_file = "/home/zhangyang/Yftp/config.json";

void initConfigSystem(){
    try{
        ConfigManager::getInstance().load(g_config_file);
    }catch(std::exception& e){
        std::cerr << "ConfigManager initialization failed: " << e.what() << std::endl;
    }
    return;
}

void initLoggerSystem() {
    try {
        // Initialize console logger
        g_console_logger = spdlog::stdout_color_mt("console");
        g_console_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        // Initialize file logger
        std::string log_file = ConfigManager::getInstance().get<std::string>("log_file", "logs/yftp.log");
        g_file_logger = spdlog::basic_logger_mt("file_logger", log_file);
        g_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        // Initialize asynchronous logger
        std::size_t max_queue_size = ConfigManager::getInstance().get<int>("max_asynclog_que_size", 8192);
        std::size_t thread_count = ConfigManager::getInstance().get<int>("max_asynclog_worker_thread", 1);
        std::string async_log_file = ConfigManager::getInstance().get<std::string>("async_log_file", "logs/async_yftp.log");
        spdlog::init_thread_pool(max_queue_size, thread_count);
        g_async_logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>(
            "async_logger", async_log_file, true);
        g_async_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        std::cout << "Logger system initialization sucess !" << std::endl;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
    return;
}



};  // namespace Yftp

int main() {
    Yftp::initConfigSystem();
    Yftp::initLoggerSystem();

    // Example usage of the logger
    //Yftp::g_console_logger->info("This is an info message:{}", __FILE__);
    //Yftp::g_file_logger->warn("This is a warning message");
    //Yftp::g_async_logger->error("This is an error message");
    //Yftp::g_async_logger->flush();
    //Yftp::g_console_logger->flush();
    //Yftp::g_file_logger->flush();
    return 0;
}