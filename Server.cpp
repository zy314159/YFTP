#include "Server.hpp"

#include <boost/asio/io_context.hpp>
#include <functional>

#include "logger.hpp"

namespace Yftp {
Server::Server(boost::asio::io_context& ioc, short port) : port_(port), ioc_(ioc), acceptor_(ioc) {
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    boost::system::error_code ec;
    if (!acceptor_.open(endpoint.protocol(), ec)) {
        LOG_ERROR_CONSOLE("Failed to open acceptor: {}", ec.message());
        LOG_ERROR_ASYNC("Failed to open acceptor: {}", ec.message());
        return;
    }

    if (!acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec)) {
        LOG_ERROR_CONSOLE("Failed to set reuse address option: {}", ec.message());
        LOG_ERROR_ASYNC("Failed to set reuse address option: {}", ec.message());
        return;
    }

    if (!acceptor_.bind(endpoint, ec)) {
        LOG_ERROR_CONSOLE("Failed to bind acceptor: {}", ec.message());
        LOG_ERROR_ASYNC("Failed to bind acceptor: {}", ec.message());
        return;
    }

    acceptor_.listen();
    LOG_INFO_CONSOLE("Server started on port: {}", port_);
    LOG_INFO_ASYNC("Server started on port: {}", port_);

    startAccept();
}

Server::~Server() {
    acceptor_.close();
    LOG_INFO_CONSOLE("Server stopped on port: {}", port_);
    LOG_INFO_ASYNC("Server stopped on port: {}", port_);
}

void Server::startAccept() {
    auto new_session = std::make_shared<Session>(ioc_);
    acceptor_.async_accept(new_session->getSocket(), std::bind(&Server::handleAccept, this,
                                                               new_session, std::placeholders::_1));
    return;
}

void Server::handleAccept(Session::ptr session, const boost::system::error_code& error){
    if(!error){
        LOG_INFO_CONSOLE("Accepted new session from: {}", session->getSocket().remote_endpoint().address().to_string());
        LOG_INFO_ASYNC("Accepted new session from: {}", session->getSocket().remote_endpoint().address().to_string());
        std::unique_lock<std::mutex> lock(session_mutex_);
        sessions_[session->getUUID()] = session;
        lock.unlock();
        session->run();
        startAccept();
    }else{
        LOG_ERROR_CONSOLE("Failed to accept new session: {}", error.message());
        LOG_ERROR_ASYNC("Failed to accept new session: {}", error.message());
    }
    return;
}

}  // namespace Yftp