#pragma once

#include <spdlog/logger.h>

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>

namespace Yftp {
using tcp = boost::asio::ip::tcp;
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_async_logger;

class Server;
class Session : public std::enable_shared_from_this<Session> {
public:
    using ptr = std::shared_ptr<Session>;

    Session(tcp::socket socket, std::shared_ptr<Server> server);

    std::string getUUID() const {
        return this->uuid_;
    }

    tcp::socket& getSocket() {
        return this->socket_;
    }

    void run();

private:
    void sendResponse(const char* response, size_t length);
    void sendResponse(const std::string& response);

    void doRead();
    void handleRead(const boost::system::error_code& error, std::size_t bytes_transferred);
    void processCommand(const std::string& cmd_line);

    void handleMkdir(const std::vector<std::string>& args);
    void handleRmdir(const std::vector<std::string>& args);
    void handleList(const std::vector<std::string>& args);
    void handleDownload(const std::vector<std::string>& args);
    void handleUpload(const std::vector<std::string>& args);


private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_context ioc_;
    std::string uuid_;
    std::shared_ptr<Server> server_;
    std::unordered_map<std::string, std::function<void(Session&, const std::vector<std::string>&)>> command_handlers_;
    std::string download_buffer_;
    std::string upload_buffer_;
    boost::asio::streambuf input_buffer_;
};
};  // namespace Yftp