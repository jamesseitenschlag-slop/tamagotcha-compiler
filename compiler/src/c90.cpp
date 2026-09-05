// ============================================================================
// c90.cpp - native (C++) Erweiterung des C-Subset fuer den E0C6S46.
//
// Arbeitsweise: Die erweiterten Konstrukte werden VOR dem Parsen in das
// Kern-Subset uebersetzt (ein reiner C++-Schritt, kein Python).
//
// Unterstuetzt:
//   Typen         char, int, short, long, signed, unsigned, void
//                 (alle Ganzzahltypen = 1 Nibble, wie 'char')
//   Speicherkl.   auto, const, extern, register, static, volatile (no-op)
//   typedef       Skalar-Aliase, struct/union-Aliase
//   enum          Konstanten 0..15 (+ optionaler Tag)
//   struct        globale Variablen, Member-Zugriff s.f
//   union         globale Variablen (alle Member = erster Nibble)
//   sizeof        sizeof(Typ|Tag|globale Var|Array) -> Nibbles
//
// Nicht unterstuetzt (4-Bit-Ziel): float, double, goto.
// ============================================================================
#include "c90.hpp"
#include "lexer.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <map>

using std::string;
using std::vector;

namespace c90 {

namespace {
const std::set<string> MODS = {"auto", "register", "static", "extern",
                               "volatile", "const", "signed", "unsigned"};
const std::set<string> SCALAR = {"char", "int", "short", "long"};
const std::set<string> TRIGGER = {
    "typedef", "struct", "union", "enum", "sizeof",
    "int", "unsigned", "signed", "short", "long",
    "auto", "register", "extern", "volatile",
    "float", "double", "goto"};

struct Expander {
    vector<Tok> t;
    size_t p = 0;
    vector<Tok> out;   // tokenisiertes Ergebnis

    // Symboltabellen
    std::map<string, string> aliasType;         // alias -> "scalar"/"struct:Tag"
    std::map<string, std::pair<vector<string>, bool>> tags;  // Tag -> (Felder, union)
    std::map<string, int> enumConsts;
    std::map<string, int> varSize;
    std::map<string, string> structOfVar;       // VarName -> Tag

    // ---- Token-Hilfen -----------------------------------------------------
    Tok& peek(size_t off = 0) {
        size_t i = p + off;
        return t[i < t.size() ? i : t.size() - 1];
    }
    Tok next() { return t[p < t.size() ? p++ : p]; }
    bool isSym(const string& s, size_t off = 0) {
        return peek(off).v == s;
    }
    bool isId(const string& s, size_t off = 0) {
        return peek(off).k == "id" && peek(off).v == s;
    }
    void emit(const string& k, const string& v) { out.push_back({k, v}); }
    bool isTypename(const string& w) {
        return SCALAR.count(w) || aliasType.count(w) || tags.count(w);
    }
    int typeSize(const string& w) {
        if (SCALAR.count(w)) return 1;
        if (aliasType.count(w)) {
            const string& a = aliasType[w];
            if (a == "scalar") return 1;
            auto it = tags.find(a.substr(7));   // "struct:Tag" | "union:Tag"
            if (it != tags.end()) return it->second.second ? 1
                                    : (int)it->second.first.size();
        }
        auto it = tags.find(w);
        if (it != tags.end())
            return it->second.second ? 1 : (int)it->second.first.size();
        throw CompileError("unbekannter Typ '" + w + "'");
    }

    string expectId() {
        Tok tk = next();
        if (tk.k != "id") throw CompileError("expected identifier, got '" + tk.v + "'");
        return tk.v;
    }
    void expectSym(const string& s) {
        Tok tk = next();
        if (tk.v != s) throw CompileError("expected '" + s + "', got '" + tk.v + "'");
    }
    int expectNum() {
        Tok tk = next();
        if (tk.k != "num") throw CompileError("expected number, got '" + tk.v + "'");
        return ParserNum(tk.v);
    }
    static int ParserNum(const string& v);

    // ---- typedef ----------------------------------------------------------
    void parse_typedef() {
        next();                                    // typedef
        Tok& first = peek();
        if (first.v == "struct" || first.v == "union") {
            bool isU = (next().v == "union");
            string tag;
            if (peek().k == "id") tag = next().v;
            vector<string> fields;
            bool haveFields = false;
            if (isSym("{")) { fields = parse_fields(); haveFields = true; }
            string alias = expectId();
            expectSym(";");
            if (haveFields) tags[alias] = {fields, isU};
            else if (!tag.empty() && tags.count(tag)) tags[alias] = tags[tag];
            else throw CompileError("typedef struct/union braucht Rumpf oder Tag");
            aliasType[alias] = isU ? "union:" + alias : "struct:" + alias;
            return;
        }
        vector<string> words;
        while (peek().k != "eof") {
            const string& v = peek().v;
            if (MODS.count(v)) { next(); continue; }
            if (SCALAR.count(v)) { words.push_back(v); next(); continue; }
            break;
        }
        if (words.empty()) throw CompileError("typedef: Typ fehlt");
        string alias = expectId();
        expectSym(";");
        aliasType[alias] = "scalar";
    }

    // ---- enum -------------------------------------------------------------
    void parse_enum() {
        next();                                     // enum
        string tag;
        if (peek().k == "id") tag = next().v;
        if (!isSym("{"))
            throw CompileError("enum nur mit Konstantenliste: enum [Tag] { .. };");
        next();                                     // {
        int val = 0;
        while (!isSym("}")) {
            string name = expectId();
            if (isSym("=")) { next(); val = expectNum(); }
            if (val < 0 || val > 15)
                throw CompileError("enum '" + name + "': Wert passt nicht ins Nibble (0..15)");
            enumConsts[name] = val;
            val = (val + 1) % 16;
            if (isSym(",")) next();
        }
        next();                                     // }
        while (peek().v != ";" && peek().k != "eof") {
            if (isSym(",")) { next(); continue; }
            if (peek().k == "id") {
                string name = next().v;
                if (varSize.count(name)) throw CompileError("doppelte Variable '" + name + "'");
                varSize[name] = 1;
                emit("id", "char"); emit("id", name); emit("sym", ";");
            } else next();
        }
        if (isSym(";")) next();
        if (!tag.empty()) tags[tag] = {{}, false};
    }

    // ---- struct / union ---------------------------------------------------
    vector<string> parse_fields() {
        expectSym("{");
        vector<string> fields;
        while (!isSym("}")) {
            Tok& tk = peek();
            const string& v = tk.v;
            if (MODS.count(v) || isTypename(v)) { next(); continue; }
            if (tk.k == "id") {
                string f = next().v;
                if (isSym("["))
                    throw CompileError("struct-Felder sind skalare Nibbles");
                fields.push_back(f);
                if (isSym(",")) next();
            } else if (isSym(";")) next();
            else throw CompileError("unerwartetes Token in struct/union: '" + v + "'");
        }
        expectSym("}");
        return fields;
    }

    void expandVar(const string& name, const string& tag,
                   const vector<string>& fields, bool isU) {
        if (varSize.count(name)) throw CompileError("doppelte Variable '" + name + "'");
        structOfVar[name] = tag;
        varSize[name] = isU ? 1 : (int)fields.size();
        if (isU) {
            emit("id", "char");
            emit("id", name + "_" + fields[0]);
            emit("sym", ";");
            return;
        }
        for (const string& f : fields) {
            emit("id", "char");
            emit("id", name + "_" + f);
            emit("sym", ";");
        }
    }

    void declScalar(const string& name) {
        if (varSize.count(name)) throw CompileError("doppelte Variable '" + name + "'");
        varSize[name] = 1;
        emit("id", "char"); emit("id", name); emit("sym", ";");
    }

    void parse_su() {
        bool isU = (next().v == "union");           // struct | union
        string tag;
        if (peek().k == "id") tag = next().v;
        vector<string> fields;
        bool haveFields = false;
        if (isSym("{")) { fields = parse_fields(); haveFields = true;
                          if (!tag.empty()) tags[tag] = {fields, isU}; }
        else {
            auto it = tags.find(tag);
            if (it == tags.end())
                throw CompileError("struct/union ohne bekannten Tag oder Rumpf");
            fields = it->second.first;
        }
        while (peek().v != ";" && peek().k != "eof") {
            if (isSym(",")) { next(); continue; }
            if (peek().k == "id")
                expandVar(next().v, tag.empty() ? "anon" : tag, fields, isU);
            else next();
        }
        if (isSym(";")) next();
    }

    // ---- normale Top-Level-Zeile ------------------------------------------
    void parse_toplevel() {
        string typ = "char";
        while (true) {
            Tok& tk = peek();
            const string& v = tk.v;
            if (MODS.count(v)) { next(); continue; }
            if (SCALAR.count(v) || v == "void") { typ = v; next(); continue; }
            if (tk.k == "id" && aliasType.count(v)) { typ = v; next(); continue; }
            break;
        }
        // Funktion?
        if (peek().k == "id" && isSym("(", 1)) {
            emit("id", typ == "void" ? "void" : "char");
            emit("id", next().v);                   // Name
            while (peek().v != "{" && peek().k != "eof") { emit(peek().k, peek().v); next(); }
            if (peek().v == "{") { next(); emit("sym", "{"); }
            copy_block();
            return;
        }
        // struct/union-Alias-Variablen
        auto at = aliasType.find(typ);
        if (at != aliasType.end() && at->second != "scalar") {
            auto it = tags.find(at->second.substr(7));
            if (it == tags.end()) throw CompileError("unbekannter struct-Tag");
            while (peek().v != ";" && peek().k != "eof") {
                if (isSym(",")) { next(); continue; }
                if (peek().k == "id")
                    expandVar(next().v, at->second.substr(7), it->second.first, it->second.second);
                else next();
            }
            if (isSym(";")) next();
            return;
        }
        if (tags.count(typ) && tags[typ].first.empty()) {   // enum-Tag als Skalar
            while (peek().v != ";" && peek().k != "eof") {
                if (isSym(",")) { next(); continue; }
                if (peek().k == "id") declScalar(next().v);
                else next();
            }
            if (isSym(";")) next();
            return;
        }
        // skalare Variablen (evtl. Arrays, ROM-Daten)
        bool first = true;
        while (peek().v != ";" && peek().k != "eof") {
            if (isSym(",")) { next(); emit("sym", ","); continue; }
            if (peek().k != "id") { next(); continue; }
            string name = next().v;
            int size = 1;
            bool rom = false;
            if (isSym("[")) {
                next(); size = expectNum(); expectSym("]");
                if (isSym("=")) { rom = true; next(); }
            }
            if (rom) {
                emit("id", "const"); emit("id", "char");
                emit("id", name); emit("sym", "[");
                char b[16]; snprintf(b, sizeof b, "%d", size);
                emit("num", b); emit("sym", "]"); emit("sym", "=");
                while (peek().v != ";" && peek().k != "eof") { emit(peek().k, peek().v); next(); }
                emit("sym", ";"); next();
                varSize[name] = size;
                return;
            }
            if (first) { emit("id", "char"); first = false; }
            emit("id", name);
            if (size != 1) {
                emit("sym", "[");
                char b[16]; snprintf(b, sizeof b, "%d", size);
                emit("num", b); emit("sym", "]");
            }
            varSize[name] = size;
        }
        if (peek().v == ";") { next(); emit("sym", ";"); }
    }

    // ---- Block kopieren ---------------------------------------------------
    void copy_block() {
        int depth = 1;
        bool stmtHead = true;
        while (depth && peek().k != "eof") {
            Tok& tk = peek();
            const string& v = tk.v;
            if (v == "{") { depth++; emit("sym", "{"); next(); stmtHead = true; continue; }
            if (v == "}") { depth--; emit("sym", "}"); next(); stmtHead = false; continue; }
            if (v == ";") { emit("sym", ";"); next(); stmtHead = true; continue; }
            // s . f  ->  s_f  (union: erstes Feld)
            if (tk.k == "id" && isSym(".", 1) && peek(2).k == "id" &&
                structOfVar.count(v)) {
                string field = peek(2).v;
                string tag = structOfVar[v];
                auto it = tags.find(tag);
                if (it == tags.end()) throw CompileError("struct-Tag fehlt");
                const auto& fields = it->second.first;
                bool in = false;
                for (auto& f : fields) if (f == field) in = true;
                if (!in) throw CompileError("'" + v + "' hat kein Feld '" + field + "'");
                string g = v + "_" + field;
                if (it->second.second) g = v + "_" + fields[0];
                emit("id", g);
                next(); next(); next();
                stmtHead = false;
                continue;
            }
            // enum-Konstante -> Zahl
            if (tk.k == "id" && enumConsts.count(v)) {
                char b[16]; snprintf(b, sizeof b, "%d", enumConsts[v]);
                emit("num", b);
                next();
                stmtHead = false;
                continue;
            }
            // sizeof
            if (tk.k == "id" && v == "sizeof") {
                next();
                int sz = parse_sizeof();
                char b[16]; snprintf(b, sizeof b, "%d", sz);
                emit("num", b);
                stmtHead = false;
                continue;
            }
            // lokale Deklaration mit Typ-Praefix -> 'char ...'
            if (stmtHead) {
                size_t save = p;
                bool sawType = false;
                while (true) {
                    Tok& q = peek();
                    const string& w = q.v;
                    if (MODS.count(w)) { next(); continue; }
                    if (SCALAR.count(w) || w == "void") { sawType = true; next(); continue; }
                    if (q.k == "id" && aliasType.count(w)) { sawType = true; next(); continue; }
                    if (q.k == "id" && tags.count(w) && tags[w].first.empty()) {
                        sawType = true; next(); continue;
                    }
                    break;
                }
                if (sawType && peek().k == "id") {
                    emit("id", "char");
                    stmtHead = false;
                    continue;
                }
                p = save;
            }
            emit(tk.k, tk.v);
            if (tk.k == "id" || tk.k == "num") stmtHead = false;
            next();
        }
    }

    int parse_sizeof() {
        bool paren = isSym("(");
        if (paren) next();
        const string& v = peek().v;
        int sz;
        if (v == "struct" || v == "union") {
            next();
            string tag = expectId();
            auto it = tags.find(tag);
            if (it == tags.end()) throw CompileError("sizeof: unbekannter Tag '" + tag + "'");
            sz = it->second.second ? 1 : (int)it->second.first.size();
        } else if (SCALAR.count(v)) { next(); sz = 1; }
        else if (aliasType.count(v)) { sz = typeSize(next().v); }
        else if (v == "enum") { next(); expectId(); sz = 1; }
        else if (peek().k == "id" && varSize.count(v)) { sz = varSize[next().v]; }
        else throw CompileError("sizeof: unterstuetzt werden Typen, Tags, globale Variablen");
        if (paren) {
            if (!isSym(")")) throw CompileError("sizeof: ')' fehlt");
            next();
        }
        return sz;
    }

    string render() {
        string txt;
        string prevKind;
        for (auto& tk : out) {
            bool word = (tk.k == "id" || tk.k == "num" || tk.k == "str" || tk.k == "kw");
            if (!txt.empty() && word &&
                (prevKind == "id" || prevKind == "num" || prevKind == "str"))
                txt += " ";
            txt += tk.v;
            prevKind = tk.k;
        }
        return txt;
    }

    string run() {
        while (peek().k != "eof") {
            const string& v = peek().v;
            if (v == "typedef") parse_typedef();
            else if (v == "enum") parse_enum();
            else if (v == "struct" || v == "union") parse_su();
            else parse_toplevel();
        }
        return render();
    }
};

int Expander::ParserNum(const string& v) {
    if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0)
        return (int)strtol(v.c_str() + 2, nullptr, 16);
    if (v.rfind("0b", 0) == 0 || v.rfind("0B", 0) == 0)
        return (int)strtol(v.c_str() + 2, nullptr, 2);
    int n = 0;
    for (char c : v) {
        if (c < '0' || c > '9')
            throw CompileError("invalid number '" + v + "'");
        n = n * 10 + (c - '0');
    }
    return n;
}
}  // namespace

std::string maybe_expand(const std::string& src) {
    bool use = false;
    {
        // Erkennung ohne volles Tokenisieren ueber die Lexer-Schluesselwoerter:
        // einfache Wortsuche reicht fuer das Trigger-Set.
        static const std::vector<string> TR =
            {"typedef","struct","union","enum","sizeof","goto",
             "float","double","unsigned","signed","long","short","int",
             "auto","register","extern","volatile"};
        for (auto& w : TR) {
            size_t pos = 0;
            while ((pos = src.find(w, pos)) != string::npos) {
                bool left = pos == 0 || !(isalnum((unsigned char)src[pos-1]) || src[pos-1]=='_');
                size_t e = pos + w.size();
                bool right = e >= src.size() || !(isalnum((unsigned char)src[e]) || src[e]=='_');
                if (left && right) { use = true; break; }
                pos = e;
            }
            if (use) break;
        }
    }
    if (!use) return src;                     // klassisches Subset unveraendert

    for (auto& w : {"float", "double", "goto"}) {
        size_t pos = 0;
        while ((pos = src.find(w, pos)) != string::npos) {
            size_t e = pos + strlen(w);
            bool l = pos == 0 || !(isalnum((unsigned char)src[pos-1]) || src[pos-1]=='_');
            bool r = e >= src.size() || !(isalnum((unsigned char)src[e]) || src[e]=='_');
            if (l && r)
                throw CompileError(string(w) + " wird im 4-Bit-C-Subset nicht unterstuetzt");
            pos = e;
        }
    }

    vector<Tok> toks = tokenize(src);
    Expander ex;
    ex.t = std::move(toks);
    return ex.run();
}

}  // namespace c90
