#pragma once
#include "const.hpp"
#include <cstring>
#include <boost/asio.hpp>

class MsgNode {
   public:
    MsgNode(short max_len) : cur_len_(0), total_len_(max_len) {
        data_ = new char[total_len_ + 1]();
        data_[total_len_] = '\0';
    }

    ~MsgNode() { delete[] data_; }

    void clear() {
        ::memset(data_, 0, total_len_);
        cur_len_ = 0;
    }

    short cur_len_;
    short total_len_;
    char* data_;
};

// 接收消息节点
class RecvNode : public MsgNode {
   public:
    RecvNode(short max_len, short msg_id) : MsgNode(max_len), msg_id_(msg_id) {}
    short msg_id_;
};

// 发送消息节点
class SendNode : public MsgNode {
   public:
    SendNode(const char* msg, short max_len, short msg_id)
        : MsgNode(max_len + HEAD_TOTAL_LEN), msg_id_(msg_id) {
        short msg_id_host =
            boost::asio::detail::socket_ops::host_to_network_short(msg_id);
        memcpy(data_, &msg_id_host, HEAD_ID_LEN);
        short max_len_host =
            boost::asio::detail::socket_ops::host_to_network_short(max_len);
        memcpy(data_ + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
        memcpy(data_ + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
    }
    short msg_id_;
};