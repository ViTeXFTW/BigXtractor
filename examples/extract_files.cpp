#include <filesystem>
#include <iostream>

#include <bigx/bigx.hpp>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <archive.big> <output_dir>\n";
    return 1;
  }

  std::string error;
  auto archive = bigx::Archive::open(argv[1], &error);

  if (!archive) {
    std::cerr << "Error: " << error << "\n";
    return 1;
  }

  std::filesystem::path outputDir = argv[2];
  std::filesystem::create_directories(outputDir);

  int extractedCount = 0;
  for (const auto &file : archive->files()) {
    std::filesystem::path outputPath = outputDir / file.path;

    if (!archive->extract(file, outputPath, &error)) {
      std::cerr << "Failed to extract " << file.path << ": " << error << "\n";
      continue;
    }
    ++extractedCount;
  }

  std::cout << "Extracted " << extractedCount << " files to " << outputDir << "\n";
  return 0;
}
