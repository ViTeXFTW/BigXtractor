// Test case for incomplete type issue in Archive class
// This test should fail to compile before the fix, and succeed after

#include <bigx/archive.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <unordered_map>
#include <vector>

// Runtime test to verify the incomplete type fix works correctly
// This ensures unique_ptr, containers, and move semantics all work
TEST(ArchiveIncompleteTypeTest, UniquePtrCompilation) {
    // Test 1: unique_ptr of Archive
    // This requires the destructor to be defined in the .cpp file
    auto archive_ptr = std::make_unique<big::Archive>();
    ASSERT_NE(archive_ptr, nullptr);
}

TEST(ArchiveIncompleteTypeTest, UnorderedMapCompilation) {
    // Test 2: unordered_map with Archive value
    // This requires the destructor when the map is destroyed
    std::unordered_map<std::string, big::Archive> map;
    map["test"] = big::Archive::create();
    EXPECT_FALSE(map.empty());
}

TEST(ArchiveIncompleteTypeTest, VectorCompilation) {
    // Test 3: vector of Archive (requires move constructor/destructor)
    std::vector<big::Archive> vec;
    vec.push_back(big::Archive::create());
    vec.emplace_back(big::Archive::create());
    EXPECT_EQ(vec.size(), 2);
}

TEST(ArchiveIncompleteTypeTest, MoveSemantics) {
    // Test 4: Move semantics
    big::Archive a = big::Archive::create();
    big::Archive b = std::move(a);  // Requires move constructor
    a = big::Archive::create();      // Requires move assignment
    b = std::move(a);                // Requires move assignment
    // If we got here without crashing, move semantics work
    SUCCEED();
}
