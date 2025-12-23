#pragma once
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#define TEST_ASSERT(cond) do { \
    if(!(cond)) { \
        std::cerr << "ASSERTION FAILED: " << #cond << "\n" \
                  << "  at " << __FILE__ << ":" << __LINE__ << "\n"; \
        std::exit(1); \
    } \
} while(0)

inline int run_test(const char* name, void (*fn)()) {
    try {
        fn();
        std::cout << "[PASS] " << name << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << name << " - exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[FAIL] " << name << " - unknown exception\n";
        return 1;
    }
}
