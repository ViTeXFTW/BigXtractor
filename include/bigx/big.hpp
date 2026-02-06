#pragma once

// BIG Archive Library
// A modern C++20 library for reading and writing the .big file archive format
// used by Command & Conquer Generals and other Westwood Studios games.

#include "archive.hpp"
#include "reader.hpp"
#include "types.hpp"
#include "writer.hpp"

// The library provides three levels of abstraction:
//
// 1. Low-level: Reader / Writer classes
//    - Direct access to archive reading/writing functionality
//    - Use Reader::open() to read existing archives
//    - Use Writer to create new archives
//
// 2. High-level: Archive class
//    - Unified interface for both reading and writing
//    - Use Archive::open() to read, Archive::create() to write
//
// Example usage:
//
//   // Reading an archive
//   auto archive = big::Archive::open("myfile.big");
//   if (archive) {
//     for (const auto& file : archive->files()) {
//       std::cout << file.path << std::endl;
//     }
//     const auto* entry = archive->findFile("data/file.txt");
//     if (entry) {
//       archive->extract(*entry, "output.txt");
//     }
//   }
//
//   // Creating a new archive
//   auto archive = big::Archive::create();
//   archive.addFile("source.txt", "data/file.txt");
//   archive.write("output.big");

namespace big {}
