// cpu.cpp
// erstellt: 06. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#include "cpu.hpp"
#include "os.hpp"
#include <iostream>
#include <fstream>
#include <string>

bool buzzerOn = false;
int buzzerFreq = 4096;
unsigned long long emuTotalCycles = 0;
static bool buzzerDirectOn = false;
static bool buzzerOneShotOn = false;
static unsigned long long buzzerOneShotUntil = 0;

static void updateBuzzer() {
    buzzerOn = buzzerDirectOn || buzzerOneShotOn;
}

Byte readDataMem(unsigned int addr) {
    addr &= 0xFFF;
    Byte val = DATA_RAM[addr];
    // Per Technical Manual Section 11.1: Interrupt factor flags (F00H-F05H) are reset to 0 upon reading
    if (addr >= 0xF00 && addr <= 0xF05) {
        DATA_RAM[addr].v = 0;
    }
    return val;
}

void writeDataMem(unsigned int addr, unsigned char val) {
    addr &= 0xFFF;
    DATA_RAM[addr].v = val & 0xF;

    // Buzzer: R43 (0xF54) bit 3 = buzzer on/off, 0xF74 bits 0-2 = frequency.
    if (addr == 0xF54) {
        // R43 BZ output is ACTIVE LOW: bit3 set = buzzer off, bit3 clear = on.
        buzzerDirectOn = (val & 0x8) == 0;
        updateBuzzer();
    } else if (addr == 0xF74) {
        // E0C6S46 buzzer frequency table (8 steps, 4096 Hz down to ~1170 Hz).
        static const int bz_freq[8] = {4096, 3276, 2730, 2340, 2048, 1638, 1365, 1170};
        buzzerFreq = bz_freq[val & 0x7];
    }

    // Debug trigger: writing to the debug register raises the debug
    // interrupt -> the emulator flushes the debug string buffer.
    if (addr == DBG_TRIGGER_ADDR) {
        DATA_RAM[addr].v = 0;      // self-clearing, like an int factor
        debugFlush();
    }

    // OS-Syscall-Trigger: ein Gast schreibt 0xF0D -> der Host fuehrt den
    // Dienst der Syscall-Mailbox aus (externer Interrupt an den Emulator).
    if (addr == os::OS_TRIGGER) {
        DATA_RAM[addr].v = 0;      // self-clearing
        os::syscall();
    }
}

// Debug channel: print the string stored in the debug buffer (chars are
// 8 bit: low nibble at even address, high nibble at odd address, NUL
// terminated, max DBG_BUFFER_CHARS).
void debugFlush(void) {
    if (os::echoLineOpen) { std::cout << '\n'; os::echoLineOpen = false; }
    std::string out;
    out.reserve(DBG_BUFFER_CHARS);
    for (int i = 0; i < DBG_BUFFER_CHARS; i++) {
        unsigned lo = DATA_RAM[(DBG_BUFFER_ADDR + 2 * i) & 0xFFF].v;
        unsigned hi = DATA_RAM[(DBG_BUFFER_ADDR + 2 * i + 1) & 0xFFF].v;
        unsigned ch = (hi << 4) | lo;
        if (ch == 0) break;                 // NUL terminator
        out.push_back(static_cast<char>(ch));
    }
    std::cout << "[DBG] " << out << "\n";   // ein gepufferter Ausgabevorgang
    debugMessageCount++;
}

template <typename T, typename U> bool instructionValue(T x, U y){
    if (y <= x.max && y >= x.min){
        return true;
    }
    else{
        return false;
    }
}

void reset(){
    programCounter.PCS = 0x00;
    programCounter.PCP = 0x01; // Bank 0, Page 1, Step 00H
    programCounter.PCB = 0x00;
    programCounter.NPP = 0x01;
    programCounter.NBP = 0x00;
    prevOpcode = 0xFFF; // NOP7: no pending PSET after reset
    pendingInputInterrupt = false;
    pendingClockTimerInterrupt = false;
    pendingProgTimerInterrupt = false;
    flags.I = 0x0;
    flags.D = 0x0;
    flags.Z = 0x0;
    flags.C = 0x0;
    buzzerDirectOn = false;
    buzzerOneShotOn = false;
    buzzerOneShotUntil = 0;
    updateBuzzer();
    
    // Initialize Input Ports K0 and K1 with pull-up High (0xF)
    DATA_RAM[0xF40].v = 0xF;
    DATA_RAM[0xF41].v = 0xF;
    DATA_RAM[0xF42].v = 0xF;
    
    // Additional MAME-verified hardware initializations
    DATA_RAM[0xF54].v = 0xF;
    DATA_RAM[0xF70].v = 0x0; // LCD control
    DATA_RAM[0xF71].v = 0x8; // LCD contrast / OSC select
    DATA_RAM[0xF73].v = 0x0; // SVD
    DATA_RAM[0xF74].v = 0x0;
    DATA_RAM[0xF75].v = 0x4;
    DATA_RAM[0xF76].v = 0x3;
    DATA_RAM[0xF77].v = 0x2;
}

// ---------------------------------------------------------------------------
// Save state: serialize the full machine (RAM, registers, PC, flags, pending
// IRQs, cycle counter, buzzer) into a small binary file.
// ---------------------------------------------------------------------------
static void putU32(std::ofstream& f, unsigned v) {
    f.put((char)(v & 0xFF)); f.put((char)((v >> 8) & 0xFF));
    f.put((char)((v >> 16) & 0xFF)); f.put((char)((v >> 24) & 0xFF));
}
static void putU64(std::ofstream& f, unsigned long long v) {
    for (int i = 0; i < 8; i++) f.put((char)((v >> (8 * i)) & 0xFF));
}
static unsigned getU32(std::ifstream& f) {
    unsigned v = 0;
    for (int i = 0; i < 4; i++) v |= (unsigned)f.get() << (8 * i);
    return v;
}
static unsigned long long getU64(std::ifstream& f) {
    unsigned long long v = 0;
    for (int i = 0; i < 8; i++) v |= (unsigned long long)(unsigned char)f.get() << (8 * i);
    return v;
}

bool cpuSaveState(const std::string& path) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << "TAMAS1";                      // magic + version
    putU64(f, emuTotalCycles);
    f.put((char)A.v); f.put((char)B.v);
    f.put((char)(IX.v & 0xFF)); f.put((char)((IX.v >> 8) & 0xF));
    f.put((char)(IY.v & 0xFF)); f.put((char)((IY.v >> 8) & 0xF));
    f.put((char)SP.v); f.put((char)RP.v);
    f.put((char)(flags.I | (flags.D << 1) | (flags.Z << 2) | (flags.C << 3)));
    f.put((char)programCounter.PCB); f.put((char)programCounter.PCP); f.put((char)programCounter.PCS);
    f.put((char)programCounter.NBP); f.put((char)programCounter.NPP);
    f.put((char)(prevOpcode & 0xFF)); f.put((char)((prevOpcode >> 8) & 0xF));
    f.put((char)(pendingInputInterrupt ? 1 : 0));
    f.put((char)(pendingClockTimerInterrupt ? 1 : 0));
    f.put((char)(pendingProgTimerInterrupt ? 1 : 0));
    f.put((char)(buzzerOn ? 1 : 0)); f.put((char)buzzerFreq);
    f.put((char)(buzzerDirectOn ? 1 : 0));
    f.put((char)(buzzerOneShotOn ? 1 : 0)); putU64(f, buzzerOneShotUntil);
    // RAM als zusammenhängender Nibble-Block (ein write statt 4096 put-Aufrufe)
    unsigned char ram[4096];
    for (int a = 0; a < 4096; a++) ram[a] = (unsigned char)DATA_RAM[a].v;
    f.write(reinterpret_cast<const char*>(ram), sizeof ram);
    return f.good();
}

bool cpuLoadState(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[7] = {0};
    f.read(magic, 6);
    if (std::string(magic) != "TAMAS1") return false;
    emuTotalCycles = getU64(f);
    A.v = (unsigned char)f.get() & 0xF;
    B.v = (unsigned char)f.get() & 0xF;
    IX.v = ((unsigned)f.get() & 0xFF) | (((unsigned)f.get() & 0xF) << 8);
    IY.v = ((unsigned)f.get() & 0xFF) | (((unsigned)f.get() & 0xF) << 8);
    SP.v = (unsigned char)f.get();
    RP.v = (unsigned char)f.get() & 0xF;
    unsigned char fl = (unsigned char)f.get();
    flags.I = fl & 1; flags.D = (fl >> 1) & 1; flags.Z = (fl >> 2) & 1; flags.C = (fl >> 3) & 1;
    programCounter.PCB = (unsigned char)f.get() & 1;
    programCounter.PCP = (unsigned char)f.get() & 0xF;
    programCounter.PCS = (unsigned char)f.get();
    programCounter.NBP = (unsigned char)f.get() & 1;
    programCounter.NPP = (unsigned char)f.get() & 0xF;
    prevOpcode = ((unsigned)f.get() & 0xFF) | (((unsigned)f.get() & 0xF) << 8);
    pendingInputInterrupt = f.get() != 0;
    pendingClockTimerInterrupt = f.get() != 0;
    pendingProgTimerInterrupt = f.get() != 0;
    buzzerOn = f.get() != 0;
    buzzerFreq = (unsigned char)f.get();
    buzzerDirectOn = f.get() != 0;
    buzzerOneShotOn = f.get() != 0;
    buzzerOneShotUntil = getU64(f);
    // RAM-Block in einem Stueck lesen, dann entpacken
    unsigned char ram[4096];
    f.read(reinterpret_cast<char*>(ram), sizeof ram);
    if (!f.good()) return false;
    for (int a = 0; a < 4096; a++) DATA_RAM[a].v = ram[a] & 0xF;
    updateBuzzer();
    return true;
}

Byte* ABRegisterPointer(unsigned char r_in){
    switch (r_in)
    {
    case A_R: return &A;
    case B_R: return &B;
    default: break;
    }
    return nullptr;
} 

Word* XYRegisterPointer(unsigned char r_in){
    switch (r_in)
    {
    case MX_R: return &IX;
    case MY_R: return &IY;
    default: break;
    }
    return nullptr;
}

void incrementClock(int num){
    cpu_clock = cpu_clock + num;
}

void setFlagsADD(int num){
    if (num >= 16){
        flags.C = 1;  
    }
    if ((num & 0xF) == 0){
        flags.Z = 1;
    } else {
        flags.Z = 0;
    }
}

void INS_JP_s(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    programCounter.PCB = programCounter.NBP;
    programCounter.PCP = programCounter.NPP;
    programCounter.PCS = s;
}

void INS_RETD_e(unsigned int op, Memory mem) {
    unsigned char e = op & 0xFF;
    programCounter.PCS = (readDataMem((SP.v + 1) & 0xFF).v << 4) | readDataMem((SP.v + 2) & 0xFF).v;
    programCounter.PCP = readDataMem(SP.v).v;
    programCounter.NPP = programCounter.PCP;
    SP.v = (SP.v + 3) & 0xFF;
    writeDataMem(IX.v, e & 0xF);
    writeDataMem((IX.v + 1) & 0xFFF, e >> 4);
    IX.v = (IX.v + 2) & 0xFFF;
}

void INS_JP_Cs(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    if (flags.C) {
        programCounter.PCB = programCounter.NBP;
        programCounter.PCP = programCounter.NPP;
        programCounter.PCS = s;
    } else {
        programCounter.NBP = programCounter.PCB;
        programCounter.NPP = programCounter.PCP;
    }
}

void INS_JP_NCs(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    if (!flags.C) {
        programCounter.PCB = programCounter.NBP;
        programCounter.PCP = programCounter.NPP;
        programCounter.PCS = s;
    } else {
        programCounter.NBP = programCounter.PCB;
        programCounter.NPP = programCounter.PCP;
    }
}

void INS_CALL_s(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    SP.v = (SP.v - 3) & 0xFF;
    writeDataMem(SP.v, programCounter.PCP);
    writeDataMem((SP.v + 1) & 0xFF, programCounter.PCS >> 4);
    writeDataMem((SP.v + 2) & 0xFF, programCounter.PCS & 0xF);
    // CALL stays in the current bank. PSET may select the destination page,
    // but its bank bit is ignored (MAME E0C6200 CALL semantics).
    programCounter.PCP = programCounter.NPP;
    programCounter.PCS = s;
}

void INS_CALZ_S(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    SP.v = (SP.v - 3) & 0xFF;
    writeDataMem(SP.v, programCounter.PCP);
    writeDataMem((SP.v + 1) & 0xFF, programCounter.PCS >> 4);
    writeDataMem((SP.v + 2) & 0xFF, programCounter.PCS & 0xF);
    programCounter.PCP = 0; // CALZ always calls into Page 0
    programCounter.NPP = 0;
    programCounter.PCS = s;
}

void INS_JP_Z_s(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    if (flags.Z) {
        programCounter.PCB = programCounter.NBP;
        programCounter.PCP = programCounter.NPP;
        programCounter.PCS = s;
    } else {
        programCounter.NBP = programCounter.PCB;
        programCounter.NPP = programCounter.PCP;
    }
}

void INS_JP_NZ_s(unsigned int op, Memory mem) {
    unsigned char s = op & 0xFF;
    if (!flags.Z) {
        programCounter.PCB = programCounter.NBP;
        programCounter.PCP = programCounter.NPP;
        programCounter.PCS = s;
    } else {
        programCounter.NBP = programCounter.PCB;
        programCounter.NPP = programCounter.PCP;
    }
}

void INS_LD_Y_e(unsigned int op, Memory mem) {
    IY.v = (IY.v & 0xF00) | (op & 0xFF);
}

void INS_LBPX_MX_e(unsigned int op, Memory mem) {
    unsigned char e = op & 0xFF; writeDataMem(IX.v, e & 0xF); writeDataMem((IX.v + 1) & 0xFFF, e >> 4); IX.v = (IX.v + 2) & 0xFFF;
}

void INS_ADC_XH_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = ((IX.v >> 4) & 0xF) + i + flags.C; IX.v = (IX.v & 0xF0F) | ((res & 0xF) << 4); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_ADC_XL_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = (IX.v & 0xF) + i + flags.C; IX.v = (IX.v & 0xFF0) | (res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_ADC_YH_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = ((IY.v >> 4) & 0xF) + i + flags.C; IY.v = (IY.v & 0xF0F) | ((res & 0xF) << 4); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_ADC_YL_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = (IY.v & 0xF) + i + flags.C; IY.v = (IY.v & 0xFF0) | (res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_CP_XH_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = ((IX.v >> 4) & 0xF) - i; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_CP_XL_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = (IX.v & 0xF) - i; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_CP_YH_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = ((IY.v >> 4) & 0xF) - i; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_CP_YL_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; unsigned int res = (IY.v & 0xF) - i; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_ADD_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_ADC_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + flags.C; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_SUB_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_SBC_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - flags.C; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_AND_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) & (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_OR_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) | (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_XOR_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) ^ (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_RLC_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); unsigned int c = flags.C; flags.C = (val >> 3) & 1; unsigned int res = ((val << 1) & 0xF) | c; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_LD_X_e(unsigned int op, Memory mem) {
    IX.v = (IX.v & 0xF00) | (op & 0xFF);
}

void INS_ADD_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + i; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_ADC_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + i + flags.C; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15);
}

void INS_AND_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) & i; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_OR_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) | i; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_XOR_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) ^ i; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_NOT_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int res = (~(r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v)) & 0xF; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_SBC_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - i - flags.C; if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_FAN_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) & i; flags.Z = (res == 0);
}

void INS_CP_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - i; flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_LD_r_i(unsigned int op, Memory mem) {
    unsigned char r = (op >> 4) & 3; unsigned char i = op & 0xF; if (r == 0) A.v = i; else if (r == 1) B.v = i; else if (r == 2) writeDataMem(IX.v, i); else writeDataMem(IY.v, i);;
}

void INS_PSET_p(unsigned int op, Memory mem) {
    unsigned char p = op & 0x1F; programCounter.NBP = p >> 4; programCounter.NPP = p & 0xF;
}

void INS_LDPX_MX_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; writeDataMem(IX.v, i); IX.v = (IX.v + 1) & 0xFFF;
}

void INS_LDPY_MY_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; writeDataMem(IY.v, i); IY.v = (IY.v + 1) & 0xFFF;
}

void INS_LD_XP_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IX.v = (IX.v & 0x0FF) | ((r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) << 8);
}

void INS_LD_XH_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IX.v = (IX.v & 0xF0F) | ((r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) << 4);
}

void INS_LD_XL_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IX.v = (IX.v & 0xFF0) | (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v);
}

void INS_RRC_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); unsigned int c = flags.C; flags.C = val & 1; unsigned int res = (val >> 1) | (c << 3); if (r == 0) A.v = res; else if (r == 1) B.v = res; else if (r == 2) writeDataMem(IX.v, res); else writeDataMem(IY.v, res);; flags.Z = (res == 0);
}

void INS_LD_YP_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IY.v = (IY.v & 0x0FF) | ((r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) << 8);
}

void INS_LD_YH_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IY.v = (IY.v & 0xF0F) | ((r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) << 4);
}

void INS_LD_YL_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; IY.v = (IY.v & 0xFF0) | (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v);
}

void INS_LD_r_XP(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (IX.v >> 8) & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_XH(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (IX.v >> 4) & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_XL(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = IX.v & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_YP(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (IY.v >> 8) & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_YH(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (IY.v >> 4) & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_YL(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = IY.v & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_LD_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int val = (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_INC_X(unsigned int op, Memory mem) {
    IX.v = (IX.v + 1) & 0xFFF;
}

void INS_LDPX_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int val = (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);; IX.v = (IX.v + 1) & 0xFFF;
}

void INS_INC_Y(unsigned int op, Memory mem) {
    IY.v = (IY.v + 1) & 0xFFF;
}

void INS_LDPY_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int val = (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);; IY.v = (IY.v + 1) & 0xFFF;
}

void INS_CP_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0);
}

void INS_FAN_r_q(unsigned int op, Memory mem) {
    unsigned char r = (op >> 2) & 3; unsigned char q = op & 3; unsigned int res = (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) & (q == 0 ? A.v : q == 1 ? B.v : q == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v); flags.Z = (res == 0);
}

void INS_ACPX_MX_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int res = readDataMem(IX.v).v + (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + flags.C; writeDataMem(IX.v, res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15); IX.v = (IX.v + 1) & 0xFFF;
}

void INS_ACPY_MY_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int res = readDataMem(IY.v).v + (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) + flags.C; writeDataMem(IY.v, res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15); IY.v = (IY.v + 1) & 0xFFF;
}

void INS_SCPX_MX_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int res = readDataMem(IX.v).v - (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - flags.C; writeDataMem(IX.v, res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0); IX.v = (IX.v + 1) & 0xFFF;
}

void INS_SCPY_MY_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int res = readDataMem(IY.v).v - (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) - flags.C; writeDataMem(IY.v, res & 0xF); flags.Z = ((res & 0xF) == 0); flags.C = (res > 15 ? 1 : 0); IY.v = (IY.v + 1) & 0xFFF;
}

void INS_SET_F_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; flags.I |= (i>>3)&1; flags.D |= (i>>2)&1; flags.Z |= (i>>1)&1; flags.C |= i&1;
}

void INS_SCF(unsigned int op, Memory mem) {
    flags.C = 1;
}

void INS_SZF(unsigned int op, Memory mem) {
    flags.Z = 1;
}

void INS_SDF(unsigned int op, Memory mem) {
    flags.D = 1;
}

void INS_EI(unsigned int op, Memory mem) {
    flags.I = 1;
}

void INS_RST_F_i(unsigned int op, Memory mem) {
    unsigned char i = op & 0xF; flags.I &= (i>>3)&1; flags.D &= (i>>2)&1; flags.Z &= (i>>1)&1; flags.C &= i&1;
}

void INS_DI(unsigned int op, Memory mem) {
    flags.I = 0;
}

void INS_RDF(unsigned int op, Memory mem) {
    flags.D = 0;
}

void INS_RZF(unsigned int op, Memory mem) {
    flags.Z = 0;
}

void INS_RCF(unsigned int op, Memory mem) {
    flags.C = 0;
}

void INS_INC_Mn(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF;
    unsigned int res = readDataMem(n).v + 1;
    writeDataMem(n, res & 0xF);
    flags.Z = ((res & 0xF) == 0);
    flags.C = (res > 15);
}

void INS_DEC_Mn(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF;
    unsigned int res = readDataMem(n).v - 1;
    writeDataMem(n, res & 0xF);
    flags.Z = ((res & 0xF) == 0);
    flags.C = (res > 15);
}

void INS_LD_Mn_A(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF; writeDataMem(n, A.v);
}

void INS_LD_Mn_B(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF; writeDataMem(n, B.v);
}

void INS_LD_A_Mn(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF; A.v = readDataMem(n).v;
}

void INS_LD_B_Mn(unsigned int op, Memory mem) {
    unsigned char n = op & 0xF; B.v = readDataMem(n).v;
}

void INS_PUSH_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v));
}

void INS_PUSH_XP(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (IX.v >> 8) & 0xF);
}

void INS_PUSH_XH(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (IX.v >> 4) & 0xF);
}

void INS_PUSH_XL(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, IX.v & 0xF);
}

void INS_PUSH_YP(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (IY.v >> 8) & 0xF);
}

void INS_PUSH_YH(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (IY.v >> 4) & 0xF);
}

void INS_PUSH_YL(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, IY.v & 0xF);
}

void INS_PUSH_F(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF; writeDataMem(SP.v, (flags.I << 3) | (flags.D << 2) | (flags.Z << 1) | flags.C);
}

void INS_DEC_SP(unsigned int op, Memory mem) {
    SP.v = (SP.v - 1) & 0xFF;
}

void INS_POP_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = readDataMem(SP.v).v; SP.v = (SP.v + 1) & 0xFF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_POP_XP(unsigned int op, Memory mem) {
    IX.v = (IX.v & 0x0FF) | (readDataMem(SP.v).v << 8); SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_XH(unsigned int op, Memory mem) {
    IX.v = (IX.v & 0xF0F) | (readDataMem(SP.v).v << 4); SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_XL(unsigned int op, Memory mem) {
    IX.v = (IX.v & 0xFF0) | readDataMem(SP.v).v; SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_YP(unsigned int op, Memory mem) {
    IY.v = (IY.v & 0x0FF) | (readDataMem(SP.v).v << 8); SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_YH(unsigned int op, Memory mem) {
    IY.v = (IY.v & 0xF0F) | (readDataMem(SP.v).v << 4); SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_YL(unsigned int op, Memory mem) {
    IY.v = (IY.v & 0xFF0) | readDataMem(SP.v).v; SP.v = (SP.v + 1) & 0xFF;
}

void INS_POP_F(unsigned int op, Memory mem) {
    unsigned char f = readDataMem(SP.v).v; SP.v = (SP.v + 1) & 0xFF; flags.I = (f >> 3) & 1; flags.D = (f >> 2) & 1; flags.Z = (f >> 1) & 1; flags.C = f & 1;
}

void INS_INC_SP(unsigned int op, Memory mem) {
    SP.v = (SP.v + 1) & 0xFF;
}

void INS_RETS(unsigned int op, Memory mem) {
    programCounter.PCS = (readDataMem((SP.v + 1) & 0xFF).v << 4) | readDataMem((SP.v + 2) & 0xFF).v;
    programCounter.PCP = readDataMem(SP.v).v;
    programCounter.NPP = programCounter.PCP;
    SP.v = (SP.v + 3) & 0xFF;
    programCounter.increment();
}

void INS_RET(unsigned int op, Memory mem) {
    programCounter.PCS = (readDataMem((SP.v + 1) & 0xFF).v << 4) | readDataMem((SP.v + 2) & 0xFF).v;
    programCounter.PCP = readDataMem(SP.v).v;
    programCounter.NPP = programCounter.PCP;
    SP.v = (SP.v + 3) & 0xFF;
}

void INS_LD_SPH_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; SP.v = (SP.v & 0x0F) | ((r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v) << 4);
}

void INS_LD_r_SPH(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = (SP.v >> 4) & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_JPBA(unsigned int op, Memory mem) {
    programCounter.PCB = programCounter.NBP;
    programCounter.PCP = programCounter.NPP;
    programCounter.PCS = (B.v << 4) | A.v;
}

void INS_LD_SPL_r(unsigned int op, Memory mem) {
    unsigned char r = op & 3; SP.v = (SP.v & 0xF0) | (r == 0 ? A.v : r == 1 ? B.v : r == 2 ? readDataMem(IX.v).v : readDataMem(IY.v).v);
}

void INS_LD_r_SPL(unsigned int op, Memory mem) {
    unsigned char r = op & 3; unsigned int val = SP.v & 0xF; if (r == 0) A.v = val; else if (r == 1) B.v = val; else if (r == 2) writeDataMem(IX.v, val); else writeDataMem(IY.v, val);;
}

void INS_HALT(unsigned int op, Memory mem) {
    /* HALT */
}

void INS_SLP(unsigned int op, Memory mem) {
    /* SLP */
}

void INS_NOP5(unsigned int op, Memory mem) {
    /* NOP 5 */
}

void INS_NOP7(unsigned int op, Memory mem) {
    /* NOP 7 */
}


#include <sstream>
#include <iomanip>

static const char* regName(unsigned char r) {
    switch (r & 3) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "MX";
        case 3: return "MY";
        default: return "?";
    }
}

static std::string hexByte(unsigned char val) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << (int)val;
    return ss.str();
}

static std::string hexNibble(unsigned char val) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << (int)(val & 0xF);
    return ss.str();
}

std::string disassembleInstruction(unsigned int op) {
    std::stringstream ss;
    
    if (instructionValue(JP_s, op)) return "JP " + hexByte(op & 0xFF);
    if (instructionValue(RETD_e, op)) return "RETD " + hexByte(op & 0xFF);
    if (instructionValue(JP_Cs, op)) return "JP C, " + hexByte(op & 0xFF);
    if (instructionValue(JP_NCs, op)) return "JP NC, " + hexByte(op & 0xFF);
    if (instructionValue(CALL_s, op)) return "CALL " + hexByte(op & 0xFF);
    if (instructionValue(CALZ_S, op)) return "CALZ " + hexByte(op & 0xFF);
    if (instructionValue(JP_Z_s, op)) return "JP Z, " + hexByte(op & 0xFF);
    if (instructionValue(JP_NZ_s, op)) return "JP NZ, " + hexByte(op & 0xFF);
    if (instructionValue(LD_Y_e, op)) return "LD Y, " + hexByte(op & 0xFF);
    if (instructionValue(LBPX_MX_e, op)) return "LBPX MX, " + hexByte(op & 0xFF);
    if (instructionValue(ADC_XH_i, op)) return "ADC XH, " + hexNibble(op & 0xF);
    if (instructionValue(ADC_XL_i, op)) return "ADC XL, " + hexNibble(op & 0xF);
    if (instructionValue(ADC_YH_i, op)) return "ADC YH, " + hexNibble(op & 0xF);
    if (instructionValue(ADC_YL_i, op)) return "ADC YL, " + hexNibble(op & 0xF);
    if (instructionValue(CP_XH_i, op)) return "CP XH, " + hexNibble(op & 0xF);
    if (instructionValue(CP_XL_i, op)) return "CP XL, " + hexNibble(op & 0xF);
    if (instructionValue(CP_YH_i, op)) return "CP YH, " + hexNibble(op & 0xF);
    if (instructionValue(CP_YL_i, op)) return "CP YL, " + hexNibble(op & 0xF);
    if (instructionValue(ADD_r_q, op)) return std::string("ADD ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(ADC_r_q, op)) return std::string("ADC ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(SUB_r_q, op)) return std::string("SUB ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(SBC_r_q, op)) return std::string("SBC ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(AND_r_q, op)) return std::string("AND ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(OR_r_q, op)) return std::string("OR ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(XOR_r_q, op)) return std::string("XOR ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(RLC_r, op)) return std::string("RLC ") + regName(op&3);
    if (instructionValue(LD_X_e, op)) return "LD X, " + hexByte(op & 0xFF);
    if (instructionValue(ADD_r_i, op)) return std::string("ADD ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(ADC_r_i, op)) return std::string("ADC ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(AND_r_i, op)) return std::string("AND ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(OR_r_i, op)) return std::string("OR ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(XOR_r_i, op)) {
        if ((op & 0xF) == 0xF) return std::string("NOT ") + regName((op>>4)&3);
        return std::string("XOR ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    }
    if (instructionValue(NOT_r, op)) return std::string("NOT ") + regName((op>>4)&3);
    if (instructionValue(SBC_r_i, op)) return std::string("SBC ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(FAN_r_i, op)) return std::string("FAN ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(CP_r_i, op)) return std::string("CP ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(LD_r_i, op)) return std::string("LD ") + regName((op>>4)&3) + ", " + hexNibble(op&0xF);
    if (instructionValue(PSET_p, op)) return "PSET " + hexByte(op & 0x1F);
    if (instructionValue(LDPX_MX_i, op)) return "LDPX MX, " + hexNibble(op & 0xF);
    if (instructionValue(LDPY_MY_i, op)) return "LDPY MY, " + hexNibble(op & 0xF);
    if (instructionValue(LD_XP_r, op)) return std::string("LD XP, ") + regName(op&3);
    if (instructionValue(LD_XH_r, op)) return std::string("LD XH, ") + regName(op&3);
    if (instructionValue(LD_XL_r, op)) return std::string("LD XL, ") + regName(op&3);
    if (instructionValue(RRC_r, op)) return std::string("RRC ") + regName(op&3);
    if (instructionValue(LD_YP_r, op)) return std::string("LD YP, ") + regName(op&3);
    if (instructionValue(LD_YH_r, op)) return std::string("LD YH, ") + regName(op&3);
    if (instructionValue(LD_YL_r, op)) return std::string("LD YL, ") + regName(op&3);
    if (instructionValue(LD_r_XP, op)) return std::string("LD ") + regName(op&3) + ", XP";
    if (instructionValue(LD_r_XH, op)) return std::string("LD ") + regName(op&3) + ", XH";
    if (instructionValue(LD_r_XL, op)) return std::string("LD ") + regName(op&3) + ", XL";
    if (instructionValue(LD_r_YP, op)) return std::string("LD ") + regName(op&3) + ", YP";
    if (instructionValue(LD_r_YH, op)) return std::string("LD ") + regName(op&3) + ", YH";
    if (instructionValue(LD_r_YL, op)) return std::string("LD ") + regName(op&3) + ", YL";
    if (instructionValue(LD_r_q, op)) return std::string("LD ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(INC_X, op)) return "INC X";
    if (instructionValue(LDPX_r_q, op)) return std::string("LDPX ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(INC_Y, op)) return "INC Y";
    if (instructionValue(LDPY_r_q, op)) return std::string("LDPY ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(CP_r_q, op)) return std::string("CP ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(FAN_r_q, op)) return std::string("FAN ") + regName((op>>2)&3) + ", " + regName(op&3);
    if (instructionValue(ACPX_MX_r, op)) return std::string("ACPX MX, ") + regName(op&3);
    if (instructionValue(ACPY_MY_r, op)) return std::string("ACPY MY, ") + regName(op&3);
    if (instructionValue(SCPX_MX_r, op)) return std::string("SCPX MX, ") + regName(op&3);
    if (instructionValue(SCPY_MY_r, op)) return std::string("SCPY MY, ") + regName(op&3);
    if (instructionValue(SET_F_i, op)) {
        if (op == SCF.min) return "SCF";
        if (op == SZF.min) return "SZF";
        if (op == SDF.min) return "SDF";
        if (op == EI.min) return "EI";
        return "SET F, " + hexNibble(op & 0xF);
    }
    if (instructionValue(SCF, op)) return "SCF";
    if (instructionValue(SZF, op)) return "SZF";
    if (instructionValue(SDF, op)) return "SDF";
    if (instructionValue(EI, op)) return "EI";
    if (instructionValue(RST_F_i, op)) {
        if (op == DI.min) return "DI";
        if (op == RDF.min) return "RDF";
        if (op == RZF.min) return "RZF";
        if (op == RCF.min) return "RCF";
        return "RST F, " + hexNibble(op & 0xF);
    }
    if (instructionValue(DI, op)) return "DI";
    if (instructionValue(RDF, op)) return "RDF";
    if (instructionValue(RZF, op)) return "RZF";
    if (instructionValue(RCF, op)) return "RCF";
    if (instructionValue(INC_Mn, op)) return "INC M" + hexNibble(op & 0xF);
    if (instructionValue(DEC_Mn, op)) return "DEC M" + hexNibble(op & 0xF);
    if (instructionValue(LD_Mn_A, op)) return "LD M" + hexNibble(op & 0xF) + ", A";
    if (instructionValue(LD_Mn_B, op)) return "LD M" + hexNibble(op & 0xF) + ", B";
    if (instructionValue(LD_A_Mn, op)) return "LD A, M" + hexNibble(op & 0xF);
    if (instructionValue(LD_B_Mn, op)) return "LD B, M" + hexNibble(op & 0xF);
    if (instructionValue(PUSH_r, op)) return std::string("PUSH ") + regName(op&3);
    if (instructionValue(PUSH_XP, op)) return "PUSH XP";
    if (instructionValue(PUSH_XH, op)) return "PUSH XH";
    if (instructionValue(PUSH_XL, op)) return "PUSH XL";
    if (instructionValue(PUSH_YP, op)) return "PUSH YP";
    if (instructionValue(PUSH_YH, op)) return "PUSH YH";
    if (instructionValue(PUSH_YL, op)) return "PUSH YL";
    if (instructionValue(PUSH_F, op)) return "PUSH F";
    if (instructionValue(DEC_SP, op)) return "DEC SP";
    if (instructionValue(POP_r, op)) return std::string("POP ") + regName(op&3);
    if (instructionValue(POP_XP, op)) return "POP XP";
    if (instructionValue(POP_XH, op)) return "POP XH";
    if (instructionValue(POP_XL, op)) return "POP XL";
    if (instructionValue(POP_YP, op)) return "POP YP";
    if (instructionValue(POP_YH, op)) return "POP YH";
    if (instructionValue(POP_YL, op)) return "POP YL";
    if (instructionValue(POP_F, op)) return "POP F";
    if (instructionValue(INC_SP, op)) return "INC SP";
    if (instructionValue(RETS, op)) return "RETS";
    if (instructionValue(RET, op)) return "RET";
    if (instructionValue(LD_SPH_r, op)) return std::string("LD SPH, ") + regName(op&3);
    if (instructionValue(LD_r_SPH, op)) return std::string("LD ") + regName(op&3) + ", SPH";
    if (instructionValue(JPBA, op)) return "JPBA";
    if (instructionValue(LD_SPL_r, op)) return std::string("LD SPL, ") + regName(op&3);
    if (instructionValue(LD_r_SPL, op)) return std::string("LD ") + regName(op&3) + ", SPL";
    if (instructionValue(HALT, op)) return "HALT";
    if (instructionValue(SLP, op)) return "SLP";
    if (instructionValue(NOP5, op)) return "NOP5";
    if (instructionValue(NOP7, op)) return "NOP7";

    return "UNKNOWN (0x" + hexByte(op) + ")";
}

void executeInstruction(unsigned int op, Memory mem) {
    if (instructionValue(JP_s, op)) { INS_JP_s(op, mem); return; }
    if (instructionValue(RETD_e, op)) { INS_RETD_e(op, mem); return; }
    if (instructionValue(JP_Cs, op)) { INS_JP_Cs(op, mem); return; }
    if (instructionValue(JP_NCs, op)) { INS_JP_NCs(op, mem); return; }
    if (instructionValue(CALL_s, op)) { INS_CALL_s(op, mem); return; }
    if (instructionValue(CALZ_S, op)) { INS_CALZ_S(op, mem); return; }
    if (instructionValue(JP_Z_s, op)) { INS_JP_Z_s(op, mem); return; }
    if (instructionValue(JP_NZ_s, op)) { INS_JP_NZ_s(op, mem); return; }
    if (instructionValue(LD_Y_e, op)) { INS_LD_Y_e(op, mem); return; }
    if (instructionValue(LBPX_MX_e, op)) { INS_LBPX_MX_e(op, mem); return; }
    if (instructionValue(ADC_XH_i, op)) { INS_ADC_XH_i(op, mem); return; }
    if (instructionValue(ADC_XL_i, op)) { INS_ADC_XL_i(op, mem); return; }
    if (instructionValue(ADC_YH_i, op)) { INS_ADC_YH_i(op, mem); return; }
    if (instructionValue(ADC_YL_i, op)) { INS_ADC_YL_i(op, mem); return; }
    if (instructionValue(CP_XH_i, op)) { INS_CP_XH_i(op, mem); return; }
    if (instructionValue(CP_XL_i, op)) { INS_CP_XL_i(op, mem); return; }
    if (instructionValue(CP_YH_i, op)) { INS_CP_YH_i(op, mem); return; }
    if (instructionValue(CP_YL_i, op)) { INS_CP_YL_i(op, mem); return; }
    if (instructionValue(ADD_r_q, op)) { INS_ADD_r_q(op, mem); return; }
    if (instructionValue(ADC_r_q, op)) { INS_ADC_r_q(op, mem); return; }
    if (instructionValue(SUB_r_q, op)) { INS_SUB_r_q(op, mem); return; }
    if (instructionValue(SBC_r_q, op)) { INS_SBC_r_q(op, mem); return; }
    if (instructionValue(AND_r_q, op)) { INS_AND_r_q(op, mem); return; }
    if (instructionValue(OR_r_q, op)) { INS_OR_r_q(op, mem); return; }
    if (instructionValue(XOR_r_q, op)) { INS_XOR_r_q(op, mem); return; }
    if (instructionValue(RLC_r, op)) { INS_RLC_r(op, mem); return; }
    if (instructionValue(LD_X_e, op)) { INS_LD_X_e(op, mem); return; }
    if (instructionValue(ADD_r_i, op)) { INS_ADD_r_i(op, mem); return; }
    if (instructionValue(ADC_r_i, op)) { INS_ADC_r_i(op, mem); return; }
    if (instructionValue(AND_r_i, op)) { INS_AND_r_i(op, mem); return; }
    if (instructionValue(OR_r_i, op)) { INS_OR_r_i(op, mem); return; }
    if (instructionValue(XOR_r_i, op)) { INS_XOR_r_i(op, mem); return; }
    if (instructionValue(NOT_r, op)) { INS_NOT_r(op, mem); return; }
    if (instructionValue(SBC_r_i, op)) { INS_SBC_r_i(op, mem); return; }
    if (instructionValue(FAN_r_i, op)) { INS_FAN_r_i(op, mem); return; }
    if (instructionValue(CP_r_i, op)) { INS_CP_r_i(op, mem); return; }
    if (instructionValue(LD_r_i, op)) { INS_LD_r_i(op, mem); return; }
    if (instructionValue(PSET_p, op)) { INS_PSET_p(op, mem); return; }
    if (instructionValue(LDPX_MX_i, op)) { INS_LDPX_MX_i(op, mem); return; }
    if (instructionValue(LDPY_MY_i, op)) { INS_LDPY_MY_i(op, mem); return; }
    if (instructionValue(LD_XP_r, op)) { INS_LD_XP_r(op, mem); return; }
    if (instructionValue(LD_XH_r, op)) { INS_LD_XH_r(op, mem); return; }
    if (instructionValue(LD_XL_r, op)) { INS_LD_XL_r(op, mem); return; }
    if (instructionValue(RRC_r, op)) { INS_RRC_r(op, mem); return; }
    if (instructionValue(LD_YP_r, op)) { INS_LD_YP_r(op, mem); return; }
    if (instructionValue(LD_YH_r, op)) { INS_LD_YH_r(op, mem); return; }
    if (instructionValue(LD_YL_r, op)) { INS_LD_YL_r(op, mem); return; }
    if (instructionValue(LD_r_XP, op)) { INS_LD_r_XP(op, mem); return; }
    if (instructionValue(LD_r_XH, op)) { INS_LD_r_XH(op, mem); return; }
    if (instructionValue(LD_r_XL, op)) { INS_LD_r_XL(op, mem); return; }
    if (instructionValue(LD_r_YP, op)) { INS_LD_r_YP(op, mem); return; }
    if (instructionValue(LD_r_YH, op)) { INS_LD_r_YH(op, mem); return; }
    if (instructionValue(LD_r_YL, op)) { INS_LD_r_YL(op, mem); return; }
    if (instructionValue(LD_r_q, op)) { INS_LD_r_q(op, mem); return; }
    if (instructionValue(INC_X, op)) { INS_INC_X(op, mem); return; }
    if (instructionValue(LDPX_r_q, op)) { INS_LDPX_r_q(op, mem); return; }
    if (instructionValue(INC_Y, op)) { INS_INC_Y(op, mem); return; }
    if (instructionValue(LDPY_r_q, op)) { INS_LDPY_r_q(op, mem); return; }
    if (instructionValue(CP_r_q, op)) { INS_CP_r_q(op, mem); return; }
    if (instructionValue(FAN_r_q, op)) { INS_FAN_r_q(op, mem); return; }
    if (instructionValue(ACPX_MX_r, op)) { INS_ACPX_MX_r(op, mem); return; }
    if (instructionValue(ACPY_MY_r, op)) { INS_ACPY_MY_r(op, mem); return; }
    if (instructionValue(SCPX_MX_r, op)) { INS_SCPX_MX_r(op, mem); return; }
    if (instructionValue(SCPY_MY_r, op)) { INS_SCPY_MY_r(op, mem); return; }
    if (instructionValue(SET_F_i, op)) { INS_SET_F_i(op, mem); return; }
    if (instructionValue(SCF, op)) { INS_SCF(op, mem); return; }
    if (instructionValue(SZF, op)) { INS_SZF(op, mem); return; }
    if (instructionValue(SDF, op)) { INS_SDF(op, mem); return; }
    if (instructionValue(EI, op)) { INS_EI(op, mem); return; }
    if (instructionValue(RST_F_i, op)) { INS_RST_F_i(op, mem); return; }
    if (instructionValue(DI, op)) { INS_DI(op, mem); return; }
    if (instructionValue(RDF, op)) { INS_RDF(op, mem); return; }
    if (instructionValue(RZF, op)) { INS_RZF(op, mem); return; }
    if (instructionValue(RCF, op)) { INS_RCF(op, mem); return; }
    if (instructionValue(INC_Mn, op)) { INS_INC_Mn(op, mem); return; }
    if (instructionValue(DEC_Mn, op)) { INS_DEC_Mn(op, mem); return; }
    if (instructionValue(LD_Mn_A, op)) { INS_LD_Mn_A(op, mem); return; }
    if (instructionValue(LD_Mn_B, op)) { INS_LD_Mn_B(op, mem); return; }
    if (instructionValue(LD_A_Mn, op)) { INS_LD_A_Mn(op, mem); return; }
    if (instructionValue(LD_B_Mn, op)) { INS_LD_B_Mn(op, mem); return; }
    if (instructionValue(PUSH_r, op)) { INS_PUSH_r(op, mem); return; }
    if (instructionValue(PUSH_XP, op)) { INS_PUSH_XP(op, mem); return; }
    if (instructionValue(PUSH_XH, op)) { INS_PUSH_XH(op, mem); return; }
    if (instructionValue(PUSH_XL, op)) { INS_PUSH_XL(op, mem); return; }
    if (instructionValue(PUSH_YP, op)) { INS_PUSH_YP(op, mem); return; }
    if (instructionValue(PUSH_YH, op)) { INS_PUSH_YH(op, mem); return; }
    if (instructionValue(PUSH_YL, op)) { INS_PUSH_YL(op, mem); return; }
    if (instructionValue(PUSH_F, op)) { INS_PUSH_F(op, mem); return; }
    if (instructionValue(DEC_SP, op)) { INS_DEC_SP(op, mem); return; }
    if (instructionValue(POP_r, op)) { INS_POP_r(op, mem); return; }
    if (instructionValue(POP_XP, op)) { INS_POP_XP(op, mem); return; }
    if (instructionValue(POP_XH, op)) { INS_POP_XH(op, mem); return; }
    if (instructionValue(POP_XL, op)) { INS_POP_XL(op, mem); return; }
    if (instructionValue(POP_YP, op)) { INS_POP_YP(op, mem); return; }
    if (instructionValue(POP_YH, op)) { INS_POP_YH(op, mem); return; }
    if (instructionValue(POP_YL, op)) { INS_POP_YL(op, mem); return; }
    if (instructionValue(POP_F, op)) { INS_POP_F(op, mem); return; }
    if (instructionValue(INC_SP, op)) { INS_INC_SP(op, mem); return; }
    if (instructionValue(RETS, op)) { INS_RETS(op, mem); return; }
    if (instructionValue(RET, op)) { INS_RET(op, mem); return; }
    if (instructionValue(LD_SPH_r, op)) { INS_LD_SPH_r(op, mem); return; }
    if (instructionValue(LD_r_SPH, op)) { INS_LD_r_SPH(op, mem); return; }
    if (instructionValue(JPBA, op)) { INS_JPBA(op, mem); return; }
    if (instructionValue(LD_SPL_r, op)) { INS_LD_SPL_r(op, mem); return; }
    if (instructionValue(LD_r_SPL, op)) { INS_LD_r_SPL(op, mem); return; }
    if (instructionValue(HALT, op)) { INS_HALT(op, mem); return; }
    if (instructionValue(SLP, op)) { INS_SLP(op, mem); return; }
    if (instructionValue(NOP5, op)) { INS_NOP5(op, mem); return; }
    if (instructionValue(NOP7, op)) { INS_NOP7(op, mem); return; }
}

unsigned int cpuStep(Memory mem, unsigned int &outPc, unsigned int &outOp) {
    // MAME computes the jump bank/page latch (NBP/NPP) at the start of each
    // instruction. It is normally the current PC bank/page, except when the
    // previous instruction was PSET, in which case PSET's value is used.
    if ((prevOpcode & 0xFE0) != 0xE40) {
        programCounter.NBP = programCounter.PCB;
        programCounter.NPP = programCounter.PCP;
    }

    outPc = programCounter.CurrentAddress();
    outOp = mem[outPc].v;
    programCounter.increment();
    executeInstruction(outOp, mem);
    prevOpcode = outOp;
    return outPc;
}

unsigned int instructionCycles(unsigned int op) {
    // E0C6200 opcode timing: baseline 5 cycles, many ALU/stack ops 7,
    // RETD/RETS 12. See MAME e0c6200.cpp / e0c6200op.cpp.
    if (instructionValue(RETD_e, op) || instructionValue(RETS, op)) return 12;
    if (instructionValue(CALL_s, op) || instructionValue(CALZ_S, op)) return 7;
    if (instructionValue(RET, op)) return 7;
    if (instructionValue(NOP7, op)) return 7;
    if (instructionValue(SET_F_i, op) || instructionValue(RST_F_i, op)) return 7;

    if (instructionValue(ADC_XH_i, op) || instructionValue(ADC_XL_i, op) ||
        instructionValue(ADC_YH_i, op) || instructionValue(ADC_YL_i, op) ||
        instructionValue(CP_XH_i, op) || instructionValue(CP_XL_i, op) ||
        instructionValue(CP_YH_i, op) || instructionValue(CP_YL_i, op) ||
        instructionValue(ADD_r_q, op) || instructionValue(ADC_r_q, op) ||
        instructionValue(SUB_r_q, op) || instructionValue(SBC_r_q, op) ||
        instructionValue(AND_r_q, op) || instructionValue(OR_r_q, op) ||
        instructionValue(XOR_r_q, op) || instructionValue(RLC_r, op) ||
        instructionValue(ADD_r_i, op) || instructionValue(ADC_r_i, op) ||
        instructionValue(AND_r_i, op) || instructionValue(OR_r_i, op) ||
        instructionValue(XOR_r_i, op) || instructionValue(SBC_r_i, op) ||
        instructionValue(FAN_r_i, op) || instructionValue(CP_r_i, op) ||
        instructionValue(CP_r_q, op) || instructionValue(FAN_r_q, op) ||
        instructionValue(ACPX_MX_r, op) || instructionValue(ACPY_MY_r, op) ||
        instructionValue(SCPX_MX_r, op) || instructionValue(SCPY_MY_r, op) ||
        instructionValue(INC_Mn, op) || instructionValue(DEC_Mn, op)) return 7;

    return 5;
}

void triggerInterrupt(unsigned char vectorStep) {
    if (!flags.I) return; // Interrupts globally disabled
    
    // Hardware pushes return address (PCP, PCS) to Stack
    SP.v = (SP.v - 3) & 0xFF;
    writeDataMem(SP.v, programCounter.PCP);
    writeDataMem((SP.v + 1) & 0xFF, programCounter.PCS >> 4);
    writeDataMem((SP.v + 2) & 0xFF, programCounter.PCS & 0xF);
    
    // Disable interrupts until EI or RET
    flags.I = 0;
    
    // Jump to hardware interrupt vector in Bank 0, Page 1
    programCounter.PCB = 0;
    programCounter.PCP = 1;
    programCounter.NBP = 0;
    programCounter.NPP = 1;
    programCounter.PCS = vectorStep;
    prevOpcode = 0xFFF; // the vector itself sees the current bank/page, not a stale PSET latch
}

// ----------------------------------------------------------------------
// Button mapping (simplified)
//   Buttons A/B/C  ->  K0 port bits 0/1/2 (active low: 0 = pressed)
//   The K0 interrupt factor (0xF04) is raised on the PRESS EDGE only
//   (release does not interrupt).  The emulator services the K0 vector
//   (0x106) once interrupts are enabled (EI) and the K0 mask (0xF14
//   bit 0) is set - matching the hardware behaviour.
// ----------------------------------------------------------------------
#define K0_PORT 0xF40
#define K0_FACTOR 0xF04
#define K0_MASK   0xF14

void setButtonInputs(bool btnA, bool btnB, bool btnC) {
    // active-low port value
    // Tamagotchi P1 button wiring (TamaLIB/mcugotchi):
    //   LEFT  -> K02
    //   MIDDLE-> K01
    //   RIGHT -> K00
    unsigned char k0Val = 0xF;
    if (btnA) k0Val &= ~0x04; // K02
    if (btnB) k0Val &= ~0x02; // K01
    if (btnC) k0Val &= ~0x01; // K00
    writeDataMem(K0_PORT, k0Val);

    static unsigned char prevK0 = 0xFF;
    if (k0Val != prevK0) {
        // press edge: a bit changed 1 -> 0
        unsigned char pressed = (prevK0 ^ k0Val) & ~k0Val;
        if (pressed) {
            writeDataMem(K0_FACTOR, 1);           // K0 interrupt factor
            if (DATA_RAM[K0_MASK].v & 1)          // K0 interrupt mask
                pendingInputInterrupt = true;
        }
        prevK0 = k0Val;
    }
}

// Convenience wrapper: one button at a time (0=A,1=B,2=C)
void setButton(int button, bool pressed) {
    if (button == 0) setButtonInputs(pressed, false, false);
    else if (button == 1) setButtonInputs(false, pressed, false);
    else setButtonInputs(false, false, pressed);
}

unsigned char execute(int cycles, Memory mem) {
    static unsigned long long totalCycles = 0;   // hardware cycles since reset
    static unsigned long long nextClockCycle = 1024;
    static unsigned long long nextProgCycle = 2048;
    static unsigned int timerTick = 0;
    const unsigned long long CLOCK_TIMER_INTERVAL = 1024; // 32 Hz @ 32.768 kHz
    const unsigned long long PROG_TIMER_INTERVAL  = 2048; // 16 Hz @ 32.768 kHz

    while (cycles > 0) {
        // Service pending interrupts before the next instruction.
        // MAME avoids interrupts directly after PSET/EI; mirror that gate.
        if (flags.I) {
            bool gate = (prevOpcode & 0xFE0) != 0xE40 && (prevOpcode & 0xFF8) != 0xF48;
            if (gate && pendingClockTimerInterrupt) {
                pendingClockTimerInterrupt = false;
                triggerInterrupt(0x02); // VEC_INT_CLOCK_TIMER
            }
            if (gate && pendingProgTimerInterrupt) {
                pendingProgTimerInterrupt = false;
                triggerInterrupt(0x0C); // VEC_INT_PROG_TIMER (16 Hz system tick)
            }
            if (gate && pendingInputInterrupt) {
                pendingInputInterrupt = false;
                triggerInterrupt(0x06); // VEC_INT_K00_K03
            }
        }

        unsigned int pc, fetchedWord;
        cpuStep(mem, pc, fetchedWord);
        unsigned int instrCycles = instructionCycles(fetchedWord);
        totalCycles += instrCycles;
        emuTotalCycles = totalCycles;
        cycles -= (int)instrCycles;

        // One-shot buzzer: stop after the programmed duration if no direct on.
        if (buzzerOneShotOn && totalCycles >= buzzerOneShotUntil) {
            buzzerOneShotOn = false;
            updateBuzzer();
        }

        // 32 Hz clock timer factors: IT32 every tick, IT8 every 4,
        // IT2 every 16, IT1 every 32. Interrupt only if factor & mask.
        while (totalCycles >= nextClockCycle) {
            timerTick++;
            unsigned char itVal = 0x01; // IT32
            if (timerTick % 4 == 0) itVal |= 0x02; // IT8
            if (timerTick % 16 == 0) itVal |= 0x04; // IT2
            if (timerTick % 32 == 0) itVal |= 0x08; // IT1
            DATA_RAM[0xF00].v |= itVal;
            if (DATA_RAM[0xF00].v & DATA_RAM[0xF10].v) {
                pendingClockTimerInterrupt = true;
            }
            nextClockCycle += CLOCK_TIMER_INTERVAL;
        }

        // 16 Hz programmable timer: firmware uses this as the system tick
        // for key debouncing and the 1-second state machine.
        while (totalCycles >= nextProgCycle) {
            DATA_RAM[0xF02].v |= 1; // PRGTIMER factor
            if (DATA_RAM[0xF02].v & DATA_RAM[0xF12].v) {
                pendingProgTimerInterrupt = true;
            }
            nextProgCycle += PROG_TIMER_INTERVAL;
        }
    }
    return 0;
}

