// lexer.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// lexer.hpp - tokenizer for the C subset
#pragma once
#include "common.hpp"

struct Tok {
    string k;   // "num","id","kw","str","eof" or a symbol char
    string v;
};

vector<Tok> tokenize(const string& src);
