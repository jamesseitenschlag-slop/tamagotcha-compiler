// font.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// font.hpp - 5x7 font + interleaved LCD RAM mapping
#pragma once
#include "common.hpp"

// Glyph (7 rows, 5 bits each, bit4 = left column); unknown -> space.
// Pointer to internal module storage (read-only).
const std::array<int, 7>* glyph(char ch);
int max_chars();
int text_width(const string& t);
// Text -> {byte_addr: byte_val} in interleaved VRAM (TamaLIB model)
map<int, int> lcd_bytes_for_text(const string& text, int x0, int y0);
