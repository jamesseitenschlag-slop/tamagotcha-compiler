// clock.c - logical part 2: timer and clock.
//
// original firmware: the 16hz isr at 0x16F counts a 1 second countdown
// (cells 0x032/0x033) and advances the clock cells M7..M10. real timers
// and interrupts are not part of the subset ([OMIT-3]); this module is
// called from the main loop at the guest cadence.

// advance the clock by one second (M7..M10, seconds then minutes)
void clock_second(void) {
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

// one 16hz tick: countdown 0x032/0x033, then a second when it hits zero
void tick16_handler(void) {
    m6_tick = m6_tick + 1;                 // isr tick counter
    if (cd_lo != 0) {
        cd_lo = cd_lo - 1;                 // dec + jp nz past
        return;
    }
    if (cd_hi != 0) {
        cd_hi = cd_hi - 1;
        cd_lo = 15;                        // reload low nibble
        return;
    }
    // countdown empty -> one real second passed
    clock_second();
    cd_lo = 15;
    cd_hi = 5;                             // reload (approx. 1s worth)
}
