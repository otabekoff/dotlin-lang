#include <iostream>
#include <cassert>

// Simple test to verify the build system works
void test_basic() {
    assert(true); // Simple assertion to verify testing works
    std::cout << "Basic test passed!" << std::endl;
}

int main() {
    std::cout << "Running simple tests..." << std::endl;
    test_basic();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}