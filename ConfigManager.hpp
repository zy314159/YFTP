#pragma once
// for json file
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
// for xml file
#include <tinyxml2.h>
// for ini file
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
// for yaml file
#include <yaml-cpp/yaml.h>

// c++ standard library
#include <any>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

// self-defined
#include "Singleton.hpp"

namespace Yftp {

class ConfigManager : public Singleton<ConfigManager> {
public:
    using ChangeCallback = std::function<void(const std::string&)>;
    friend class Singleton<ConfigManager>;
    enum class Format { JSON, XML, YAML, INI };
    Format fileTypeToFormat(const std::string& file_path) {
        std::filesystem::path path(file_path);
        if(path.extension().string().compare(".json") == 0) {
            return Format::JSON;
        } else if (path.extension().string().compare(".xml") == 0) {
            return Format::XML;
        } else if (path.extension().string().compare(".ini") == 0) {
            return Format::INI;
        } else if (path.extension().string().compare(".yaml") == 0 ||
                   path.extension().string().compare(".yml") == 0) {
            return Format::YAML;
        }else{
            throw std::invalid_argument("Unsupported file format");
        }
    }

    ConfigManager() = default;

    void load(const std::string& file_path) {
        auto new_data = std::make_shared<ConfigData>();
        {
            std::lock_guard<std::mutex> lock(new_data->mtx);
            new_data->file_path = file_path;
            new_data->format = fileTypeToFormat(file_path);
        }
        data_ = std::move(new_data);
        parseFile();
    }

    void reload() {
        if (data_) {
            parseFile();
        }
        return;
    }

    template <typename T>
    void set(const std::string& key, const T& value) {
        if (!data_)
            return;
        {
            std::lock_guard<std::mutex> lock(data_->mtx);
            data_->values[key] = value;
        }
        notifyChanges(key);
    }
    
    bool contains(const std::string& key) const {
        if (!data_)
            return false;

        std::lock_guard<std::mutex> lock(data_->mtx);
        return data_->values.find(key) != data_->values.end();
    }

    int registerChangeCallback(const ChangeCallback& callback) {
        if (!data_)
            return -1;

        std::lock_guard<std::mutex> lock(data_->mtx);
        int id = data_->next_callback_id++;
        data_->callbacks[id] = callback;
        return id;
    }

    void unregisterChangeCallback(int id) {
        if (!data_)
            return;

        std::lock_guard<std::mutex> lock(data_->mtx);
        data_->callbacks.erase(id);
    }

    template <typename T>
    T get(const std::string& key, const T& default_value = T{}) const {
        std::lock_guard<std::mutex> lock(data_->mtx);
        auto it = data_->values.find(key);
        if (it != data_->values.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return default_value;
            }
        }
        return default_value;
    }

    void saveToFile() {
        std::lock_guard<std::mutex> lock(data_->mtx);
        switch (data_->format) {
            case Format::JSON: {
                saveJson(data_->file_path);
            } break;
            case Format::XML: {
                saveXml(data_->file_path);
            } break;
            case Format::INI: {
                saveIni(data_->file_path);
            } break;
            case Format::YAML: {
                saveYaml(data_->file_path);
            } break;
            default: {
                throw std::invalid_argument("Unsupported format");
            } break;
        }
    }

    void saveToFile(const std::string& file_path) {
        std::lock_guard<std::mutex> lock(data_->mtx);
        data_->file_path = file_path;
        saveToFile();
    }

    void enableAutoSave(bool enable, int intervalMs = 5000) {
        if (enable == enable_autosave_.load())
            return;
        enable_autosave_.store(enable);
        if (enable) {
            autosave_interval_ = std::chrono::milliseconds(intervalMs);
            autosave_thread_ = std::thread(&ConfigManager::autoSaveWorker, this);
        } else {
            {
                std::lock_guard<std::mutex> lock(autosave_mutex_);
                autosave_cond_.notify_all();
            }
            if (autosave_thread_.joinable()) {
                autosave_thread_.join();
            }
        }
        return;
    }

    void clear(){
        if(data_ == nullptr) return;
        std::lock_guard<std::mutex> lock(data_->mtx);
        enable_autosave_.store(false);
        data_.reset();
    }

    void reset(std::string file_path){
        if(data_) clear();
        load(file_path);
        return;
    }

private:
    void parseFile() {
        std::lock_guard<std::mutex> lock(data_->mtx);
        switch (data_->format) {
            case Format::JSON: {
                parseJson(data_->file_path);
            } break;
            case Format::XML: {
                parseXml(data_->file_path);
            } break;
            case Format::INI: {
                parseIni(data_->file_path);
            } break;
            case Format::YAML: {
                parseYaml(data_->file_path);
            } break;
            default: {
                throw std::invalid_argument("Unsupported format");
            } break;
        }
        data_->last_write_time = std::filesystem::last_write_time(data_->file_path);
        return;
    }

    void parseJson(std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open json file: " + file_path);
        }

        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        if (!Json::parseFromStream(reader, file, &root, &errs)) {
            throw std::runtime_error("Failed to parse JSON: " + errs);
        }

        for (const auto& key : root.getMemberNames()) {
            if (root[key].isString()) {
                data_->values[key] = root[key].asString();
            } else if (root[key].isInt()) {
                data_->values[key] = root[key].asInt();
            } else if (root[key].isBool()) {
                data_->values[key] = root[key].asBool();
            } else if (root[key].isDouble()) {
                data_->values[key] = root[key].asDouble();
            } else {
                throw std::runtime_error("Unsupported JSON type for key: " + key);
            }
        }
        return;
    }

    void parseXml(std::string& file_path) {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(file_path.c_str()) != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error("Failed to open XML file: " + file_path);
        }

        tinyxml2::XMLElement* root = doc.RootElement();
        if (!root) {
            throw std::runtime_error("Failed to parse XML: No root element");
        }

        for (tinyxml2::XMLElement* element = root->FirstChildElement(); element != nullptr;
             element = element->NextSiblingElement()) {
            const char* key = element->Name();
            const char* value = element->GetText();
            if (key && value) {
                data_->values[key] = std::string(value);
            }
        }
        return;
    }

    void parseIni(std::string& file_path) {
        boost::property_tree::ptree pt;
        try {
            boost::property_tree::ini_parser::read_ini(file_path, pt);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to open INI file: " + file_path + ", " + e.what());
        }

        for (const auto& section : pt) {
            for (const auto& key_value : section.second) {
                data_->values[section.first + "." + key_value.first] = key_value.second.data();
            }
        }
        return;
    }

    void parseYaml(std::string& file_path) {
        YAML::Node config = YAML::LoadFile(file_path);
        if (!config) {
            throw std::runtime_error("Failed to open YAML file: " + file_path);
        }

        for (const auto& key : config) {
            if (key.second.IsScalar()) {
                data_->values[key.first.as<std::string>()] = key.second.as<std::string>();
            } else if (key.second.IsSequence()) {
                // Handle sequences if needed
            } else {
                throw std::runtime_error("Unsupported YAML type for key: " +
                                         key.first.as<std::string>());
            }
        }
        return;
    }

    void saveJson(std::string& file_path) {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open json file: " + file_path);
        }

        Json::Value root;
        for (const auto& [key, value] : data_->values) {
            if (value.type() == typeid(std::string)) {
                root[key] = std::any_cast<std::string>(value);
            } else if (value.type() == typeid(int)) {
                root[key] = std::any_cast<int>(value);
            } else if (value.type() == typeid(bool)) {
                root[key] = std::any_cast<bool>(value);
            } else if (value.type() == typeid(double)) {
                root[key] = std::any_cast<double>(value);
            } else {
                throw std::runtime_error("Unsupported JSON type for key: " + key);
            }
        }

        file << root;
        if (file.fail()) {
            throw std::runtime_error("Failed to write JSON file: " + file_path);
        }
        file.close();
        if (file.fail()) {
            throw std::runtime_error("Failed to close JSON file: " + file_path);
        }
        return;
    }

    void saveXml(std::string& file_path) {
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLElement* root = doc.NewElement("Config");
        doc.InsertFirstChild(root);

        for (const auto& [key, value] : data_->values) {
            tinyxml2::XMLElement* element = doc.NewElement(key.c_str());
            element->SetText(std::any_cast<std::string>(value).c_str());
            root->InsertEndChild(element);
        }

        if (doc.SaveFile(file_path.c_str()) != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error("Failed to write XML file: " + file_path);
        }
        return;
    }

    void saveIni(std::string& file_path) {
        boost::property_tree::ptree pt;
        for (const auto& [key, value] : data_->values) {
            std::string section = key.substr(0, key.find('.'));
            std::string sub_key = key.substr(key.find('.') + 1);
            pt.put(section + "." + sub_key, std::any_cast<std::string>(value));
        }

        try {
            boost::property_tree::ini_parser::write_ini(file_path, pt);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to write INI file: " + file_path + ", " + e.what());
        }
        return;
    }

    void saveYaml(std::string& file_path) {
        YAML::Node config;
        for (const auto& [key, value] : data_->values) {
            config[key] = std::any_cast<std::string>(value);
        }

        std::ofstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open YAML file: " + file_path);
        }
        file << config;
        if (file.fail()) {
            throw std::runtime_error("Failed to write YAML file: " + file_path);
        }
        file.close();
        if (file.fail()) {
            throw std::runtime_error("Failed to close YAML file: " + file_path);
        }
        return;
    }

    void notifyChanges(const std::string& key) {
        if (!data_)
            return;

        std::lock_guard<std::mutex> lock(data_->mtx);

        for (const auto& [id, callback] : data_->callbacks) {
            try {
                callback(key);
            } catch (const std::exception& e) {
                std::cerr << "Config change callback failed: " << e.what() << std::endl;
            }
        }

        if (enable_autosave_) {
            autosave_cond_.notify_one();
        }
        return;
    }

    void autoSaveWorker() {
        std::unique_lock<std::mutex> lock(autosave_mutex_);

        while (enable_autosave_) {
            autosave_cond_.wait_for(lock, autosave_interval_,
                                    [this] { return !enable_autosave_; });

            if (!enable_autosave_)
                break;

            try {
                bool needsSave = false;
                {
                    std::lock_guard<std::mutex> dataLock(data_->mtx);
                    auto currentWriteTime = std::filesystem::last_write_time(data_->file_path);
                    needsSave = (currentWriteTime != data_->last_write_time);
                }

                if (needsSave) {
                    saveToFile();
                    std::cout << "Config auto-saved at "
                              << std::chrono::system_clock::to_time_t(
                                     std::chrono::system_clock::now())
                              << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Auto-save failed: " << e.what() << std::endl;
            }
        }
    }

private:
    struct ConfigData {
        using ptr = std::shared_ptr<ConfigData>;

        std::mutex mtx;
        std::unordered_map<std::string, std::any> values;
        std::filesystem::file_time_type last_write_time;
        std::string file_path;
        Format format;

        std::unordered_map<int, ChangeCallback> callbacks;
        int next_callback_id = 0;
    };

    ConfigData::ptr data_;

    std::atomic_bool enable_autosave_{false};
    std::thread autosave_thread_;
    std::condition_variable autosave_cond_;
    std::mutex autosave_mutex_;
    std::chrono::milliseconds autosave_interval_;
};
}  // namespace Yftp