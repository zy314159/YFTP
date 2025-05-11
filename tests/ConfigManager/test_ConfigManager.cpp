#include <gtest/gtest.h>

#include "../../ConfigManager.hpp"

const char* g_config_file = "/home/zhangyang/Yftp/config.json";

TEST(ConfigManagerTest, CONTAIN) {
    Yftp::ConfigManager::getInstance().load(g_config_file);

    EXPECT_TRUE(Yftp::ConfigManager::getInstance().contains("max_asynclog_que_size"));
    EXPECT_TRUE(Yftp::ConfigManager::getInstance().contains("max_asynclog_worker_thread"));
}

TEST(ConfigManagerTest, GET) {
    Yftp::ConfigManager::getInstance().load(g_config_file);

    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_asynclog_que_size"), 8192);
    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_asynclog_worker_thread"), 1);
}

TEST(ConfigManagerTest, SET) {
    Yftp::ConfigManager::getInstance().load(g_config_file);

    Yftp::ConfigManager::getInstance().set("max_time_out", 60);
    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_time_out"), 60);
}

TEST(ConfigManagerTest, SAVE){
    Yftp::ConfigManager::getInstance().load(g_config_file);

    Yftp::ConfigManager::getInstance().set("max_time_out", 60);
    Yftp::ConfigManager::getInstance().saveToFile();

    Yftp::ConfigManager::getInstance().reset(g_config_file);
    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_time_out"), 60);
}

TEST(ConfigManagerTest, AUTO_SAVE) {
    Yftp::ConfigManager::getInstance().load(g_config_file);

    Yftp::ConfigManager::getInstance().enableAutoSave(true, 1000);
    Yftp::ConfigManager::getInstance().set("max_time_out", 120);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    Yftp::ConfigManager::getInstance().reset(g_config_file);
    EXPECT_EQ(Yftp::ConfigManager::getInstance().get<int>("max_time_out"), 120);
    Yftp::ConfigManager::getInstance().enableAutoSave(false);
    Yftp::ConfigManager::getInstance().clear();
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    int results = RUN_ALL_TESTS();
    return results;
}