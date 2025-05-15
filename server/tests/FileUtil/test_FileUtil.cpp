#include "../../FileUtil.hpp"
#include <gtest/gtest.h>

TEST(FileUtilTest, LIST_DIRECTORY) {
    std::string path = "/home/zhangyang/";
    std::string result = Yftp::FileUtil::getList(path);
    std::cout << result << std::endl;
    EXPECT_FALSE(result.empty());
}

TEST(FileUtilTest, FILE_EXISTS) {
    std::string file_path = "/home/zhangyang/test.txt";
    EXPECT_FALSE(Yftp::FileUtil::fileExists(file_path));
    EXPECT_TRUE(Yftp::FileUtil::createFile(file_path));
    EXPECT_TRUE(Yftp::FileUtil::fileExists(file_path));
    EXPECT_TRUE(Yftp::FileUtil::removeFile(file_path));
    EXPECT_FALSE(Yftp::FileUtil::fileExists(file_path));
}

TEST(FileUtilTest, GET_FILE_CONTENT) {
    std::string file_path = "/home/zhangyang/test.txt";
    std::string content = "Hello, World!";
    EXPECT_TRUE(Yftp::FileUtil::writeFile(file_path, content));
    EXPECT_EQ(Yftp::FileUtil::readFile(file_path), content);
    EXPECT_TRUE(Yftp::FileUtil::removeFile(file_path));
}

TEST(FileUtilTest, CREATE_DIRECTORY) {
    std::string dir_path = "/home/zhangyang/test_dir";
    EXPECT_TRUE(Yftp::FileUtil::createDirectory(dir_path));
    EXPECT_TRUE(Yftp::FileUtil::directoryExists(dir_path));
    EXPECT_TRUE(Yftp::FileUtil::removeDirectory(dir_path));
}


TEST(FileUtilTest, CREATE_MULTI_LEVEL_DIRECTORY) {
    std::string dir_path = "/home/zhangyang/test_dir/sub_dir";
    EXPECT_TRUE(Yftp::FileUtil::createDirectory(dir_path));
    EXPECT_TRUE(Yftp::FileUtil::directoryExists(dir_path));
    EXPECT_TRUE(Yftp::FileUtil::removeDirectory("/home/zhangyang/test_dir"));
}

int main(){
    testing::InitGoogleTest();
    int result = RUN_ALL_TESTS();
    return result;
}