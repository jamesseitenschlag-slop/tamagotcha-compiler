// keys.c - logical part 1: key input.
// original firmware: K0 port (0xF40) is active low; the 16hz isr (0x16F)
// samples it into M5 and raises M4 on a press edge. the low level port
// read is [OMIT-4] (io is not part of the subset) -> read_key() instead.

void sample_keys(void) {
    m5_keys = read_key();
}

// edge detection per 16hz tick: m4_keyev gets the mask of keys that went
// from "up" to "down". key_last holds the previous state.
void debounce_keys(void) {
    char now;
    char ev;
    sample_keys();
    now = m5_keys;
    ev = now & (~key_last);
    m4_keyev = ev;
    key_last = now;
}
