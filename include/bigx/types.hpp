#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace bigx {

// File entry in the BIG archive
struct FileEntry {
  std::string path;          // Original case, normalized to forward slashes
  std::string lowercasePath; // For case-insensitive lookup
  uint32_t offset = 0;       // Offset within archive (big-endian when stored)
  uint32_t size = 0;         // File size in bytes (big-endian when stored)
};

// Archive header (16 bytes)
struct ArchiveHeader {
  char magic[4] = {'B', 'I', 'G', 'F'}; // File identifier
  uint32_t archiveSize = 0;             // Total archive size (big-endian, mostly unused)
  uint32_t fileCount = 0;               // Number of files (big-endian)
  uint32_t padding = 0;                 // Padding/reserved

  static constexpr size_t headerSize = 16;
};

// Exception for parsing errors
class ParseError : public std::runtime_error {
public:
  explicit ParseError(const std::string &msg) : std::runtime_error(msg) {}
};

} // namespace bigx
