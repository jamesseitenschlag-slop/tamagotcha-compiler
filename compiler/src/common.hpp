// common.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// common.hpp - common base: includes, type aliases, errors, hexstr
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <sstream>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::pair;
using std::to_string;

// ---------------------------------------------------------------------------
// Errors
// ----------------------------------------------------------------------------
struct CompileError : std::runtime_error {
    explicit CompileError(const string& m) : std::runtime_error(m) {}
};
struct AsmError : std::runtime_error {
    explicit AsmError(const string& m) : std::runtime_error(m) {}
};

inline string hexstr(unsigned v, int width = 0) {
    char b[32];
    if (width)
        snprintf(b, sizeof b, "0x%0*X", width, v);
    else
        snprintf(b, sizeof b, "0x%X", v);
    return b;
}
