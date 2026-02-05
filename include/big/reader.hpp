#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "mmap.hpp"
#include "types.hpp"

namespace big {

class Reader {
public:
  Reader() = default;
  ~Reader() = default;

  // Delete copy, enable move
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  Reader(Reader &&) noexcept = default;
  Reader &operator=(Reader &&) noexcept = default;

  // Open BIG archive from file (memory-mapped)
  // Returns std::nullopt on failure, with error message in outError if provided
  static std::optional<Reader> open(const std::filesystem::path &path,
                                    std::string *outError = nullptr);

  // Get list of all files
  const std::vector<FileEntry> &files() const { return files_; }

  // Get total number of files
  size_t fileCount() const { return files_.size(); }

  // Case-insensitive file lookup
  // Returns nullptr if file not found
  const FileEntry *findFile(const std::string &path) const;

  // Extract file to disk
  // Returns true on success, false on failure (error in outError if provided)
  bool extract(const FileEntry &entry, const std::filesystem::path &destPath,
               std::string *outError = nullptr) const;

  // Extract file to memory
  // Returns std::nullopt on failure, with error message in outError if provided
  std::optional<std::vector<uint8_t>> extractToMemory(const FileEntry &entry,
                                                      std::string *outError = nullptr) const;

  // Get file view (zero-copy if memory-mapped)
  // Returns empty span if file bounds are invalid
  std::span<const uint8_t> getFileView(const FileEntry &entry) const;

  // Check if archive is open
  bool isOpen() const;

  // Close archive
  void close();

private:
  bool parse(std::string *outError);

  // Normalize path to lowercase with forward slashes for lookup
  static std::string normalizePath(const std::string &path);

  MappedFile mappedFile_;
  std::vector<FileEntry> files_;
  std::unordered_map<std::string, size_t> lookup_; // lowercase path -> index
};

} // namespace big
