#include <format>

#include <bigx/mmap.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace big {

MappedFile::MappedFile() = default;

MappedFile::~MappedFile() {
  close();
}

MappedFile::MappedFile(MappedFile &&other) noexcept
    :
#ifdef _WIN32
      fileHandle_(other.fileHandle_), mappingHandle_(other.mappingHandle_),
#else
      fd_(other.fd_),
#endif
      data_(other.data_), size_(other.size_), writable_(other.writable_) {
#ifdef _WIN32
  other.fileHandle_ = nullptr;
  other.mappingHandle_ = nullptr;
#else
  other.fd_ = -1;
#endif
  other.data_ = nullptr;
  other.size_ = 0;
  other.writable_ = false;
}

MappedFile &MappedFile::operator=(MappedFile &&other) noexcept {
  if (this != &other) {
    close();

#ifdef _WIN32
    fileHandle_ = other.fileHandle_;
    mappingHandle_ = other.mappingHandle_;
    other.fileHandle_ = nullptr;
    other.mappingHandle_ = nullptr;
#else
    fd_ = other.fd_;
    other.fd_ = -1;
#endif
    data_ = other.data_;
    size_ = other.size_;
    writable_ = other.writable_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.writable_ = false;
  }
  return *this;
}

bool MappedFile::openRead(const std::filesystem::path &path, std::string *outError) {
  close();

#ifdef _WIN32
  // Windows implementation
  fileHandle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);

  if (fileHandle_ == INVALID_HANDLE_VALUE) {
    fileHandle_ = nullptr;
    if (outError) {
      *outError = std::format("Failed to open file for reading: {}", path.string());
    }
    return false;
  }

  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(static_cast<HANDLE>(fileHandle_), &fileSize)) {
    if (outError) {
      *outError = std::format("Failed to get file size: {}", path.string());
    }
    close();
    return false;
  }

  if (fileSize.QuadPart == 0) {
    if (outError) {
      *outError = std::format("File is empty: {}", path.string());
    }
    close();
    return false;
  }

  size_ = static_cast<size_t>(fileSize.QuadPart);

  mappingHandle_ =
      CreateFileMappingW(static_cast<HANDLE>(fileHandle_), nullptr, PAGE_READONLY, 0, 0, nullptr);

  if (!mappingHandle_) {
    if (outError) {
      *outError = std::format("Failed to create file mapping: {}", path.string());
    }
    close();
    return false;
  }

  data_ = MapViewOfFile(static_cast<HANDLE>(mappingHandle_), FILE_MAP_READ, 0, 0, 0);

  if (!data_) {
    if (outError) {
      *outError = std::format("Failed to map view of file: {}", path.string());
    }
    close();
    return false;
  }

  writable_ = false;
  return true;

#else
  // POSIX implementation (Linux, macOS)
  fd_ = ::open(path.string().c_str(), O_RDONLY);
  if (fd_ < 0) {
    if (outError) {
      *outError =
          std::format("Failed to open file for reading: {} (errno: {})", path.string(), errno);
    }
    return false;
  }

  struct stat st;
  if (fstat(fd_, &st) < 0) {
    if (outError) {
      *outError = std::format("Failed to get file size: {} (errno: {})", path.string(), errno);
    }
    close();
    return false;
  }

  if (st.st_size == 0) {
    if (outError) {
      *outError = std::format("File is empty: {}", path.string());
    }
    close();
    return false;
  }

  size_ = static_cast<size_t>(st.st_size);

  data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
  if (data_ == MAP_FAILED) {
    if (outError) {
      *outError = std::format("Failed to map file: {} (errno: {})", path.string(), errno);
    }
    close();
    return false;
  }

  writable_ = false;
  return true;
#endif
}

bool MappedFile::openWrite(const std::filesystem::path &path, size_t size, std::string *outError) {
  close();

  if (size == 0) {
    if (outError) {
      *outError = "Cannot create file mapping with zero size";
    }
    return false;
  }

  size_ = size;

#ifdef _WIN32
  // Windows implementation
  fileHandle_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);

  if (fileHandle_ == INVALID_HANDLE_VALUE) {
    fileHandle_ = nullptr;
    if (outError) {
      *outError = std::format("Failed to create file for writing: {}", path.string());
    }
    return false;
  }

  // Extend file to required size
  LARGE_INTEGER fileSize;
  fileSize.QuadPart = static_cast<LONGLONG>(size);
  if (!SetFilePointerEx(static_cast<HANDLE>(fileHandle_), fileSize, nullptr, FILE_BEGIN) ||
      !SetEndOfFile(static_cast<HANDLE>(fileHandle_))) {
    if (outError) {
      *outError = std::format("Failed to set file size: {}", path.string());
    }
    close();
    return false;
  }

  mappingHandle_ =
      CreateFileMappingW(static_cast<HANDLE>(fileHandle_), nullptr, PAGE_READWRITE, 0, 0, nullptr);

  if (!mappingHandle_) {
    if (outError) {
      *outError = std::format("Failed to create file mapping: {}", path.string());
    }
    close();
    return false;
  }

  data_ = MapViewOfFile(static_cast<HANDLE>(mappingHandle_), FILE_MAP_WRITE, 0, 0, 0);

  if (!data_) {
    if (outError) {
      *outError = std::format("Failed to map view of file: {}", path.string());
    }
    close();
    return false;
  }

  writable_ = true;
  return true;

#else
  // POSIX implementation
  fd_ = ::open(path.string().c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd_ < 0) {
    if (outError) {
      *outError =
          std::format("Failed to create file for writing: {} (errno: {})", path.string(), errno);
    }
    return false;
  }

  // Extend file to required size
  if (ftruncate(fd_, static_cast<off_t>(size)) < 0) {
    if (outError) {
      *outError = std::format("Failed to set file size: {} (errno: {})", path.string(), errno);
    }
    close();
    return false;
  }

  data_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (data_ == MAP_FAILED) {
    if (outError) {
      *outError = std::format("Failed to map file: {} (errno: {})", path.string(), errno);
    }
    close();
    return false;
  }

  writable_ = true;
  return true;
#endif
}

bool MappedFile::flush(std::string *outError) {
  if (!data_ || !writable_) {
    if (outError) {
      *outError = "Cannot flush: file not open or not writable";
    }
    return false;
  }

#ifdef _WIN32
  if (!FlushViewOfFile(data_, 0)) {
    if (outError) {
      *outError = std::format("Failed to flush view of file (error: {})", GetLastError());
    }
    return false;
  }
  if (!FlushFileBuffers(static_cast<HANDLE>(fileHandle_))) {
    if (outError) {
      *outError = std::format("Failed to flush file buffers (error: {})", GetLastError());
    }
    return false;
  }
#else
  if (msync(data_, size_, MS_SYNC) < 0) {
    if (outError) {
      *outError = std::format("Failed to sync mapped file (errno: {})", errno);
    }
    return false;
  }
#endif

  return true;
}

void MappedFile::close() {
  cleanup();
}

void MappedFile::cleanup() noexcept {
  if (data_) {
#ifdef _WIN32
    UnmapViewOfFile(data_);
#else
    munmap(data_, size_);
#endif
    data_ = nullptr;
  }

#ifdef _WIN32
  if (mappingHandle_) {
    CloseHandle(static_cast<HANDLE>(mappingHandle_));
    mappingHandle_ = nullptr;
  }
  if (fileHandle_) {
    CloseHandle(static_cast<HANDLE>(fileHandle_));
    fileHandle_ = nullptr;
  }
#else
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
#endif

  size_ = 0;
  writable_ = false;
}

} // namespace big
