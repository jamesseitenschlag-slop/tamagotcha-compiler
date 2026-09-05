// ast.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// ast.hpp - abstract syntax tree for the C subset
#pragma once
#include "common.hpp"

// AST
// ===========================================================================
enum K {
    K_PROGRAM, K_FUNC, K_VARS, K_BLOCK, K_EMPTY, K_LOCALVARS,
    K_IF, K_WHILE, K_FOR, K_RETURN, K_EXPR,
    K_NUM, K_VAR, K_ARR, K_ARRI, K_CALL, K_PRINT, K_PRINTNUM, K_DRAWTEXT,
    K_DISPDIGIT, K_READKEY, K_PRINTARR, K_UN, K_BIN, K_CMP, K_ASSIGN,
    K_PREINC, K_POSTINC, K_DEREF, K_ADDR, K_SETTEXT, K_ASSERTSTMT, K_MEMFN, K_PRINTMSG,
    K_DO, K_BREAK, K_CONTINUE, K_SWITCH, K_CASE,
    K_DRAWSPRITE, K_SETICON, K_CLEARICONS, K_CLEARVRAM
};
struct Node {
    K k;
    long long iv = 0;          // num value / array index
    string s, s2;              // text / name / op / pkind
    long long ix = 0, iy = 0;  // drawtext x/y
    bool has_ix = false;       // drawtext x present
    bool o0 = false, o1 = false, o2 = false;  // optional kid presence
    vector<Node> kids;
    vector<pair<string, int>> vdecls;
    vector<string> names;        // array arguments (K_PRINTARR)
    Node() : k(K_EMPTY) {}
    explicit Node(K kk) : k(kk) {}
};

// ===========================================================================
// parser
// ==========================================================================
