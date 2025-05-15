#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>
#include <mutex>
#include <queue>

#include "MsgNode.hpp"
#include "const.hpp"

using tcp = boost::asio::ip::tcp;
namespace Yftp {
class Server;
class LogicSystem;
class Session : public std::enable_shared_from_this<Session> {
public:
    using ptr = std::shared_ptr<Session>;
    Session(boost::asio::io_context& io_context, Server* server);
    ~Session();
    tcp::socket& getSocket();
    std::string& getUUID();
    std::string getRemoteIp();
    void start();
    void send(char* msg, short max_length, short msgid);
    void send(std::string msg, short msgid);
    void close();
    std::shared_ptr<Session> sharedSelf();

private:
    void handleRead(const boost::system::error_code& error, size_t bytes_transferred,
                    std::shared_ptr<Session> shared_self);
    void handleWrite(const boost::system::error_code& error, std::shared_ptr<Session> shared_self);
    tcp::socket socket_;
    std::string uuid_;
    char data_[MAX_LENGTH];
    Server* server_;
    bool b_close_;
    std::queue<shared_ptr<SendNode> > send_que_;
    std::mutex send_lock_;
    std::shared_ptr<RecvNode> recv_msg_node_;
    bool b_head_parse_;
    std::shared_ptr<MsgNode> recv_head_node_;
};

class LogicNode {
    friend class LogicSystem;

public:
    LogicNode(shared_ptr<Session>, shared_ptr<RecvNode>);

private:
    shared_ptr<Session> session_;
    shared_ptr<RecvNode> recvnode_;
};

}  // namespace Yftp