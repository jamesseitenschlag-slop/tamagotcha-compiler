// needs.c - logical part 3: hunger, happiness, discipline and poo.
//
// original firmware: hearts (M11 hunger, M12 happy) drop over time; food
// and the mini game raise them. poo (M15) grows and has to be cleaned,
// otherwise the pet gets sick. the exact decay cadence lives in the isr
// sub blocks that use indexed cells ([OMIT-1] where dynamic).

// called once per game minute: hearts decay, poo can grow
void needs_tick(void) {
    if (mb_hunger != 0) {
        mb_hunger = mb_hunger - 1;
    }
    if (mc_happy != 0) {
        mc_happy = mc_happy - 1;
    }
    if (mf_poo < 4) {
        mf_poo = mf_poo + 1;               // poo grows each minute
    }
}

// meal raises hunger; snack raises hunger but costs happiness
void feed_meal(void) {
    if (mb_hunger < 4) {
        mb_hunger = mb_hunger + 1;
    }
}

void feed_snack(void) {
    if (mb_hunger < 4) {
        mb_hunger = mb_hunger + 1;
    }
    if (mc_happy != 0) {
        mc_happy = mc_happy - 1;
    }
}

// the mini game raises happiness
void play_game(void) {
    if (mc_happy < 4) {
        mc_happy = mc_happy + 1;
    }
}

// discipline bar; the original uses it for training the pet
void add_discipline(void) {
    if (md_disc < 4) {
        md_disc = md_disc + 1;
    }
}

// poo cleanup
void clean_poo(void) {
    mf_poo = 0;
}
