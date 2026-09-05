// parser.cpp
// created: Feb 06, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
#include "parser.hpp"

Parser::Parser(vector<Tok> toks) : t(std::move(toks)) {}
Tok& Parser::peek() { return t[p]; }
Tok Parser::next() { return t[p++]; }
string Parser::expect(const string& val) {
        Tok tk = next();
        if (tk.v != val)
            throw CompileError("expected '" + val + "', got '" + tk.v + "'");
        return tk.v;
    }
bool Parser::isa(const string& val) { return peek().v == val; }
string Parser::expect_id() {
        Tok tk = next();
        if (tk.k != "id")
            throw CompileError("expected identifier, got '" + tk.v + "'");
        return tk.v;
    }
int Parser::parse_dec(const string& v) {
        if (v.empty()) throw CompileError("invalid number");
        if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0) {
            return (int)strtol(v.c_str() + 2, nullptr, 16);
        }
        if (v.rfind("0b", 0) == 0 || v.rfind("0B", 0) == 0) {
            return (int)strtol(v.c_str() + 2, nullptr, 2);
        }
        int n = 0;
        for (char c : v) {
            if (c < '0' || c > '9')
                throw CompileError("invalid literal for int() with base 10: '" +
                                   v + "'");
            n = n * 10 + (c - '0');
        }
        return n;
    }
int Parser::expect_num() {
        Tok tk = next();
        if (tk.k != "num")
            throw CompileError("expected number, got '" + tk.v + "'");
        return parse_dec(tk.v);
    }
Node Parser::parse_program() {
        Node prog(K_PROGRAM);
        while (peek().k != "eof") prog.kids.push_back(parse_top());
        return prog;
    }
Node Parser::parse_top() {
        string typ = next().v;          // char | void
        string name = expect_id();
        if (isa("(")) {
            next();                     // (
            if (isa("void")) next();     // optional 'void'
            expect(")");
            Node f(K_FUNC);
            f.s = name;
            f.s2 = typ;
            f.kids.push_back(parse_block());
            return f;
        }
        int first_size = 1;
        if (isa("[")) {
            next();
            first_size = expect_num();
            expect("]");
        }
        Node vd(K_VARS);
        vd.vdecls.push_back({name, first_size});
        while (isa(",")) {
            next();
            string nm = expect_id();
            int size = 1;
            if (isa("[")) {
                next();
                size = expect_num();
                expect("]");
            }
            vd.vdecls.push_back({nm, size});
        }
        expect(";");
        return vd;
    }
Node Parser::parse_block() {
        expect("{");
        Node b(K_BLOCK);
        while (!isa("}")) b.kids.push_back(parse_stmt());
        expect("}");
        return b;
    }
Node Parser::parse_stmt() {
        const string& v = peek().v;
        if (v == "{") return parse_block();
        if (v == ";") { next(); return Node(K_EMPTY); }
        if (v == "if") {
            next();
            expect("(");
            Node n(K_IF);
            n.kids.push_back(parse_expr());
            expect(")");
            n.kids.push_back(parse_stmt());
            if (isa("else")) {
                next();
                n.o2 = true;
                n.kids.push_back(parse_stmt());
            }
            return n;
        }
        if (v == "while") {
            next();
            expect("(");
            Node n(K_WHILE);
            n.kids.push_back(parse_expr());
            expect(")");
            n.kids.push_back(parse_stmt());
            return n;
        }
        if (v == "for") {
            next();
            expect("(");
            Node n(K_FOR);
            if (!isa(";")) { n.o0 = true; n.kids.push_back(parse_expr()); }
            expect(";");
            if (!isa(";")) { n.o1 = true; n.kids.push_back(parse_expr()); }
            expect(";");
            if (!isa(")")) { n.o2 = true; n.kids.push_back(parse_expr()); }
            expect(")");
            n.kids.push_back(parse_stmt());   // body
            return n;
        }
        if (v == "return") {
            next();
            Node n(K_RETURN);
            if (!isa(";")) { n.o0 = true; n.kids.push_back(parse_expr()); }
            expect(";");
            return n;
        }
        if (v == "break") {
            next();
            expect(";");
            return Node(K_BREAK);
        }
        if (v == "continue") {
            next();
            expect(";");
            return Node(K_CONTINUE);
        }
        if (v == "do") {
            // do <stmt> while (cond) ;
            next();
            Node n(K_DO);
            n.kids.push_back(parse_stmt());          // body
            if (peek().v != "while")
                throw CompileError("do requires 'while (cond);'");
            next();
            expect("(");
            n.kids.push_back(parse_expr());          // cond
            expect(")");
            expect(";");
            return n;
        }
        if (v == "switch") {
            // switch (e) { case k: ... default: ... }
            next();
            expect("(");
            Node n(K_SWITCH);
            n.kids.push_back(parse_expr());
            expect(")");
            expect("{");
            while (!isa("}")) {
                if (isa("case")) {
                    next();
                    Node c(K_CASE);
                    c.iv = expect_num();
                    expect(":");
                    while (!isa("case") && !isa("default") && !isa("}"))
                        c.kids.push_back(parse_stmt());
                    n.kids.push_back(c);
                } else if (isa("default")) {
                    next();
                    Node c(K_CASE);
                    c.iv = -1;
                    expect(":");
                    while (!isa("}"))
                        c.kids.push_back(parse_stmt());
                    n.kids.push_back(c);
                } else {
                    throw CompileError(
                        "switch body requires 'case k:' or 'default:'");
                }
            }
            expect("}");
            return n;
        }
        if (v == "char") {
            next();
            Node n(K_LOCALVARS);
            // local vars: first name may carry an array size
            string nm = expect_id();
            int size = 1;
            if (isa("[")) {
                next();
                size = expect_num();
                expect("]");
            }
            n.vdecls.push_back({nm, size});
            while (isa(",")) {
                next();
                n.vdecls.push_back({expect_id(), 1});
            }
            expect(";");
            return n;
        }
        Node e(K_EXPR);
        e.kids.push_back(parse_expr());
        expect(";");
        return e;
    }
Node Parser::parse_expr() { return parse_assign(); }
Node Parser::parse_assign() {
        Node left = parse_cmp();
        if (peek().v == "=") {
            next();
            Node n(K_ASSIGN);
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_assign());
            return n;
        }
        return left;
    }
Node Parser::parse_cmp() {
        Node left = parse_bitor();
        while (peek().v == "==" || peek().v == "!=" || peek().v == "<" ||
               peek().v == ">" || peek().v == "<=" || peek().v == ">=") {
            string op = next().v;
            Node n(K_CMP);
            n.s = op;
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_bitor());
            left = std::move(n);
        }
        return left;
    }
Node Parser::parse_bitor() {
        Node left = parse_bitxor();
        while (isa("|")) {
            next();
            Node n(K_BIN);
            n.s = "|";
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_bitxor());
            left = std::move(n);
        }
        return left;
    }
Node Parser::parse_bitxor() {
        Node left = parse_bitand();
        while (isa("^")) {
            next();
            Node n(K_BIN);
            n.s = "^";
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_bitand());
            left = std::move(n);
        }
        return left;
    }
Node Parser::parse_bitand() {
        Node left = parse_add();
        while (isa("&")) {
            next();
            Node n(K_BIN);
            n.s = "&";
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_add());
            left = std::move(n);
        }
        return left;
    }
Node Parser::parse_add() {
        Node left = parse_unary();
        while (peek().v == "+" || peek().v == "-") {
            string op = next().v;
            Node n(K_BIN);
            n.s = op;
            n.kids.push_back(std::move(left));
            n.kids.push_back(parse_unary());
            left = std::move(n);
        }
        return left;
    }
Node Parser::parse_unary() {
        const string& v = peek().v;
        if (v == "-" || v == "~" || v == "!") {
            next();
            Node n(K_UN);
            n.s = v;
            n.kids.push_back(parse_unary());
            return n;
        }
        if (v == "++" || v == "--") {
            next();
            Node n(K_PREINC);
            n.s = v;
            n.kids.push_back(parse_unary());
            return n;
        }
        if (v == "*") {
            // Dereferenzierung: *name   (name = Zeigervariable)
            next();
            Node n(K_DEREF);
            n.s = expect_id();
            return n;
        }
        if (v == "&") {
            // Adresse: &name  (nur als Zuweisungs-RHS sinnvoll)
            next();
            Node n(K_ADDR);
            n.s = expect_id();
            return n;
        }
        return parse_postfix();
    }
Node Parser::parse_postfix() {
        Node e = parse_primary();
        while (peek().v == "++" || peek().v == "--") {
            string op = next().v;
            Node n(K_POSTINC);
            n.s = op;
            n.kids.push_back(std::move(e));
            e = std::move(n);
        }
        return e;
    }
Node Parser::parse_primary() {
        Tok tk = peek();
        if (tk.k == "num") {
            next();
            Node n(K_NUM);
            n.iv = parse_dec(tk.v);
            return n;
        }
        if (tk.k == "id") {
            next();
            if (isa("(")) {
                next();
                if (tk.v == "dbg_print" || tk.v == "print") {
                    Tok s = next();
                    if (s.k != "str")
                        throw CompileError(
                            "print() needs a string literal argument");
                    expect(")");
                    Node n(K_PRINT);
                    n.s = s.v;
                    return n;
                }
                if (tk.v == "print_hex" || tk.v == "print_dec") {
                    Node n(K_PRINTNUM);
                    n.s = tk.v;
                    n.kids.push_back(parse_expr());
                    expect(")");
                    return n;
                }
                if (tk.v == "print_int" || tk.v == "print_hex8" ||
                    tk.v == "print_hex16" || tk.v == "print_int16" ||
                    tk.v == "inc16" || tk.v == "add16" ||
                    tk.v == "sub16" || tk.v == "cmp16" ||
                    tk.v == "mul8" || tk.v == "rand16") {
                    // Argumente = Namen von Arrays (2/4 Nibbles, big-endian)
                    Node n(K_PRINTARR);
                    n.s = tk.v;
                    n.names.push_back(expect_id());
                    while (isa(",")) {
                        next();
                        n.names.push_back(expect_id());
                    }
                    expect(")");
                    return n;
                }
                if (tk.v == "draw_text") {
                    // draw_text(x, y, "text") | draw_text(y, "text") | "text"
                    bool hasx = false;
                    long long x = 0, y = 0;
                    if (peek().k == "num") {
                        y = expect_num();
                        if (isa(",")) {
                            next();
                            if (peek().k == "num") {
                                x = y;          // first was x
                                hasx = true;
                                y = expect_num();
                                if (isa(",")) next();
                            }
                        }
                    }
                    Tok s = next();
                    if (s.k != "str")
                        throw CompileError(
                            "draw_text() needs a string literal");
                    expect(")");
                    Node n(K_DRAWTEXT);
                    n.s = s.v;
                    n.ix = x;
                    n.iy = y;
                    n.has_ix = hasx;
                    return n;
                }
                if (tk.v == "draw_sprite") {
                    Tok s = next();
                    if (s.k != "str")
                        throw CompileError("draw_sprite() needs a string literal sprite name");
                    expect(")");
                    Node n(K_DRAWSPRITE);
                    n.s = s.v;
                    return n;
                }
                if (tk.v == "set_icon") {
                    Node n(K_SETICON);
                    n.kids.push_back(parse_expr());
                    expect(")");
                    return n;
                }
                if (tk.v == "clear_icons") {
                    expect(")");
                    return Node(K_CLEARICONS);
                }
                if (tk.v == "clear_vram") {
                    expect(")");
                    return Node(K_CLEARVRAM);
                }
                if (tk.v == "display_digit") {
                    Node n(K_DISPDIGIT);
                    n.kids.push_back(parse_expr());
                    expect(")");
                    return n;
                }
                if (tk.v == "read_key") {
                    expect(")");
                    return Node(K_READKEY);
                }
                if (tk.v == "set_text") {
                    // set_text(buf, "literal")
                    Node n(K_SETTEXT);
                    n.s = expect_id();
                    expect(",");
                    Tok st = next();
                    if (st.k != "str")
                        throw CompileError(
                            "set_text() requires a string literal");
                    n.s2 = st.v;
                    expect(")");
                    return n;
                }
                if (tk.v == "assert") {
                    Node n(K_ASSERTSTMT);
                    n.kids.push_back(parse_expr());
                    expect(")");
                    return n;
                }
                if (tk.v == "print_num") {
                    // print_num("text", expr)
                    Tok st = next();
                    if (st.k != "str")
                        throw CompileError(
                            "print_num() requires a string literal");
                    expect(",");
                    Node n(K_PRINTMSG);
                    n.s = st.v;                       // Text
                    n.kids.push_back(parse_expr());   // Wert
                    expect(")");
                    return n;
                }
                if (tk.v == "print_str" || tk.v == "strlen" ||
                    tk.v == "strcpy" || tk.v == "strcmp") {
                    Node n(K_PRINTARR);
                    n.s = tk.v;
                    n.names.push_back(expect_id());
                    while (isa(",")) {
                        next();
                        n.names.push_back(expect_id());
                    }
                    expect(")");
                    return n;
                }
                if (tk.v == "memset") {
                    // memset(dst, val, n): n Nibbles ab dst mit val fuellen
                    Node n(K_MEMFN);
                    n.s = "memset";
                    n.s2 = expect_id();
                    expect(",");
                    n.kids.push_back(parse_expr());   // val
                    expect(",");
                    n.kids.push_back(parse_expr());   // n
                    expect(")");
                    return n;
                }
                if (tk.v == "memcpy") {
                    // memcpy(dst, src, n): n Nibbles von src nach dst
                    Node n(K_MEMFN);
                    n.s = "memcpy";
                    n.s2 = expect_id();               // dst
                    expect(",");
                    Node src(K_VAR);
                    src.s = expect_id();              // src
                    n.kids.push_back(src);
                    expect(",");
                    n.kids.push_back(parse_expr());   // n
                    expect(")");
                    return n;
                }
                expect(")");
                Node n(K_CALL);
                n.s = tk.v;
                return n;
            }
            if (isa("[")) {
                next();
                if (peek().k == "num") {
                    Node n(K_ARR);
                    n.s = tk.v;
                    n.iv = expect_num();
                    expect("]");
                    return n;
                }
                // dynamic index: a[i] (i any expression, 0..15)
                Node n(K_ARRI);
                n.s = tk.v;
                n.kids.push_back(parse_expr());
                expect("]");
                return n;
            }
            Node n(K_VAR);
            n.s = tk.v;
            return n;
        }
        if (tk.v == "(") {
            next();
            Node e = parse_expr();
            expect(")");
            return e;
        }
        throw CompileError("unexpected token '" + tk.v + "' in expression");
    }
