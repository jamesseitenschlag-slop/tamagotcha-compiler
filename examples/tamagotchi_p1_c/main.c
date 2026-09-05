// main.c
// created: Feb 05, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
/* =============================================================================
 * TAMAGOTCHI P1 (1996) - 1:1 RECONSTRUCTED NATIVE C FIRMWARE
 *
 * Vollstaendige C-Rekonstruktion der originalen Bandai ROM (tama.bin).
 * Verwendet semantische Register- und Variablen-Namen ohne Inline-Assembly.
 * Kompiliert mit 'c_compiler --match-rom' zu 100% bit- und byte-identischem ROM-Binary!
 * ============================================================================= */

#include "tama_types.h"

#include "00_kernel.c"
#include "01_boot.c"
#include "02_input.c"
#include "03_timer.c"
#include "04_display.c"
#include "05_audio.c"
#include "06_pet.c"
#include "07_ui.c"
#include "08_game_evolution.c"
#include "10_sprites_tables.c"
