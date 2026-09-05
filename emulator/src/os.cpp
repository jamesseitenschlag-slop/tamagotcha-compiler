// os.cpp
// erstellt: 07. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
// ============================================================================
// os.cpp - Host-OS: Syscall-Dispatcher + Dateisystem (osdisk/) + Terminal-
// Echo fuer die Gast-Shell. Siehe os.hpp fuer die Mailbox-ABI.
// ============================================================================
#include "os.hpp"
#include "cpu.hpp"

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace os {

static std::string g_root = "osdisk";

const char* diskRoot() { return g_root.c_str(); }

// ---------------------------------------------------------------------------
// Ausgabe. Tippt der Nutzer (feedKey), bleibt die Zeile offen (echoLineOpen).
// Jede Gast-/System-Meldung setzt davor eine neue Zeile, damit nichts
// zusammenklebt - ohne dass jede Taste eine eigene Zeile erzeugt.
// ---------------------------------------------------------------------------
void logf(const char* fmt, ...) {
    if (echoLineOpen) { std::fputc('\n', stdout); echoLineOpen = false; }
    std::fputs("[OS] ", stdout);
    va_list ap; va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::fputc('\n', stdout);
}

static unsigned char ramByte(unsigned a) {
    // Byte b aus Nibble-Paaren: low bei gerader Adresse, high bei ungerader.
    unsigned lo = DATA_RAM[a & 0xFFF].v;
    unsigned hi = DATA_RAM[(a + 1) & 0xFFF].v;
    return (unsigned char)(lo | (hi << 4));
}

static void ramSetByte(unsigned a, unsigned char b) {
    DATA_RAM[a & 0xFFF].v = b & 0xF;
    DATA_RAM[(a + 1) & 0xFFF].v = (b >> 4) & 0xF;
}

static unsigned mailboxPtr() {
    unsigned p = 0;
    p |= (DATA_RAM[MAIL_PTR].v & 0xF) << 8;        // hi
    p |= (DATA_RAM[MAIL_PTR + 1].v & 0xF) << 4;    // mid
    p |= DATA_RAM[MAIL_PTR + 2].v & 0xF;           // lo
    return p;
}

static unsigned mailboxLen() {
    unsigned lo = DATA_RAM[MAIL_LEN].v & 0xF;
    unsigned hi = DATA_RAM[MAIL_LEN + 1].v & 0xF;
    return lo | (hi << 4);
}

static void setStatus(unsigned s) { DATA_RAM[MAIL_STATUS].v = s & 0xF; }

static std::string mailboxName() {
    unsigned n = DATA_RAM[MAIL_NAME_LEN].v;
    if (n > OS_DISK_MAX) n = OS_DISK_MAX;
    std::string name;
    for (unsigned i = 0; i < n; i++)
        name.push_back((char)ramByte(MAIL_NAME + 2 * i));
    return name;
}

static bool validName(const std::string& n) {
    if (n.empty() || n.size() > OS_DISK_MAX) return false;
    if (n == "." || n == "..") return false;
    for (char c : n) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Dateisystem-Dienste
// ---------------------------------------------------------------------------
static std::vector<unsigned char> readFile(const std::string& name) {
    std::vector<unsigned char> out;
    std::ifstream f(g_root + "/" + name, std::ios::binary);
    if (!f) return out;
    char buf[256];
    while (f) {
        f.read(buf, sizeof buf);
        std::streamsize n = f.gcount();
        for (std::streamsize i = 0; i < n; i++) out.push_back((unsigned char)buf[i]);
    }
    return out;
}

static bool writeFile(const std::string& name, const unsigned char* data, size_t n) {
    std::ofstream f(g_root + "/" + name, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data), (std::streamsize)n);
    return f.good();
}

static void doList() {
    fs::create_directories(g_root);
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(g_root))
        if (e.is_regular_file()) files.push_back(e.path());
    std::sort(files.begin(), files.end());
    if (files.empty()) { logf("ls: (leer)"); return; }
    for (const auto& p : files) {
        auto sz = fs::file_size(p);
        logf("ls: %-20s %6llu Bytes", p.filename().string().c_str(),
             (unsigned long long)sz);
    }
    setStatus(OK);
}

static void doRead() {
    std::string name = mailboxName();
    if (!validName(name)) { setStatus(BAD_NAME); return; }
    auto data = readFile(name);
    if (data.empty()) { setStatus(NOT_FOUND); return; }
    if (data.size() > OS_DATA_MAX) { setStatus(TOO_BIG); return; }
    unsigned p = mailboxPtr();
    for (size_t i = 0; i < data.size(); i++) ramSetByte(p + 2 * (unsigned)i, data[i]);
    DATA_RAM[MAIL_LEN].v = data.size() & 0xF;
    DATA_RAM[MAIL_LEN + 1].v = (data.size() >> 4) & 0xF;
    setStatus(OK);
}

static void doWrite() {
    std::string name = mailboxName();
    if (!validName(name)) { setStatus(BAD_NAME); return; }
    unsigned n = mailboxLen();
    if (n > OS_DATA_MAX) { setStatus(TOO_BIG); return; }
    unsigned p = mailboxPtr();
    std::vector<unsigned char> data(n);
    for (unsigned i = 0; i < n; i++) data[i] = ramByte(p + 2 * i);
    fs::create_directories(g_root);
    if (!writeFile(name, data.data(), data.size())) { setStatus(IO_ERR); return; }
    setStatus(OK);
}

static void doDelete() {
    std::string name = mailboxName();
    if (!validName(name)) { setStatus(BAD_NAME); return; }
    fs::path p = g_root + "/" + name;
    std::error_code ec;
    if (!fs::remove(p, ec)) { setStatus(NOT_FOUND); return; }
    setStatus(OK);
}

static void doStat() {
    std::string name = mailboxName();
    if (!validName(name)) { setStatus(BAD_NAME); return; }
    fs::path p = g_root + "/" + name;
    std::error_code ec;
    auto sz = fs::file_size(p, ec);
    if (ec) { setStatus(NOT_FOUND); return; }
    logf("stat: %s = %llu Bytes", name.c_str(), (unsigned long long)sz);
    setStatus(OK);
}

void syscall() {
    unsigned fn = DATA_RAM[MAIL_FN].v & 0xF;
    switch (fn) {
        case FS_LIST:   doList();                         break;
        case FS_READ:   doRead();                         break;
        case FS_WRITE:  doWrite();                        break;
        case FS_DELETE: doDelete();                       break;
        case FS_STAT:   doStat();                         break;
        default:        setStatus(BAD_FN);                break;
    }
    unsigned st = DATA_RAM[MAIL_STATUS].v & 0xF;
    std::string nm = mailboxName();
    bool hasName = (fn == FS_READ || fn == FS_WRITE ||
                    fn == FS_DELETE || fn == FS_STAT);
    logf("syscall fn=%u%s status=%u", fn,
         hasName ? (" name=" + nm).c_str() : "", st);
    DATA_RAM[OS_TRIGGER].v = 0;   // self-clearing, wie DBG-Kanal
}

// ---------------------------------------------------------------------------
// Tastatureingabe Host -> Gast mit Terminal-Echo (OHNE Zeile je Taste)
// ---------------------------------------------------------------------------
void feedKey(unsigned char c) {
    DATA_RAM[MAIL_KEY].v = c & 0xF;
    DATA_RAM[MAIL_KEY + 1].v = (c >> 4) & 0xF;
    DATA_RAM[MAIL_KEYRDY].v = 1;

    if (c >= 32 && c <= 126) {                 // druckbares Zeichen
        std::fputc(c, stdout);
        echoLineOpen = true;
    } else if (c == 10 || c == 13) {           // ENTER schliesst die Zeile
        if (echoLineOpen) { std::fputc('\n', stdout); echoLineOpen = false; }
    } else if (c == 8 && echoLineOpen) {       // Backspace (visual)
        std::fputs("\b \b", stdout);
    }
    std::fflush(stdout);
}

}  // namespace os
