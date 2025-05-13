#pragma once

#include <filesystem>
#include <fstream>
#include "Lister.hpp"

namespace Yftp{
    class FileUtil{
        public:
            static std::string getFileName(const std::string& file_path) {
                return std::filesystem::path(file_path).filename().string();
            }

            static std::string getFileExtension(const std::string& file_path) {
                return std::filesystem::path(file_path).extension().string();
            }

            static bool fileExists(const std::string& file_path) {
                return std::filesystem::exists(file_path);
            }

            static bool directoryExists(const std::string& dir_path) {
                return std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path);
            }

            static bool createDirectory(const std::string& dir_path) {
                return std::filesystem::create_directories(dir_path);
            }

            static bool removeDirectory(const std::string& dir_path) {
                return std::filesystem::remove_all(dir_path);
            }

            static bool removeFile(const std::string& file_path) {
                return std::filesystem::remove(file_path);
            }

            static bool createFile(const std::string& file_path) {
                std::ofstream file(file_path);
                if (!file) {
                    return false;
                }
                file.close();
                return true;
            }

            static bool writeFile(const std::string& file_path, const std::string& content) {
                std::ofstream file(file_path);
                if (!file) {
                    return false;
                }
                file << content;
                file.close();
                return true;
            }

            static std::string readFile(const std::string& file_path) {
                std::ifstream file(file_path);
                if (!file) {
                    return "";
                }
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                return content;
            }

            static bool copyFile(const std::string& source, const std::string& destination) {
                return std::filesystem::copy_file(source, destination);
            }

            static std::string getList(const std::string& dir_path){
                return DirectoryLister::listDirectory(dir_path);
            }
    };
}