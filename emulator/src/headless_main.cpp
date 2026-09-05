// ============================================================================
// headless_main.cpp - Konsolen-Variante des Emulators OHNE SDL/Display.
//
// Reiner Shell-Betrieb: ROM wird geladen, die CPU laeuft im Kontext,
// [DBG]-/[OS]-Ausgaben erscheinen auf stdout. Tastatureingaben werden als
// OS-Mailbox-Zeichen (os::feedKey) eingespiesen - genau wie im SDL-Fenster,
// aber eben ohne Fenster.
//
// Tastenquelle:
//   --keys "abc|def"   Tasten automatisch ('|' = Enter), wie im GUI-Modus
//   --stdin            alle stdin-Zeichen als Tasten (Pipes/Test)
//   (default)          interaktive Konsole: _kbhit/_getch (Windows)
//
// Build (kein SDL, kein ws2_32):
//   g++ -std=c++17 -O2 -Isrc src/cpu.cpp src/os.cpp src/headless_main.cpp \
//       -o tamagotcha_headless.exe
// ============================================================================
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include "cpu.hpp"
#include "os.hpp"

#ifdef _WIN32
#include <conio.h>
#define HAVE_KBHIT 1
#endif

static const unsigned ROM_WORDS_MAX = 6144;

static bool loadRom(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf((size_t)size);
    if (size > 0 && !f.read(reinterpret_cast<char*>(buf.data()), size)) return false;

    bool be = false;
    if ((size_t)size >= 0x202 && buf[0x200] == 0x00 && buf[0x201] != 0x00) be = true;

    int n = 0;
    for (size_t i = 0; i + 1 < buf.size() && n < (int)ROM_WORDS_MAX; i += 2) {
        uint16_t w = be ? (uint16_t)((buf[i] << 8) | buf[i + 1])
                        : (uint16_t)(buf[i] | (buf[i + 1] << 8));
        ROM[n++].v = w & 0xFFF;
    }
    std::printf(">>> Loaded %d instructions (%s) from \"%s\"\n", n,
                be ? "Big-Endian" : "Little-Endian", path.c_str());
    return true;
}

static void feedPrintableOrCtrl(unsigned char c) {
    // '|' = Enter (wie GUI --keys), Zeilenvorschub ebenso
    if (c == 124 || c == 10 || c == 13) os::feedKey(13);
    else os::feedKey(c);
}

int main(int argc, char** argv) {
    std::string romPath;
    std::string autoKeys;
    bool useStdin = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--keys" && i + 1 < argc) autoKeys = argv[++i];
        else if (a == "--stdin") useStdin = true;
        else if (a[0] == '-') { /* ignore */ }
        else romPath = a;
    }
    if (romPath.empty()) {
        std::printf("usage: tamagotcha_headless rom.bin [--keys TEXT] [--stdin]\n");
        return 1;
    }
    if (!loadRom(romPath)) {
        std::fprintf(stderr, "ROM nicht lesbar: %s\n", romPath.c_str());
        return 1;
    }
    reset();

    // stdin als Tastenquelle einlesen
    std::string stdinKeys;
    if (useStdin) {
        char c;
        while (std::fread(&c, 1, 1, stdin) == 1) stdinKeys.push_back(c);
    }

    const int CYCLES_PER_FRAME = 32768 / 50;
    size_t feedPos = 0;
    bool feedDone = autoKeys.empty() && stdinKeys.empty();
    const std::string& feedSrc = !autoKeys.empty() ? autoKeys : stdinKeys;

    std::printf("[headless] shell aktiv - ROM laeuft, Tasten: %s\n",
                useStdin ? "stdin" : (autoKeys.empty() ? "Konsole (_kbhit)" : "--keys"));

    unsigned long long frame = 0;
    for (;;) {
        // naechste Taste einfuegen, sobald der Gast die letzte abgeholt hat
        if (!feedDone && frame > 20 && DATA_RAM[os::MAIL_KEYRDY].v == 0) {
            if (feedPos < feedSrc.size()) {
                feedPrintableOrCtrl((unsigned char)feedSrc[feedPos++]);
            } else {
                feedDone = true;
            }
        }
        // 1 Frame CPU ausfuehren
        execute(CYCLES_PER_FRAME, ROM);

#ifdef HAVE_KBHIT
        if (!useStdin && autoKeys.empty() && _kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 0xE0) { _getch(); continue; }  // Pfeiltasten
            if (ch == 13 || ch == 10) os::feedKey(13);
            else if (ch == 8 || ch == 127) os::feedKey(8);
            else os::feedKey((unsigned char)ch);
        }
#endif
        frame++;
    }
    return 0;
}
