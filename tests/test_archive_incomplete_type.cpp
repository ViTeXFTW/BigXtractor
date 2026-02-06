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
    auto archive_ptr = std::make_unique<bigx::Archive>();
    ASSERT_NE(archive_ptr, nullptr);
}

TEST(ArchiveIncompleteTypeTest, UnorderedMapCompilation) {
    // Test 2: unordered_map with Archive value
    // This requires the destructor when the map is destroyed
    std::unordered_map<std::string, bigx::Archive> map;
    map["test"] = bigx::Archive::create();
    EXPECT_FALSE(map.empty());
}

TEST(ArchiveIncompleteTypeTest, VectorCompilation) {
    // Test 3: vector of Archive (requires move constructor/destructor)
    std::vector<bigx::Archive> vec;
    vec.push_back(bigx::Archive::create());
    vec.emplace_back(bigx::Archive::create());
    EXPECT_EQ(vec.size(), 2);
}

TEST(ArchiveIncompleteTypeTest, MoveSemantics) {
    // Test 4: Move semantics
    bigx::Archive a = bigx::Archive::create();
    bigx::Archive b = std::move(a);  // Requires move constructor
    a = bigx::Archive::create();      // Requires move assignment
    b = std::move(a);                // Requires move assignment
    // If we got here without crashing, move semantics work
    SUCCEED();
}
