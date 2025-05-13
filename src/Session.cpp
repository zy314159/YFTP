#include "Session.hpp"

#include <jsoncpp/json/json.h>

#include <iostream>
#include <sstream>

#include "../include/LogicSystem.hpp"
#include "../include/Server.hpp"

namespace Yftp {

Session::Session(boost::asio::io_context& io_context, Server* server)
    : socket_(io_context), server_(server), b_close_(false), b_head_parse_(false) {
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    uuid_ = boost::uuids::to_string(a_uuid);
    recv_head_node_ = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}
Session::~Session() {
    std::cout << "~Session destruct" << std::endl;
}

tcp::socket& Session::getSocket() {
    return socket_;
}

std::string& Session::getUUID() {
    return uuid_;
}

void Session::start() {
    ::memset(data_, 0, MAX_LENGTH);
    socket_.async_read_some(boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this, std::placeholders::_1,
                                      std::placeholders::_2, sharedSelf()));
}

void Session::send(std::string msg, short msgid) {
    std::lock_guard<std::mutex> lock(send_lock_);
    int send_que_size = send_que_.size();
    if (send_que_size > MAX_SENDQUE) {
        std::cout << "session: " << uuid_ << " send queue full, size is " << MAX_SENDQUE
                  << std::endl;
        return;
    }

    send_que_.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = send_que_.front();
    boost::asio::async_write(
        socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
        std::bind(&Session::handleWrite, this, std::placeholders::_1, sharedSelf()));
}

void Session::send(char* msg, short max_length, short msgid) {
    std::lock_guard<std::mutex> lock(send_lock_);
    int send_que_size = send_que_.size();
    if (send_que_size > MAX_SENDQUE) {
        std::cout << "session: " << uuid_ << " send queue full, size is " << MAX_SENDQUE
                  << std::endl;
        return;
    }

    send_que_.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = send_que_.front();
    boost::asio::async_write(
        socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
        std::bind(&Session::handleWrite, this, std::placeholders::_1, sharedSelf()));
}

void Session::close() {
    socket_.close();
    b_close_ = true;
}

std::shared_ptr<Session> Session::sharedSelf() {
    return shared_from_this();
}

void Session::handleWrite(const boost::system::error_code& error,
                          std::shared_ptr<Session> shared_self) {
    try {
        if (!error) {
            std::lock_guard<std::mutex> lock(send_lock_);
            send_que_.pop();
            if (!send_que_.empty()) {
                auto& msgnode = send_que_.front();
                boost::asio::async_write(
                    socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
                    std::bind(&Session::handleWrite, this, std::placeholders::_1, shared_self));
            }
        } else {
            std::cout << "handle write failed, error is " << error.what() << std::endl;
            close();
            server_->clearSession(uuid_);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception code : " << e.what() << std::endl;
    }
}

void Session::handleRead(const boost::system::error_code& error, size_t bytes_transferred,
                         std::shared_ptr<Session> shared_self) {
    try {
        if (!error) {
            int copy_len = 0;
            while (bytes_transferred > 0) {
                if (!b_head_parse_) {
                    if (bytes_transferred + recv_head_node_->cur_len_ < HEAD_TOTAL_LEN) {
                        memcpy(recv_head_node_->data_ + recv_head_node_->cur_len_, data_ + copy_len,
                               bytes_transferred);
                        recv_head_node_->cur_len_ += bytes_transferred;
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this, std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    int head_remain = HEAD_TOTAL_LEN - recv_head_node_->cur_len_;
                    memcpy(recv_head_node_->data_ + recv_head_node_->cur_len_, data_ + copy_len,
                           head_remain);
                    copy_len += head_remain;
                    bytes_transferred -= head_remain;
                    short msg_id = 0;
                    memcpy(&msg_id, recv_head_node_->data_, HEAD_ID_LEN);
                    msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                    std::cout << "msg_id is " << msg_id << std::endl;
                    if (msg_id > MAX_LENGTH) {
                        std::cout << "invalid msg_id is " << msg_id << std::endl;
                        server_->clearSession(uuid_);
                        return;
                    }
                    short msg_len = 0;
                    memcpy(&msg_len, recv_head_node_->data_ + HEAD_ID_LEN, HEAD_DATA_LEN);
                    msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
                    std::cout << "msg_len is " << msg_len << std::endl;
                    if (msg_len > MAX_LENGTH) {
                        std::cout << "invalid data length is " << msg_len << std::endl;
                        server_->clearSession(uuid_);
                        return;
                    }

                    recv_msg_node_ = make_shared<RecvNode>(msg_len, msg_id);

                    if (bytes_transferred < msg_len) {
                        memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_, data_ + copy_len,
                               bytes_transferred);
                        recv_msg_node_->cur_len_ += bytes_transferred;
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this, std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        b_head_parse_ = true;
                        return;
                    }

                    memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_, data_ + copy_len,
                           msg_len);
                    recv_msg_node_->cur_len_ += msg_len;
                    copy_len += msg_len;
                    bytes_transferred -= msg_len;
                    recv_msg_node_->data_[recv_msg_node_->total_len_] = '\0';
                    LogicSystem::getInstance().postMsgToQue(
                        make_shared<LogicNode>(shared_from_this(), recv_msg_node_));

                    b_head_parse_ = false;
                    recv_head_node_->clear();
                    if (bytes_transferred <= 0) {
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this, std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    continue;
                }

                int remain_msg = recv_msg_node_->total_len_ - recv_msg_node_->cur_len_;
                if (bytes_transferred < remain_msg) {
                    memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_, data_ + copy_len,
                           bytes_transferred);
                    recv_msg_node_->cur_len_ += bytes_transferred;
                    ::memset(data_, 0, MAX_LENGTH);
                    socket_.async_read_some(
                        boost::asio::buffer(data_, MAX_LENGTH),
                        std::bind(&Session::handleRead, this, std::placeholders::_1,
                                  std::placeholders::_2, shared_self));
                    return;
                }
                memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_, data_ + copy_len,
                       remain_msg);
                recv_msg_node_->cur_len_ += remain_msg;
                bytes_transferred -= remain_msg;
                copy_len += remain_msg;
                recv_msg_node_->data_[recv_msg_node_->total_len_] = '\0';
                LogicSystem::getInstance().postMsgToQue(
                    make_shared<LogicNode>(shared_from_this(), recv_msg_node_));

                b_head_parse_ = false;
                recv_head_node_->clear();
                if (bytes_transferred <= 0) {
                    ::memset(data_, 0, MAX_LENGTH);
                    socket_.async_read_some(
                        boost::asio::buffer(data_, MAX_LENGTH),
                        std::bind(&Session::handleRead, this, std::placeholders::_1,
                                  std::placeholders::_2, shared_self));
                    return;
                }
                continue;
            }
        } else {
            std::cout << "handle read failed, error is " << error.what() << std::endl;
            close();
            server_->clearSession(uuid_);
        }
    } catch (std::exception& e) {
        std::cout << "Exception code is " << e.what() << std::endl;
    }
}

LogicNode::LogicNode(shared_ptr<Session> session, shared_ptr<RecvNode> recvnode)
    : session_(session), recvnode_(recvnode) {
}

}  // namespace Yftp
