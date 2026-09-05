// codegen.hpp
// created: Feb 05, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// codegen.hpp - code generator: AST -> E0C6S46 assembly
#pragma once
#include "common.hpp"
#include "ast.hpp"
#include "font.hpp"

// Runtime assembly texts (defined in codegen.cpp)
const char* runtime_text_def(const char* name);

class Gen {
public:
vector<string>* outp = nullptr;
long long lbl = 0;
map<string, pair<int, int>> vars;
// name -> (addr, size)
    vector<string> varOrder;
int var_addr = 0x40;
int temp_addr = 0x38;
int depth = 0;
set<string> funcs;
set<string> used_runtime;
vector<string> brk;    // break targets (loop/switch)
vector<string> cont;   // continue targets (loop)
vector<string> bodyBuf;
// scratch for phase 1
string newlbl(const string& tag);
void emit(const string& line);
void label(const string& name);
void rawline(const string& l);
void alloc_var(const string& name, int size);
void decl_vars(const vector<pair<string, int>>& vv);
int var_base(const string& name);
// ---------- top level ----------
void gen_all(Node& prog);
vector<string> finalAsm;
static void split_append(const char* text, vector<string>& out);
static vector<string> disp_digit_runtime_lines();
static vector<int> digit_bytes(int d);
static const char* runtime_text(const char* name);
static vector<string> peephole_optimize(const vector<string>& lines);
// ---------- functions / statements ----------
void gen_func(Node& f);
void gen_block(Node& b);
void gen_stmt(Node& s);
// ---------- expressions (result in A) ----------
void gen_expr(Node& e, bool discard = false);
void gen_binop(const string& op, Node& l, Node& r);
void gen_cmp(const string& op, Node& l, Node& r);
bool cmp_simple(Node& c);
void emit_cmp_branch(Node& c, const string& skip);
void emit_branch_if_false(Node& cond, const string& skip);
void cmp_after_compare(const string& op);
void set_a_bool(const string& cond);
void set_bool_from_flags(const vector<string>& true_when);
void alu_instr(const string& op);
int push_temp();
void pop_temp();
void load_a_var(const string& name, int idx);
void store_a(Node& lhs);
void load_a_dyn(Node& arr);   // A = arr[i], i any expr
void store_a_dyn(Node& arr);  // A = value to store at arr[i]
void dyn_advance(Node& arr, const string& done); // X = base, skip i cells
// ---------- draw_text / draw_sprite ----------
void gen_drawtext(bool hasx, int x, int y, const string& text);
void gen_drawsprite(const string& name);
void gen_seticon(Node& expr);
void gen_clearicons();
void gen_clearvram();
// ---------- Pointer/string helpers ----------
void load_pointer_x(const string& name);
void load_pointer_y(const string& name);
void gen_settext(const string& name, const string& text);
void gen_assert(Node& e);
// ---------- Array-Builtins (8/16-bit) ----------
void gen_arrfn(const string& fn, const vector<string>& names);
// ---------- print -> debug channel (inline) ----------
void gen_dbgprint(const string& text);
};
