// parser.hpp
// created: Jan 30, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// parser.hpp - recursive-descent parser: tokens -> AST
#pragma once
#include "common.hpp"
#include "lexer.hpp"
#include "ast.hpp"

class Parser {
vector<Tok> t;
size_t p = 0;
public:
explicit Parser(vector<Tok> toks);
Tok& peek();
Tok next();
string expect(const string& val);
bool isa(const string& val);
string expect_id();
// decimal only, like the Python original (int(v,10))
static int parse_dec(const string& v);
int expect_num();
Node parse_program();
Node parse_top();
Node parse_block();
Node parse_stmt();
// ---- expressions ----
Node parse_expr();
Node parse_assign();
Node parse_cmp();
Node parse_bitor();
Node parse_bitxor();
Node parse_bitand();
Node parse_add();
Node parse_unary();
Node parse_postfix();
Node parse_primary();
};
