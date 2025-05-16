#pragma once
#include <string>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

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