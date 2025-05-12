#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>

namespace po = boost::program_options;

namespace Yftp {

class CommandParser {
public:
    struct CommandOptions {
        std::string description;
        std::vector<std::pair<std::string, std::string>> options;  // <option_name, description>
        std::vector<std::string> positional_args;                  // <arg_name>
    };

    struct CommandResult {
        std::string command;
        std::unordered_map<std::string, std::string> options;
        std::vector<std::string> arguments;
        bool is_valid{false};
        std::string error_message;
    };

    CommandParser() {
        global_options_.add_options()("help,h", "Display help information");
        addCommand("HELP", {"Display help information"}, [this](const CommandResult& result) {
            std::cout << getHelp() << std::endl;
        });
    }

    CommandResult parse(const std::string& input) {
        CommandResult result;

        if (input.empty()) {
            result.error_message = "Empty command";
            return result;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, input, boost::is_space(), boost::token_compress_on);

        if (tokens.empty()) {
            result.error_message = "No command specified";
            return result;
        }

        std::string command = boost::to_upper_copy(tokens[0]);
        tokens.erase(tokens.begin());

        return parseCommand(command, tokens);
    }

    std::string getHelp() const {
        std::stringstream ss;
        ss << "Available commands: \n";
        for(const auto& cmd : commands_){
            ss << "\n" << cmd.first << ":\n";
            ss << *cmd.second.options_desc << "\n";
        }
        return ss.str();
    }

    void execute(const CommandResult& result){
        if(!result.is_valid){
            throw std::runtime_error("Cannot execute invalid command: " + result.error_message);
        }

        auto it = commands_.find(result.command);
        if(it != commands_.end() && it->second.handler){
            it->second.handler(result);
        } else {
            throw std::runtime_error("No handler for command: " + result.command);
        }
        return;
    }

    void addCommand(const std::string& name, const CommandOptions& options,
                    std::function<void(const CommandResult&)> handler = nullptr) {
        std::string upper_name = boost::to_upper_copy(name);

        auto desc = std::make_shared<po::options_description>(options.description);
        for (const auto& opt : options.options) {
            std::vector<std::string> parts;
            boost::split(parts, opt.first, boost::is_any_of(","));

            if (parts.size() == 2) {
                desc->add_options()(parts[0].c_str(), opt.second.c_str());
                desc->add_options()(parts[1].c_str(), opt.second.c_str());
            } else {
                desc->add_options()(parts[0].c_str(), opt.second.c_str());
            }
        }

        auto pos_desc = std::make_shared<po::positional_options_description>();
        for (std::size_t i = 0; i < options.positional_args.size(); i++) {
            pos_desc->add(options.positional_args[i].c_str(), 1);
        }
        commands_[upper_name] = {desc, pos_desc, std::move(handler)};
        return;
    }

private:
    void initializeParsers();

    CommandResult parseCommand(const std::string& command, const std::vector<std::string>& tokens) {
        CommandResult result;
        result.command = command;

        auto it = commands_.find(command);
        if (it == commands_.end()) {
            result.error_message = "Unknown command: " + command;
            return result;
        }

        try {
            po::variables_map vm;
            auto parser = po::command_line_parser(tokens)
                              .options(*(it->second.options_desc))
                              .positional(*(it->second.positional_desc))
                              .run();
            po::store(parser, vm);
            po::notify(vm);

            for (const auto& opt : it->second.options_desc->options()) {
                if (vm.count(opt->long_name())) {
                    if (opt->semantic()->is_required() || vm[opt->long_name()].defaulted()) {
                        try {
                            result.options[opt->long_name()] =
                                vm[opt->long_name()].as<std::string>();
                        } catch (...) {
                            result.options[opt->long_name()] = "true";
                        }
                    }
                }
            }

            for (std::size_t i = 0; i < it->second.positional_desc->max_total_count(); i++) {
                const auto& name = it->second.positional_desc->name_for_position(i);
                if (vm.count(name)) {
                    result.arguments.push_back(vm[name].as<std::string>());
                }

                result.is_valid = true;
            }

        } catch (const po::error& e) {
            result.error_message = e.what();
        } catch (std::exception& e) {
            result.error_message = e.what();
        } catch (...) {
            result.error_message = "Unknown error";
        }
        return result;
    }

    using CommandHandler = std::function<void(const CommandResult&)>;

    struct CommandInfo {
        std::shared_ptr<po::options_description> options_desc;
        std::shared_ptr<po::positional_options_description> positional_desc;
        CommandHandler handler;
    };

private:
    std::unordered_map<std::string, CommandInfo> commands_;
    po::options_description global_options_;
};

}  // namespace Yftp