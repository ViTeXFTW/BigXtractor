#include <algorithm>
#include <format>
#include <fstream>
#include <unordered_set>

#include <big/endian.hpp>
#include <big/mmap.hpp>
#include <big/writer.hpp>

namespace big {

bool Writer::addFile(const std::filesystem::path &sourcePath, const std::string &archivePath,
                     std::string *outError) {
  // Check if file exists
  std::error_code ec;
  if (!std::filesystem::exists(sourcePath, ec)) {
    if (outError) {
      *outError = std::format("Source file does not exist: {}", sourcePath.string());
    }
    return false;
  }

  // Check for duplicate paths (case-insensitive)
  std::string normalized = normalizePath(archivePath);
  for (const auto &pending : pendingFiles_) {
    if (normalizePath(pending.archivePath) == normalized) {
      if (outError) {
        *outError = std::format("Duplicate file path in archive: {}", archivePath);
      }
      return false;
    }
  }

  // Add to pending files
  PendingFile pending;
  pending.archivePath = archivePath;
  pending.sourcePath = sourcePath;
  // data remains empty (will be read from source during write)
  pendingFiles_.push_back(std::move(pending));

  return true;
}

bool Writer::addFile(std::span<const uint8_t> data, const std::string &archivePath,
                     std::string *outError) {
  // Check for duplicate paths (case-insensitive)
  std::string normalized = normalizePath(archivePath);
  for (const auto &pending : pendingFiles_) {
    if (normalizePath(pending.archivePath) == normalized) {
      if (outError) {
        *outError = std::format("Duplicate file path in archive: {}", archivePath);
      }
      return false;
    }
  }

  // Add to pending files
  PendingFile pending;
  pending.archivePath = archivePath;
  pending.data.assign(data.begin(), data.end());
  // sourcePath remains empty (data is in memory)
  pendingFiles_.push_back(std::move(pending));

  return true;
}

bool Writer::write(const std::filesystem::path &destPath, std::string *outError) {
  if (pendingFiles_.empty()) {
    if (outError) {
      *outError = "Cannot write archive with no files";
    }
    return false;
  }

  // Step 1: Calculate total archive size
  size_t headerSize = ArchiveHeader::headerSize;
  size_t directorySize = 0;

  for (const auto &pending : pendingFiles_) {
    // Each entry: offset (4) + size (4) + null-terminated path
    directorySize += 8 + pending.archivePath.size() + 1;
  }

  // Calculate file data section size
  size_t filesDataSize = 0;
  for (const auto &pending : pendingFiles_) {
    if (pending.data.empty()) {
      std::error_code ec;
      filesDataSize += std::filesystem::file_size(pending.sourcePath, ec);
      if (ec) {
        if (outError) {
          *outError = std::format("Failed to get file size: {}", pending.sourcePath.string());
        }
        return false;
      }
    } else {
      filesDataSize += pending.data.size();
    }
  }

  size_t totalSize = headerSize + directorySize + filesDataSize;

  // Step 2: Create memory-mapped file
  MappedFile outputFile;
  if (!outputFile.openWrite(destPath, totalSize, outError)) {
    return false;
  }

  auto outputData = outputFile.data();

  // Step 3: Write header
  size_t pos = 0;

  // Magic "BIGF"
  std::memcpy(outputData.data() + pos, "BIGF", 4);
  pos += 4;

  // Archive size (big-endian)
  uint32_t archiveSizeBE = htobe32(static_cast<uint32_t>(totalSize));
  std::memcpy(outputData.data() + pos, &archiveSizeBE, 4);
  pos += 4;

  // File count (big-endian)
  uint32_t fileCountBE = htobe32(static_cast<uint32_t>(pendingFiles_.size()));
  std::memcpy(outputData.data() + pos, &fileCountBE, 4);
  pos += 4;

  // Padding
  uint32_t padding = 0;
  std::memcpy(outputData.data() + pos, &padding, 4);
  pos += 4;

  // Step 4: Write directory entries (placeholders for now, we'll come back)
  size_t directoryStart = pos;
  for (const auto &pending : pendingFiles_) {
    // Offset placeholder (will be filled later)
    pos += 4;
    // Size placeholder (will be filled later)
    pos += 4;
    // Path string
    std::memcpy(outputData.data() + pos, pending.archivePath.data(), pending.archivePath.size());
    pos += pending.archivePath.size();
    // Null terminator
    outputData[pos++] = '\0';
  }

  // Step 5: Write file data and update directory entries
  entries_.clear();
  entries_.reserve(pendingFiles_.size());

  for (size_t i = 0; i < pendingFiles_.size(); ++i) {
    const auto &pending = pendingFiles_[i];

    // Get file data
    std::span<const uint8_t> fileData;
    std::vector<uint8_t> tempBuffer;

    if (pending.data.empty()) {
      // Read from disk
      std::ifstream inFile(pending.sourcePath, std::ios::binary | std::ios::ate);
      if (!inFile) {
        if (outError) {
          *outError = std::format("Failed to open source file: {}", pending.sourcePath.string());
        }
        return false;
      }

      size_t fileSize = inFile.tellg();
      inFile.seekg(0, std::ios::beg);

      tempBuffer.resize(fileSize);
      if (!inFile.read(reinterpret_cast<char *>(tempBuffer.data()), fileSize)) {
        if (outError) {
          *outError = std::format("Failed to read source file: {}", pending.sourcePath.string());
        }
        return false;
      }

      fileData = tempBuffer;
    } else {
      fileData = pending.data;
    }

    // Write file data
    std::memcpy(outputData.data() + pos, fileData.data(), fileData.size());

    // Update directory entry
    size_t entryPos = directoryStart;
    for (size_t j = 0; j < i; ++j) {
      entryPos += 8 + pendingFiles_[j].archivePath.size() + 1;
    }

    // Write offset (big-endian)
    uint32_t offsetBE = htobe32(static_cast<uint32_t>(pos));
    std::memcpy(outputData.data() + entryPos, &offsetBE, 4);

    // Write size (big-endian)
    uint32_t sizeBE = htobe32(static_cast<uint32_t>(fileData.size()));
    std::memcpy(outputData.data() + entryPos + 4, &sizeBE, 4);

    // Create entry for tracking
    FileEntry entry;
    entry.path = normalizePath(pending.archivePath);
    entry.lowercasePath = entry.path;
    entry.offset = pos;
    entry.size = static_cast<uint32_t>(fileData.size());
    entries_.push_back(std::move(entry));

    pos += fileData.size();
  }

  // Step 6: Flush to disk
  if (!outputFile.flush(outError)) {
    return false;
  }

  return true;
}

void Writer::clear() {
  pendingFiles_.clear();
  entries_.clear();
}

std::string Writer::normalizePath(const std::string &path) {
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
