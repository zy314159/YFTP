#include "LogicSystem.hpp"

namespace Yftp {
LogicSystem::LogicSystem() : b_stop_(false) {
    registerCallBacks();
    worker_thread_ = std::thread(&LogicSystem::dealMsg, this);
}

LogicSystem::~LogicSystem() {
    b_stop_ = true;
    consume_.notify_one();
    worker_thread_.join();
}

void LogicSystem::postMsgToQue(shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> unique_lk(mutex_);
    msg_que_.push(msg);
    // 由0变为1则发送通知信号
    if (msg_que_.size() == 1) {
        unique_lk.unlock();
        consume_.notify_one();
    }
}

void LogicSystem::dealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> unique_lk(mutex_);
        // 判断队列为空则用条件变量阻塞等待，并释放锁
        while (msg_que_.empty() && !b_stop_) {
            consume_.wait(unique_lk);
        }

        // 判断是否为关闭状态，把所有逻辑执行完后则退出循环
        if (b_stop_) {
            while (!msg_que_.empty()) {
                auto msg_node = msg_que_.front();
                cout << "recv_msg id  is " << msg_node->recvnode_->msg_id_ << endl;
                auto call_back_iter = fun_callbacks_.find(msg_node->recvnode_->msg_id_);
                if (call_back_iter == fun_callbacks_.end()) {
                    msg_que_.pop();
                    continue;
                }
                call_back_iter->second(
                    msg_node->session_, msg_node->recvnode_->msg_id_,
                    std::string(msg_node->recvnode_->data_, msg_node->recvnode_->cur_len_));
                msg_que_.pop();
            }
            break;
        }

        // 如果没有停服，且说明队列中有数据
        auto msg_node = msg_que_.front();
        cout << "recv_msg id  is " << msg_node->recvnode_->msg_id_ << endl;
        auto call_back_iter = fun_callbacks_.find(msg_node->recvnode_->msg_id_);
        if (call_back_iter == fun_callbacks_.end()) {
            msg_que_.pop();
            continue;
        }
        call_back_iter->second(
            msg_node->session_, msg_node->recvnode_->msg_id_,
            std::string(msg_node->recvnode_->data_, msg_node->recvnode_->cur_len_));
        msg_que_.pop();
    }
}

void LogicSystem::registerCallBacks() {
    fun_callbacks_[MSG_HELLO_WORD] =
        std::bind(&LogicSystem::helloWordCallBack, this, placeholders::_1, placeholders::_2,
                  placeholders::_3);
}

void LogicSystem::helloWordCallBack(shared_ptr<Session> session, const short &msg_id,
                                    const string &msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std::cout << "recevie msg id  is " << root["id"].asInt() << " msg data is "
              << root["data"].asString() << endl;
    root["data"] = "server has received msg, msg data is " + root["data"].asString();
    std::string return_str = root.toStyledString();
    session->send(return_str, root["id"].asInt());
}

}  // namespace Yftp