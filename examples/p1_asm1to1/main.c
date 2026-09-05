// p1_asm1to1 - 1:1 transcription of the original p1 firmware into the C
// subset, as far as the subset can express it.
//
// RULES:
//  * every ram cell that is addressable as a global nibble becomes a global
//  * every branch/call becomes if/while/function-call with the rom address
//    as a comment anchor
//  * whatever the subset cannot express is OMITTED and marked [OMIT] with
//    the original opcode(s); see the list at the bottom.
//
// LIMITATIONS that make a 100% identical result impossible:
//  [OMIT-1] dynamic ram access via IX/IY (e.g. LD A,MX with X from a
//           runtime counter) -> needs runtime indexing, subset has none
//  [OMIT-2] nibble-pair strings and 12-bit pointers in user code
//  [OMIT-3] interrupt vectors, stack, EI/DI, the 16hz isr as a separate
//           routine (no timers in the language)
//  [OMIT-4] io registers (k0 0xF40, buzzer 0xF54, lcd ram writes via lbpx)
//  [OMIT-5] page/bank switching, sprite streams in bank 1 (use draw_sprite
//           builtins instead, see p1_clone)
// ============================================================================

// original ram cells that map to globals (nibble values)
char m0_ui;        // 0x006 M0 main ui state
char m1_sub;       // 0x007 M1 ui sub state
char m2_icon;      // 0x008 M2 selected icon
char m3_menu;      // 0x009 M3 menu position
char m4_keyev;     // 0x00A M4 key event
char m5_keys;      // 0x00B M5 key mask
char m6_tick;      // 0x00C M6 16hz tick counter
char m7_sec1;      // 0x00D M7 clock seconds, ones
char m8_sec10;     // 0x00E M8 clock seconds, tens
char m9_min1;      // 0x00F M9 clock minutes, ones
char ma_min10;     // 0x010 M10 clock minutes, tens
char mb_hunger;    // 0x011 M11 hunger hearts
char mc_happy;     // 0x012 M12 happy hearts
char md_disc;      // 0x013 M13 discipline bars
char me_age;       // 0x014 M14 age years
char mf_poo;       // 0x015 M15 poo counter
char c1s;          // 0x032/0x033 1s countdown lo
char c1s2;         // 0x033 (hi part of the countdown)
char csec;         // model second counter
char blink;        // blink phase of icons and egg

// ---------------------------------------------------------------------------
// boot path, original around 0x100 .. 0x12F. the rom first clears ram via
// 0x1200 reset_all_ram, sets clock cells and jumps into the ui loop.
// cells that reset_all_ram touches are zeroed here 1:1; everything reset
// via dynamic loops is marked [OMIT-1].
// ---------------------------------------------------------------------------
void boot_init(void) {
    m0_ui = 0;
    m1_sub = 0;
    m2_icon = 0;
    m3_menu = 0;
    m4_keyev = 0;
    m5_keys = 0;
    m6_tick = 0;
    m7_sec1 = 0;
    m8_sec10 = 0;
    m9_min1 = 0;
    ma_min10 = 0;
    mb_hunger = 0;
    mc_happy = 0;
    md_disc = 0;
    me_age = 0;
    mf_poo = 0;
    c1s = 0;
    c1s2 = 0;
    csec = 0;
    blink = 0;
    // [OMIT-1] original clears 0x00..0x3F with a runtime index loop
}

// ---------------------------------------------------------------------------
// 16hz isr core, original 0x16F. the rom runs this on the timer vector
// 0x10C with EI/DI; the subset has no interrupts, so this is called from
// the main loop at the same cadence as m6_tick wraps. logic is transcribed
// 1:1 where it touches global cells.
// ---------------------------------------------------------------------------
void isr16(void) {
    m6_tick = m6_tick + 1;               // asm 0x170: inc counter

    // 1s countdown at 0x032/0x033 (lo, hi)
    if (c1s != 0) {
        c1s = c1s - 1;                   // asm: dec + jp nz past
    } else {
        if (c1s2 != 0) {
            c1s2 = c1s2 - 1;
            c1s = 15;                    // reload lo (0xF)
        } else {
            // one real second passed
            csec = csec + 1;
            if (m7_sec1 < 9) {
                m7_sec1 = m7_sec1 + 1;
            } else {
                m7_sec1 = 0;
                if (m8_sec10 < 5) {
                    m8_sec10 = m8_sec10 + 1;
                } else {
                    m8_sec10 = 0;
                    if (m9_min1 < 9) {
                        m9_min1 = m9_min1 + 1;
                    } else {
                        m9_min1 = 0;
                        if (ma_min10 < 5) {
                            ma_min10 = ma_min10 + 1;
                        } else {
                            ma_min10 = 0;
                        }
                    }
                }
            }
        }
    }

    // key handling from the isr (M4/M5): the isr samples K0 via
    // [OMIT-4] and stores the mask into m5_keys, then raises m4_keyev.
    m5_keys = read_key();
    // [OMIT-1][OMIT-4] original compares input bits and branches
}

// ---------------------------------------------------------------------------
// ui state machine, main routine around 0x0624. the state dispatch itself
// is expressible (if/else on m0_ui), per-state drawing is not expressible
// (vram/sprite work, marked [OMIT-5]).
// ---------------------------------------------------------------------------
void ui_main(void) {
    if (m0_ui == 0) {
        // egg screen: blink phase toggles, hatch after the countdown
        // [OMIT-5] egg animation is sprite/vram based
        if (m6_tick == 0) {
            blink = ~blink;
        }
        if (c1s == 0) {
            if (c1s2 == 0) {
                m0_ui = 1;               // egg -> baby
                // [OMIT-1] original resets several counters by index
            }
        }
    } else if (m0_ui == 1) {
        // baby screen; hunger check
        if (mb_hunger == 0) {
            // [OMIT-4] original raises food icon and buzzer request
            m4_keyev = 1;                // attention flag placeholder
        }
        if (me_age > 10) {
            m0_ui = 2;                   // baby -> child (age logic 0x01AB)
        }
    } else {
        // child/teen/adult states, death on neglect
        if (mb_hunger == 0) {
            if (mc_happy == 0) {
                m0_ui = 5;               // dead
            }
        }
    }
}

// ---------------------------------------------------------------------------
// main loop. the subset cannot host the timer interrupt, so this loop calls
// the isr core directly. every original vector or bank switch is [OMIT-5].
// ---------------------------------------------------------------------------
void main(void) {
    boot_init();
    m0_ui = 0;

    while (1) {
        isr16();          // [OMIT-3] real cadence comes from the rom timer
        ui_main();
        // [OMIT-5] rendering: original writes vram per state, use the
        // sprite builtins (p1_clone) or run the original rom instead.
        if (m6_tick == 0) {
            print_num("ST", m0_ui);
        }
    }
}

// ===========================================================================
// WHY NOT 1:1
//  - the rom addresses ram through X/Y registers all over the place;
//    the subset only has globals and constant-index arrays
//  - io, timers, interrupts, bank switching and the sprite streams are not
//    part of the language; that is the emulator hardware layer that runs
//    the original binary 1:1
//  -> for the exact original game run the original rom in the emulator;
//     this file transcribes everything that maps onto the subset 1:1 and
//     marks everything else with [OMIT].
// ===========================================================================
