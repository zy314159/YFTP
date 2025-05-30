#include "../include/Server.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>

#include "../include/AsioIOServicePool.hpp"
#include "../include/logger.hpp"

namespace Yftp {
Server::Server(boost::asio::io_context& ioc, short port)
    : port_(port),
      ioc_(ioc),
      acceptor_(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                    port)) {
    LOG_INFO("Server started on port: {}", port_);
    if (!acceptor_.is_open()) {
        LOG_ERROR("Failed to open acceptor on port: {}", port_);
        return;
    }
    startAccept();
}

Server::~Server() { LOG_INFO("Server stopped on port: {}", port_); }

void Server::startAccept() {
    auto& io_context = AsioIOServicePool::getInstance().getIOService();
    auto new_session = std::make_shared<Session>(io_context, this);
    acceptor_.async_accept(new_session->getSocket(),
                           std::bind(&Server::handleAccept, this, new_session,
                                     std::placeholders::_1));
}

void Server::handleAccept(Session::ptr session,
                          const boost::system::error_code& error) {
    if (!error) {
        LOG_INFO("Accepted new session from: {}",
                 session->getSocket().remote_endpoint().address().to_string());
        session->start();
        std::unique_lock<std::mutex> lock(session_mutex_);
        sessions_[session->getUUID()] = session;
    } else {
        LOG_ERROR("Failed to accept new session: {}", error.message());
    }
    startAccept();
    return;
}

void Server::clearSession(const std::string& uuid) {
    std::unique_lock<std::mutex> lock(session_mutex_);
    auto it = sessions_.find(uuid);
    if (it != sessions_.end()) {
        sessions_.erase(it);
        LOG_INFO("Session {} cleared", uuid);
    }
}

}  // namespace Yftp