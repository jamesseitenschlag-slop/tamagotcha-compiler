// lexer.cpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
#include "lexer.hpp"

vector<Tok> tokenize(const string& src) {
    static const set<string> KEYWORDS = {"char", "void", "if", "else",
                                         "while", "for", "return", "switch", "case", "default",
                                         "do", "break", "continue"};
    vector<Tok> toks;
    size_t i = 0, n = src.size();
    while (i < n) {
        char c = src[i];
        if (isspace((unsigned char)c)) { i++; continue; }
        if (c == '/' && i + 1 < n && src[i + 1] == '/') {
            size_t j = src.find('\n', i);
            i = (j == string::npos) ? n : j;
            continue;
        }
        if (c == '#') {
            size_t j = src.find('\n', i);
            i = (j == string::npos) ? n : j;
            continue;
        }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            size_t j = src.find("*/", i + 2);
            i = (j == string::npos) ? n : j + 2;
            continue;
        }
        string two = src.substr(i, 2);
        if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
            two == "++" || two == "--") {
            toks.push_back({two, two});
            i += 2;
            continue;
        }
        if (strchr("+-*/%&|^~!<>=(){}[];,:.",
                   c)) {
            toks.push_back({string(1, c), string(1, c)});
            i++;
            continue;
        }
        if (c == '"') {
            size_t j = i + 1;
            string buf;
            while (j < n && src[j] != '"') {
                if (src[j] == '\\' && j + 1 < n) {
                    char e = src[j + 1];
                    if (e == 'n') buf += '\n';
                    else if (e == 't') buf += '\t';
                    else if (e == 'r') buf += '\r';
                    else if (e == '0') buf += '\0';
                    else if (e == '\\') buf += '\\';
                    else if (e == '"') buf += '"';
                    else buf += e;
                    j += 2;
                } else {
                    buf += src[j];
                    j++;
                }
            }
            if (j >= n) throw CompileError("unterminated string");
            toks.push_back({"str", buf});
            i = j + 1;
            continue;
        }
        if (isdigit((unsigned char)c)) {
            size_t j = i;
            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '_'))
                j++;
            toks.push_back({"num", src.substr(i, j - i)});
            i = j;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t j = i;
            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '_'))
                j++;
            string w = src.substr(i, j - i);
            toks.push_back({KEYWORDS.count(w) ? "kw" : "id", w});
            i = j;
            continue;
        }
        throw CompileError(string("unexpected char '") + c + "'");
    }
    toks.push_back({"eof", ""});
    return toks;
}
