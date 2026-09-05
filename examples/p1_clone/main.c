// p1_clone - a playable reconstruction of the original tamagotchi p1,
// using only our C subset. the pet is drawn with the rom sprite set
// (egg, baby, child, teen, adult, poo, skull, zzz, bowl, tomb).
// NOTE: real minute timings are compressed so the demo feels alive.

char st;            // 0 egg 1 baby 2 child 3 teen 4 adult 5 dead
char sub;           // 0 main 1 menu 2 feed 3 play 4 light 5 med 6 disc 7 poo
char icon;          // highlighted device icon while in the menu
char hearts;        // hunger 0..4
char happy;         // happiness 0..4
char disc;          // discipline 0..4
char poo;           // poo counter 0..4
char sick;          // 1 = sick, shows skull
char sleepn;        // 1 = sleeping, shows zzz
char lamp;          // 1 = lamp on
char age;           // fake age in years
char fr;            // frame counter for animations
char t10;           // sub counter for timing
char rnd[4];        // 16 bit random state for the game
char tmp;           // scratch
char last;          // last key mask for edges
char ev;            // edge event of this frame
char blink;         // blink phase for icons and egg
char done;          // action finished flag
char waitcnt;       // wait helper

// small sleep helper, burns ticks so sprites stay visible
void pause_frames(void) {
    tmp = 0;
    while (tmp < 15) {
        tmp = tmp + 1;
    }
}

void reset(void) {
    st = 0;
    sub = 0;
    icon = 0;
    hearts = 2;
    happy = 3;
    disc = 0;
    poo = 0;
    sick = 0;
    sleepn = 0;
    lamp = 1;
    age = 0;
    fr = 0;
    t10 = 0;
    done = 0;
    waitcnt = 0;
}

// one-sprite helpers keep every function inside one rom page
void spr_zzz(void) { draw_sprite("ZZZ"); }
void spr_skull(void) { draw_sprite("SKULL"); }
void spr_egg(void) { draw_sprite("EGG"); }
void spr_eggw(void) { draw_sprite("EGG_WIGGLE"); }
void spr_baby(void) { draw_sprite("BABYTCHI"); }
void spr_child(void) { draw_sprite("MARUTCHI"); }
void spr_teen(void) { draw_sprite("MAMETCHI"); }
void spr_adult(void) { draw_sprite("MAMETCHI"); }
void spr_tomb(void) { draw_sprite("TOMB"); }
void spr_poop(void) { draw_sprite("POOP"); }
void spr_bowl(void) { draw_sprite("BOWL"); }

void draw_egg(void) {
    if (blink == 0) {
        spr_egg();
    } else {
        spr_eggw();
    }
}

void draw_pet(void) {
    if (sleepn != 0) {
        spr_zzz();
        return;
    }
    if (sick != 0) {
        spr_skull();
        return;
    }
    if (st == 0) {
        draw_egg();
    } else if (st == 1) {
        spr_baby();
    } else if (st == 2) {
        spr_child();
    } else if (st == 3) {
        spr_teen();
    } else if (st == 4) {
        spr_adult();
    } else {
        spr_tomb();
    }
}

void draw_body(void) {
    if (poo > 2) {
        spr_poop();
        return;
    }
    if (sub == 2) {
        spr_bowl();
        return;
    }
    draw_pet();
}

// keys: read_key gives bit0=A bit1=B bit2=C, active low
void read_keys(void) {
    char k;
    k = read_key();
    // invert to "pressed = 1"
    if ((k & 1) == 0) {
        ev = 1;
    } else {
        ev = 0;
    }
    if ((k & 2) == 0) {
        ev = ev | 2;
    }
    if ((k & 4) == 0) {
        ev = ev | 4;
    }
    // edge: only when something new got pressed
    if (ev == last) {
        ev = 0;
    }
    last = ev;
}

void tick_slow(void) {
    // called roughly 1/3 of all frames
    t10 = t10 + 1;
    if (t10 < 3) {
        return;
    }
    t10 = 0;
    // one "game minute" passed
    age = age + 1;
    if (hearts != 0) {
        hearts = hearts - 1;
    }
    if (happy != 0) {
        happy = happy - 1;
    }
    if (age > 12) {
        if (st == 1) {
            st = 2;
        }
    }
    if (age > 24) {
        if (st == 2) {
            st = 3;
        }
    }
    if (age > 36) {
        if (st == 3) {
            st = 4;
        }
    }
    if (hearts == 0) {
        if (happy == 0) {
            sick = 1;
        }
    }
    if (poo > 4) {
        sick = 1;
    }
    if (sick != 0) {
        if (age > 40) {
            st = 5;
        }
    }
}

void do_meal(void) {
    // eat: hunger rises, bowl shows for a moment
    sub = 2;
    waitcnt = 0;
    if (hearts < 4) {
        hearts = hearts + 1;
    }
}

void do_med(void) {
    sick = 0;
    done = 1;
}

void do_light(void) {
    lamp = ~lamp;
    if (lamp == 0) {
        sleepn = 1;
    } else {
        sleepn = 0;
    }
    done = 1;
}

void do_disc(void) {
    if (disc < 4) {
        disc = disc + 1;
    }
    done = 1;
}

void do_poo(void) {
    poo = 0;
    done = 1;
}

void run_action(void) {
    // execute the highlighted icon action
    if (icon == 0) {
        do_meal();
    } else if (icon == 1) {
        // play: bump happiness (no game sprite in this set)
        if (happy < 4) {
            happy = happy + 1;
        }
        done = 1;
    } else if (icon == 2) {
        do_med();
    } else if (icon == 3) {
        do_light();
    } else if (icon == 4) {
        do_disc();
    } else {
        do_poo();
    }
}

void menu_loop(void) {
    // menu is open: icons light up, B runs the action
    if ((ev & 2) != 0) {
        run_action();
    }
    if ((ev & 4) != 0) {
        if (icon < 5) {
            icon = icon + 1;
        } else {
            icon = 0;
        }
    }
    if ((ev & 1) != 0) {
        sub = 0;
        done = 1;
    }
}

void main(void) {
    reset();
    rnd[0] = 1; rnd[1] = 2; rnd[2] = 3; rnd[3] = 4;
    print("P1 CLONE");

    while (1) {
        read_keys();
        fr = fr + 1;
        if (fr == 50) {
            fr = 0;
        }
        if (fr == 0) {
            blink = ~blink;
        }
        tick_slow();
        // egg hatches when the fake age passes some mark
        if (st == 0) {
            if (age > 8) {
                st = 1;
                age = 0;
            }
        }
        // any attention: A opens menu in main state
        if (sub == 0) {
            if ((ev & 1) != 0) {
                sub = 1;
                icon = 0;
                clear_icons();
            }
        } else if (sub == 1) {
            menu_loop();
        } else if (sub == 2) {
            // eating animation runs a while, then back to main
            waitcnt = waitcnt + 1;
            if (waitcnt > 20) {
                sub = 0;
            }
        }
        // light icons: the selected device icon is shown on the frame
        if (sub == 1) {
            set_icon(icon);
        } else {
            clear_icons();
            if (poo != 0) {
                set_icon(6);
            }
        }
        // draw the screen
        clear_vram();
        draw_body();
        pause_frames();
    }
}
