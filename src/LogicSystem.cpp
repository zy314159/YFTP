#include "../include/LogicSystem.hpp"

#include <exception>
#include <string>

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

void LogicSystem::registerHandler(MSG_IDS msg_id, FunCallBack func) {
    auto iter = fun_callbacks_.find(msg_id);
    if (iter != fun_callbacks_.end()) {
        std::cout << "msg id " << msg_id << " has been registered" << std::endl;
        return;
    }
    fun_callbacks_.insert({msg_id, func});
    std::cout << "msg id " << msg_id << " register success" << std::endl;
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
    return;
}

void LogicSystem::handleListCallBack(Session::ptr session, const short &msg_id,
                                     const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_LIST);
        std::string path = root["path"].asString();
        std::string list_result = FileUtil::getList(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_LIST;
        return_root["data"] = list_result;
        std::string return_str = return_root.toStyledString();
        session->send(return_str, MSG_LIST);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_LIST;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_LIST);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_LIST;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_LIST);
    }
    return;
}

void LogicSystem::handleMkdirCallBack(Session::ptr session, const short &msg_id,
                                      const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_MKDIR);
        std::string path = root["path"].asString();
        bool result = FileUtil::createDirectory(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_MKDIR;
        return_root["data"] =
            result ? "Directory created successfully" : "Failed to create directory";
        std::string return_str = return_root.toStyledString();
        session->send(return_str, MSG_MKDIR);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_MKDIR;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_MKDIR);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_MKDIR;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_MKDIR);
    }
    return;
}

void LogicSystem::handleRmdirCallBack(Session::ptr session, const short &msg_id,
                                      const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_RMDIR);
        std::string path = root["path"].asString();
        bool result = FileUtil::removeDirectory(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_RMDIR;
        return_root["data"] =
            result ? "Directory removed successfully" : "Failed to remove directory";
        std::string return_str = return_root.toStyledString();
        session->send(return_str, MSG_RMDIR);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_RMDIR;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_RMDIR);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_RMDIR;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_RMDIR);
    }
    return;
}

void LogicSystem::handleUploadCallBack(Session::ptr session, const short &msg_id,
                                       const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_UPLOAD);
        std::string path = root["path"].asString();
        std::string content = root["content"].asString();
        bool result = FileUtil::writeFile(path, content);
        Json::Value return_root;
        return_root["msg_id"] = MSG_UPLOAD;
        return_root["data"] = result ? "File uploaded successfully" : "Failed to upload file";
        std::string return_str = return_root.toStyledString();
        session->send(return_str, MSG_UPLOAD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_UPLOAD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_UPLOAD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_UPLOAD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_UPLOAD);
    }
    return;
}

void LogicSystem::handleDownloadCallBack(Session::ptr session, const short &msg_id,
                                         const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_DOWNLOAD);
        std::string path = root["path"].asString();
        std::string content = FileUtil::readFile(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_DOWNLOAD;
        return_root["content"] = content;
        std::string return_str = return_root.toStyledString();
        session->send(return_str, MSG_DOWNLOAD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_DOWNLOAD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_DOWNLOAD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_DOWNLOAD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        session->send(error_str, MSG_DOWNLOAD);
    }
    return;
}
}  // namespace Yftp