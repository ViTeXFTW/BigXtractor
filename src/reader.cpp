#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>
#include <unordered_set>

#include <big/endian.hpp>
#include <big/mmap.hpp>
#include <big/reader.hpp>

namespace big {

std::optional<Reader> Reader::open(const std::filesystem::path &path, std::string *outError) {
  Reader reader;
  if (!reader.mappedFile_.openRead(path, outError)) {
    return std::nullopt;
  }

  if (!reader.parse(outError)) {
    reader.close();
    return std::nullopt;
  }

  return reader;
}

bool Reader::parse(std::string *outError) {
  auto fileData = mappedFile_.data();

  // Check minimum size (header is 16 bytes)
  if (fileData.size() < ArchiveHeader::headerSize) {
    if (outError) {
      *outError = std::format("File too small to be a BIG archive (size: {})", fileData.size());
    }
    return false;
  }

  // Verify magic number
  const char *magic = reinterpret_cast<const char *>(fileData.data());
  if (std::memcmp(magic, "BIGF", 4) != 0) {
    if (outError) {
      *outError = std::format("Invalid BIG file magic (expected 'BIGF', got '{:.4s}')", magic);
    }
    return false;
  }

  // Read header
  ArchiveHeader header;
  std::memcpy(&header, fileData.data(), ArchiveHeader::headerSize);

  // Convert from big-endian
  uint32_t fileCount = betoh32(header.fileCount);

  // Validate file count (sanity check)
  if (fileCount > 1000000) { // Arbitrary large number to detect corruption
    if (outError) {
      *outError = std::format("Invalid file count in BIG header: {}", fileCount);
    }
    return false;
  }

  // Parse file entries starting at offset 0x10
  size_t pos = ArchiveHeader::headerSize;
  files_.reserve(fileCount);

  for (uint32_t i = 0; i < fileCount; ++i) {
    // Check if we have enough data for offset (4) + size (4)
    if (pos + 8 > fileData.size()) {
      if (outError) {
        *outError = std::format("File entry {} extends beyond file bounds", i);
      }
      return false;
    }

    // Read offset and size (big-endian)
    uint32_t offset, size;
    std::memcpy(&offset, fileData.data() + pos, 4);
    std::memcpy(&size, fileData.data() + pos + 4, 4);
    pos += 8;

    offset = betoh32(offset);
    size = betoh32(size);

    // Validate offset and size
    if (offset + size > fileData.size()) {
      if (outError) {
        *outError =
            std::format("File entry {} has invalid offset/size (offset={}, size={}, fileSize={})",
                        i, offset, size, fileData.size());
      }
      return false;
    }

    // Read null-terminated path string
    const char *pathStart = reinterpret_cast<const char *>(fileData.data() + pos);
    size_t pathLen = 0;
    while (pos + pathLen < fileData.size() && pathStart[pathLen] != '\0') {
      ++pathLen;
    }

    if (pos + pathLen >= fileData.size()) {
      if (outError) {
        *outError = std::format("File entry {} has unterminated path string", i);
      }
      return false;
    }

    std::string path(pathStart, pathLen);
    pos += pathLen + 1; // +1 for null terminator

    // Create file entry
    FileEntry entry;
    entry.path = normalizePath(path);
    entry.lowercasePath = normalizePath(path);
    entry.offset = offset;
    entry.size = size;

    files_.push_back(std::move(entry));
  }

  // Build lookup table
  lookup_.reserve(files_.size());
  for (size_t i = 0; i < files_.size(); ++i) {
    const auto &entry = files_[i];

    // Check for duplicate paths
    if (lookup_.contains(entry.lowercasePath)) {
      if (outError) {
        *outError = std::format("Duplicate file path in archive: {}", entry.path);
      }
      return false;
    }

    lookup_[entry.lowercasePath] = i;
  }

  return true;
}

const FileEntry *Reader::findFile(const std::string &path) const {
  std::string normalized = normalizePath(path);
  auto it = lookup_.find(normalized);
  if (it == lookup_.end()) {
    return nullptr;
  }
  return &files_[it->second];
}

bool Reader::extract(const FileEntry &entry, const std::filesystem::path &destPath,
                     std::string *outError) const {
  auto fileData = getFileView(entry);
  if (fileData.empty()) {
    if (outError) {
      *outError = std::format("Invalid file bounds for: {}", entry.path);
    }
    return false;
  }

  // Create parent directories if needed
  std::filesystem::create_directories(destPath.parent_path());

  // Write file
  std::ofstream out(destPath, std::ios::binary);
  if (!out) {
    if (outError) {
      *outError = std::format("Failed to create output file: {}", destPath.string());
    }
    return false;
  }

  out.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
  if (!out) {
    if (outError) {
      *outError = std::format("Failed to write to output file: {}", destPath.string());
    }
    return false;
  }

  return true;
}

std::optional<std::vector<uint8_t>> Reader::extractToMemory(const FileEntry &entry,
                                                            std::string *outError) const {
  auto fileData = getFileView(entry);
  if (fileData.empty()) {
    if (outError) {
      *outError = std::format("Invalid file bounds for: {}", entry.path);
    }
    return std::nullopt;
  }

  std::vector<uint8_t> result(fileData.size());
  std::memcpy(result.data(), fileData.data(), fileData.size());
  return result;
}

std::span<const uint8_t> Reader::getFileView(const FileEntry &entry) const {
  auto archiveData = mappedFile_.data();

  // Validate bounds
  if (entry.offset + entry.size > archiveData.size()) {
    return {};
  }

  return std::span<const uint8_t>(archiveData.data() + entry.offset, entry.size);
}

bool Reader::isOpen() const {
  return mappedFile_.isOpen();
}

void Reader::close() {
  mappedFile_.close();
  files_.clear();
  lookup_.clear();
}

std::string Reader::normalizePath(const std::string &path) {
  std::string result;
  result.reserve(path.size());

  // Convert to lowercase and replace backslashes with forward slashes
  for (char c : path) {
    if (c == '\\') {
      result += '/';
    } else {
      result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
  }

  return result;
}

} // namespace big
