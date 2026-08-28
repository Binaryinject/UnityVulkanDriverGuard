#include "tests.h"

#include <exception>
#include <iostream>

int main() {
    try {
        RunConfigTests();
        RunDriverVersionTests();
        std::cout << "All tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}

