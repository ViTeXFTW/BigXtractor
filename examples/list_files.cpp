#include <iostream>

#include <bigx/big.hpp>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <archive.big>\n";
    return 1;
  }

  std::string error;
  auto archive = big::Archive::open(argv[1], &error);

  if (!archive) {
    std::cerr << "Error: " << error << "\n";
    return 1;
  }

  std::cout << "Archive: " << argv[1] << "\n";
  std::cout << "Files: " << archive->fileCount() << "\n\n";

  for (const auto &file : archive->files()) {
    std::cout << "  " << file.path << " (" << file.size << " bytes)\n";
  }

  return 0;
}
