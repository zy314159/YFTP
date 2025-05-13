#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <unordered_map>

#include "Session.hpp"

namespace Yftp {
class Server {
public:
    friend class Session;
    using ptr = std::shared_ptr<Server>;
    Server(boost::asio::io_context& ioc, short port);
    ~Server();
    void clearSession(const std::string& uuid);

private:
    void startAccept();
    void handleAccept(Session::ptr session, const boost::system::error_code& error);

private:
    short port_;
    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unordered_map<std::string, Session::ptr> sessions_;
    std::mutex session_mutex_;
};
};  // namespace Yftp