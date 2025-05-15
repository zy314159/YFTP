#pragma once
#include <spdlog/logger.h>

#include <memory>

namespace Yftp {
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;
extern std::shared_ptr<spdlog::logger> g_async_logger;

#define LOG_INFO_CONSOLE(...)                            \
    g_console_logger->info(__FILE__ + std::string(":") + \
                           std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_INFO_FILE(...)                            \
    g_file_logger->info(__FILE__ + std::string(":") + \
                        std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_INFO_ASYNC(...)                            \
    g_async_logger->info(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_ERROR_CONSOLE(...)                            \
    g_console_logger->error(__FILE__ + std::string(":") + \
                            std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_ERROR_FILE(...)                            \
    g_file_logger->error(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_ERROR_ASYNC(...)                            \
    g_async_logger->error(__FILE__ + std::string(":") + \
                          std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_DEBUG_CONSOLE(...)                            \
    g_console_logger->debug(__FILE__ + std::string(":") + \
                            std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_DEBUG_FILE(...)                            \
    g_file_logger->debug(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_DEBUG_ASYNC(...)                            \
    g_async_logger->debug(__FILE__ + std::string(":") + \
                          std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_WARN_CONSOLE(...)                            \
    g_console_logger->warn(__FILE__ + std::string(":") + \
                           std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_WARN_FILE(...)                            \
    g_file_logger->warn(__FILE__ + std::string(":") + \
                        std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_WARN_ASYNC(...)                            \
    g_async_logger->warn(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_CRITICAL_CONSOLE(...)                            \
    g_console_logger->critical(__FILE__ + std::string(":") + \
                               std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_CRITICAL_FILE(...)                            \
    g_file_logger->critical(__FILE__ + std::string(":") + \
                            std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_CRITICAL_ASYNC(...)                            \
    g_async_logger->critical(__FILE__ + std::string(":") + \
                             std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_TRACE_CONSOLE(...)                            \
    g_console_logger->trace(__FILE__ + std::string(":") + \
                            std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_TRACE_FILE(...)                            \
    g_file_logger->trace(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_TRACE_ASYNC(...)                            \
    g_async_logger->trace(__FILE__ + std::string(":") + \
                          std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_FATAL_CONSOLE(...)                            \
    g_console_logger->fatal(__FILE__ + std::string(":") + \
                            std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_FATAL_FILE(...)                            \
    g_file_logger->fatal(__FILE__ + std::string(":") + \
                         std::to_string(__LINE__) + " " + __VA_ARGS__)
#define LOG_FATAL_ASYNC(...)                            \
    g_async_logger->fatal(__FILE__ + std::string(":") + \
                          std::to_string(__LINE__) + " " + __VA_ARGS__)

#define LOG_INFO(...)                  \
    {                                  \
        LOG_INFO_CONSOLE(__VA_ARGS__); \
        LOG_INFO_ASYNC(__VA_ARGS__);   \
    }

#define LOG_ERROR(...)                  \
    {                                   \
        LOG_ERROR_CONSOLE(__VA_ARGS__); \
        LOG_ERROR_ASYNC(__VA_ARGS__);   \
    }

#define LOG_DEBUG(...)                  \
    {                                   \
        LOG_DEBUG_CONSOLE(__VA_ARGS__); \
        LOG_DEBUG_ASYNC(__VA_ARGS__);   \
    }

#define LOG_WARN(...)                  \
    {                                  \
        LOG_WARN_CONSOLE(__VA_ARGS__); \
        LOG_WARN_ASYNC(__VA_ARGS__);   \
    }

#define LOG_CRITICAL(...)                  \
    {                                      \
        LOG_CRITICAL_CONSOLE(__VA_ARGS__); \
        LOG_CRITICAL_ASYNC(__VA_ARGS__);   \
    }

#define LOG_TRACE(...)                  \
    {                                   \
        LOG_TRACE_CONSOLE(__VA_ARGS__); \
        LOG_TRACE_ASYNC(__VA_ARGS__);   \
    }

#define LOG_FATAL(...)                  \
    {                                   \
        LOG_FATAL_CONSOLE(__VA_ARGS__); \
        LOG_FATAL_ASYNC(__VA_ARGS__);   \
    }
};  // namespace Yftp