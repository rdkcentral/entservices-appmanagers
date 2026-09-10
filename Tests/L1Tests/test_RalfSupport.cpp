#include <gtest/gtest.h>
#include "RalfSupport.h"
#include <fstream>

using namespace ralf;

TEST(ParseMemorySizeTest, Valid)
{
    EXPECT_EQ(parseMemorySize("512"), 512);
    EXPECT_EQ(parseMemorySize("1K"), 1024);
    EXPECT_EQ(parseMemorySize("1M"), 1024 * 1024);
}

TEST(ParseMemorySizeTest, Invalid)
{
    EXPECT_EQ(parseMemorySize(""), 0);
    EXPECT_EQ(parseMemorySize("abc"), 0);
}

TEST(CheckPathExistsTest, Basic)
{
    std::ofstream f("temp.txt");
    f << "data";
    f.close();

    EXPECT_TRUE(checkIfPathExists("temp.txt"));

    remove("temp.txt");
}

TEST(SerializeJsonTest, Basic)
{
    Json::Value root;
    root["key"] = "value";

    std::string out = serializeJsonNode(root);
    EXPECT_NE(out.find("key"), std::string::npos);
}
