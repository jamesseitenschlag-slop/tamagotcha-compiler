// cpu.hpp
// erstellt: 09. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag

#include <string>

/// <summary> This file contains the architecture implementation of the
/// E06S46 4-bit CMOS Microcomputer. In this file: CPU/ RAM and ROM </summary>

// Warning: This file is heavily under construction and NOT for production



#pragma once



// Defines for Datatypes of the E06S46 


struct Byte {
    unsigned int v : 4;
};

struct Double {
    unsigned int v : 8;
};

struct Word {
    unsigned int v : 12;
};

struct DWord {
    unsigned int v : 16;
};

struct QWord {
    unsigned int v : 20;
};




// Low and high define the range of a given instruction since OP-codes can
// start with different bits even though their mnemonic is the same.
// The OP-codes heavily depend on the kinds of operands given. Maybe
// a limitation of a 4-bit processor.


// TODO implement static_assert

struct Instruction {
    unsigned int max : 12;
    unsigned int min : 12;

    Instruction(unsigned int x, unsigned int y){
        min = x;
        max = y;
    }

    Instruction(unsigned int x){
        max = x;
        min = x;
    }
};



// Instruction ranges in Hex
inline Instruction JP_s            (0x000, 0x0FF);
inline Instruction RETD_e          (0x100, 0x1FF);
inline Instruction JP_Cs           (0x200, 0x2FF);
inline Instruction JP_NCs          (0x300, 0x3FF);
inline Instruction CALL_s          (0x400, 0x4FF);
inline Instruction CALZ_S          (0x500, 0x5FF);
inline Instruction JP_Z_s          (0x600, 0x6FF);
inline Instruction JP_NZ_s         (0x700, 0x7FF);
inline Instruction LD_Y_e          (0x800, 0x8FF);
inline Instruction LBPX_MX_e       (0x900, 0x9FF);
inline Instruction ADC_XH_i        (0xA00, 0xA0F);
inline Instruction ADC_XL_i        (0xA10, 0xA1F);
inline Instruction ADC_YH_i        (0xA20, 0xA2F);
inline Instruction ADC_YL_i        (0xA30, 0xA3F);
inline Instruction CP_XH_i         (0xA40, 0xA4F);
inline Instruction CP_XL_i         (0xA50, 0xA5F);
inline Instruction CP_YH_i         (0xA60, 0xA6F);
inline Instruction CP_YL_i         (0xA70, 0xA7F);
inline Instruction ADD_r_q         (0xA80, 0xA8F);
inline Instruction ADC_r_q         (0xA90, 0xA9F);
inline Instruction SUB_r_q         (0xAA0, 0xAAF);
inline Instruction SBC_r_q         (0xAB0, 0xABF);
inline Instruction AND_r_q         (0xAC0, 0xACF);
inline Instruction OR_r_q          (0xAD0, 0xADF);
inline Instruction XOR_r_q         (0xAE0, 0xAEF);
inline Instruction RLC_r           (0xAF0, 0xAFF);
inline Instruction LD_X_e          (0xB00, 0xBFF);
inline Instruction ADD_r_i         (0xC00, 0xC3F);
inline Instruction ADC_r_i         (0xC40, 0xC7F);
inline Instruction AND_r_i         (0xC80, 0xCBF);
inline Instruction OR_r_i          (0xCC0, 0xCFF);
inline Instruction XOR_r_i         (0xD00, 0xD3F);
inline Instruction NOT_r           (0xD0F, 0xD3F);
inline Instruction SBC_r_i         (0xD40, 0xD7F);
inline Instruction FAN_r_i         (0xD80, 0xDBF);
inline Instruction CP_r_i          (0xDC0, 0xDFF);
inline Instruction LD_r_i          (0xE00, 0xE3F);
inline Instruction PSET_p          (0xE40, 0xE5F);
inline Instruction LDPX_MX_i       (0xE60, 0xE6F);
inline Instruction LDPY_MY_i       (0xE70, 0xE7F);
inline Instruction LD_XP_r         (0xE80, 0xE83);
inline Instruction LD_XH_r         (0xE84, 0xE87);
inline Instruction LD_XL_r         (0xE88, 0xE8B);
inline Instruction RRC_r           (0xE8C, 0xE8F);
inline Instruction LD_YP_r         (0xE90, 0xE93);
inline Instruction LD_YH_r         (0xE94, 0xE97);
inline Instruction LD_YL_r         (0xE98, 0xE9B);
inline Instruction LD_r_XP         (0xEA0, 0xEA3);
inline Instruction LD_r_XH         (0xEA4, 0xEA7);
inline Instruction LD_r_XL         (0xEA8, 0xEAB);
inline Instruction LD_r_YP         (0xEB0, 0xEB3);
inline Instruction LD_r_YH         (0xEB4, 0xEB7);
inline Instruction LD_r_YL         (0xEB8, 0xEBB);
inline Instruction LD_r_q          (0xEC0, 0xECF);
inline Instruction INC_X           (0xEE0);
inline Instruction LDPX_r_q        (0xEE0, 0xEEF);
inline Instruction INC_Y           (0xEF0); 
inline Instruction LDPY_r_q        (0xEF0, 0xEFF);
inline Instruction CP_r_q          (0xF00, 0xF0F);
inline Instruction FAN_r_q         (0xF10, 0xF1F);
inline Instruction ACPX_MX_r       (0xF28, 0xF2B);
inline Instruction ACPY_MY_r       (0xF2C, 0xF2F);
inline Instruction SCPX_MX_r       (0xF38, 0xF3B);
inline Instruction SCPY_MY_r       (0xF3C, 0xF3F);
inline Instruction SET_F_i         (0xF40, 0xF4F);
inline Instruction SCF             (0xF41); 
inline Instruction SZF             (0xF42); 
inline Instruction SDF             (0xF44); 
inline Instruction EI              (0xF48); 
inline Instruction RST_F_i         (0xF50, 0xF5F);
inline Instruction DI              (0xF57); 
inline Instruction RDF             (0xF5B); 
inline Instruction RZF             (0xF5D); 
inline Instruction RCF             (0xF5E); 
inline Instruction INC_Mn          (0xF60, 0xF6F);
inline Instruction DEC_Mn          (0xF70, 0xF7F);
inline Instruction LD_Mn_A         (0xF80, 0xF8F);
inline Instruction LD_Mn_B         (0xF90, 0xF9F);
inline Instruction LD_A_Mn         (0xFA0, 0xFAF);
inline Instruction LD_B_Mn         (0xFB0, 0xFBF);
inline Instruction PUSH_r          (0xFC0, 0xFC3);
inline Instruction PUSH_XP         (0xFC4);
inline Instruction PUSH_XH         (0xFC5);
inline Instruction PUSH_XL         (0xFC6);
inline Instruction PUSH_YP         (0xFC7);
inline Instruction PUSH_YH         (0xFC8);
inline Instruction PUSH_YL         (0xFC9);
inline Instruction PUSH_F          (0xFCA);
inline Instruction DEC_SP          (0xFCB);
inline Instruction POP_r           (0xFD0, 0xFD3);
inline Instruction POP_XP          (0xFD4);
inline Instruction POP_XH          (0xFD5);
inline Instruction POP_XL          (0xFD6);
inline Instruction POP_YP          (0xFD7);
inline Instruction POP_YH          (0xFD8);
inline Instruction POP_YL          (0xFD9);
inline Instruction POP_F           (0xFDA);
inline Instruction INC_SP          (0xFDB);
inline Instruction RETS            (0xFDE);
inline Instruction RET             (0xFDF);
inline Instruction LD_SPH_r        (0xFE0, 0xFE3);
inline Instruction LD_r_SPH        (0xFE4, 0xFE7);
inline Instruction JPBA            (0xFE8);
inline Instruction LD_SPL_r        (0xFF0, 0xFF3);
inline Instruction LD_r_SPL        (0xFF4, 0xFF7);
inline Instruction HALT            (0xFF8);
inline Instruction SLP             (0xFF9);
inline Instruction NOP5            (0xFFB);
inline Instruction NOP7            (0xFFF);


// The memory consists of 4,096 12-bit words

typedef Word Memory[6144];
inline Word ROM[6144];
inline Byte DATA_RAM[4096];

Byte readDataMem(unsigned int addr);
void writeDataMem(unsigned int addr, unsigned char val);


// Memory addressing

// Registers and pointer for data memory addressing 
//
// |-------------------|----------|---------------|
// |Register/ Pointer  | Mnemonic | Size (bits)   |
// |-------------------|----------|---------------|
// |Conditional        | IX       | 12            |
// |___________________|__________|_______________|
// |Subroutine Call    | IY       | 12            |
// |___________________|__________|_______________|
// |Stack Pointer      |          | 8             |
// |___________________|__________|_______________|
// |Register           | RP       | 4             |
// |___________________|__________|_______________|



// Jump instructions
// |---------------|--------------------------|
// |Unconditional  | JP                       |
// |---------------|--------------------------|
// |Conditional    | JP C, JP NC, JP Z, JP NZ |
// |_______________|__________________________|
// |Subroutine Call| CALL, CALZ               |
// |_______________|__________________________|
// |Page set       | PSET                     |
// |_______________|__________________________|
// |Indirect       | JPBA                     |
// |_______________|__________________________|

inline int cpu_clock;

enum class INSTRUCTIONS{

    // Jump/ branch
    JP,JPC,JPNC,JPZ,JPNZ,CALL,CALZ,RET,RETS,RETD,PSET,JPBA,

    // System control 
    NOP5,NOP7,HALT,SLP,

    // Index operation
    INC,LD,CP,

    // Data transfer
    // LD
    LDPX,LDPY,LBPX,

    // Flag operation
    SET,RST,RCF,SZF,RZF,SDF,RDF,EI,DI,

    // Stack operation
    // INC,LD
    DEC,PUSH,POP,

    // Arithmetic
    //CP,INC,DEC
    ADD,ADC,SUB,SBC,AND,OR,XOR,FAN,RLC,RRC,ACPX,ACPY,SCPX,SCPY,NOT,
};





// Defining register Address-values for OP-codes

#define A_R 0b00
#define B_R 0b01
#define MX_R 0b10
#define MY_R 0b11

enum class Register{
    A,B,MX,MY,
};

// TODO: Make the structs safe, that they return the correct, shiftet bit values depending on the input

void run();
void _INIT();

// The RAM consists of 640 4 bit integers.
// Data Memory is now mapped in DATA_RAM (4096 x 4-bit) with MMU readDataMem/writeDataMem

// The video RAM consists of 160 4 bit integers.
inline Byte VIDEO_RAM[160];

// Input port is a simple 8 bit int (or 4 bit double)
inline Double INPUT_PORT;

// Output port is an unusual 20 bit int (or qword plus 4 bit int)
inline QWord OUTPUT_PORT;

// The input port is an 16bit int (or 4 bit ord)
inline DWord IO_PORT;

// The program counter block


struct ProgramCounter{
    unsigned char PCB : 1;
    unsigned char PCP : 4;
    unsigned char NBP : 1;
    unsigned char NPP : 4;
    unsigned char PCS : 8;

    unsigned int CurrentAddress() const {
        return ((PCB & 1) << 12) | ((PCP & 0xF) << 8) | (PCS & 0xFF);
    }

    // Advance the 12-bit PC by one instruction word. The bank bit is preserved,
    // the page increments when the step overflows (matches MAME's pc increment).
    void increment() {
        unsigned int pc = CurrentAddress();
        unsigned int bank = pc & 0x1000;
        unsigned int low = (pc + 1) & 0x0FFF;
        PCB = (bank >> 12) & 1;
        PCP = (low >> 8) & 0xF;
        PCS = low & 0xFF;
    }
}; inline ProgramCounter programCounter;

// Previous opcode, used to determine whether the current jump page latch
// (NBP/NPP) comes from a PSET instruction or from the current PC.
inline unsigned int prevOpcode = 0xFFF;

// Set when a K0 input edge requests an interrupt but I flag is currently 0.
// The execute loop services it as soon as interrupts are enabled again.
inline bool pendingInputInterrupt = false;
inline bool pendingClockTimerInterrupt = false;
inline bool pendingProgTimerInterrupt = false;

// ----------------------------------------------------------------------
// Debug channel (semihosting style)
// ----------------------------------------------------------------------
// A program running on the CPU can send text messages to the host
// terminal:
//   1. write the string (8-bit chars, low nibble first, NUL-terminated)
//      into the debug buffer in DATA RAM,
//   2. write any value to the debug trigger register -> this raises a
//      debug interrupt in the emulator which flushes the buffer to
//      stdout and clears the trigger.
// The buffer and trigger live in DATA RAM addresses that the firmware
// never touches, so the original Tamagotchi ROM is unaffected.
#define DBG_BUFFER_ADDR   0x0C0   // string buffer (chars = 2 RAM nibbles)
#define DBG_BUFFER_CHARS  64      // max 64 chars (0x0C0 .. 0x17F)
#define DBG_TRIGGER_ADDR  0xF0E   // write = "new debug message"

// flush DBG_BUFFER to stdout (called by the write hook on DBG_TRIGGER_ADDR)
void debugFlush(void);
inline int debugMessageCount = 0;   // incremented per flushed message (tests)


// Flags I, D, Z, C; Interrupt, Decimal mode, Zero, Carry

struct Flags{
    unsigned int I : 1;
    unsigned int D : 1;
    unsigned int Z : 1;
    unsigned int C : 1;
}; inline Flags flags;

// Registers


inline Word IX, IY; // Mnemonic IX, IY

// General porpuse Registers

inline Byte A;
inline Byte B;

// Stack pointer
inline Double SP;  // Mnemonic SP

// Register pointer
inline Byte RP;     // Mnemonic RP




// Operands for reference: 
// p 5-bit immediate data or labels 00H to 1FH. Used to specify a destination address.
// s 8-bit immediate data or labels 00H to FFH. Used to specify a destination address.
// e 8-bit immediate data 00H to FFH.
// i 4-bit immediate data 00H to 0FH.
// r 2-bit immediate data. 
// q 2-bit immediate data. 
// ALU Operations



// Instruction ADD
// OP Code = 1100 00r1 r0 i3 i2 i1 i0

// 7 cycles
// r = YH/ YL, XH, XL
// TYPE II 6-bit OP Code, 6-bit Operand
// Flag C Set if carry is generated || reset
// Flag Z Set if result is 0 || reset
// Flag D not affected
// Flag I not affected

// MX Data memory location whose address is specified by IX
// MY Data memory location whose address is specified by IY
// Operation r <- r+i3 to i0
// Adds immediate data i to the contents of r-register



// A A register
// B B register
// XP XP register---four high-order bits of IX
// YP YP register---four high-order bits of IY
// X XHL register---eight low-order bits of IX
// Y YHL register---eight low-order bits of IY
// XH XH register---four high-order bits of XHL
// XL XL register---four low-order bits of XHL
// YH YH register---four high-order bits of YHL
// YL YL register---four low-order bits of YHL
// SP Stack pointer SP
// SPH Four high-order bits of SP
// SPL Four low-order bits of SP
// F Flag register (IF, DF, ZF, CF)
// MX Data memory location whose address is specified by IX
// MY Data memory location whose address is specified by IY
// Mn Data memory location within the register area (000H to 00FH), specified by immediate data n (0H to FH)
// C Carry
// NC No carry
// Z Zero
// NZ Not zero




std::string disassembleInstruction(unsigned int op);
void reset();

// Save/load full machine state (registers, PC, flags, RAM, pending IRQs,
// buzzer) to/from a binary file.
bool cpuSaveState(const std::string& path);
bool cpuLoadState(const std::string& path);
void triggerInterrupt(unsigned char vectorStep);
void setButtonInputs(bool btnA, bool btnB, bool btnC);
// simplified per-button setter: 0=A, 1=B, 2=C (press edge raises K0 int)
void setButton(int button, bool pressed);
void executeInstruction(unsigned int op, Memory mem);
unsigned int cpuStep(Memory mem, unsigned int &outPc, unsigned int &outOp);
unsigned char execute(int cycles, Memory mem);

extern bool buzzerOn;
extern int buzzerFreq;
extern unsigned long long emuTotalCycles;
