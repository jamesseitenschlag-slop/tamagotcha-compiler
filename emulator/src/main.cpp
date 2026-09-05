// main.cpp
// erstellt: 28. Jan. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cassert>
#include <map>
#include <set>
#include <filesystem>
#include <cstdlib>
#include <cstdint>
#include "cpu.hpp"
#include "display.hpp"
#include "feed.hpp"
#include "os.hpp"
#include "tests.cpp"

FeedServer g_feed;   // JSON display feed (http://127.0.0.1:8017)

void printCpuState(int step, unsigned int pc, unsigned int op) {
    std::cout << "[" << std::setw(4) << std::setfill('0') << step << "] "
              << "PC: 0x" << std::setw(3) << std::hex << std::uppercase << pc 
              << " (B:" << (int)programCounter.PCB 
              << " P:" << (int)programCounter.PCP 
              << " S:0x" << std::setw(2) << (int)programCounter.PCS << ") "
              << "OP: 0x" << std::setw(3) << op << " | "
              << std::left << std::setw(18) << std::setfill(' ') << disassembleInstruction(op) << std::right
              << " | A=" << std::hex << (int)A.v 
              << " B=" << (int)B.v 
              << " IX=0x" << std::setw(3) << (int)IX.v 
              << " IY=0x" << std::setw(3) << (int)IY.v 
              << " SP=0x" << std::setw(2) << (int)SP.v 
              << " F:[" << (flags.I ? "I" : ".") 
                       << (flags.D ? "D" : ".") 
                       << (flags.Z ? "Z" : ".") 
                       << (flags.C ? "C" : ".") << "]"
              << std::dec << "\n";
}

bool loadRomFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return false;
    }
    
    // Auto-detect Big-Endian vs Little-Endian
    bool isBigEndian = false;
    if (fileSize >= 0x202 && buffer[0x200] == 0x00 && buffer[0x201] != 0x00) {
        isBigEndian = true;
    }
    
    int wordsLoaded = 0;
    for (size_t i = 0; i + 1 < buffer.size() && wordsLoaded < 6144; i += 2) {
        uint16_t word;
        if (isBigEndian) {
            word = (static_cast<uint16_t>(buffer[i]) << 8) | buffer[i + 1];
        } else {
            word = buffer[i] | (static_cast<uint16_t>(buffer[i + 1]) << 8);
        }
        ROM[wordsLoaded].v = word & 0xFFF;
        wordsLoaded++;
    }
    
    std::cout << ">>> Loaded " << wordsLoaded << " instructions (" 
              << (isBigEndian ? "Big-Endian MAME Dump" : "Little-Endian") 
              << ") from \"" << filename << "\"\n\n";
    return true;
}

void dumpDisassemblyToFile(const std::string& outputPath) {
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Failed to create disassembly output file: " << outputPath << "\n";
        return;
    }
    
    out << "; ==============================================================================\n";
    out << "; TAMAGOTCHI - ASSEMBLY DISASSEMBLY DUMP\n";
    out << "; CPU Architecture: EPSON E0C6S46 (4-bit CMOS Microcontroller)\n";
    out << "; ROM Size: 6,144 words x 12 bits\n";
    out << "; ==============================================================================\n\n";

    std::map<unsigned int, std::string> labels;
    labels[0x100] = "VEC_RESET";
    labels[0x102] = "VEC_INT_CLOCK_TIMER";
    labels[0x104] = "VEC_INT_STOPWATCH";
    labels[0x106] = "VEC_INT_K00_K03";
    labels[0x108] = "VEC_INT_K10_K13";
    labels[0x10A] = "VEC_INT_SERIAL";
    labels[0x10C] = "VEC_INT_PROG_TIMER";
    labels[0x110] = "INIT_STARTUP";

    // Pass 1: Discover Branch Targets
    int current_npp = 0;
    int current_nbp = 0;
    
    for (int i = 0; i < 6144; i++) {
        unsigned int op = ROM[i].v;
        if ((op & 0xFE0) == 0xE40) { // PSET p
            int p = op & 0x1F;
            current_npp = p & 0xF;
            current_nbp = (p >> 4) & 1;
            continue;
        } 
        
        if ((op & 0xF00) == 0x000 || (op & 0xF00) == 0x200 || (op & 0xF00) == 0x300 || (op & 0xF00) == 0x600 || (op & 0xF00) == 0x700) { // Jumps
            int s = op & 0xFF;
            unsigned int target = (current_nbp << 12) | (current_npp << 8) | s;
            if (labels.find(target) == labels.end()) {
                std::stringstream ss;
                ss << "LAB_B" << current_nbp << "_P" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << current_npp << "_S" << std::setw(2) << s;
                labels[target] = ss.str();
            }
        }
        else if ((op & 0xF00) == 0x400) { // CALL
            int s = op & 0xFF;
            unsigned int target = (current_nbp << 12) | (current_npp << 8) | s;
            if (labels.find(target) == labels.end()) {
                std::stringstream ss;
                ss << "SUB_B" << current_nbp << "_P" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << current_npp << "_S" << std::setw(2) << s;
                labels[target] = ss.str();
            }
        }
        else if ((op & 0xF00) == 0x500) { // CALZ
            int s = op & 0xFF;
            unsigned int target = s; // Bank 0, Page 0
            if (labels.find(target) == labels.end()) {
                std::stringstream ss;
                ss << "SUB_B0_P00_S" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << s;
                labels[target] = ss.str();
            }
        }
        
        // Reset NPP/NBP tracking on branches or general instructions?
        // Let's assume they persist until another PSET.
    }
    
    // Pass 2: Output
    for (int i = 0; i < 6144; i++) {
        unsigned int op = ROM[i].v;
        int bank = (i < 4096) ? 0 : 1;
        int page = (i >> 8) & 0xF;
        int step = i & 0xFF;
        
        if (step == 0) {
            out << "\n; ==========================================================================\n";
            out << "; BANK " << bank << ", PAGE " << std::setw(2) << std::setfill('0') << page 
                << " (0x" << std::hex << page << std::dec << ") - Base Address: 0x" 
                << std::hex << std::setw(4) << std::setfill('0') << i << std::dec << "\n";
            out << "; ==========================================================================\n";
        }
        
        std::string labelStr = "";
        if (labels.find(i) != labels.end()) {
            labelStr = labels[i] + ":";
        }
        
        out << std::left << std::setw(24) << std::setfill(' ') << labelStr 
            << " " << std::left << std::setw(20) << std::setfill(' ') << disassembleInstruction(op) 
            << " ; [0x" << std::hex << std::setw(4) << std::setfill('0') << i 
            << " | B:" << bank << " P:" << std::setw(2) << page << " S:" << std::setw(2) << step << "] "
            << "Opcode: 0x" << std::setw(3) << op << std::dec << "\n";
    }
    
    std::cout << ">>> Full annotated assembly successfully exported to \"" << outputPath << "\"\n";
}

void skipFiveMinutes() {
    // Advance the firmware age/evolution counter by 5 emulated minutes.
    // The 1 Hz state machine adds 2 to [0x036:0x037] per second; 5 minutes
    // is 300 seconds -> +600. The field is 8-bit, so wrap mod 256.
    uint16_t age = (DATA_RAM[0x037].v << 4) | DATA_RAM[0x036].v;
    age = (age + 600) & 0xFF;
    DATA_RAM[0x036].v = age & 0xF;
    DATA_RAM[0x037].v = (age >> 4) & 0xF;
    DATA_RAM[0x032].v = 0;
    DATA_RAM[0x033].v = 0;
}

std::string stateSlotPath(int slot) {
    std::error_code ec;
    std::filesystem::create_directories("states", ec);
    return "states/slot" + std::to_string(slot) + ".state";
}

int main(int argc, char** argv){
    std::string romPath = "tama_extracted/tama.bin";
    bool runTestsOnly = false;
    bool decompileOnly = false;
    bool statetest = false;
    int dumpVramFrames = 0;
    int traceInstrs = 0;
    bool feedEnabled = true;
    unsigned short feedPort = 8017;
    std::string autoKeys;
    std::string disasmOutput = "tamagotchi_disassembly.asm";
    
    if (argc > 1) {
        for (int a = 1; a < argc; a++) {
            std::string arg = argv[a];
            if (arg == "--test" || arg == "-t") {
                runTestsOnly = true;
            } else if (arg == "--statetest") {
                statetest = true;
            } else if (arg == "--dumpvram") {
                if (a + 1 < argc) dumpVramFrames = std::atoi(argv[++a]);
            } else if (arg == "--trace") {
                if (a + 1 < argc) traceInstrs = std::atoi(argv[++a]);
            } else if (arg == "--disasm" || arg == "-d") {
                decompileOnly = true;
                if (a + 1 < argc && argv[a + 1][0] != '-') {
                    disasmOutput = argv[++a];
                }
            } else if (arg == "--no-feed") {
                feedEnabled = false;
            } else if (arg == "--keys") {
                if (a + 1 < argc) autoKeys = argv[++a];
            } else if (arg == "--port") {
                if (a + 1 < argc) feedPort = (unsigned short)std::atoi(argv[++a]);
            } else {
                romPath = arg;
            }
        }
    }
    
    if (runTestsOnly) {
        run_all_tests();
        return 0;
    }
    
    std::cout << "================================================================================\n";
    std::cout << "                TAMAGOTCHA - EPSON E0C6S46 HARDWARE EMULATOR                    \n";
    std::cout << "================================================================================\n";
    
    if (!loadRomFile(romPath)) {
        std::cerr << "Failed to open primary ROM \"" << romPath << "\". Falling back to unit tests...\n\n";
        run_all_tests();
        return 1;
    }
    
    if (decompileOnly) {
        dumpDisassemblyToFile(disasmOutput);
        return 0;
    }
    
    std::cout << "--- Initializing CPU ---\n";
    reset();

    // Headless state save/load round-trip test (used by the build scripts).
    if (statetest) {
        const int C = 32768 / 50;
        for (int f = 0; f < 500; f++) execute(C, ROM);
        std::vector<uint8_t> ram0(4096), ramA(4096), ramB(4096);
        for (int i = 0; i < 4096; i++) ram0[i] = DATA_RAM[i].v;
        unsigned pc0 = programCounter.CurrentAddress();
        unsigned cyc0 = (unsigned)emuTotalCycles;
        unsigned a0 = A.v, b0 = B.v, sp0 = SP.v;
        unsigned fl0 = flags.I | (flags.D << 1) | (flags.Z << 2) | (flags.C << 3);
        std::string p = stateSlotPath(1);
        if (!cpuSaveState(p)) { std::cout << "STATETEST FAIL (save)\n"; return 1; }
        DATA_RAM[0x005].v ^= 0xF;                       // tamper
        programCounter.PCS = 0x42;
        A.v = 0xA;
        if (!cpuLoadState(p)) { std::cout << "STATETEST FAIL (load)\n"; return 1; }
        bool ok = true;
        for (int i = 0; i < 4096; i++) {
            if (DATA_RAM[i].v != ram0[i]) ok = false;
            ramA[i] = DATA_RAM[i].v;
        }
        ok = ok && programCounter.CurrentAddress() == pc0 && (unsigned)emuTotalCycles == cyc0
             && A.v == a0 && B.v == b0 && SP.v == sp0
             && (flags.I | (flags.D << 1) | (flags.Z << 2) | (flags.C << 3)) == fl0;
        std::cout << (ok ? "STATETEST PASS" : "STATETEST FAIL (state mismatch)")
                  << " pc=0x" << std::hex << programCounter.CurrentAddress() << std::dec << "\n";
        (void)ramA; (void)ramB;
        return ok ? 0 : 1;
    }

    // Headless raw-VRAM dump for comparing old vs new builds.
    if (dumpVramFrames > 0) {
        const int C = 32768 / 50;
        for (int f = 0; f < dumpVramFrames; f++) execute(C, ROM);
        // Deterministic B-button press (goes to clock screen), then 50 more frames.
        setButtonInputs(false, true, false);
        for (int f = 0; f < 3; f++) execute(C, ROM);
        setButtonInputs(false, false, false);
        for (int f = 0; f < 50; f++) execute(C, ROM);
        std::cout << "[DUMPVRAM] frames=" << dumpVramFrames
                  << " cycles=" << emuTotalCycles
                  << " pc=" << std::hex << programCounter.CurrentAddress() << std::dec
                  << " A=" << std::hex << (int)A.v << " B=" << (int)B.v
                  << " IX=" << IX.v << " IY=" << IY.v << " RP=" << (int)RP.v
                  << " SP=" << (int)SP.v << " F="
                  << (flags.I?'I':'.') << (flags.D?'D':'.') << (flags.Z?'Z':'.') << (flags.C?'C':'.')
                  << std::dec << "\n";
        std::cout << "[VRAM] ";
        for (int a = 0xE00; a <= 0xECF; a++) {
            std::cout << std::hex << (int)DATA_RAM[a].v;
        }
        std::cout << std::dec << "\n";
        std::cout << "[RAM] ";
        for (int a = 0; a < 4096; a++) { std::cout << std::hex << (int)DATA_RAM[a].v; }
        std::cout << std::dec << "\n";
        std::cout << "[SCREEN]\n";
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 32; x++) std::cout << (display.getPixel(DATA_RAM, x, y) ? '#' : '.');
            std::cout << "\n";
        }
        return 0;
    }

    // Headless instruction trace: print core state after each instruction.
    if (traceInstrs > 0) {
        for (int i = 0; i < traceInstrs; i++) {
            execute(1, ROM);
            std::cout << i
                      << " pc=" << std::hex << programCounter.CurrentAddress()
                      << " A=" << (int)A.v << " B=" << (int)B.v
                      << " IX=" << IX.v << " IY=" << IY.v
                      << " SP=" << (int)SP.v
                      << " F=" << (flags.I?'I':'.') << (flags.D?'D':'.') << (flags.Z?'Z':'.') << (flags.C?'C':'.')
                      << std::dec << "\n";
        }
        return 0;
    }

    // Initialize Display
    display.init("Tamagotchi Emulator (EPSON E0C6S46)");

    // JSON display feed for website integration (localhost only).
    if (feedEnabled && feedPort) {
        if (g_feed.start(feedPort))
            std::cout << "[Feed] JSON-Display-Feed: http://127.0.0.1:" << feedPort
                      << "   (GET /feed, Demo: GET /)\n";
        else
            std::cout << "[Feed] Port " << feedPort << " not available - feed disabled\n";
    }

    // The E0C6S46 runs at 32.768 kHz. We emulate 50 frames per second, i.e.
    // 32768 / 50 = 655 hardware cycles per frame (rounded down).
    const int CYCLES_PER_SECOND = 32768;
    const int FRAME_HZ = 50;
    const int CYCLES_PER_FRAME = CYCLES_PER_SECOND / FRAME_HZ;
    const std::chrono::milliseconds FRAME_DURATION(1000 / FRAME_HZ);

    // Speed presets (cycles per frame are scaled by the multiplier).
    static const double SPEEDS[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0 };
    const int SPEED_COUNT = (int)(sizeof(SPEEDS) / sizeof(SPEEDS[0]));
    int speedIdx = 3;                    // start at 1.0x
    bool paused = false;

    bool running = true;
    int frame = 0;
    bool turboMode = false;              // hatch: run 5 emulated minutes as fast as possible
    unsigned long long turboTarget = 0;
    int autoPressFrames = 0;
    int latchA = 0, latchB = 0, latchC = 0;
    const int LATCH_FRAMES = 8;          // ~160 ms, so the 16 Hz ROM debounce always sees a tap

    // --keys: automatische Tastatureingabe fuer Headless-Shell-Tests ('|'=Enter)
    int feedPos = 0;
    bool feedDone = autoKeys.empty();

    // ---- Host action dispatcher (keyboard + web /action) ----------------
    auto applyHost = [&](const HostAction& act) {
        if (act.reset) {
            turboMode = false; autoPressFrames = 0;
            latchA = latchB = latchC = 0;
            paused = false;
            reset();
            hostStatus = "RESET";
        }
        if (act.pauseToggle) {
            if (turboMode) { turboMode = false; }
            else paused = !paused;
        }
        if (act.speedDelta != 0) {
            speedIdx += act.speedDelta;
            if (speedIdx < 0) speedIdx = 0;
            if (speedIdx >= SPEED_COUNT) speedIdx = SPEED_COUNT - 1;
        }
        if (act.saveState) {
            if (cpuSaveState(stateSlotPath(hostSaveSlot))) hostStatus = "SAVED";
            else hostStatus = "SAVE FAIL";
        }
        if (act.loadState) {
            if (cpuLoadState(stateSlotPath(hostSaveSlot))) {
                paused = false; turboMode = false; autoPressFrames = 0;
                hostStatus = "LOADED";
            } else hostStatus = "LOAD FAIL";
        }
        if (act.skipMin) { skipFiveMinutes(); hostStatus = "+5 MIN"; }
        if (act.hatch) {
            paused = false; turboMode = true;
            turboTarget = emuTotalCycles + 5ULL * 60ULL * CYCLES_PER_SECOND;
            hostStatus = "HATCH 5MIN";
        }
    };

    std::cout << "\n>>> Real-Time Emulation: F1 Hatch, F2 +5min, F3/F4 speed, F5 save, F8 load <<<\n";

    while (running) {
        auto frameStart = std::chrono::steady_clock::now();

        // --keys: erst senden, wenn der Gast die letzte Taste abgeholt hat
        if (!feedDone && frame > 40 && DATA_RAM[0x2D8].v == 0) {
            char c = autoKeys[feedPos++];
            if (c == '|') os::feedKey(13); else os::feedKey((unsigned char)c);
            if (feedPos >= (int)autoKeys.size()) feedDone = true;
        }

        g_feed.poll(reinterpret_cast<const std::uint8_t*>(DATA_RAM));   // serve web feed

        HostAction act;
        bool btnA = false, btnB = false, btnC = false;
        if (!display.pollEvents(act, btnA, btnB, btnC)) break;
        if (act.quit) break;
        if (autoPressFrames > 0) btnA = true;   // auto-start A after hatch turbo

        // Press-latch: extend short taps so the ROM debounce always sees them.
        if (btnA) latchA = LATCH_FRAMES; else if (latchA > 0) latchA--;
        if (btnB) latchB = LATCH_FRAMES; else if (latchB > 0) latchB--;
        if (btnC) latchC = LATCH_FRAMES; else if (latchC > 0) latchC--;

        // Web actions (GET /action?key=...) - A/B/C become button presses,
        // everything else maps to the same handlers as the hotkeys.
        std::string webAct;
        while (g_feed.popAction(webAct)) {
            if (webAct == "A") latchA = LATCH_FRAMES;
            else if (webAct == "B") latchB = LATCH_FRAMES;
            else if (webAct == "C") latchC = LATCH_FRAMES;
            else {
                HostAction ha;
                if (webAct == "HATCH") ha.hatch = true;
                else if (webAct == "SKIP") ha.skipMin = true;
                else if (webAct == "PAUSE") ha.pauseToggle = true;
                else if (webAct == "SAVE") ha.saveState = true;
                else if (webAct == "LOAD") ha.loadState = true;
                else if (webAct == "RESET") ha.reset = true;
                else if (webAct == "SLOWER") ha.speedDelta = -1;
                else if (webAct == "FASTER") ha.speedDelta = +1;
                applyHost(ha);
            }
        }
        setButtonInputs(latchA > 0, latchB > 0, latchC > 0);

        // ---- Host control actions --------------------------------------
        applyHost(act);

        hostSpeed = SPEEDS[speedIdx];
        hostPaused = paused;

        // ---- Turbo (hatch) ---------------------------------------------
        if (turboMode) {
            execute(CYCLES_PER_SECOND, ROM);   // ~1 emulated second per iteration
            if (emuTotalCycles >= turboTarget) {
                turboMode = false;
                autoPressFrames = 25;          // briefly press A to start/hatch
            }
            if ((frame & 0x3F) == 0) display.render(DATA_RAM);
            frame++;
            continue;
        }
        if (autoPressFrames > 0) autoPressFrames--;

        // ---- Normal execution (speed scaled, pause = 0 cycles) ----------
        if (!paused) {
            int cyc = (int)(CYCLES_PER_FRAME * hostSpeed + 0.5);
            if (cyc < 1) cyc = 1;
            execute(cyc, ROM);
        }
        frame++;
        display.render(DATA_RAM);

        // Keep the display frame rate near 50 FPS * speed (capped at 240 Hz).
        double renderHz = paused ? FRAME_HZ : FRAME_HZ * hostSpeed;
        if (renderHz < 1) renderHz = 1;
        if (renderHz > 240) renderHz = 240;
        auto target = std::chrono::microseconds((long long)(1'000'000.0 / renderHz));
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
        if (elapsed < target) {
            std::this_thread::sleep_for(target - elapsed);
        }
    }
    
    std::cout << "\n================================================================================\n";
    std::cout << "                             FINAL EMULATOR STATE                               \n";
    std::cout << "================================================================================\n";
    std::cout << "Program Counter : 0x" << std::hex << std::uppercase << programCounter.CurrentAddress() 
              << " [Bank:" << (int)programCounter.PCB 
              << " Page:" << (int)programCounter.PCP 
              << " Step:0x" << (int)programCounter.PCS << "]\n";
    std::cout << "Registers       : A=0x" << (int)A.v << " B=0x" << (int)B.v 
              << " | IX=0x" << (int)IX.v << " IY=0x" << (int)IY.v 
              << " | SP=0x" << (int)SP.v << "\n";
    std::cout << "Flags           : Interrupt(I)=" << flags.I 
              << " Decimal(D)=" << flags.D 
              << " Zero(Z)=" << flags.Z 
              << " Carry(C)=" << flags.C << std::dec << "\n";
    std::cout << "================================================================================\n";
    
    display.close();
    return 0;
}
