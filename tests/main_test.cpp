#include "../libs/include/Pack.hpp"
#include <gtest/gtest.h>
#include <fstream>

using namespace K;

class PackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test files
        std::ofstream test_file("test_input.txt");
        test_file << "Hello, World!";
        test_file.close();

        std::ofstream test_file2("test_input2.txt");
        test_file2 << "Different content";
        test_file2.close();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove("test_input.txt");
        std::filesystem::remove("test_input2.txt");
        std::filesystem::remove("test_output.txt");
    }
};

TEST_F(PackTest, FileExistsTest) {
    EXPECT_TRUE(Pack::exists("test_input.txt"));
    EXPECT_FALSE(Pack::exists("nonexistent.txt"));
}

TEST_F(PackTest, HashComparisonTest) {
    const auto hash1 = Pack::hash("test_input.txt");
    const auto hash2 = Pack::hash("test_input.txt");
    const auto hash3 = Pack::hash("test_input2.txt");
    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, hash3);
}

TEST_F(PackTest, FileCopyTest) {
    Pack::copy("test_output.txt", "test_input.txt");
    EXPECT_TRUE(Pack::exists("test_output.txt"));

    const auto source_hash = Pack::hash("test_input.txt");
    const auto dest_hash = Pack::hash("test_output.txt");
    EXPECT_EQ(source_hash, dest_hash);
}

TEST_F(PackTest, HexConversionTest) {
    const std::vector<uint8_t> data = {0x00, 0xFF, 0xA5, 0x5A};
    const std::string hex = Pack::to_hex(data.data(), data.size());
    EXPECT_EQ(hex, "00ffa55a");
}
