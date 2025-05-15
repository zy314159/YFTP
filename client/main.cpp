#include <jsoncpp/json/json.h>

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using boost::asio::ip::tcp;

// 定义与服务器相同的消息头格式
#define MAX_LENGTH 1024 * 2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE 10000
#define MAX_SENDQUE 1000

// 定义与服务器相同的消息ID
enum MSG_IDS {
    MSG_HELLO_WORD = 1001,
    MSG_MKDIR,    // 创建目录指令
    MSG_RMDIR,    // 删除目录
    MSG_LIST,     // 列出目录内容
    MSG_UPLOAD,   // 上传文件
    MSG_DOWNLOAD  // 下载文件
};

// 消息节点类
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

// 客户端会话类
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
            std::cout << "Disconnected from server." << std::endl;
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

// 文件工具类
class FileUtil {
   public:
    static std::string readFile(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    static bool writeFile(const std::string& file_path,
                          const std::string& content) {
        std::ofstream file(file_path, std::ios::binary);
        if (!file) {
            return false;
        }
        file << content;
        file.close();
        return true;
    }

    static std::string getFileName(const std::string& file_path) {
        return fs::path(file_path).filename().string();
    }

    static bool fileExists(const std::string& file_path) {
        return fs::exists(file_path) && !fs::is_directory(file_path);
    }

    static bool directoryExists(const std::string& dir_path) {
        return fs::exists(dir_path) && fs::is_directory(dir_path);
    }
};

// 客户端主类
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

   private:
    std::string host_;
    short port_;
    boost::asio::io_context io_context_;
    std::shared_ptr<ClientSession> session_;
    std::thread io_thread_;
};

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

        while(!client_.session_->isConnected()){
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

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;

    std::string host = "127.0.0.1";
    short port = 8080;

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Show help message")(
        "host,H", po::value<std::string>(&host)->default_value("127.0.0.1"),
        "Server host")("port,p", po::value<short>(&port)->default_value(8080),
                       "Server port");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    try {
        CommandLineInterface cli(host, port);
        cli.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
