// menu.c - logical part 5: menu, icons and actions.
//
// original firmware: A opens the action menu, the icons on the device
// frame (seg8/seg28) light up, B confirms and C switches the icon.
// hardware icon control is [OMIT-4], so the action is just remembered
// here; drawing is done by the emulator sprite layer ([OMIT-5]).

// run the action that is currently selected
void run_action(void) {
    if (m2_icon == 0) {
        feed_meal();
    } else if (m2_icon == 1) {
        play_game();
    } else if (m2_icon == 2) {
        clean_poo();
    } else if (m2_icon == 3) {
        add_discipline();
    } else {
        // medicine and light are [OMIT-4] (io driven in the rom)
    }
}

// one menu tick
void menu_tick(void) {
    if ((m4_keyev & 2) != 0) {             // B = confirm
        run_action();
        m1_sub = 0;                        // back to the main view
    }
    if ((m4_keyev & 4) != 0) {             // C = next icon
        if (m2_icon < 4) {
            m2_icon = m2_icon + 1;
        } else {
            m2_icon = 0;
        }
    }
}
