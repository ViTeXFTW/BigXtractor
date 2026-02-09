#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "types.hpp"

namespace bigx {

// Forward declarations
class Reader;
class Writer;

// High-level archive interface that combines reading and writing capabilities
class Archive {
public:
  Archive();
  ~Archive();

  // Delete copy, enable move
  Archive(const Archive &) = delete;
  Archive &operator=(const Archive &) = delete;
  Archive(Archive &&) noexcept;
  Archive &operator=(Archive &&) noexcept;

  // Open existing BIG archive for reading
  // Returns std::nullopt on failure, with error message in outError if provided
  static std::optional<Archive> open(const std::filesystem::path &path,
                                     std::string *outError = nullptr);

  // Create new BIG archive for writing
  static Archive create();

  // Add file to archive (from disk)
  bool addFile(const std::filesystem::path &sourcePath, const std::string &archivePath,
               std::string *outError = nullptr);

  // Add file to archive (from memory)
  bool addFile(std::span<const uint8_t> data, const std::string &archivePath,
               std::string *outError = nullptr);

  // Write archive to disk
  bool write(const std::filesystem::path &destPath, std::string *outError = nullptr);

  // Get list of all files (only available when reading)
  const std::vector<FileEntry> &files() const;

  // Get file count
  size_t fileCount() const;

  // Case-insensitive file lookup (only available when reading)
  const FileEntry *findFile(const std::string &path) const;

  // Extract file to disk (only available when reading)
  bool extract(const FileEntry &entry, const std::filesystem::path &destPath,
               std::string *outError = nullptr) const;

  // Extract file to memory (only available when reading)
  std::optional<std::vector<uint8_t>> extractToMemory(const FileEntry &entry,
                                                      std::string *outError) const;

  // Get file view (zero-copy if memory-mapped, only available when reading)
  std::span<const uint8_t> getFileView(const FileEntry &entry) const;

  // Check if archive is open for reading
  bool isReading() const { return reader_.get() != nullptr; }

  // Check if archive is open for writing
  bool isWriting() const { return writer_.get() != nullptr; }

  // Check if archive is open (either mode)
  bool isOpen() const { return isReading() || isWriting(); }

  // Close archive
  void close();

  // Clear all files (only available when writing)
  void clear();

private:
  std::unique_ptr<Reader> reader_;
  std::unique_ptr<Writer> writer_;
};

} // namespace bigx
