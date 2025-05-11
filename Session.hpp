#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>

namespace Yftp{
    using tcp = boost::asio::ip::tcp;
    class Session : public std::enable_shared_from_this<Session>{
        public:
            Session(tcp::socket socket);

            tcp::socket& getSocket() {return this->socket_;}
        private:
            boost::asio::ip::tcp::socket socket_;
            boost::asio::io_context ioc_;
    };
};