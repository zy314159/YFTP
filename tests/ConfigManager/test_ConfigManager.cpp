#include <gtest/gtest.h>
#include "../../ConfigManager.hpp"

const char* g_config_file = "/home/zhangyang/Yftp/config.json";

TEST(ConfigManagerTest, CONTAIN){
    Yftp::ConfigManager::getInstance().load(g_config_file);

    EXPECT_TRUE(Yftp::ConfigManager::getInstance().contains("max_asynclog_que_size"));
    EXPECT_TRUE(Yftp::ConfigManager::getInstance().contains("max_asynclog_worker_thread"));
}

TEST(ConfigManagerTest, GET){
    Yftp::ConfigManager::getInstance().load(g_config_file);

    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_asynclog_que_size"), 8192);
    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_asynclog_worker_thread"), 1);
}


int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    int results = RUN_ALL_TESTS();
    return results;
}