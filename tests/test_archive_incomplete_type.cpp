// Test case for incomplete type issue in Archive class
// This test should fail to compile before the fix, and succeed after

#include <bigx/archive.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

// This function should compile and link successfully after the fix
void test_compilation() {
    // Test 1: unique_ptr of Archive
    // This requires the destructor to be defined in the .cpp file
    auto archive_ptr = std::make_unique<big::Archive>();

    // Test 2: unordered_map with Archive value
    // This requires the destructor when the map is destroyed
    std::unordered_map<std::string, big::Archive> map;
    map["test"] = big::Archive::create();

    // Test 3: vector of Archive (requires move constructor/destructor)
    std::vector<big::Archive> vec;
    vec.push_back(big::Archive::create());
    vec.emplace_back(big::Archive::create());

    // Test 4: Move semantics
    big::Archive a = big::Archive::create();
    big::Archive b = std::move(a);  // Requires move constructor
    a = big::Archive::create();      // Requires move assignment
    b = std::move(a);                // Requires move assignment
}

// Note: This is a compilation test, not a runtime test
// The main purpose is to ensure that the code compiles
int main() {
    test_compilation();
    return 0;
}
