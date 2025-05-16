#include "../include/LogicSystem.hpp"

#include <exception>
#include <string>

#include "../include/logger.hpp"

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
    if (msg_que_.size() == 1) {
        unique_lk.unlock();
        consume_.notify_one();
    }
}

void LogicSystem::dealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> unique_lk(mutex_);
        while (msg_que_.empty() && !b_stop_) {
            consume_.wait(unique_lk);
        }

        if (b_stop_) {
            while (!msg_que_.empty()) {
                auto msg_node = msg_que_.front();
                LOG_INFO("recv_msg id is {}", msg_node->recvnode_->msg_id_);
                auto call_back_iter =
                    fun_callbacks_.find(msg_node->recvnode_->msg_id_);
                if (call_back_iter == fun_callbacks_.end()) {
                    msg_que_.pop();
                    continue;
                }
                call_back_iter->second(
                    msg_node->session_, msg_node->recvnode_->msg_id_,
                    std::string(msg_node->recvnode_->data_,
                                msg_node->recvnode_->cur_len_));
                msg_que_.pop();
            }
            break;
        }

        auto msg_node = msg_que_.front();
        LOG_INFO("recv_msg id is {}", msg_node->recvnode_->msg_id_);
        auto call_back_iter = fun_callbacks_.find(msg_node->recvnode_->msg_id_);
        if (call_back_iter == fun_callbacks_.end()) {
            msg_que_.pop();
            continue;
        }
        call_back_iter->second(msg_node->session_, msg_node->recvnode_->msg_id_,
                               std::string(msg_node->recvnode_->data_,
                                           msg_node->recvnode_->cur_len_));
        msg_que_.pop();
    }
}

void LogicSystem::registerCallBacks() {
    registerHandler(
        MSG_HELLO_WORD,
        std::bind(&LogicSystem::helloWordCallBack, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
    registerHandler(
        MSG_LIST,
        std::bind(&LogicSystem::handleListCallBack, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
    registerHandler(MSG_MKDIR,
                    std::bind(&LogicSystem::handleMkdirCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    registerHandler(MSG_RMDIR,
                    std::bind(&LogicSystem::handleRmdirCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    registerHandler(MSG_UPLOAD,
                    std::bind(&LogicSystem::handleUploadCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    registerHandler(MSG_DOWNLOAD,
                    std::bind(&LogicSystem::handleDownloadCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    registerHandler(MSG_CD,
                    std::bind(&LogicSystem::handleCDCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    registerHandler(MSG_PWD,
                    std::bind(&LogicSystem::handlePWDCallBack, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
    LOG_INFO("register all msg id success");
    return;
}

void LogicSystem::registerHandler(MSG_IDS msg_id, FunCallBack func) {
    auto iter = fun_callbacks_.find(msg_id);
    if (iter != fun_callbacks_.end()) {
        LOG_WARN("msg id {} has been registered", msg_id);
        return;
    }
    fun_callbacks_.insert({msg_id, func});
    LOG_INFO("msg id {} register success", msg_id);
}

void LogicSystem::helloWordCallBack(shared_ptr<Session> session,
                                    const short &msg_id,
                                    const string &msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    LOG_INFO("recevie msg id is {} msg data is {}", root["id"].asInt(),
             root["data"].asString());
    root["data"] =
        "server has received msg, msg data is " + root["data"].asString();
    std::string return_str = root.toStyledString();
    LOG_INFO("Send {} bytes to {}", return_str.size(), session->getRemoteIp());
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
        path = session->resolvePath(path);
        std::string list_result = FileUtil::getList(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_LIST;
        return_root["data"] = list_result;
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_LIST);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_LIST;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_LIST);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_LIST;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
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
        path = session->resolvePath(path);
        bool result = FileUtil::createDirectory(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_MKDIR;
        return_root["data"] = result ? "Directory created successfully"
                                     : "Failed to create directory";
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_MKDIR);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_MKDIR;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_MKDIR);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_MKDIR;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
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
        path = session->resolvePath(path);
        bool result = FileUtil::removeDirectory(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_RMDIR;
        return_root["data"] = result ? "Directory removed successfully"
                                     : "Failed to remove directory";
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_RMDIR);
    } catch (const std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_RMDIR;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_RMDIR);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_RMDIR;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_RMDIR);
    }
    return;
}

void LogicSystem::handleUploadCallBack(Session::ptr session,
                                       const short &msg_id,
                                       const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_UPLOAD);
        std::string path = root["path"].asString();
        path = session->resolvePath(path);
        if (!fs::exists(path)) {
            if (fs::is_directory(path)) {
                LOG_ERROR("Path {} is a directory.", path);
                throw std::runtime_error("Path is a directory");
            }

            std::ofstream file(path);  // Create the file if it doesn't exist
            if (!file) {
                LOG_ERROR("Failed to create file at path: {}", path);
                throw std::runtime_error("Failed to create file");
            }
            file.close();
        }
        std::string content = root["content"].asString();
        bool result = FileUtil::writeFile(path, content);
        Json::Value return_root;
        return_root["msg_id"] = MSG_UPLOAD;
        return_root["data"] =
            result ? "File uploaded successfully" : "Failed to upload file";
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_UPLOAD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_UPLOAD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_UPLOAD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_UPLOAD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_UPLOAD);
    }
    return;
}

void LogicSystem::handleDownloadCallBack(Session::ptr session,
                                         const short &msg_id,
                                         const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_DOWNLOAD);
        std::string path = root["path"].asString();
        path = session->resolvePath(path);
        std::string content = FileUtil::readFile(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_DOWNLOAD;
        return_root["content"] = content;
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_DOWNLOAD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_DOWNLOAD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_DOWNLOAD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_DOWNLOAD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_DOWNLOAD);
    }
    return;
}

void LogicSystem::handleCDCallBack(Session::ptr session, const short &msg_id,
                                   const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_CD);
        std::string path = root["path"].asString();
        path = session->resolvePath(path);
        session->setCurrentPath(path);
        Json::Value return_root;
        return_root["msg_id"] = MSG_CD;
        return_root["data"] = "Changed directory to " + path;
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_CD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_CD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_CD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_CD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_CD);
    }
    return;
}

void LogicSystem::handlePWDCallBack(Session::ptr session, const short &msg_id,
                                    const string &msg_data) {
    try {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        assert(root["msg_id"].asInt() == MSG_PWD);
        std::string path = session->getCurrentPath();
        Json::Value return_root;
        return_root["msg_id"] = MSG_PWD;
        return_root["data"] = path;
        std::string return_str = return_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", return_str.size(),
                 session->getRemoteIp());
        session->send(return_str, MSG_PWD);
    } catch (std::exception &e) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_PWD;
        error_root["data"] = std::string("Exception occurred: ") + e.what();
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_PWD);
    } catch (...) {
        Json::Value error_root;
        error_root["msg_id"] = MSG_PWD;
        error_root["data"] = "Unknown exception occurred";
        std::string error_str = error_root.toStyledString();
        LOG_INFO("Send {} bytes to {}", error_str.size(),
                 session->getRemoteIp());
        session->send(error_str, MSG_PWD);
    }

    return;
}
}  // namespace Yftp