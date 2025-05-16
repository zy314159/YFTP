#pragma once
#include "MsgNode.hpp"
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>

using boost::asio::ip::tcp;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
   public:
    friend class YFTPClient;
    using ptr = std::shared_ptr<ClientSession>;

    ClientSession(boost::asio::io_context& io_context)
        : socket_(io_context), b_close_(false), b_head_parse_(false), connected_(false){
        boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
        uuid_ = boost::uuids::to_string(a_uuid);
        recv_head_node_ = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    }

    ~ClientSession() { close(); }

    tcp::socket& getSocket() { return socket_; }

    std::string& getUUID() { return uuid_; }

    bool isConnected() const { return connected_; }

    void connect(const std::string& host, short port) {
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(host),
                               port);
        socket_.async_connect(
            endpoint, [this](const boost::system::error_code& error) {
                if (!error) {
                    std::cout << "Connected to server successfully!"
                              << std::endl;
                    connected_ = true;
                    start();
                } else {
                    std::cerr << "Connection failed: " << error.message()
                              << std::endl;
                }
            });
    }

    void start() {
        ::memset(data_, 0, MAX_LENGTH);
        socket_.async_read_some(
            boost::asio::buffer(data_, MAX_LENGTH),
            std::bind(&ClientSession::handleRead, this, std::placeholders::_1,
                      std::placeholders::_2, shared_from_this()));
    }

    void send(std::string msg, short msgid) {
        std::lock_guard<std::mutex> lock(send_lock_);
        int send_que_size = send_que_.size();
        if (send_que_size > MAX_SENDQUE) {
            std::cout << "session: " << uuid_ << " send queue full, size is "
                      << MAX_SENDQUE << std::endl;
            return;
        }

        send_que_.push(
            std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
        if (send_que_size > 0) {
            return;
        }
        auto& msgnode = send_que_.front();
        boost::asio::async_write(
            socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
            std::bind(&ClientSession::handleWrite, this, std::placeholders::_1,
                      shared_from_this()));
    }

    void send(char* msg, short max_length, short msgid) {
        std::lock_guard<std::mutex> lock(send_lock_);
        int send_que_size = send_que_.size();
        if (send_que_size > MAX_SENDQUE) {
            std::cout << "session: " << uuid_ << " send queue full, size is "
                      << MAX_SENDQUE << std::endl;
            return;
        }

        send_que_.push(std::make_shared<SendNode>(msg, max_length, msgid));
        if (send_que_size > 0) {
            return;
        }
        auto& msgnode = send_que_.front();
        boost::asio::async_write(
            socket_, boost::asio::buffer(msgnode->data_, msgnode->total_len_),
            std::bind(&ClientSession::handleWrite, this, std::placeholders::_1,
                      shared_from_this()));
    }

    void close() {
        if (!b_close_) {
            socket_.close();
            b_close_ = true;
        }

        if(connected_) {
            connected_ = false;
            std::cout << "Disconnected from server. Thank you for using YFTP." << std::endl;
        }
    }

    void setResponseCallback(std::function<void()> callback) {
        on_response_callback_ = std::move(callback);
    }

   private:
    void handleWrite(const boost::system::error_code& error, ptr shared_self) {
        try {
            if (!error) {
                std::lock_guard<std::mutex> lock(send_lock_);
                send_que_.pop();
                if (!send_que_.empty()) {
                    auto& msgnode = send_que_.front();
                    boost::asio::async_write(
                        socket_,
                        boost::asio::buffer(msgnode->data_,
                                            msgnode->total_len_),
                        std::bind(&ClientSession::handleWrite, this,
                                  std::placeholders::_1, shared_self));
                }
            } else {
                std::cerr << "handle write failed: " << error.message()
                          << std::endl;
                close();
            }
        } catch (std::exception& e) {
            std::cerr << "Exception in handleWrite: " << e.what() << std::endl;
        }
    }

    void handleRead(const boost::system::error_code& error,
                    size_t bytes_transferred, ptr shared_self) {
        try {
            if (!error) {
                int copy_len = 0;
                while (bytes_transferred > 0) {
                    if (!b_head_parse_) {
                        if (bytes_transferred + recv_head_node_->cur_len_ <
                            HEAD_TOTAL_LEN) {
                            memcpy(recv_head_node_->data_ +
                                       recv_head_node_->cur_len_,
                                   data_ + copy_len, bytes_transferred);
                            recv_head_node_->cur_len_ += bytes_transferred;
                            ::memset(data_, 0, MAX_LENGTH);
                            socket_.async_read_some(
                                boost::asio::buffer(data_, MAX_LENGTH),
                                std::bind(&ClientSession::handleRead, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2, shared_self));
                            return;
                        }
                        int head_remain =
                            HEAD_TOTAL_LEN - recv_head_node_->cur_len_;
                        memcpy(
                            recv_head_node_->data_ + recv_head_node_->cur_len_,
                            data_ + copy_len, head_remain);
                        copy_len += head_remain;
                        bytes_transferred -= head_remain;
                        short msg_id = 0;
                        memcpy(&msg_id, recv_head_node_->data_, HEAD_ID_LEN);
                        msg_id = boost::asio::detail::socket_ops::
                            network_to_host_short(msg_id);

                        short msg_len = 0;
                        memcpy(&msg_len, recv_head_node_->data_ + HEAD_ID_LEN,
                               HEAD_DATA_LEN);
                        msg_len = boost::asio::detail::socket_ops::
                            network_to_host_short(msg_len);

                        recv_msg_node_ =
                            std::make_shared<RecvNode>(msg_len, msg_id);

                        if (bytes_transferred < msg_len) {
                            memcpy(recv_msg_node_->data_ +
                                       recv_msg_node_->cur_len_,
                                   data_ + copy_len, bytes_transferred);
                            recv_msg_node_->cur_len_ += bytes_transferred;
                            ::memset(data_, 0, MAX_LENGTH);
                            socket_.async_read_some(
                                boost::asio::buffer(data_, MAX_LENGTH),
                                std::bind(&ClientSession::handleRead, this,
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
                        recv_msg_node_->data_[recv_msg_node_->total_len_] =
                            '\0';

                        // 处理接收到的消息
                        processMessage(recv_msg_node_);

                        b_head_parse_ = false;
                        recv_head_node_->clear();
                        if (bytes_transferred <= 0) {
                            ::memset(data_, 0, MAX_LENGTH);
                            socket_.async_read_some(
                                boost::asio::buffer(data_, MAX_LENGTH),
                                std::bind(&ClientSession::handleRead, this,
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
                            std::bind(&ClientSession::handleRead, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    memcpy(recv_msg_node_->data_ + recv_msg_node_->cur_len_,
                           data_ + copy_len, remain_msg);
                    recv_msg_node_->cur_len_ += remain_msg;
                    bytes_transferred -= remain_msg;
                    copy_len += remain_msg;
                    recv_msg_node_->data_[recv_msg_node_->total_len_] = '\0';

                    // 处理接收到的消息
                    processMessage(recv_msg_node_);

                    b_head_parse_ = false;
                    recv_head_node_->clear();
                    if (bytes_transferred <= 0) {
                        ::memset(data_, 0, MAX_LENGTH);
                        socket_.async_read_some(
                            boost::asio::buffer(data_, MAX_LENGTH),
                            std::bind(&ClientSession::handleRead, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2, shared_self));
                        return;
                    }
                    continue;
                }
            } else {
                std::cerr << "handle read failed: " << error.message()
                          << std::endl;
                close();
            }
        } catch (std::exception& e) {
            std::cerr << "Exception in handleRead: " << e.what() << std::endl;
        }
    }

    void processMessage(std::shared_ptr<RecvNode> recv_node) {
        std::string msg_data(recv_node->data_, recv_node->total_len_);

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(msg_data, root)) {
            std::cerr << "Failed to parse server response: " << msg_data
                      << std::endl;
            return;
        }

        short msg_id = root["msg_id"].asInt();

        switch (msg_id) {
            case MSG_HELLO_WORD: {
                std::cout << "Server response: " << root["data"].asString()
                          << std::endl;
                break;
            }
            case MSG_LIST: {
                std::cout << "\nDirectory listing:\n"
                          << root["data"].asString() << std::endl;
                break;
            }
            case MSG_MKDIR: {
                std::cout << "Server response: " << root["data"].asString()
                          << std::endl;
                break;
            }
            case MSG_RMDIR: {
                std::cout << "Server response: " << root["data"].asString()
                          << std::endl;
                break;
            }
            case MSG_UPLOAD: {
                std::cout << "Server response: " << root["data"].asString()
                          << std::endl;
                break;
            }
            case MSG_DOWNLOAD: {
                if (root.isMember("content")) {
                    std::string filename = current_download_file_;
                    std::ofstream outfile(filename, std::ios::binary);
                    if (outfile) {
                        outfile << root["content"].asString();
                        outfile.close();
                        std::cout
                            << "File downloaded successfully to: " << filename
                            << std::endl;
                    } else {
                        std::cerr
                            << "Failed to save downloaded file: " << filename
                            << std::endl;
                    }
                } else {
                    std::cerr << "Server response: " << root["data"].asString()
                              << std::endl;
                }
                break;
            }case MSG_CD: {
                std::cout << "The current directory is " << root["data"].asString()
                          << std::endl;
                break;
            }
            case MSG_PWD: {
                std::cout << "The current directory is " << root["data"].asString()
                          << std::endl;
                break;
            }case MSG_CAT: {
                std::cout << "File content:\n" << root["content"].asString()
                          << std::endl;
                break;
            }case MSG_EXIT: {
                std::cout << "Server response: " << root["data"].asString()
                          << std::endl;
                close();
                break;
            }
            default: {
                std::cerr << "Unknown message ID: " << msg_id << std::endl;
                break;
            }
        }

        if (on_response_callback_) {
            on_response_callback_();
        }
    }

   private:
    tcp::socket socket_;
    std::string uuid_;
    char data_[MAX_LENGTH];
    bool b_close_;
    std::queue<std::shared_ptr<SendNode>> send_que_;
    std::mutex send_lock_;
    std::shared_ptr<RecvNode> recv_msg_node_;
    bool b_head_parse_;
    std::shared_ptr<MsgNode> recv_head_node_;
    std::string current_download_file_;
    std::function<void()> on_response_callback_;
    bool connected_;
};
