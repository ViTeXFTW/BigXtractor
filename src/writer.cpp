#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>
#include <unordered_set>

#include <bigx/endian.hpp>
#include <bigx/mmap.hpp>
#include <bigx/writer.hpp>

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

  // Normalize the archive path (forward slashes)
  std::string normalizedArchivePath = normalizeSlashes(archivePath);

  // Check for duplicate paths (case-insensitive)
  std::string lowered = normalizePath(archivePath);
  for (const auto &pending : pendingFiles_) {
    if (normalizePath(pending.archivePath) == lowered) {
      if (outError) {
        *outError = std::format("Duplicate file path in archive: {}", archivePath);
      }
      return false;
    }
  }

  // Add to pending files
  PendingFile pending;
  pending.archivePath = normalizedArchivePath;
  pending.sourcePath = sourcePath;
  pending.fromDisk = true;
  pendingFiles_.push_back(std::move(pending));

  return true;
}

bool Writer::addFile(std::span<const uint8_t> data, const std::string &archivePath,
                     std::string *outError) {
  // Normalize the archive path (forward slashes)
  std::string normalizedArchivePath = normalizeSlashes(archivePath);

  // Check for duplicate paths (case-insensitive)
  std::string lowered = normalizePath(archivePath);
  for (const auto &pending : pendingFiles_) {
    if (normalizePath(pending.archivePath) == lowered) {
      if (outError) {
        *outError = std::format("Duplicate file path in archive: {}", archivePath);
      }
      return false;
    }
  }

  // Add to pending files
  PendingFile pending;
  pending.archivePath = normalizedArchivePath;
  pending.data.assign(data.begin(), data.end());
  pending.fromDisk = false;
  pendingFiles_.push_back(std::move(pending));

  return true;
}

bool Writer::write(const std::filesystem::path &destPath, std::string *outError) {
  // Handle empty archives (header only) by writing directly without mmap
  if (pendingFiles_.empty()) {
    std::ofstream out(destPath, std::ios::binary);
    if (!out) {
      if (outError) {
        *outError = std::format("Failed to create output file: {}", destPath.string());
      }
      return false;
    }

    // Write header with zero files
    out.write("BIGF", 4);
    uint32_t archiveSizeBE = htobe32(static_cast<uint32_t>(ArchiveHeader::headerSize));
    out.write(reinterpret_cast<const char *>(&archiveSizeBE), 4);
    uint32_t fileCountBE = 0;
    out.write(reinterpret_cast<const char *>(&fileCountBE), 4);
    uint32_t pad = 0;
    out.write(reinterpret_cast<const char *>(&pad), 4);

    entries_.clear();
    return true;
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
    if (pending.fromDisk) {
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
    // Path string (already normalized to forward slashes)
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

    if (pending.fromDisk) {
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

      if (fileSize > 0) {
        tempBuffer.resize(fileSize);
        if (!inFile.read(reinterpret_cast<char *>(tempBuffer.data()), fileSize)) {
          if (outError) {
            *outError = std::format("Failed to read source file: {}", pending.sourcePath.string());
          }
          return false;
        }
      }

      fileData = tempBuffer;
    } else {
      fileData = pending.data;
    }

    // Write file data
    if (!fileData.empty()) {
      std::memcpy(outputData.data() + pos, fileData.data(), fileData.size());
    }

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
    entry.path = pending.archivePath;
    entry.lowercasePath = normalizePath(pending.archivePath);
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

std::string Writer::normalizeSlashes(const std::string &path) {
  std::string result;
  result.reserve(path.size());

  // Replace backslashes with forward slashes, preserving case
  for (char c : path) {
    if (c == '\\') {
      result += '/';
    } else {
      result += c;
    }
  }

  return result;
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
