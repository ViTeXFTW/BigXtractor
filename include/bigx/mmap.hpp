#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace bigx {

// RAII wrapper for memory-mapped files
// Supports both read-only and read-write modes
class MappedFile {
public:
  MappedFile();
  ~MappedFile();

  // Delete copy, enable move
  MappedFile(const MappedFile &) = delete;
  MappedFile &operator=(const MappedFile &) = delete;
  MappedFile(MappedFile &&other) noexcept;
  MappedFile &operator=(MappedFile &&other) noexcept;

  // Open file for reading (memory-mapped)
  bool openRead(const std::filesystem::path &path, std::string *outError = nullptr);

  // Create new file for writing with specified size (memory-mapped)
  bool openWrite(const std::filesystem::path &path, size_t size, std::string *outError = nullptr);

  // Get mapped data (const view for read mode)
  std::span<const uint8_t> data() const {
    return std::span<const uint8_t>(static_cast<const uint8_t *>(data_), size_);
  }

  // Get mapped data (mutable view for write mode)
  std::span<uint8_t> data() { return std::span<uint8_t>(static_cast<uint8_t *>(data_), size_); }

  // Flush changes to disk (write mode only)
  bool flush(std::string *outError = nullptr);

  // Close mapping and file handle
  void close();

  // Check if file is open
  bool isOpen() const { return data_ != nullptr; }

  // Get size of mapped region
  size_t size() const { return size_; }

private:
  void cleanup() noexcept;

#ifdef _WIN32
  void *fileHandle_ = nullptr;    // HANDLE on Windows
  void *mappingHandle_ = nullptr; // HANDLE on Windows
#else
  int fd_ = -1; // File descriptor on POSIX
#endif
  void *data_ = nullptr;
  size_t size_ = 0;
  bool writable_ = false;
};

} // namespace bigx
