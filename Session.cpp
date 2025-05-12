#include "Session.hpp"

#include <memory>

#include "FileUtil.hpp"
#include "logger.hpp"
#include <boost/algorithm/string.hpp>
#include "Server.hpp"

namespace Yftp {
Session::Session(tcp::socket socket, std::shared_ptr<Server> server)
    : socket_(std::move(socket)), server_(server) {
    uuid_ = boost::uuids::to_string(boost::uuids::random_generator()());
    LOG_INFO_CONSOLE("Session created with UUID: {}", uuid_);
    LOG_INFO_ASYNC("Session created with UUID: {}", uuid_);

    command_handlers_["MKDIR"] = [this](Session& session, const std::vector<std::string>& args) {
        handleMkdir(args);
    };
    LOG_INFO_CONSOLE("Registered MKDIR command handler");
    LOG_INFO_ASYNC("Registered MKDIR command handler");

    command_handlers_["RMDIR"] = [this](Session& session, const std::vector<std::string>& args) {
        handleRmdir(args);
    };
    LOG_INFO_ASYNC("Registered RMDIR command handler");
    LOG_INFO_CONSOLE("Registered RMDIR command handler");

    command_handlers_["LIST"] = [this](Session& session, const std::vector<std::string>& args) {
        handleList(args);
    };
    LOG_INFO_ASYNC("Registered LIST command handler");
    LOG_INFO_CONSOLE("Registered LIST command handler");

    command_handlers_["DOWNLOAD"] = [this](Session& session, const std::vector<std::string>& args) {
        handleDownload(args);
    };
    LOG_INFO_ASYNC("Registered DOWNLOAD command handler");
    LOG_INFO_CONSOLE("Registered DOWNLOAD command handler");

    command_handlers_["UPLOAD"] = [this](Session& session, const std::vector<std::string>& args) {
        handleUpload(args);
    };
    LOG_INFO_ASYNC("Registered UPLOAD command handler");
    LOG_INFO_CONSOLE("Registered UPLOAD command handler");
}

void Session::handleMkdir(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        sendResponse("Usage: MKDIR <directory>");
        return;
    }
    std::string path = args[0];
    if (FileUtil::createDirectory(path)) {
        sendResponse("Directory created successfully");
    } else {
        sendResponse("Failed to create directory");
    }
}

void Session::handleRmdir(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        sendResponse("Usage: RMDIR <directory>");
        return;
    }
    std::string path = args[0];
    if (FileUtil::removeDirectory(path)) {
        sendResponse("Directory removed successfully");
    } else {
        sendResponse("Failed to remove directory");
    }
}

void Session::handleList(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        sendResponse("Usage: LIST <directory>");
        return;
    }
    std::string path = args[0];
    std::string result = FileUtil::getList(path);
    sendResponse(result);
}

void Session::handleUpload(const std::vector<std::string>& args) {
    if (args.empty()) {
        sendResponse("UPLOAD: Missing filename\n");
        return;
    }
    std::string filename = args[0];
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        sendResponse("Failed to open file for writing\n");
        return;
    }

    // 开始接收文件数据
    boost::asio::async_read(
        socket_, boost::asio::buffer(upload_buffer_),
        [this, &ofs](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                ofs.write(upload_buffer_.data(), bytes_transferred);
                handleUpload({});
            } else {
                if (ec != boost::asio::error::eof) {
                    sendResponse("Upload failed: " + ec.message() + "\n");
                } else {
                    sendResponse("File uploaded successfully\n");
                }
                ofs.close();
            }
        });
}

void Session::handleDownload(const std::vector<std::string>& args) {
    if (args.empty()) {
        sendResponse("DOWNLOAD: Missing filename\n");
        return;
    }
    std::string filename = args[0];
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        sendResponse("File not found\n");
        return;
    }

    // Read file content synchronously
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string file_content = oss.str();

    // Send file content asynchronously
    boost::asio::async_write(
        socket_, boost::asio::buffer(file_content),
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                sendResponse("File downloaded successfully\n");
            } else {
                sendResponse("Download failed: " + ec.message() + "\n");
            }
        });
}

void Session::run() {
    LOG_INFO_CONSOLE("New session started with UUID: {}", uuid_);
    LOG_INFO_ASYNC("New session started with UUID: {}", uuid_);
    sendResponse("Welcome to YFTP server!\n");
    doRead();
    return;
}

void Session::doRead() {
    boost::asio::async_read_until(
        socket_, input_buffer_, "\n",
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string command;
                std::getline(std::istream(&input_buffer_), command);
                processCommand(command);
                doRead();
            } else {
                LOG_WARN_CONSOLE("Session closed [UID: {}]", uuid_);
                server_->sessions_.erase(uuid_);
            }
        });
}

void Session::processCommand(const std::string& cmd_line) {
    std::vector<std::string> args;
    boost::split(args, cmd_line, boost::is_any_of(" \t"), boost::token_compress_on);

    if (args.empty())
        return;

    auto it = command_handlers_.find(args[0]);
    if (it != command_handlers_.end()) {
        try {
            it->second(*this, std::vector<std::string>(args.begin() + 1, args.end()));
        } catch (const std::exception& e) {
            sendResponse("Error: " + std::string(e.what()) + "\n");
            LOG_ERROR_CONSOLE("Command execution failed: {}", e.what());
        }
    } else {
        sendResponse("Unknown command: " + args[0] + "\n");
    }
}

void Session::sendResponse(const char* response, size_t length) {
    boost::asio::async_write(
        socket_, boost::asio::buffer(response, length),
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                LOG_INFO_CONSOLE("Sent {} bytes to client", bytes_transferred);
                LOG_INFO_ASYNC("Sent {} bytes to client", bytes_transferred);
            } else {
                LOG_ERROR_CONSOLE("Failed to send response: {}", error.message());
                LOG_ERROR_ASYNC("Failed to send response: {}", error.message());
            }
        });
    return;
}

void Session::sendResponse(const std::string& response) {
    sendResponse(response.c_str(), response.size());
    return;
}

}  // namespace Yftp