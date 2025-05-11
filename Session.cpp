#include "Session.hpp"

namespace  Yftp{
    Session::Session(tcp::socket socket):socket_(std::move(socket)){}

    
}