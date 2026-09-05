// display.hpp
// erstellt: 30. Jan. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#pragma once
#include <cstdint>
#include <string>
#include "cpu.hpp"

// Forward declaration for SDL types
struct SDL_Window;
struct SDL_Renderer;

// ---------------------------------------------------------------------------
// Host UI control state (owned by main loop, read by the SDL control bar)
// ---------------------------------------------------------------------------
struct HostAction {
    bool saveState   = false;   // save machine state to slot
    bool loadState   = false;   // load machine state from slot
    bool reset       = false;   // CPU reset
    bool pauseToggle = false;   // pause / unpause
    int  speedDelta  = 0;       // -1 = slower, +1 = faster
    bool hatch       = false;   // skip egg (turbo 5 min + auto A)
    bool skipMin     = false;   // advance +5 emulated minutes
    bool quit        = false;
};

inline double hostSpeed = 1.0;       // emulation speed multiplier
inline bool   hostPaused = false;    // emulation paused
inline int    hostSaveSlot = 1;      // active save slot (1..3)
inline const char* hostStatus = ""; // short status text shown in the control bar

// Plays a short audible click for host button feedback (SDL audio device).
void hostUiClick();

class TamagotchiDisplay {
public:
    // Tamagotchi P1 dot matrix dimensions
    static constexpr int SCREEN_WIDTH = 32;
    static constexpr int SCREEN_HEIGHT = 16;
    static constexpr int TOTAL_SEGMENTS = 40; // Hardware supports up to 40 columns
    static constexpr int ICON_COUNT = 8;
    
    // Pixel Color Palette (Retro LCD Green theme)
    static constexpr uint8_t BG_R = 0x9B, BG_G = 0xBC, BG_B = 0x88; // Light LCD green-gray
    static constexpr uint8_t FG_R = 0x1A, FG_G = 0x24, FG_B = 0x18; // Dark LCD pixel
    static constexpr uint8_t GRID_R = 0x8E, GRID_G = 0xAF, GRID_B = 0x7B; // Subtle pixel grid

    TamagotchiDisplay();
    ~TamagotchiDisplay();

    // Initializes SDL window and renderer (or fallback mode)
    bool init(const std::string& title = "Tamagotchi Emulator (E0C6S46)", int pixelScale = 16);
    
    // Renders the 32x16 dot matrix and icons from Display Data RAM (0xE00-0xECF)
    void render(const Byte dataRam[4096]);
    
    // Fallback terminal/ASCII rendering
    void renderAscii(const Byte dataRam[4096]);
    // Debug-Overlay: zeigt den Wert einer RAM-Zelle als grosse 5x7-Ziffer
    // ("CNT n") unter dem LCD an - nuetzlich fuer eigene Programme, deren
    // Zaehler/Variablen nicht als LCD-Pixel dargestellt werden.

    // Debug: render raw 40x16 VRAM linearly (SEG 0..39 on X, COM 0..15 on Y)
    void renderRawVram(const Byte dataRam[4096]) const;
    
    // Polls input events: host control actions + Tamagotchi buttons A, B, C.
    bool pollEvents(HostAction& action, bool& buttonA, bool& buttonB, bool& buttonC);
    
    // Clean up
    void close();

    bool isInitialized() const { return initialized; }

    // Helpers to decode Display RAM based on Epson E0C6S46 Section 7.3
    bool getPixel(const Byte dataRam[4096], int x, int y) const;
    bool getIcon(const Byte dataRam[4096], int index) const;

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int scale = 16;
    bool initialized = false;
    bool sdlAvailable = false;
};

inline TamagotchiDisplay display;
