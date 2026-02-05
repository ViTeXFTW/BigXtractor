#include <filesystem>
#include <format>
#include <fstream>

#include "endian.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <gtest/gtest.h>

namespace fs = std::filesystem;

class WriterTest : public ::testing::Test {
protected:
  void SetUp() override {
    tempDir_ = fs::temp_directory_path() / "big_test_writer";
    fs::create_directories(tempDir_);
  }

  void TearDown() override { fs::remove_all(tempDir_); }

  // Create a test file with specified content
  fs::path createTestFile(const std::string &name, const std::string &content) {
    fs::path filePath = tempDir_ / name;
    std::ofstream file(filePath);
    file.write(content.data(), content.size());
    return filePath;
  }

  fs::path tempDir_;
};

// Test creating an archive with files from disk
TEST_F(WriterTest, CreateArchiveFromDisk) {
  // Create test files
  createTestFile("file1.txt", "Hello, World!");
  createTestFile("file2.dat", "BinaryData");

  big::Writer writer;
  std::string error;

  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "data/file1.txt", &error)) << error;
  ASSERT_TRUE(writer.addFile(tempDir_ / "file2.dat", "data/file2.dat", &error)) << error;

  fs::path archivePath = tempDir_ / "output.big";
  ASSERT_TRUE(writer.write(archivePath, &error)) << error;

  // Verify archive can be read
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  EXPECT_EQ(reader->fileCount(), 2);

  const auto *file1 = reader->findFile("data/file1.txt");
  ASSERT_NE(file1, nullptr);
  EXPECT_EQ(file1->size, 13);

  const auto *file2 = reader->findFile("data/file2.dat");
  ASSERT_NE(file2, nullptr);
  EXPECT_EQ(file2->size, 10);
}

// Test creating an archive with files from memory
TEST_F(WriterTest, CreateArchiveFromMemory) {
  std::vector<uint8_t> data1 = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
  std::vector<uint8_t> data2 = {0, 1, 2, 3, 4};

  big::Writer writer;
  std::string error;

  ASSERT_TRUE(writer.addFile(data1, "test/file1.bin", &error)) << error;
  ASSERT_TRUE(writer.addFile(data2, "test/file2.bin", &error)) << error;

  fs::path archivePath = tempDir_ / "memory.big";
  ASSERT_TRUE(writer.write(archivePath, &error)) << error;

  // Verify archive can be read
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  EXPECT_EQ(reader->fileCount(), 2);

  const auto *file1 = reader->findFile("test/file1.bin");
  ASSERT_NE(file1, nullptr);

  auto content = reader->extractToMemory(*file1, &error);
  ASSERT_TRUE(content.has_value()) << error;
  EXPECT_EQ(*content, data1);
}

// Test duplicate file detection
TEST_F(WriterTest, DuplicateFileDetection) {
  createTestFile("file1.txt", "Content");

  big::Writer writer;
  std::string error;

  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "data/file.txt", &error)) << error;

  // Try to add the same path again
  EXPECT_FALSE(writer.addFile(tempDir_ / "file1.txt", "data/file.txt", &error));
  EXPECT_FALSE(error.empty());

  // Case-insensitive duplicate
  EXPECT_FALSE(writer.addFile(tempDir_ / "file1.txt", "DATA/FILE.TXT", &error));
}

// Test empty archive
TEST_F(WriterTest, EmptyArchive) {
  big::Writer writer;
  std::string error;

  fs::path archivePath = tempDir_ / "empty.big";
  EXPECT_FALSE(writer.write(archivePath, &error));
  EXPECT_TRUE(error.empty() || error.find("no files") != std::string::npos);
}

// Test non-existent source file
TEST_F(WriterTest, NonExistentSourceFile) {
  big::Writer writer;
  std::string error;

  EXPECT_FALSE(writer.addFile(tempDir_ / "does_not_exist.txt", "data/file.txt", &error));
  EXPECT_FALSE(error.empty());
}

// Test path normalization
TEST_F(WriterTest, PathNormalization) {
  createTestFile("file1.txt", "Content");

  big::Writer writer;
  std::string error;

  // Add with backslashes
  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "data\\subdir\\file.txt", &error)) << error;

  fs::path archivePath = tempDir_ / "paths.big";
  ASSERT_TRUE(writer.write(archivePath, &error)) << error;

  // Verify paths are normalized in the archive
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  const auto *file = reader->findFile("data/subdir/file.txt");
  ASSERT_NE(file, nullptr);
}

// Test clear functionality
TEST_F(WriterTest, Clear) {
  createTestFile("file1.txt", "Content");

  big::Writer writer;
  std::string error;

  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "data/file.txt", &error)) << error;
  EXPECT_EQ(writer.fileCount(), 1);

  writer.clear();
  EXPECT_EQ(writer.fileCount(), 0);
}

// Test move semantics
TEST_F(WriterTest, MoveConstruction) {
  createTestFile("file1.txt", "Content");

  big::Writer writer1;
  std::string error;
  ASSERT_TRUE(writer1.addFile(tempDir_ / "file1.txt", "data/file.txt", &error)) << error;

  big::Writer writer2(std::move(writer1));
  EXPECT_EQ(writer2.fileCount(), 1);
}

// Test round-trip (write then read)
TEST_F(WriterTest, RoundTrip) {
  // Create test files with various content
  createTestFile("file1.txt", "Hello, World!");
  createTestFile("file2.txt", "Another file");

  big::Writer writer;
  std::string error;

  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "original/file1.txt", &error)) << error;
  ASSERT_TRUE(writer.addFile(tempDir_ / "file2.txt", "original/file2.txt", &error)) << error;

  fs::path archivePath = tempDir_ / "roundtrip.big";
  ASSERT_TRUE(writer.write(archivePath, &error)) << error;

  // Read back
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << error;

  // Extract and verify
  fs::path extractPath1 = tempDir_ / "extracted1.txt";
  fs::path extractPath2 = tempDir_ / "extracted2.txt";

  const auto *file1 = reader->findFile("original/file1.txt");
  const auto *file2 = reader->findFile("original/file2.txt");

  ASSERT_NE(file1, nullptr);
  ASSERT_NE(file2, nullptr);

  ASSERT_TRUE(reader->extract(*file1, extractPath1, &error)) << error;
  ASSERT_TRUE(reader->extract(*file2, extractPath2, &error)) << error;

  // Verify content
  std::ifstream extracted1(extractPath1);
  std::string content1((std::istreambuf_iterator<char>(extracted1)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(content1, "Hello, World!");

  std::ifstream extracted2(extractPath2);
  std::string content2((std::istreambuf_iterator<char>(extracted2)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(content2, "Another file");
}

// Test archive header
TEST_F(WriterTest, ArchiveHeader) {
  createTestFile("file1.txt", "X");

  big::Writer writer;
  std::string error;
  ASSERT_TRUE(writer.addFile(tempDir_ / "file1.txt", "f.txt", &error)) << error;

  fs::path archivePath = tempDir_ / "header.big";
  ASSERT_TRUE(writer.write(archivePath, &error)) << error;

  // Read raw file to verify header
  std::ifstream archiveFile(archivePath, std::ios::binary);
  char magic[4];
  archiveFile.read(magic, 4);
  EXPECT_EQ(std::string(magic, 4), "BIGF");

  uint32_t archiveSize, fileCount, padding;
  archiveFile.read(reinterpret_cast<char *>(&archiveSize), 4);
  archiveFile.read(reinterpret_cast<char *>(&fileCount), 4);
  archiveFile.read(reinterpret_cast<char *>(&padding), 4);

  EXPECT_EQ(padding, 0);
  EXPECT_EQ(big::betoh32(fileCount), 1); // Convert from big-endian
}
