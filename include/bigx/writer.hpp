#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "types.hpp"

namespace big {

class Writer {
public:
  Writer() = default;
  ~Writer() = default;

  // Delete copy, enable move
  Writer(const Writer &) = delete;
  Writer &operator=(const Writer &) = delete;
  Writer(Writer &&) noexcept = default;
  Writer &operator=(Writer &&) noexcept = default;

  // Add file to archive from disk
  // The archivePath is the path that will be stored in the archive
  // Returns true on success, false on failure (error in outError if provided)
  bool addFile(const std::filesystem::path &sourcePath, const std::string &archivePath,
               std::string *outError = nullptr);

  // Add file to archive from memory
  // The archivePath is the path that will be stored in the archive
  // Returns true on success, false on failure (error in outError if provided)
  bool addFile(std::span<const uint8_t> data, const std::string &archivePath,
               std::string *outError = nullptr);

  // Write archive to disk
  // Returns true on success, false on failure (error in outError if provided)
  bool write(const std::filesystem::path &destPath, std::string *outError = nullptr);

  // Clear all files
  void clear();

  // Get current file entries
  const std::vector<FileEntry> &files() const { return entries_; }

  // Get number of files to be written
  size_t fileCount() const { return pendingFiles_.size(); }

private:
  // Normalize slashes only (backslashes to forward slashes), preserving case
  static std::string normalizeSlashes(const std::string &path);

  // Normalize path to lowercase with forward slashes for lookup
  static std::string normalizePath(const std::string &path);

  struct PendingFile {
    std::string archivePath;          // Normalized path (forward slashes)
    std::filesystem::path sourcePath; // Empty if from memory
    std::vector<uint8_t> data;        // File data if from memory
    bool fromDisk = false;            // True if file should be read from sourcePath
  };

  std::vector<PendingFile> pendingFiles_;
  std::vector<FileEntry> entries_;
};

} // namespace big
