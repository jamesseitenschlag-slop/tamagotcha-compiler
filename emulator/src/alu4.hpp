// alu4.hpp
// erstellt: 10. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
// ============================================================================
// alu4.hpp - portable C-Kerne fuer die E0C6S46-Nibble-ALU.
//
// Historie: frueher standen hier native Inline-Assembly-Pfade (x86-64 und
// ARMv7). Ein direkter Vergleich (examples/native_test.cpp, 100 Mio.
// Ausfuehrungen, -O2/-O3) zeigte, dass die handgeschriebene Assembly
// ~30-40 % LANGSAMER ist als die identischen C-Kerne (0.71x):
//   __asm__ volatile wirkt als Optimierungs-Barriere, GCC/Clang erzeugen
//   aus den kurzen Formeln besseren Code (Scheduling, Unrolling).
// Die Assembly wurde daher wieder entfernt; diese Datei enthaelt nur noch
// portable, dokumentierte C-Inline-Kerne (Standard-C++, keine Builtins).
//
// Semantik (exakt wie die INS_*-Funktionen in cpu.cpp):
//     ADD:  v = (a+b+cin)&0xF,  c = 1 wenn a+b+cin >= 16,  z = (v == 0)
//     SUB:  v = (a-b-borrow)&0xF,  c = 1 bei Unterlauf
//           (wie Emulator: res = a-b-borrow; c = res>15),  z = (v == 0)
//     CP :  wie SUB, Ergebnis wird nicht gespeichert (nur Flags).
// ============================================================================
#pragma once
#include <cstdint>

struct TamaAlu {
    uint8_t v;   // Ergebnis 0..15
    uint8_t c;   // Carry/Borrow-Flag
    uint8_t z;   // Zero-Flag
};

// ADD / ADC (cin = 0 bzw. vorheriges Carry-Flag)
inline TamaAlu tama_add(uint8_t a, uint8_t b, uint8_t cin) {
    uint32_t r = a + b + (cin & 1);
    TamaAlu o;
    o.v = (uint8_t)(r & 0xF);
    o.c = (r >= 16) ? 1 : 0;
    o.z = (o.v == 0) ? 1 : 0;
    return o;
}

// SUB / SBC / CP (borrow = 0 bzw. vorheriges Borrow-Flag)
inline TamaAlu tama_sub(uint8_t a, uint8_t b, uint8_t borrow) {
    uint32_t r = (uint32_t)a - b - (borrow & 1);   // unsigned wrap wie Emulator
    TamaAlu o;
    o.v = (uint8_t)(r & 0xF);
    o.c = (r > 15) ? 1 : 0;
    o.z = (o.v == 0) ? 1 : 0;
    return o;
}

// Boolesche Kerne (Z aus Wert 0)
inline uint8_t tama_and(uint8_t a, uint8_t b) { return a & b; }
inline uint8_t tama_or (uint8_t a, uint8_t b) { return a | b; }
inline uint8_t tama_xor(uint8_t a, uint8_t b) { return a ^ b; }
inline uint8_t tama_not4(uint8_t a) { return (~a) & 0xF; }

// RLC r: v = ((v<<1)&0xF) | C_in ;  C_out = altes Bit 3
inline TamaAlu tama_rlc(uint8_t v, uint8_t cin) {
    TamaAlu o;
    o.c = (v >> 3) & 1;
    o.v = (uint8_t)(((v << 1) & 0xF) | (cin & 1));
    o.z = (o.v == 0) ? 1 : 0;
    return o;
}

// RRC r: v = (v>>1) | (C_in<<3) ;  C_out = altes Bit 0
inline TamaAlu tama_rrc(uint8_t v, uint8_t cin) {
    TamaAlu o;
    o.c = v & 1;
    o.v = (uint8_t)((v >> 1) | ((cin & 1) << 3));
    o.z = (o.v == 0) ? 1 : 0;
    return o;
}
