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

// Test that extracts all W3D files from W3DZH.big and verifies they match reference files
TEST(W3DArchiveTest, ExtractAllW3DFilesAndVerify) {
  // Path to the test archive and reference directory
  fs::path testDataDir(TEST_DATA_DIR);
  fs::path testDir = testDataDir / "test03";
  fs::path archivePath = testDir / "W3DZH.big";

  // Check if test data exists - skip gracefully if not (files too large for git)
  if (!fs::exists(archivePath)) {
    std::cout << "\n[SKIPPED] W3D archive not found: " << archivePath
              << "\n         (W3D files are too large for git - test requires local test data)\n\n";
    GTEST_SKIP() << "W3D archive not found (test data not present)";
    return;
  }

  // Open the archive
  std::string error;
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;

  // Get all files from the archive
  const auto &files = reader->files();
  ASSERT_FALSE(files.empty()) << "Archive contains no files";

  // Count W3D files and verify each one
  int w3dCount = 0;
  int verifiedCount = 0;
  int missingReference = 0;
  int mismatchCount = 0;

  for (const auto &file : files) {
    // Check if this is a W3D file
    if (file.path.size() < 4 ||
        file.path.substr(file.path.size() - 4) != ".W3D") {
      continue;
    }

    w3dCount++;

    // Get the filename from the path (handle possible directory structure)
    std::string filename = file.path;
    size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != std::string::npos) {
      filename = filename.substr(lastSlash + 1);
    }
    // Also check for backslashes
    lastSlash = filename.find_last_of('\\');
    if (lastSlash != std::string::npos) {
      filename = filename.substr(lastSlash + 1);
    }

    fs::path referenceFile = testDir / filename;

    // Check if reference file exists
    if (!fs::exists(referenceFile)) {
      missingReference++;
      continue;
    }

    // Extract the file to memory
    auto extractedData = reader->extractToMemory(file, &error);
    ASSERT_TRUE(extractedData.has_value())
        << "Failed to extract " << filename << ": " << error;

    // Read the reference file
    std::ifstream refStream(referenceFile, std::ios::binary);
    ASSERT_TRUE(refStream.is_open())
        << "Failed to open reference file: " << referenceFile;

    refStream.seekg(0, std::ios::end);
    size_t refSize = refStream.tellg();
    refStream.seekg(0, std::ios::beg);

    std::vector<uint8_t> refData(refSize);
    refStream.read(reinterpret_cast<char *>(refData.data()), refSize);

    // Compare sizes first
    if (extractedData->size() != refData.size()) {
      mismatchCount++;
      ADD_FAILURE() << "File " << filename << " size mismatch: extracted "
                    << extractedData->size() << " bytes, reference " << refSize
                    << " bytes";
      continue;
    }

    // Compare content
    if (!std::equal(extractedData->begin(), extractedData->end(), refData.begin())) {
      mismatchCount++;
      ADD_FAILURE() << "File " << filename << " content mismatch";
      continue;
    }

    verifiedCount++;
  }

  // Report summary
  std::cout << "\n=== W3D Archive Test Summary ===\n";
  std::cout << "Total W3D files in archive: " << w3dCount << "\n";
  std::cout << "Verified against reference: " << verifiedCount << "\n";
  std::cout << "Missing reference files: " << missingReference << "\n";
  std::cout << "Mismatched files: " << mismatchCount << "\n";
  std::cout << "===============================\n\n";

  // Assert that we found and verified W3D files
  EXPECT_GT(w3dCount, 0) << "No W3D files found in archive";
  EXPECT_GT(verifiedCount, 0) << "No W3D files were verified";
  EXPECT_EQ(mismatchCount, 0) << "Some files did not match their references";
}

// Test that counts total files in the archive
TEST(W3DArchiveTest, CountArchiveFiles) {
  fs::path testDataDir(TEST_DATA_DIR);
  fs::path testDir = testDataDir / "test03";
  fs::path archivePath = testDir / "W3DZH.big";

  // Check if test data exists - skip gracefully if not (files too large for git)
  if (!fs::exists(archivePath)) {
    std::cout << "\n[SKIPPED] W3D archive not found: " << archivePath
              << "\n         (W3D files are too large for git - test requires local test data)\n\n";
    GTEST_SKIP() << "W3D archive not found (test data not present)";
    return;
  }

  std::string error;
  auto reader = big::Reader::open(archivePath, &error);
  ASSERT_TRUE(reader.has_value()) << "Failed to open archive: " << error;

  std::cout << "\nTotal files in W3DZH.big: " << reader->fileCount() << "\n";

  // Count W3D files specifically
  int w3dCount = 0;
  for (const auto &file : reader->files()) {
    if (file.path.size() >= 4 &&
        file.path.substr(file.path.size() - 4) == ".W3D") {
      w3dCount++;
    }
  }

  std::cout << "W3D files in archive: " << w3dCount << "\n\n";
}
