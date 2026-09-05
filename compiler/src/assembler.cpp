// assembler.cpp
// created: Feb 03, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// assembler.cpp - opcode encoder + assembler (2-pass, PSET)
#include "assembler.hpp"

static const map<string, int> REGMAP = {{"A", 0}, {"B", 1}, {"MX", 2}, {"MY", 3}};
static bool is_reg(const string& t) {
    string u = t;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    return REGMAP.count(u) != 0;
}
static int reg(const string& t) {
    string u = t;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    return REGMAP.at(u);
}
static long long num(const string& tin) {
    string t = tin;
    // strip spaces
    t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end());
    const char* cstr = t.c_str();
    char* end = nullptr;
    long long v;
    int base = 10;
    if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
        base = 16;
        cstr = t.c_str() + 2;
    } else if (t.size() > 2 && t[0] == '0' &&
               (t[1] == 'b' || t[1] == 'B')) {
        base = 2;
        cstr = t.c_str() + 2;
    } else if (t.size() > 1 && (t[0] == '+' || t[0] == '-')) {
        base = 10;
    }
    v = strtoll(cstr, &end, base);
    if (end == cstr || *end != '\0')
        throw AsmError("not a number: '" + tin + "'");
    return v;
}
static void chk(long long v, long long lo, long long hi, const string& what) {
    if (v < lo || v > hi) {
        char b[128];
        snprintf(b, sizeof b, "%s out of range %#llx (need %#llx..%#llx)",
                 what.c_str(), v, lo, hi);
        throw AsmError(b);
    }
}
// M0..M15 direct RAM operand
static int mnum(string t) {
    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
    if (t.size() >= 3 && t[0] == 'M' && t[1] == '(' && t.back() == ')') {
        t = "M" + t.substr(2, t.size() - 3);
    }
    if (t.size() >= 4 && (t.rfind("M0X", 0) == 0 || t.rfind("M0x", 0) == 0)) {
        t = "M" + t.substr(3);
    }
    if (t.size() >= 2 && t[0] == 'M' && t.size() <= 3) {
        const char* c = t.c_str() + 1;
        char* end;
        long n = strtol(c, &end, 16);
        if (*end == 0 && n >= 0 && n <= 15) return (int)n;
    }
    return -1;
}

// parse one source line -> list of (kind, payload) ; kinds:
//   "label" name | "org" int | "word" vector<int> | "fill" pair<int,int>
//   "code" pair<mnemonic, vector<string>>
struct Parsed {
    vector<string> labels;
    bool is_org = false; long long org_val = 0;
    bool is_word = false; vector<long long> word_vals;
    bool is_fill = false; long long fill_n = 0, fill_v = 0;
    bool is_code = false; string mnem; vector<string> ops;
};
static Parsed parse_line(const string& raw) {
    string text = raw;
    size_t sc = text.find(';');
    if (sc != string::npos) text = text.substr(0, sc);
    // trim
    size_t b = text.find_first_not_of(" \t\r\n");
    if (b == string::npos) return Parsed{};
    size_t e2 = text.find_last_not_of(" \t\r\n");
    text = text.substr(b, e2 - b + 1);
    Parsed out;
    // labels: may stand alone or precede code on the same line
    for (;;) {
        if (text.size() && text.back() == ':') {
            string nm = text.substr(0, text.size() - 1);
            size_t bs = nm.find_last_not_of(" \t");
            if (bs != string::npos) nm = nm.substr(0, bs + 1);
            out.labels.push_back(nm);
            return out;
        }
        size_t colon = text.find(':');
        if (colon != string::npos) {
            string low = text;
            std::transform(low.begin(), low.end(), low.begin(), ::tolower);
            if (!(low.rfind(".org", 0) == 0 ||
                  low.rfind(".word", 0) == 0 ||
                  low.rfind(".fill", 0) == 0)) {
                string pre = text.substr(0, colon);
                size_t pb = pre.find_first_not_of(" \t");
                if (pb != string::npos) {
                    string rest = text.substr(colon + 1);
                    size_t rb = rest.find_first_not_of(" \t");
                    if (rb == string::npos ||
                        rest.substr(rb, 1) != "=") {
                        string nm = pre.substr(pb);
                        size_t pe = nm.find_last_not_of(" \t");
                        nm = nm.substr(0, pe + 1);
                        out.labels.push_back(nm);
                        text = (rb == string::npos) ? "" : rest.substr(rb);
                        if (text.empty()) return out;
                        continue;
                    }
                }
            }
            break;
        }
        break;
    }
    if (text.empty()) return out;
    string low = text;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
    if (low.rfind(".org", 0) == 0) {
        out.is_org = true;
        size_t sp = text.find_first_of(" \t");
        out.org_val = num(text.substr(sp + 1));
        return out;
    }
    if (low.rfind(".word", 0) == 0) {
        out.is_word = true;
        size_t sp = text.find_first_of(" \t");
        string rest = text.substr(sp + 1);
        std::replace(rest.begin(), rest.end(), ',', ' ');
        std::istringstream ss(rest);
        string tk;
        while (ss >> tk) out.word_vals.push_back(num(tk));
        return out;
    }
    if (low.rfind(".fill", 0) == 0) {
        out.is_fill = true;
        size_t sp = text.find_first_of(" \t");
        string rest = text.substr(sp + 1);
        size_t comma = rest.find(',');
        out.fill_n = num(rest.substr(0, comma));
        out.fill_v = num(rest.substr(comma + 1));
        return out;
    }
    // instruction
    size_t sp = text.find_first_of(" \t");
    string mnem = (sp == string::npos) ? text : text.substr(0, sp);
    std::transform(mnem.begin(), mnem.end(), mnem.begin(), ::toupper);
    string ops = (sp == string::npos) ? "" : text.substr(sp + 1);
    // conditional jump 'JP Z, label' -> mnem "JP Z"
    if (mnem == "JP" && !ops.empty()) {
        string head = ops.substr(0, ops.find(','));
        size_t hb = head.find_first_not_of(" \t");
        size_t he = head.find_last_not_of(" \t");
        head = (hb == string::npos) ? "" : head.substr(hb, he - hb + 1);
        std::transform(head.begin(), head.end(), head.begin(), ::toupper);
        if (head == "Z" || head == "NZ" || head == "C" || head == "NC") {
            mnem = "JP " + head;
            size_t comma = ops.find(',');
            ops = ops.substr(comma + 1);
        }
    }
    // split ops
    std::replace(ops.begin(), ops.end(), ',', ' ');
    std::istringstream ss(ops);
    string tk;
    while (ss >> tk) out.ops.push_back(tk);
    out.is_code = true;
    out.mnem = mnem;
    return out;
}

static unsigned encode(const string& mn, const vector<string>& ops) {
    string m = mn;
    std::transform(m.begin(), m.end(), m.begin(), ::toupper);
    string full = m;
    for (auto& o : ops) full += " " + o;

    struct SimpleEnt { const char* s; int v; };
    static const SimpleEnt SIMPLE[] = {
        {"INC X", 0xEE0}, {"INC Y", 0xEF0}, {"SCF", 0xF41}, {"SZF", 0xF42},
        {"SDF", 0xF44}, {"EI", 0xF48}, {"DI", 0xF57}, {"RDF", 0xF5B},
        {"RZF", 0xF5D}, {"RCF", 0xF5E}, {"RETS", 0xFDE}, {"RET", 0xFDF},
        {"JPBA", 0xFE8}, {"HALT", 0xFF8}, {"SLP", 0xFF9}, {"NOP5", 0xFFB},
        {"NOP7", 0xFFF},
    };
    for (auto& s : SIMPLE)
        if (full == s.s) return s.v;
    if (ops.empty()) throw AsmError("'" + m + "' needs an operand");

    // RETD e / PSET p
    if (m == "RETD") { long long v = num(ops[0]); chk(v, 0, 0xFF, "RETD"); return 0x100 | v; }
    if (m == "PSET") { long long v = num(ops[0]); chk(v, 0, 0x1F, "PSET"); return 0xE40 | v; }

    // immediate ops on XH/XL/YH/YL
    struct { const char* n; int c; } halfs[] = {{"XH", 0}, {"XL", 1}, {"YH", 2}, {"YL", 3}};
    if (m == "ADC" || m == "CP") {
        if (ops.size() == 2) {
            for (auto& h : halfs) {
                if (ops[0] == h.n) {
                    long long v = num(ops[1]);
                    chk(v, 0, 0xF, ops[0]);
                    return ((m == "ADC") ? 0xA00 : 0xA40) | (h.c << 4) | v;
                }
            }
        }
    }
    string o0 = ops[0];
    string o1 = ops.size() > 1 ? ops[1] : string();
    // LD Y, imm8
    if (m == "LD" && o0 == "Y") {
        long long v = num(o1); chk(v, 0, 0xFF, "LD Y"); return 0x800 | v;
    }
    // LBPX MX, imm8
    if (m == "LBPX" && o0 == "MX") {
        long long v = num(o1); chk(v, 0, 0xFF, "LBPX"); return 0x900 | v;
    }
    // LD X, imm8
    if (m == "LD" && o0 == "X") {
        long long v = num(o1); chk(v, 0, 0xFF, "LD X"); return 0xB00 | v;
    }
    // LDPX MX, imm / LDPY MY, imm
    if (m == "LDPX" && o0 == "MX" && !is_reg(o1)) {
        long long v = num(o1); chk(v, 0, 0xF, "LDPX"); return 0xE60 | v;
    }
    if (m == "LDPY" && o0 == "MY" && !is_reg(o1)) {
        long long v = num(o1); chk(v, 0, 0xF, "LDPY"); return 0xE70 | v;
    }
    // LD XP/XH/XL/YP/YH/YL, r | LD r, XP/...
    struct { const char* n; int c; } dsts[] = {
        {"XP", 0xE80}, {"XH", 0xE84}, {"XL", 0xE88},
        {"YP", 0xE90}, {"YH", 0xE94}, {"YL", 0xE98}};
    struct { const char* n; int c; } srcs[] = {
        {"XP", 0xEA0}, {"XH", 0xEA4}, {"XL", 0xEA8},
        {"YP", 0xEB0}, {"YH", 0xEB4}, {"YL", 0xEB8}};
    if (m == "LD" && ops.size() == 2) {
        for (auto& d : dsts)
            if (o0 == d.n && is_reg(o1)) return d.c | reg(o1);
        for (auto& s2 : srcs)
            if (o1 == s2.n && is_reg(o0)) return s2.c | reg(o0);
        if (o0 == "SPH" && is_reg(o1)) return 0xFE0 | reg(o1);
        if (o0 == "SPL" && is_reg(o1)) return 0xFF0 | reg(o1);
        if (o1 == "SPH" && is_reg(o0)) return 0xFE4 | reg(o0);
        if (o1 == "SPL" && is_reg(o0)) return 0xFF4 | reg(o0);
    }
    // direct RAM M0..M15
    if (ops.size() == 2) {
        int n = mnum(o0);
        if (n >= 0 && m == "LD" && o1 == "A") return 0xF80 | n;
        if (n >= 0 && m == "LD" && o1 == "B") return 0xF90 | n;
        n = mnum(o1);
        if (n >= 0 && m == "LD" && o0 == "A") return 0xFA0 | n;
        if (n >= 0 && m == "LD" && o0 == "B") return 0xFB0 | n;
    }
    if (ops.size() == 1) {
        int n = mnum(o0);
        if (n >= 0 && m == "INC") return 0xF60 | n;
        if (n >= 0 && m == "DEC") return 0xF70 | n;
    }
    // register-register ALU
    struct { const char* n; int c; } alu_rr[] = {
        {"ADD", 0xA80}, {"ADC", 0xA90}, {"SUB", 0xAA0}, {"SBC", 0xAB0},
        {"AND", 0xAC0}, {"OR", 0xAD0}, {"XOR", 0xAE0},
        {"CP", 0xF00}, {"FAN", 0xF10}};
    if (ops.size() == 2) {
        for (auto& a : alu_rr)
            if (m == a.n && is_reg(o0) && is_reg(o1))
                return a.c | (reg(o0) << 2) | reg(o1);
    }
    // LD r,q / LDPX r,q / LDPY r,q / ACPX/ACPY/SCPX/SCPY
    if (m == "LD" && ops.size() == 2 && is_reg(o0) && is_reg(o1))
        return 0xEC0 | (reg(o0) << 2) | reg(o1);
    if (m == "LDPX" && ops.size() == 2 && is_reg(o0) && is_reg(o1))
        return 0xEE0 | (reg(o0) << 2) | reg(o1);
    if (m == "LDPY" && ops.size() == 2 && is_reg(o0) && is_reg(o1))
        return 0xEF0 | (reg(o0) << 2) | reg(o1);
    struct { const char* n; int c; } acs[] = {
        {"ACPX", 0xF28}, {"ACPY", 0xF2C}, {"SCPX", 0xF38}, {"SCPY", 0xF3C}};
    for (auto& a : acs) {
        string suf = string(a.n).substr(2);
        if (m == a.n && ops.size() == 2 && (o0 == "MX" || o0 == "MY" || o0 == suf)) {
            if (is_reg(o1)) return a.c | reg(o1);
        }
    }
    // RLC / RRC / NOT
    if (ops.size() == 1 && is_reg(o0)) {
        if (m == "RLC") return 0xAF0 | reg(o0);
        if (m == "RRC") return 0xE8C | reg(o0);
        if (m == "NOT") return 0xD00 | (reg(o0) << 4) | 0xF;
    }
    // immediate ALU on regs (incl LD r, imm)
    struct { const char* n; int c; } alu_ri[] = {
        {"ADD", 0xC00}, {"ADC", 0xC40}, {"AND", 0xC80}, {"OR", 0xCC0},
        {"XOR", 0xD00}, {"SBC", 0xD40}, {"FAN", 0xD80}, {"CP", 0xDC0},
        {"LD", 0xE00}};
    if (ops.size() == 2 && is_reg(o0) && !is_reg(o1)) {
        long long v = num(o1);
        for (auto& a : alu_ri) {
            if (m != a.n) continue;
            if (m == "XOR" && v == 0xF)   // XOR r,0xF == NOT r
                return 0xD00 | (reg(o0) << 4) | 0xF;
            chk(v, 0, 0xF, m + " " + o0);
            return a.c | (reg(o0) << 4) | v;
        }
    }
    // PUSH / POP
    struct { const char* n; int c; } push_t[] = {
        {"XP", 0xFC4}, {"XH", 0xFC5}, {"XL", 0xFC6}, {"YP", 0xFC7},
        {"YH", 0xFC8}, {"YL", 0xFC9}, {"F", 0xFCA}, {"SP", 0xFCB}};
    struct { const char* n; int c; } pop_t[] = {
        {"XP", 0xFD4}, {"XH", 0xFD5}, {"XL", 0xFD6}, {"YP", 0xFD7},
        {"YH", 0xFD8}, {"YL", 0xFD9}, {"F", 0xFDA}, {"SP", 0xFDB}};
    if (m == "PUSH" && ops.size() == 1) {
        if (is_reg(o0)) return 0xFC0 | reg(o0);
        for (auto& p : push_t)
            if (o0 == p.n) return p.c;
    }
    if (m == "POP" && ops.size() == 1) {
        if (is_reg(o0)) return 0xFD0 | reg(o0);
        for (auto& p : pop_t)
            if (o0 == p.n) return p.c;
    }
    // SET F,i / RST F,i
    if ((m == "SET" || m == "RST") && ops.size() == 2 && o0 == "F")
        return ((m == "SET") ? 0xF40 : 0xF50) | (int)num(o1);

    string msg = "cannot encode: " + m;
    for (auto& o : ops) msg += " " + o;
    throw AsmError(msg);
}



void Assembler::emit(int op) {
        if (addr >= 0x1800)
            throw AsmError("ROM full at 0x" + hexstr((unsigned)addr) +
                           " (max 0x1800)");
        words[(int)addr] = op & 0xFFF;
        addr++;
    }
int Assembler::page_of(int a) { return (a >> 8) & 0xF; }
int Assembler::bank_of(int a) { return a < 0x1000 ? 0 : 1; }
void Assembler::assemble(const vector<string>& lines) {
        // pass 1: labels
        for (auto& raw : lines) {
            Parsed p = parse_line(raw);
            for (auto& lab : p.labels) {
                if (labels.count(lab))
                    throw AsmError("duplicate label '" + lab + "'");
                labels[lab] = (int)addr;
            }
            if (p.is_org) addr = p.org_val;
            else if (p.is_word) addr += (long long)p.word_vals.size();
            else if (p.is_fill) addr += p.fill_n;
            else if (p.is_code) addr += 1;
        }
        // pass 2: encode
        addr = org;
        words.clear();
        bool pending_set = false;
        int pending_bank = 0, pending_page = 0;
        auto resolve = [&](const string& target) -> long long {
            string t = target;
            // trim
            size_t b = t.find_first_not_of(" \t");
            if (b != string::npos) t = t.substr(b);
            // try number
            try {
                long long v = num(t);
                chk(v, 0, 0xFF, "branch step");
                return v;
            } catch (AsmError&) {}
            if (!labels.count(t)) throw AsmError("unknown label '" + t + "'");
            int dest = labels[t];
            int db = bank_of(dest), dp = page_of(dest);
            int here = (int)addr - 1;
            if (pending_set) {
                int pb = pending_bank, pp = pending_page;
                if (pb != db || pp != dp) {
                    char b[128];
                    snprintf(b, sizeof b,
                             "branch '%s' (0x%04X, bank%d/page%d) not "
                             "reachable: PSET selected bank%d/page%d",
                             t.c_str(), dest, db, dp, pb, pp);
                    throw AsmError(b);
                }
                pending_set = false;
            } else {
                int cb = bank_of(here), cp = page_of(here);
                if (cb != db || cp != dp) {
                    char b[192];
                    snprintf(b, sizeof b,
                             "branch '%s' (0x%04X, bank%d/page%d) not in "
                             "current page %d (branch at 0x%04X); "
                             "insert 'PSET 0x%X' first",
                             t.c_str(), dest, db, dp, cp, here,
                             dp | (db << 4));
                    throw AsmError(b);
                }
            }
            return dest & 0xFF;
        };
        for (size_t li = 0; li < lines.size(); li++) {
            const string& raw = lines[li];
            try {
            Parsed p = parse_line(raw);
            if (!p.labels.empty()) { continue; }
            if (p.is_org) {
                addr = p.org_val;
                pending_set = false;
                continue;
            }
            if (p.is_word) {
                for (auto w : p.word_vals) emit((int)w);
                continue;
            }
            if (p.is_fill) {
                for (long long i = 0; i < p.fill_n; i++) emit((int)p.fill_v);
                continue;
            }
            if (!p.is_code) continue;
            if (p.mnem == "JP") emit((int)(0x000 | resolve(p.ops[0])));
            else if (p.mnem == "JP Z") emit((int)(0x600 | resolve(p.ops[0])));
            else if (p.mnem == "JP NZ") emit((int)(0x700 | resolve(p.ops[0])));
            else if (p.mnem == "JP C") emit((int)(0x200 | resolve(p.ops[0])));
            else if (p.mnem == "JP NC") emit((int)(0x300 | resolve(p.ops[0])));
            else if (p.mnem == "CALL") emit((int)(0x400 | resolve(p.ops[0])));
            else if (p.mnem == "CALZ") {
                string t = p.ops[0];
                long long v;
                try {
                    v = num(t);
                } catch (AsmError&) {
                    if (!labels.count(t))
                        throw AsmError("unknown label '" + t + "'");
                    int dest = labels[t];
                    if (page_of(dest) != 0)
                        throw AsmError("CALZ target '" + t +
                                       "' (0x" + hexstr((unsigned)dest) +
                                       ") not in page 0");
                    v = dest & 0xFF;
                }
                chk(v, 0, 0xFF, "CALZ");
                emit((int)(0x500 | v));
            } else if (p.mnem == "PSET") {
                long long pv = num(p.ops[0]);
                chk(pv, 0, 0x1F, "PSET");
                pending_set = true;
                pending_bank = (int)(pv >> 4);
                pending_page = (int)(pv & 0xF);
                emit((int)(0xE40 | pv));
            } else {
                emit((int)encode(p.mnem, p.ops));
            }
            } catch (std::exception& e) {
                fprintf(stderr, "[asm] line %zu: %s\n", li, e.what());
                throw;
            }
        }
    }
