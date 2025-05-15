#pragma once

#include <filesystem>
#include <fstream>
#include "Lister.hpp"

namespace Yftp{
    class FileUtil{
        public:
            static std::string getFileName(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                return std::filesystem::path(path).filename().string();
            }

            static std::string getFileExtension(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                return std::filesystem::path(path).extension().string();
            }

            static bool fileExists(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                return std::filesystem::exists(path);
            }

            static bool directoryExists(const std::string& dir_path) {
                std::string path = standardizePath(dir_path);
                return std::filesystem::exists(path) && std::filesystem::is_directory(path);
            }

            static bool createDirectory(const std::string& dir_path) {
                std::string path = standardizePath(dir_path);
                return std::filesystem::create_directories(path);
            }

            static bool removeDirectory(const std::string& dir_path) {
                std::string path = standardizePath(dir_path);
                return std::filesystem::remove_all(path);
            }

            static bool removeFile(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                return std::filesystem::remove(path);
            }

            static bool createFile(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                std::ofstream file(path);
                if (!file) {
                    return false;
                }
                file.close();
                return true;
            }

            static bool writeFile(const std::string& file_path, const std::string& content) {
                std::string path = standardizePath(file_path);
                std::ofstream file(path);
                if (!file) {
                    return false;
                }
                file << content;
                file.close();
                return true;
            }

            static std::string readFile(const std::string& file_path) {
                std::string path = standardizePath(file_path);
                std::ifstream file(path);
                if (!file) {
                    return "";
                }
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                return content;
            }

            static bool copyFile(const std::string& source, const std::string& destination) {
                std::string src = standardizePath(source);
                std::string dst = standardizePath(destination);
                return std::filesystem::copy_file(src, dst);
            }

            static std::string getList(const std::string& dir_path){
                std::string path = standardizePath(dir_path);
                return DirectoryLister::listDirectory(path);
            }

            static std::string expandUser(const std::string& path){
                if(path.empty() || path[0] != '~'){
                    return path;
                }
                std::string homeDir = std::getenv("HOME");
                if(homeDir.empty()) return path;
                return homeDir + path.substr(1);
            }

            static std::string standardizePath(const std::string& path){
                try{
                    std::string standardized_path = expandUser(path);
                    standardized_path = std::filesystem::absolute(standardized_path).string();
                    return std::filesystem::weakly_canonical(standardized_path).string();
                }catch(const std::filesystem::filesystem_error& e){
                    return path;
                }
            }
    };
}