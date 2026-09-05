// main.c - decompile of the original p1 firmware, logical structure.
//
// parts (each file is one logical area of the original rom):
//   ram.h        cell mirror of the original ram + address table
//   keys.c       key input / debounce (original 0x16F isr part)
//   clock.c      timers and the clock (original 0x16F isr part)
//   needs.c      hunger, happiness, discipline, poo
//   pet_state.c  egg -> baby -> child -> teen -> adult -> dead
//   menu.c       icon menu and actions
//
// rules: every nibble cell that the firmware uses directly becomes a
// global; every branch becomes if/while; unexpressible parts are marked
// [OMIT-n] with the reason (see the list in p1_asm1to1).

#include "ram.h"
#include "keys.c"
#include "clock.c"
#include "needs.c"
#include "pet_state.c"
#include "menu.c"

// boot: the rom resets all ram via reset_all_ram (0x1200) and jumps into
// the ui loop. zeroing is done here per cell.
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
    mb_hunger = 2;
    mc_happy = 3;
    md_disc = 0;
    me_age = 0;
    mf_poo = 0;
    cd_lo = 0;
    cd_hi = 0;
    blink = 0;
    key_last = 0;
}

// main: the subset has no interrupt, so the 16hz core is called from the
// loop directly ([OMIT-3] for the real cadence).
void main(void) {
    boot_init();
    m0_ui = 0;

    while (1) {
        // 16hz tick layer
        tick16_handler();
        debounce_keys();

        // 1 minute layer: needs decay
        if (m6_tick == 0) {
            needs_tick();
            me_age = me_age + 1;
        }

        // ui layer
        if (m1_sub == 1) {
            menu_tick();
        } else {
            if ((m4_keyev & 1) != 0) {     // A opens the menu
                m1_sub = 1;
                m2_icon = 0;
            } else {
                ui_main();
            }
        }

        // [OMIT-5] rendering goes through the emulator sprite layer
        if (m6_tick == 0) {
            print_num("ST", m0_ui);
        }
    }
}
