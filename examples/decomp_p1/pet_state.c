// pet_state.c - logical part 4: the pet state machine.
//
// original firmware: egg (0) hatches into baby (1), then child (2),
// teen (3) and adult (4); neglect ends in death (5). the ui main
// routine lives around 0x0624, the age logic around 0x01AB.
// per-state drawing uses vram/sprite streams ([OMIT-5]).

// egg phase: blink animation plus hatch timer
void state_egg(void) {
    if (m6_tick == 0) {
        blink = ~blink;                    // asm equivalent toggles a cell
    }
    // hatch after the countdown empties
    if (cd_lo == 0) {
        if (cd_hi == 0) {
            m0_ui = 1;                     // egg -> baby
            // [OMIT-1] original resets several counters by index
        }
    }
}

// baby phase: hunger check and first evolution
void state_baby(void) {
    if (mb_hunger == 0) {
        // [OMIT-4] original lights the food icon and requests attention
        m4_keyev = 1;                      // attention flag placeholder
    }
    if (me_age > 10) {
        m0_ui = 2;                         // baby -> child
        me_age = 0;
    }
}

// grown phases: death by neglect
void state_grown(void) {
    if (mb_hunger == 0) {
        if (mc_happy == 0) {
            m0_ui = 5;                     // dead
        }
    }
    if (me_age > 24) {
        m0_ui = 3;                         // child -> teen
        me_age = 0;
    }
    if (me_age > 48) {
        m0_ui = 4;                         // teen -> adult
        me_age = 0;
    }
}

// one ui cycle
void ui_main(void) {
    if (m0_ui == 0) {
        state_egg();
    } else if (m0_ui == 1) {
        state_baby();
    } else if (m0_ui == 5) {
        // dead screen; nothing to do
    } else {
        state_grown();
    }
}
