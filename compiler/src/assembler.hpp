// assembler.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// assembler.hpp - E0C6S46 assembler: assembly lines -> ROM words
#pragma once
#include "common.hpp"

struct Assembler {
long long org = 0x0000;
long long addr = 0x0000;
map<int, int> words;
map<string, int> labels;
void emit(int op);
static int page_of(int a);
static int bank_of(int a);
void assemble(const vector<string>& lines);
};
