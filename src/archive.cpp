#include <bigx/archive.hpp>
#include <bigx/reader.hpp>
#include <bigx/writer.hpp>

namespace big {

std::optional<Archive> Archive::open(const std::filesystem::path &path, std::string *outError) {
  auto reader = Reader::open(path, outError);
  if (!reader) {
    return std::nullopt;
  }

  Archive archive;
  archive.reader_ = std::make_unique<Reader>(std::move(*reader));
  return archive;
}

Archive Archive::create() {
  Archive archive;
  archive.writer_ = std::make_unique<Writer>();
  return archive;
}

bool Archive::addFile(const std::filesystem::path &sourcePath, const std::string &archivePath,
                      std::string *outError) {
  if (!writer_) {
    if (outError) {
      *outError = "Archive not open for writing";
    }
    return false;
  }
  return writer_->addFile(sourcePath, archivePath, outError);
}

bool Archive::addFile(std::span<const uint8_t> data, const std::string &archivePath,
                      std::string *outError) {
  if (!writer_) {
    if (outError) {
      *outError = "Archive not open for writing";
    }
    return false;
  }
  return writer_->addFile(data, archivePath, outError);
}

bool Archive::write(const std::filesystem::path &destPath, std::string *outError) {
  if (!writer_) {
    if (outError) {
      *outError = "Archive not open for writing";
    }
    return false;
  }
  return writer_->write(destPath, outError);
}

const std::vector<FileEntry> &Archive::files() const {
  static const std::vector<FileEntry> empty;
  if (reader_) {
    return reader_->files();
  }
  if (writer_) {
    return writer_->files();
  }
  return empty;
}

size_t Archive::fileCount() const {
  if (reader_) {
    return reader_->fileCount();
  }
  if (writer_) {
    return writer_->fileCount();
  }
  return 0;
}

const FileEntry *Archive::findFile(const std::string &path) const {
  if (!reader_) {
    return nullptr;
  }
  return reader_->findFile(path);
}

bool Archive::extract(const FileEntry &entry, const std::filesystem::path &destPath,
                      std::string *outError) const {
  if (!reader_) {
    if (outError) {
      *outError = "Archive not open for reading";
    }
    return false;
  }
  return reader_->extract(entry, destPath, outError);
}

std::optional<std::vector<uint8_t>> Archive::extractToMemory(const FileEntry &entry,
                                                             std::string *outError) const {
  if (!reader_) {
    if (outError) {
      *outError = "Archive not open for reading";
    }
    return std::nullopt;
  }
  return reader_->extractToMemory(entry, outError);
}

std::span<const uint8_t> Archive::getFileView(const FileEntry &entry) const {
  if (!reader_) {
    return {};
  }
  return reader_->getFileView(entry);
}

void Archive::close() {
  reader_.reset();
  writer_.reset();
}

void Archive::clear() {
  if (writer_) {
    writer_->clear();
  }
}

} // namespace big
