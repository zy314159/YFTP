#pragma once
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <functional>
#include <string>
#include <iostream>


namespace Yftp {

class CommandParser {
public:
    using CommandHandler = std::function<void(std::vector<std::string>&)>;

    void registerCommand(const std::string& command, CommandHandler handler) {
        command_map_[command] = handler;
    }

    void parse(const std::string& input) {
        std::vector<std::string> tokens;
        boost::split(tokens, input, boost::is_space(), boost::token_compress_on);
        if (tokens.empty())
            return;
        const std::string& command = tokens[0];
        auto it = command_map_.find(command);
        if (it != command_map_.end()) {
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());
            tokens.clear();
            it->second(args);
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
        return;
    }

private:
    std::unordered_map<std::string, CommandHandler> command_map_;
};
};  // namespace Yftp