#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

#include <bigx/endian.hpp>
#include <bigx/reader.hpp>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

class ReaderTest : public ::testing::Test {
protected:
  void SetUp() override {
    tempDir_ = fs::temp_directory_path() / "big_test_reader";
    fs::create_directories(tempDir_);
  }

  void TearDown() override { fs::remove_all(tempDir_); }

  // Create a minimal valid BIG archive for testing
  fs::path createTestArchive(const std::string &name) {
    fs::path filePath = tempDir_ / name;

    // Create a simple BIG archive with a few test files
    std::ofstream file(filePath, std::ios::binary);

    // Header: "BIGF" + archiveSize (4) + fileCount (4) + padding (4)
    uint32_t fileCount = 3; // 3 test files
    uint32_t headerSize = 16;

    // Calculate sizes for directory
    std::vector<std::string> paths = {"test/file1.txt", "test/file2.dat", "test/subdir/file3.bin"};

    std::vector<std::vector<uint8_t>> fileContents = {
        {'H', 'e', 'l', 'l', 'o'},
        {0, 1, 2, 3, 4, 5},
        {'A', 'B', 'C'}
    };

    // Calculate directory size
    size_t directorySize = 0;
    for (const auto &path : paths) {
      directorySize += 8 + path.size() + 1; // offset + size + null-terminated path
    }

    // Calculate file data size
    size_t fileDataSize = 0;
    for (const auto &content : fileContents) {
      fileDataSize += content.size();
    }

    uint32_t totalSize = headerSize + static_cast<uint32_t>(directorySize + fileDataSize);

    // Convert to big-endian
    uint32_t totalSizeBE = bigx::htobe32(totalSize);
    uint32_t fileCountBE = bigx::htobe32(fileCount);

    // Write header
    file.write("BIGF", 4);
    file.write(reinterpret_cast<const char *>(&totalSizeBE), 4);
    file.write(reinterpret_cast<const char *>(&fileCountBE), 4);
    uint32_t padding = 0;
    file.write(reinterpret_cast<const char *>(&padding), 4);

    // Write directory entries
    size_t fileOffset = headerSize + directorySize;
    for (size_t i = 0; i < paths.size(); ++i) {
      uint32_t offset = static_cast<uint32_t>(fileOffset);
      uint32_t size = static_cast<uint32_t>(fileContents[i].size());

      // Convert to big-endian
      uint32_t offsetBE = bigx::htobe32(offset);
      uint32_t sizeBE = bigx::htobe32(size);

      // Write offset and size (big-endian)
      file.write(reinterpret_cast<const char *>(&offsetBE), 4);
      file.write(reinterpret_cast<const char *>(&sizeBE), 4);
      // Write path
      file.write(paths[i].c_str(), paths[i].size() + 1);

      fileOffset += fileContents[i].size();
    }

    // Write file data
    for (const auto &content : fileContents) {
      file.write(reinterpret_cast<const char *>(content.data()), content.size());
    }

    return filePath;
  }

  fs::path tempDir_;
};

// Test opening a valid archive
TEST_F(ReaderTest, OpenValidArchive) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);

  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;
  EXPECT_TRUE(reader->isOpen());
  EXPECT_EQ(reader->fileCount(), 3);
}

// Test file listing
TEST_F(ReaderTest, FileListing) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  const auto &files = reader->files();
  EXPECT_EQ(files.size(), 3);

  // Check file paths are normalized (forward slashes, lowercase)
  EXPECT_EQ(files[0].path, "test/file1.txt");
  EXPECT_EQ(files[1].path, "test/file2.dat");
  EXPECT_EQ(files[2].path, "test/subdir/file3.bin");
}

// Test case-insensitive file lookup
TEST_F(ReaderTest, FileLookup) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  // Test various case combinations
  const auto *file1 = reader->findFile("test/file1.txt");
  ASSERT_NE(file1, nullptr);
  EXPECT_EQ(file1->size, 5);

  const auto *file2 = reader->findFile("TEST/FILE1.TXT"); // Uppercase
  ASSERT_NE(file2, nullptr);
  EXPECT_EQ(file2->size, 5);

  const auto *file3 = reader->findFile("TeSt/FiLe1.TxT"); // Mixed case
  ASSERT_NE(file3, nullptr);
  EXPECT_EQ(file3->size, 5);

  const auto *file4 = reader->findFile("test\\file1.txt"); // Backslash
  ASSERT_NE(file4, nullptr);

  // Test non-existent file
  const auto *file5 = reader->findFile("does/not/exist.txt");
  EXPECT_EQ(file5, nullptr);
}

// Test file view
TEST_F(ReaderTest, FileView) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  const auto *file = reader->findFile("test/file1.txt");
  ASSERT_NE(file, nullptr);

  auto view = reader->getFileView(*file);
  EXPECT_EQ(view.size(), 5);
  EXPECT_EQ(view[0], 'H');
  EXPECT_EQ(view[1], 'e');
  EXPECT_EQ(view[2], 'l');
  EXPECT_EQ(view[3], 'l');
  EXPECT_EQ(view[4], 'o');
}

// Test extract to memory
TEST_F(ReaderTest, ExtractToMemory) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  const auto *file = reader->findFile("test/file2.dat");
  ASSERT_NE(file, nullptr);

  auto data = reader->extractToMemory(*file, &error);
  ASSERT_TRUE(data.has_value()) << error;
  EXPECT_EQ(data->size(), 6);
  EXPECT_EQ((*data)[0], 0);
  EXPECT_EQ((*data)[1], 1);
  EXPECT_EQ((*data)[2], 2);
}

// Test extract to disk
TEST_F(ReaderTest, ExtractToDisk) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  const auto *file = reader->findFile("test/file1.txt");
  ASSERT_NE(file, nullptr);

  fs::path destPath = tempDir_ / "extracted.txt";
  ASSERT_TRUE(reader->extract(*file, destPath, &error)) << error;

  // Verify extracted file
  std::ifstream extracted(destPath, std::ios::binary);
  std::vector<char> content(5);
  extracted.read(content.data(), 5);
  EXPECT_EQ(std::string(content.data(), 5), "Hello");
}

// Test opening invalid archive
TEST_F(ReaderTest, InvalidArchive) {
  fs::path invalidPath = tempDir_ / "invalid.big";

  // Create a file with invalid magic
  std::ofstream file(invalidPath, std::ios::binary);
  file.write("INVALID", 7);

  std::string error;
  auto reader = bigx::Reader::open(invalidPath, &error);

  EXPECT_FALSE(reader.has_value());
  EXPECT_FALSE(error.empty());
}

// Test opening non-existent file
TEST_F(ReaderTest, NonExistentFile) {
  fs::path nonExistent = tempDir_ / "does_not_exist.big";

  std::string error;
  auto reader = bigx::Reader::open(nonExistent, &error);

  EXPECT_FALSE(reader.has_value());
  EXPECT_FALSE(error.empty());
}

// Test move semantics
TEST_F(ReaderTest, MoveConstruction) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader1 = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader1.has_value()) << error;

  size_t fileCount = reader1->fileCount();

  bigx::Reader reader2(std::move(*reader1));
  EXPECT_EQ(reader2.fileCount(), fileCount);

  const auto *file = reader2.findFile("test/file1.txt");
  EXPECT_NE(file, nullptr);
}

// Test close
TEST_F(ReaderTest, Close) {
  fs::path archivePath = createTestArchive("test.big");

  std::string error;
  auto reader = bigx::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  EXPECT_TRUE(reader->isOpen());
  reader->close();
  EXPECT_FALSE(reader->isOpen());
}
