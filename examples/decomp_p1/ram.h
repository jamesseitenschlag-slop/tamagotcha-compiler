// ram.h - logical part 0: cell mirror of the original firmware ram.
//
// This file is included by main.c once; all parts share these globals
// (the compiler builds one translation unit).
//
// original firmware cells (nibble values):
//   0x006 M0 ui main state       0x007 M1 ui sub state
//   0x008 M2 selected icon       0x009 M3 menu position
//   0x00A M4 key event           0x00B M5 key mask
//   0x00C M6 16hz tick           0x00D M7 seconds ones
//   0x00E M8 seconds tens        0x00F M9 minutes ones
//   0x010 M10 minutes tens       0x011 M11 hunger hearts
//   0x012 M12 happy hearts       0x013 M13 discipline bars
//   0x014 M14 age years          0x015 M15 poo counter
//   0x032/0x033 1 second countdown (lo, hi)
//
// mapping rule: every original cell that the firmware uses as a plain
// nibble flag/counter becomes one global here. cells that the firmware
// only touches through X/Y-indexed loops are handled with dynamic arrays
// (see p1_asm1to1) or marked [OMIT].

char m0_ui;        // 0x006 main ui state
char m1_sub;       // 0x007 ui sub state
char m2_icon;      // 0x008 selected icon
char m3_menu;      // 0x009 menu position
char m4_keyev;     // 0x00A key event
char m5_keys;      // 0x00B key mask
char m6_tick;      // 0x00C 16hz tick counter
char m7_sec1;      // 0x00D clock seconds, ones
char m8_sec10;     // 0x00E clock seconds, tens
char m9_min1;      // 0x00F clock minutes, ones
char ma_min10;     // 0x010 clock minutes, tens
char mb_hunger;    // 0x011 hunger hearts
char mc_happy;     // 0x012 happy hearts
char md_disc;      // 0x013 discipline bars
char me_age;       // 0x014 age years
char mf_poo;       // 0x015 poo counter
char cd_lo;        // 0x032 1s countdown low nibble
char cd_hi;        // 0x033 1s countdown high nibble
char blink;        // blink phase of the icons / egg
char key_last;     // previous key mask (edge detection)
