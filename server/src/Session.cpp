#include "../include/Session.hpp"

#include <jsoncpp/json/json.h>

#include <iostream>
#include <sstream>

#include "../include/LogicSystem.hpp"
#include "../include/Server.hpp"
#include "../include/logger.hpp"

namespace Yftp {

Session::Session(boost::asio::io_context& io_context, Server* server)
    : socket_(io_context),
      server_(server),
      b_close_(false),
      b_head_parse_(false) {
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    uuid_ = boost::uuids::to_string(a_uuid);
    recv_head_node_ = make_shared<MsgNode>(HEAD_TOTAL_LEN);

    const char* home_path = std::getenv("HOME");
    if(home_path){
        user_current_path_ = std::filesystem::path(home_path);
    } else {
        LOG_ERROR("HOME environment variable not set");
        user_current_path_ = std::filesystem::current_path();
    }
}
Session::~Session() { LOG_INFO("~Session destruct"); }

tcp::socket& Session::getSocket() { return socket_; }

std::string& Session::getUUID() { return uuid_; }

void Session::start() {
    ::memset(data_, 0, MAX_LENGTH);
    socket_.async_read_some(
        boost::asio::buffer(data_, MAX_LENGTH),
        std::bind(&Session::handleRead, this, std::placeholders::_1,
                  std::placeholders::_2, sharedSelf()));
}

void Session::send(std::string msg, short msgid) {
    std::lock_guard<std::mutex> lock(send_lock_);
    int send_que_size = send_que_.size();
    if (send_que_size > MAX_SENDQUE) {
        LOG_WARN("session: {} send queue full, size is {}", uuid_, MAX_SENDQUE);
        return;
    }

    send_que_.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = send_que_.front();
    boost::asio::async_write(
        socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
        std::bind(&Session::handleWrite, this, std::placeholders::_1,
                  sharedSelf()));
}

void Session::send(char* msg, short max_length, short msgid) {
    std::lock_guard<std::mutex> lock(send_lock_);
    int send_que_size = send_que_.size();
    if (send_que_size > MAX_SENDQUE) {
        LOG_WARN("session: {} send queue full, size is {}", uuid_, MAX_SENDQUE);
        return;
    }

    send_que_.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = send_que_.front();
    boost::asio::async_write(
        socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
        std::bind(&Session::handleWrite, this, std::placeholders::_1,
                  sharedSelf()));
}

void Session::close() {
    socket_.close();
    b_close_ = true;
    return;
}

std::shared_ptr<Session> Session::sharedSelf() { return shared_from_this(); }

void Session::handleWrite(const boost::system::error_code& error,
                          std::shared_ptr<Session> shared_self) {
    try {
        if (!error) {
            std::lock_guard<std::mutex> lock(send_lock_);
            send_que_.pop();
            if (!send_que_.empty()) {
                auto& msgnode = send_que_.front();
                boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(msgnode->data_, msgnode->total_len_),
                    std::bind(&Session::handleWrite, this,
                              std::placeholders::_1, shared_self));
            }
        } else {
            LOG_ERROR("handle write failed, error is {}", error.message());
            close();
            server_->clearSession(uuid_);
        }
    } catch (std::exception& e) {
        LOG_ERROR("Exception code : {}", e.what());
    }
}

void Session::handleRead(const boost::system::error_code& error,
                         size_t bytes_transferred,
                         std::shared_ptr<Session> shared_self) {
    try {
        if (!error) {
            int copy_len = 0;
            while (bytes_transferred > 0) {
                if (!b_head_parse_) {
                    if (bytes_transferred + recv_head_node_->cur_len_ <
                        HEAD_TOTAL_LEN) {
                        memcpy(
                            recv_head_node_->data_ + recv_head_node_->cur_len_,
                            data_ + copy_len, bytes_transferred);
                        recv_head_node_->cur_len_ += bytes_transferred;
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    int head_remain =
                        HEAD_TOTAL_LEN - recv_head_node_->cur_len_;
                    memcpy(recv_head_node_->data_ + recv_head_node_->cur_len_,
                           data_ + copy_len, head_remain);
                    copy_len += head_remain;
                    bytes_transferred -= head_remain;
                    short msg_id = 0;
                    memcpy(&msg_id, recv_head_node_->data_, HEAD_ID_LEN);
                    msg_id =
                        boost::asio::detail::socket_ops::network_to_host_short(
                            msg_id);
                    LOG_DEBUG("msg_id is {}", msg_id);
                    if (msg_id > MAX_LENGTH) {
                        LOG_ERROR("invalid msg_id is {}", msg_id);
                        server_->clearSession(uuid_);
                        return;
                    }
                    short msg_len = 0;
                    memcpy(&msg_len, recv_head_node_->data_ + HEAD_ID_LEN,
                           HEAD_DATA_LEN);
                    msg_len =
                        boost::asio::detail::socket_ops::network_to_host_short(
                            msg_len);
                    LOG_DEBUG("msg_len is {}", msg_len);
                    if (msg_len > MAX_LENGTH) {
                        LOG_ERROR("invalid data length is {}", msg_len);
                        server_->clearSession(uuid_);
                        return;
                    }

                    recv_msg_node_ = make_shared<RecvNode>(msg_len, msg_id);

                    if (bytes_transferred < msg_len) {
                        memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_,
                               data_ + copy_len, bytes_transferred);
                        recv_msg_node_->cur_len_ += bytes_transferred;
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        b_head_parse_ = true;
                        return;
                    }

                    memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_,
                           data_ + copy_len, msg_len);
                    recv_msg_node_->cur_len_ += msg_len;
                    copy_len += msg_len;
                    bytes_transferred -= msg_len;
                    recv_msg_node_->data_[recv_msg_node_->total_len_] = '\0';
                    LogicSystem::getInstance().postMsgToQue(
                        make_shared<LogicNode>(shared_from_this(),
                                               recv_msg_node_));

                    b_head_parse_ = false;
                    recv_head_node_->clear();
                    if (bytes_transferred <= 0) {
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&Session::handleRead, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    continue;
                }

                int remain_msg =
                    recv_msg_node_->total_len_ - recv_msg_node_->cur_len_;
                if (bytes_transferred < remain_msg) {
                    memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_,
                           data_ + copy_len, bytes_transferred);
                    recv_msg_node_->cur_len_ += bytes_transferred;
                    ::memset(data_, 0, MAX_LENGTH);
                    socket_.async_read_some(
                        boost::asio::buffer(data_, MAX_LENGTH),
                        std::bind(&Session::handleRead, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  shared_self));
                    return;
                }
                memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_,
                       data_ + copy_len, remain_msg);
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
                        std::bind(&Session::handleRead, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  shared_self));
                    return;
                }
                continue;
            }
        } else {
            LOG_ERROR("handle read failed, error is {}", error.message());
            close();
            server_->clearSession(uuid_);
        }
    } catch (std::exception& e) {
        LOG_ERROR("Exception code is {}", e.what());
    }
}

std::string Session::getRemoteIp() {
    return socket_.remote_endpoint().address().to_string();
}

std::string Session::resolvePath(const std::string& path) {
    fs::path fs_path = path;
    if (fs_path.is_absolute()) {
        return fs::weakly_canonical(fs_path).string();
    }

    fs::path user_path = user_current_path_ / fs_path;

    std::vector<std::string> path_parts;
    for (const auto& part : user_path) {
        if (part == ".") {
            continue;
        } else if (part == "..") {
            if (!path_parts.empty()) {
                path_parts.pop_back();
            }
        } else {
            path_parts.push_back(part);
        }
    }

    fs::path result;
    for (const auto& part : path_parts) {
        result /= part;
    }

    return fs::weakly_canonical(result).string();
}

LogicNode::LogicNode(shared_ptr<Session> session, shared_ptr<RecvNode> recvnode)
    : session_(session), recvnode_(recvnode) {}

}  // namespace Yftp
