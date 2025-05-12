#include "Session.hpp"

#include <memory>

#include "logger.hpp"

namespace Yftp {
Session::Session(tcp::socket socket, std::shared_ptr<Server> server)
    : socket_(std::move(socket)), server_(server) {
    uuid_ = boost::uuids::to_string(boost::uuids::random_generator()());
    LOG_INFO_CONSOLE("Session created with UUID: {}", uuid_);
    LOG_INFO_ASYNC("Session created with UUID: {}", uuid_);
}

void Session::run() {
    auto self{shared_from_this()};
    
}

void Session::sendResponse(const char* response, size_t length) {
    boost::asio::async_write(
        socket_, boost::asio::buffer(response, length),
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                LOG_INFO_CONSOLE("Sent {} bytes to client", bytes_transferred);
                LOG_INFO_ASYNC("Sent {} bytes to client", bytes_transferred);
            } else {
                LOG_ERROR_CONSOLE("Failed to send response: {}", error.message());
                LOG_ERROR_ASYNC("Failed to send response: {}", error.message());
            }
        });
    return;
}

void Session::sendResponse(const std::string& response) {
    sendResponse(response.c_str(), response.size());
    return;
}

}  // namespace Yftp