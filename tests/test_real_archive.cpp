#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <bigx/reader.hpp>
#include <gtest/gtest.h>

// Fallback if TEST_DATA_DIR is not defined by CMake
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests"
#endif

namespace fs = std::filesystem;

// Test with a real .big archive file
// This test ensures the library can correctly extract files and match them against reference files
TEST(RealArchiveTest, ExtractAndVerifyAgainstReference) {
  // Path to the test archive and reference file
  fs::path testDataDir(TEST_DATA_DIR);
  fs::path testDir = testDataDir / "test01";
  fs::path archivePath = testDir / "FinalBIG1.big";
  fs::path referenceFile = testDir / "simple.txt";

  // Verify test files exist
  ASSERT_TRUE(fs::exists(archivePath)) << "Archive file not found: " << archivePath;
  ASSERT_TRUE(fs::exists(referenceFile)) << "Reference file not found: " << referenceFile;

  // Open the archive
  std::string error;
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;

  // List all files in the archive for debugging
  const auto &files = reader->files();
  ASSERT_FALSE(files.empty()) << "Archive contains no files";

  // Find the simple.txt file in the archive
  const big::FileEntry *targetFile = nullptr;
  for (const auto &file : files) {
    if (file.path == "simple.txt" || file.path == "Simple.txt" ||
        file.path.find("simple.txt") != std::string::npos) {
      targetFile = &file;
      break;
    }
  }

  // If not found by exact name, print all available files for debugging
  if (!targetFile) {
    std::string fileList;
    for (const auto &file : files) {
      fileList += "  - " + file.path + "\n";
    }
    FAIL() << "Could not find simple.txt in archive. Available files:\n" << fileList;
  }

  // Extract the file to memory
  auto extractedData = reader->extractToMemory(*targetFile, &error);
  ASSERT_TRUE(extractedData.has_value()) << "Failed to extract file: " << error;

  // Read the reference file
  std::ifstream refStream(referenceFile, std::ios::binary);
  ASSERT_TRUE(refStream.is_open()) << "Failed to open reference file: " << referenceFile;

  // Read reference file content
  refStream.seekg(0, std::ios::end);
  size_t refSize = refStream.tellg();
  refStream.seekg(0, std::ios::beg);

  std::vector<uint8_t> refData(refSize);
  refStream.read(reinterpret_cast<char *>(refData.data()), refSize);

  // Compare the extracted content with the reference
  ASSERT_EQ(extractedData->size(), refData.size())
      << "Extracted file size (" << extractedData->size()
      << ") does not match reference file size (" << refSize << ")";

  EXPECT_TRUE(std::equal(extractedData->begin(), extractedData->end(), refData.begin()))
      << "Extracted content does not match reference file";
}

// Test extracting to disk and verifying
TEST(RealArchiveTest, ExtractToDiskAndVerify) {
  fs::path testDataDir(TEST_DATA_DIR);
  fs::path testDir = testDataDir / "test01";
  fs::path archivePath = testDir / "FinalBIG1.big";
  fs::path referenceFile = testDir / "simple.txt";

  ASSERT_TRUE(fs::exists(archivePath)) << "Archive file not found: " << archivePath;
  ASSERT_TRUE(fs::exists(referenceFile)) << "Reference file not found: " << referenceFile;

  std::string error;
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;

  // Find the file
  const big::FileEntry *targetFile = nullptr;
  for (const auto &file : reader->files()) {
    if (file.path == "simple.txt" || file.path == "Simple.txt" ||
        file.path.find("simple.txt") != std::string::npos) {
      targetFile = &file;
      break;
    }
  }

  ASSERT_NE(targetFile, nullptr) << "simple.txt not found in archive";

  // Create temp directory for extraction
  fs::path tempDir = fs::temp_directory_path() / "big_test_real_archive";
  fs::create_directories(tempDir);

  // Extract to disk
  fs::path extractedPath = tempDir / "extracted_simple.txt";
  ASSERT_TRUE(reader->extract(*targetFile, extractedPath, &error))
      << "Failed to extract to disk: " << error;

  // Compare files byte-by-byte
  std::ifstream extractedStream(extractedPath, std::ios::binary);
  std::ifstream refStream(referenceFile, std::ios::binary);

  ASSERT_TRUE(extractedStream.is_open());
  ASSERT_TRUE(refStream.is_open());

  // Read both files
  extractedStream.seekg(0, std::ios::end);
  size_t extractedSize = extractedStream.tellg();
  extractedStream.seekg(0, std::ios::beg);

  refStream.seekg(0, std::ios::end);
  size_t refSize = refStream.tellg();
  refStream.seekg(0, std::ios::beg);

  ASSERT_EQ(extractedSize, refSize) << "File sizes don't match";

  std::vector<char> extractedContent(extractedSize);
  std::vector<char> refContent(refSize);

  extractedStream.read(extractedContent.data(), extractedSize);
  refStream.read(refContent.data(), refSize);

  EXPECT_EQ(extractedContent, refContent) << "Extracted file content doesn't match reference";

  // Explicitly close streams before cleanup (required on Windows)
  extractedStream.close();
  refStream.close();

  // Cleanup
  fs::remove_all(tempDir);
}

// Test with test02 archive (complex.docx and simple.txt)
TEST(RealArchiveTest, ExtractTest02Files) {
  fs::path testDataDir(TEST_DATA_DIR);
  fs::path testDir = testDataDir / "test02";
  fs::path archivePath = testDir / "FinalBIG1.big";
  fs::path referenceFile1 = testDir / "simple.txt";
  fs::path referenceFile2 = testDir / "complex.docx";

  ASSERT_TRUE(fs::exists(archivePath)) << "Archive file not found: " << archivePath;
  ASSERT_TRUE(fs::exists(referenceFile1)) << "Reference file not found: " << referenceFile1;
  ASSERT_TRUE(fs::exists(referenceFile2)) << "Reference file not found: " << referenceFile2;

  std::string error;
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;

  // Verify both files exist in the archive
  const big::FileEntry *simpleFile = reader->findFile("simple.txt");
  ASSERT_NE(simpleFile, nullptr) << "simple.txt not found in archive";

  const big::FileEntry *docxFile = reader->findFile("complex.docx");
  ASSERT_NE(docxFile, nullptr) << "complex.docx not found in archive";

  // Extract simple.txt and verify
  auto simpleData = reader->extractToMemory(*simpleFile, &error);
  ASSERT_TRUE(simpleData.has_value()) << "Failed to extract simple.txt: " << error;

  std::ifstream refStream1(referenceFile1, std::ios::binary);
  ASSERT_TRUE(refStream1.is_open()) << "Failed to open reference file: " << referenceFile1;

  refStream1.seekg(0, std::ios::end);
  size_t refSize1 = refStream1.tellg();
  refStream1.seekg(0, std::ios::beg);

  std::vector<uint8_t> refData1(refSize1);
  refStream1.read(reinterpret_cast<char *>(refData1.data()), refSize1);

  EXPECT_EQ(simpleData->size(), refData1.size());
  EXPECT_TRUE(std::equal(simpleData->begin(), simpleData->end(), refData1.begin()));

  // Extract complex.docx and verify
  auto docxData = reader->extractToMemory(*docxFile, &error);
  ASSERT_TRUE(docxData.has_value()) << "Failed to extract complex.docx: " << error;

  std::ifstream refStream2(referenceFile2, std::ios::binary);
  ASSERT_TRUE(refStream2.is_open()) << "Failed to open reference file: " << referenceFile2;

  refStream2.seekg(0, std::ios::end);
  size_t refSize2 = refStream2.tellg();
  refStream2.seekg(0, std::ios::beg);

  std::vector<uint8_t> refData2(refSize2);
  refStream2.read(reinterpret_cast<char *>(refData2.data()), refSize2);

  EXPECT_EQ(docxData->size(), refData2.size());
  EXPECT_TRUE(std::equal(docxData->begin(), docxData->end(), refData2.begin()));
}
