#pragma once
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>

#include "ClientSession.hpp"
#include "YFTPClient.hpp"

// 命令行界面
class CommandLineInterface {
   public:
    CommandLineInterface(const std::string& host, short port)
        : client_(host, port), running_(false), waiting_response_(false) {}

    void start() {
        client_.connect();
        running_ = true;

        dynamic_cast<ClientSession*>(client_.session_.get())
            ->setResponseCallback([this]() {
                waiting_response_ = false;
                showPrompt();
            });

        // 显示欢迎信息
        std::cout << "YFTP Client - Connected to " << client_.getHost() << ":"
                  << client_.getPort() << std::endl;
        std::cout << "Type 'help' for available commands" << std::endl;

        while (!client_.session_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        showPrompt();

        while (running_) {
            std::string input;
            std::getline(std::cin, input);

            if (input.empty()) continue;

            std::vector<std::string> tokens;
            boost::split(tokens, input, boost::is_space(),
                         boost::token_compress_on);

            if (tokens.empty()) {
                if (!waiting_response_) {
                    showPrompt();
                }
                continue;
            }

            std::string command = tokens[0];

            if (command == "help" || command == "?") {
                showHelp();
                if (!waiting_response_) {
                    showPrompt();
                }
            } else if (command == "exit" || command == "quit") {
                running_ = false;
            } else if (command == "hello") {
                client_.sendHello();
            } else if (command == "list" || command == "ls") {
                if (tokens.size() < 2) {
                    std::cerr << "Usage: list <remote_path>" << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.listDirectory(tokens[1]);
                }
            } else if (command == "mkdir") {
                if (tokens.size() < 2) {
                    std::cerr << "Usage: mkdir <remote_path>" << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.createDirectory(tokens[1]);
                }
            } else if (command == "rmdir") {
                if (tokens.size() < 2) {
                    std::cerr << "Usage: rmdir <remote_path>" << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.removeDirectory(tokens[1]);
                }
            } else if (command == "upload" || command == "put") {
                if (tokens.size() < 3) {
                    std::cerr << "Usage: upload <local_path> <remote_path>"
                              << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.uploadFile(tokens[1], tokens[2]);
                }
            } else if (command == "download" || command == "get") {
                if (tokens.size() < 3) {
                    std::cerr << "Usage: download <remote_path> <local_path>"
                              << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.downloadFile(tokens[1], tokens[2]);
                }
            } else if (command == "cd") {
                if (tokens.size() < 2) {
                    std::cerr << "Usage: cd <remote_path>" << std::endl;
                    if (!waiting_response_) {
                        showPrompt();
                    }
                } else {
                    waiting_response_ = true;
                    client_.changeDirectory(tokens[1]);
                }
            } else if (command == "pwd") {
                waiting_response_ = true;
                client_.printCurrentPath();
            } else if (command == "clear") {
                std::cout << "\033[2J\033[1;1H";  // Clear the console
                showPrompt();
            } else if (command == "clear") {
                std::cout << "\033[2J\033[1;1H";  // Clear the console
                showPrompt();
            } else if (command == "cls") {
                std::cout << "\033[2J\033[1;1H";  // Clear the console
                showPrompt();
            } else {
                std::cerr << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands" << std::endl;
                if (!waiting_response_) {
                    showPrompt();
                }
            }
        }

        client_.disconnect();
    }

   private:
    void showPrompt() {
        if (running_ && !waiting_response_) {
            std::cout << "yftp> " << std::flush;
        }
    }

    void showHelp() {
        std::cout
            << "\nAvailable commands:\n"
            << "  help, ?        - Show this help message\n"
            << "  exit, quit     - Exit the client\n"
            << "  hello          - Send a hello message to server\n"
            << "  list, ls       - List directory contents on server\n"
            << "                  Usage: list <remote_path>\n"
            << "  mkdir          - Create directory on server\n"
            << "                  Usage: mkdir <remote_path>\n"
            << "  rmdir          - Remove directory on server\n"
            << "                  Usage: rmdir <remote_path>\n"
            << "  upload, put    - Upload file to server\n"
            << "                  Usage: upload <local_path> <remote_path>\n"
            << "  download, get  - Download file from server\n"
            << "                  Usage: download <remote_path> <local_path>\n"
            << std::endl;
    }

    YFTPClient client_;
    bool running_;
    bool waiting_response_;
};