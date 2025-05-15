#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

class DirectoryLister {
public:
    //file info
    struct FileInfo {
        std::string name;
        std::string type;
        std::string size;
        std::string modified;
        std::string permissions;
    };

    static std::string listDirectory(const std::string& path) {
        std::vector<FileInfo> files;
        std::ostringstream oss;
        
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                files.push_back(getFileInfo(entry));
            }
            
            std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
                if (a.type == "DIR" && b.type != "DIR") return true;
                if (a.type != "DIR" && b.type == "DIR") return false;
                return a.name < b.name;
            });
            
            size_t maxNameLen = 4; // "Name"
            size_t maxTypeLen = 4; // "Type"
            size_t maxSizeLen = 4; // "Size"
            size_t maxModifiedLen = 8; // "Modified"
            
            for (const auto& file : files) {
                if (file.name.length() > maxNameLen) maxNameLen = file.name.length();
                if (file.type.length() > maxTypeLen) maxTypeLen = file.type.length();
                if (file.size.length() > maxSizeLen) maxSizeLen = file.size.length();
                if (file.modified.length() > maxModifiedLen) maxModifiedLen = file.modified.length();
            }
            
            oss << "Listing directory: " << fs::absolute(path).string() << "\n\n";
            
            oss << std::left 
                << std::setw(maxNameLen + 2) << "Name"
                << std::setw(maxTypeLen + 2) << "Type"
                << std::setw(maxSizeLen + 2) << "Size"
                << std::setw(maxModifiedLen + 2) << "Modified"
                << "Permissions" << "\n";
            
            oss << std::string(maxNameLen + maxTypeLen + maxSizeLen + maxModifiedLen + 20, '-') << "\n";
            
            for (const auto& file : files) {
                oss << std::left
                    << std::setw(maxNameLen + 2) << file.name
                    << std::setw(maxTypeLen + 2) << file.type
                    << std::setw(maxSizeLen + 2) << file.size
                    << std::setw(maxModifiedLen + 2) << file.modified
                    << file.permissions << "\n";
            }
            
        } catch (const fs::filesystem_error& e) {
            oss << "Error: " << e.what();
        }
        
        return oss.str();
    }

private:
    static std::string getPrettySize(uintmax_t size) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double s = static_cast<double>(size);
        
        while (s >= 1024 && unit < 4) {
            s /= 1024;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << s << " " << units[unit];
        return oss.str();
    }

    static std::string getPermissions(fs::perms p) {
        std::string perms;
        perms += (p & fs::perms::owner_read) != fs::perms::none ? "r" : "-";
        perms += (p & fs::perms::owner_write) != fs::perms::none ? "w" : "-";
        perms += (p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-";
        perms += (p & fs::perms::group_read) != fs::perms::none ? "r" : "-";
        perms += (p & fs::perms::group_write) != fs::perms::none ? "w" : "-";
        perms += (p & fs::perms::group_exec) != fs::perms::none ? "x" : "-";
        perms += (p & fs::perms::others_read) != fs::perms::none ? "r" : "-";
        perms += (p & fs::perms::others_write) != fs::perms::none ? "w" : "-";
        perms += (p & fs::perms::others_exec) != fs::perms::none ? "x" : "-";
        return perms;
    }

    static FileInfo getFileInfo(const fs::directory_entry& entry) {
        FileInfo info;
        info.name = entry.path().filename().string();
        
        if (entry.is_directory()) {
            info.type = "DIR";
            info.size = "-";
        } else {
            info.type = "FILE";
            info.size = getPrettySize(entry.file_size());
        }
        
        auto ftime = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&cftime));
        info.modified = timeStr;
        
        info.permissions = getPermissions(entry.status().permissions());
        
        return info;
    }
};