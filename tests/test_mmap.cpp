#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <vector>

#include <bigx/mmap.hpp>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

class MappedFileTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory for tests
    tempDir_ = fs::temp_directory_path() / "big_test_mmap";
    fs::create_directories(tempDir_);
  }

  void TearDown() override {
    // Clean up temporary directory
    fs::remove_all(tempDir_);
  }

  // Create a test file with specified content
  fs::path createTestFile(const std::string &name, const std::vector<uint8_t> &content) {
    fs::path filePath = tempDir_ / name;
    std::ofstream file(filePath, std::ios::binary);
    file.write(reinterpret_cast<const char *>(content.data()), content.size());
    return filePath;
  }

  fs::path tempDir_;
};

// Test opening and reading a file
TEST_F(MappedFileTest, OpenRead) {
  std::vector<uint8_t> content = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
  fs::path filePath = createTestFile("test_read.bin", content);

  bigx::MappedFile mappedFile;
  std::string error;
  ASSERT_TRUE(mappedFile.openRead(filePath, &error)) << error;
  EXPECT_TRUE(mappedFile.isOpen());
  EXPECT_EQ(mappedFile.size(), content.size());

  auto data = mappedFile.data();
  ASSERT_EQ(data.size(), content.size());

  for (size_t i = 0; i < content.size(); ++i) {
    EXPECT_EQ(data[i], content[i]) << "Mismatch at index " << i;
  }

  mappedFile.close();
  EXPECT_FALSE(mappedFile.isOpen());
}

// Test opening non-existent file
TEST_F(MappedFileTest, OpenNonExistent) {
  bigx::MappedFile mappedFile;
  std::string error;
  fs::path filePath = tempDir_ / "does_not_exist.bin";

  EXPECT_FALSE(mappedFile.openRead(filePath, &error));
  EXPECT_FALSE(mappedFile.isOpen());
  EXPECT_FALSE(error.empty());
}

// Test opening empty file
TEST_F(MappedFileTest, OpenEmptyFile) {
  fs::path filePath = tempDir_ / "empty.bin";
  std::ofstream(filePath).close(); // Create empty file

  bigx::MappedFile mappedFile;
  std::string error;

  EXPECT_FALSE(mappedFile.openRead(filePath, &error));
  EXPECT_FALSE(mappedFile.isOpen());
}

// Test creating and writing to a mapped file
TEST_F(MappedFileTest, OpenWrite) {
  fs::path filePath = tempDir_ / "test_write.bin";
  size_t fileSize = 1024;

  bigx::MappedFile mappedFile;
  std::string error;
  ASSERT_TRUE(mappedFile.openWrite(filePath, fileSize, &error)) << error;
  EXPECT_TRUE(mappedFile.isOpen());
  EXPECT_EQ(mappedFile.size(), fileSize);

  // Write some data
  auto data = mappedFile.data();
  for (size_t i = 0; i < fileSize; ++i) {
    data[i] = static_cast<uint8_t>(i & 0xFF);
  }

  // Flush
  ASSERT_TRUE(mappedFile.flush(&error)) << error;
  mappedFile.close();

  // Verify file was written correctly
  std::ifstream verifyFile(filePath, std::ios::binary);
  std::vector<uint8_t> verifyData(fileSize);
  verifyFile.read(reinterpret_cast<char *>(verifyData.data()), fileSize);

  for (size_t i = 0; i < fileSize; ++i) {
    EXPECT_EQ(verifyData[i], static_cast<uint8_t>(i & 0xFF)) << "Mismatch at index " << i;
  }
}

// Test creating zero-size file
TEST_F(MappedFileTest, OpenWriteZeroSize) {
  fs::path filePath = tempDir_ / "zero_size.bin";

  bigx::MappedFile mappedFile;
  std::string error;

  EXPECT_FALSE(mappedFile.openWrite(filePath, 0, &error));
  EXPECT_FALSE(mappedFile.isOpen());
}

// Test move semantics
TEST_F(MappedFileTest, MoveConstruction) {
  std::vector<uint8_t> content = {1, 2, 3, 4, 5};
  fs::path filePath = createTestFile("test_move.bin", content);

  bigx::MappedFile mappedFile1;
  std::string error;
  ASSERT_TRUE(mappedFile1.openRead(filePath, &error)) << error;

  // Move construct
  bigx::MappedFile mappedFile2(std::move(mappedFile1));

  EXPECT_FALSE(mappedFile1.isOpen());
  EXPECT_TRUE(mappedFile2.isOpen());

  auto data = mappedFile2.data();
  ASSERT_EQ(data.size(), content.size());
  for (size_t i = 0; i < content.size(); ++i) {
    EXPECT_EQ(data[i], content[i]);
  }
}

TEST_F(MappedFileTest, MoveAssignment) {
  std::vector<uint8_t> content1 = {1, 2, 3};
  std::vector<uint8_t> content2 = {4, 5, 6};
  fs::path filePath1 = createTestFile("test_move1.bin", content1);
  fs::path filePath2 = createTestFile("test_move2.bin", content2);

  bigx::MappedFile mappedFile1;
  bigx::MappedFile mappedFile2;
  std::string error;

  ASSERT_TRUE(mappedFile1.openRead(filePath1, &error)) << error;
  ASSERT_TRUE(mappedFile2.openRead(filePath2, &error)) << error;

  // Move assign
  mappedFile1 = std::move(mappedFile2);

  EXPECT_TRUE(mappedFile1.isOpen());
  EXPECT_FALSE(mappedFile2.isOpen());

  auto data = mappedFile1.data();
  ASSERT_EQ(data.size(), content2.size());
  for (size_t i = 0; i < content2.size(); ++i) {
    EXPECT_EQ(data[i], content2[i]);
  }
}

// Test large file (1 MB)
TEST_F(MappedFileTest, LargeFile) {
  size_t fileSize = 1024 * 1024; // 1 MB
  std::vector<uint8_t> content(fileSize);

  // Fill with random-like data
  for (size_t i = 0; i < fileSize; ++i) {
    content[i] = static_cast<uint8_t>(i * 7 % 256);
  }

  fs::path filePath = createTestFile("large.bin", content);

  bigx::MappedFile mappedFile;
  std::string error;
  ASSERT_TRUE(mappedFile.openRead(filePath, &error)) << error;

  EXPECT_EQ(mappedFile.size(), fileSize);

  auto data = mappedFile.data();
  ASSERT_EQ(data.size(), fileSize);

  // Spot check some values
  EXPECT_EQ(data[0], content[0]);
  EXPECT_EQ(data[fileSize / 2], content[fileSize / 2]);
  EXPECT_EQ(data[fileSize - 1], content[fileSize - 1]);
}
