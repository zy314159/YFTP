#include "../../CommandParser.hpp"
#include "../../FileUtil.hpp"
#include <gtest/gtest.h>
#include <string>



TEST(CommandParserTest, REGISTER_LIST){
    Yftp::CommandParser parser;
    parser.registerCommand("list", [](std::vector<std::string>& args) {
        if (args.size() != 1) {
            std::cerr << "Usage: list <directory>" << std::endl;
            return;
        }
        std::string path = args[0];
        std::string result = Yftp::FileUtil::getList(path);
        std::cout << result << std::endl;
    });

    std::string command = "list /home/zhangyang/";
    EXPECT_NO_THROW(parser.parse(command));
}

TEST(CommandParserTest, REGISTER_MKDIR){
    Yftp::CommandParser parser;
    parser.registerCommand("mkdir", [](std::vector<std::string>& args) {
        if (args.size() != 1) {
            std::cerr << "Usage: mkdir <directory>" << std::endl;
            return;
        }
        std::string path = args[0];
        Yftp::FileUtil::createDirectory(path);
    });

    std::string command = "mkdir /home/zhangyang/test_dir";
    parser.parse(command);
    EXPECT_TRUE(Yftp::FileUtil::directoryExists("/home/zhangyang/test_dir"));
}

TEST(CommandParserTest, REGISTER_RMDIR){
    Yftp::CommandParser parser;
    parser.registerCommand("rmdir", [](std::vector<std::string>& args) {
        if (args.size() != 1) {
            std::cerr << "Usage: rmdir <directory>" << std::endl;
            return;
        }
        std::string path = args[0];
        Yftp::FileUtil::removeDirectory(path);
    });

    std::string command = "rmdir /home/zhangyang/test_dir";
    parser.parse(command);
    EXPECT_FALSE(Yftp::FileUtil::directoryExists("/home/zhangyang/test_dir"));
}

int main(){
    testing::InitGoogleTest();
    int result = RUN_ALL_TESTS();
    return result;
}