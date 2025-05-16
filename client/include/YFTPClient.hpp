#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>

#include "ClientSession.hpp"
#include "FileUtil.hpp"

class YFTPClient {
   public:
    friend class CommandLineInterface;
    YFTPClient(const std::string& host, short port)
        : host_(host),
          port_(port),
          io_context_(),
          session_(std::make_shared<ClientSession>(io_context_)) {}

    ~YFTPClient() { disconnect(); }

    void connect() {
        session_->connect(host_, port_);
        io_thread_ = std::thread([this]() { io_context_.run(); });
    }

    void disconnect() {
        if (session_) {
            session_->close();
        }
        io_context_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }

    std::string getHost() const { return host_; }

    short getPort() const { return port_; }

    void sendHello() {
        Json::Value root;
        root["id"] = MSG_HELLO_WORD;
        root["data"] = "Hello from client";
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_HELLO_WORD);
    }

    void listDirectory(const std::string& path) {
        Json::Value root;
        root["msg_id"] = MSG_LIST;
        root["path"] = path;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_LIST);
    }

    void createDirectory(const std::string& path) {
        Json::Value root;
        root["msg_id"] = MSG_MKDIR;
        root["path"] = path;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_MKDIR);
    }

    void removeDirectory(const std::string& path) {
        Json::Value root;
        root["msg_id"] = MSG_RMDIR;
        root["path"] = path;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_RMDIR);
    }

    void uploadFile(const std::string& local_path,
                    const std::string& remote_path) {
        if (!FileUtil::fileExists(local_path)) {
            std::cerr << "Local file does not exist: " << local_path
                      << std::endl;
            return;
        }

        std::string content = FileUtil::readFile(local_path);
        if (content.empty()) {
            std::cerr << "Failed to read file or file is empty: " << local_path
                      << std::endl;
            return;
        }

        Json::Value root;
        root["msg_id"] = MSG_UPLOAD;
        root["path"] = remote_path;
        root["content"] = content;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_UPLOAD);
    }

    void downloadFile(const std::string& remote_path,
                      const std::string& local_path) {
        Json::Value root;
        root["msg_id"] = MSG_DOWNLOAD;
        root["path"] = remote_path;
        std::string msg = root.toStyledString();

        // 保存当前下载文件名，用于接收文件时保存
        dynamic_cast<ClientSession*>(session_.get())->current_download_file_ =
            local_path;

        session_->send(msg, MSG_DOWNLOAD);
    }

    void printCurrentPath() {
        Json::Value root;
        root["msg_id"] = MSG_PWD;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_PWD);
    }

    void changeDirectory(const std::string& path) {
        Json::Value root;
        root["msg_id"] = MSG_CD;
        root["path"] = path;
        std::string msg = root.toStyledString();
        session_->send(msg, MSG_CD);
    }

   private:
    std::string host_;
    short port_;
    boost::asio::io_context io_context_;
    std::shared_ptr<ClientSession> session_;
    std::thread io_thread_;
};
