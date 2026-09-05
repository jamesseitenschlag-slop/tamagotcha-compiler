// display.cpp
// erstellt: 06. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#include "display.hpp"
#include "os.hpp"
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>

#if __has_include(<SDL3/SDL.h>)
#include <SDL3/SDL.h>
#define USE_SDL3 1
#elif __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#define USE_SDL2 1
#endif

#include "tama_icons.inc"
#include "ui_font.inc"

// ---------------------------------------------------------------------------
// Control bar geometry (pixels, independent of the LCD pixel scale)
// ---------------------------------------------------------------------------
namespace ctl {
    constexpr float WIN_W  = 576.0f;   // 32*16 + 2*32
    constexpr float WIN_H  = 368.0f;   // 16*16 + 32 (top) + 80 (bottom bar)
    constexpr float BAR_Y  = 288.0f;   // top edge of the bottom control bar
    constexpr float BTN_Y  = 312.0f;
    constexpr float BTN_H  = 34.0f;
    constexpr float BTN_X0 = 8.0f;
    constexpr float BTN_GAP= 4.0f;

    enum Action { SPD_DN, SPD_UP, PAUSE, SAVE, LOAD, SKIP, HATCH, RESET, COUNT };
    struct Btn { const char* label; Action act; };
    constexpr Btn PANEL[COUNT] = {
        { "SPD-", SPD_DN }, { "SPD+", SPD_UP }, { "PAUSE", PAUSE },
        { "SAVE", SAVE },   { "LOAD", LOAD },   { "SKIP",  SKIP },
        { "HATCH", HATCH }, { "RESET", RESET },
    };
    struct Rect { float x, y, w, h; };
}

static SDL_Renderer* gR = nullptr;      // renderer for the current frame
static int gHoverBtn = -1;              // hovered panel button index

// ---------------------------------------------------------------------------
// Tiny 2D helpers (SDL3/SDL2 neutral)
// ---------------------------------------------------------------------------
static void setColor(int r, int g, int b, int a) {
    SDL_SetRenderDrawColor(gR, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
}
static void fillRect(float x, float y, float w, float h) {
#if defined(USE_SDL3)
    SDL_FRect r{ x, y, w, h };
    SDL_RenderFillRect(gR, &r);
#else
    SDL_Rect r{ (int)(x + 0.5f), (int)(y + 0.5f), (int)(w + 0.5f), (int)(h + 0.5f) };
    SDL_RenderFillRect(gR, &r);
#endif
}
static void outlineRect(float x, float y, float w, float h, int t = 1) {
    fillRect(x, y, w, (float)t);
    fillRect(x, y + h - t, w, (float)t);
    fillRect(x, y, (float)t, h);
    fillRect(x + w - t, y, (float)t, h);
}

// ---------------------------------------------------------------------------
// 5x7 bitmap text
// ---------------------------------------------------------------------------
static int glyphIndex(char c) {
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    const char* p = strchr(UI_FONT_ORDER, c);
    return p ? (int)(p - UI_FONT_ORDER) : -1;
}
static float textWidth(const char* s, int sc) {
    int n = 0;
    while (*s) { if (glyphIndex(*s++) >= 0) n++; }
    if (n == 0) return 0;
    return (n * 6 - 1) * sc;            // 5 px glyph + 1 px spacing (last: none)
}
static void drawText(const char* s, float x, float y, int sc, int r, int g, int b, int a = 255) {
    setColor(r, g, b, a);
    for (; *s; s++) {
        int gi = glyphIndex(*s);
        if (gi < 0) { x += 6.0f * sc; continue; }   // keep advance (spaces)
        for (int row = 0; row < 7; row++) {
            unsigned char bits = UI_FONT[gi][row];
            for (int col = 0; col < 5; col++) {
                if ((bits >> (4 - col)) & 1)
                    fillRect(x + col * sc, y + row * sc, (float)sc, (float)sc);
            }
        }
        x += 6.0f * sc;
    }
}

// ---------------------------------------------------------------------------
// Panel layout / hit test
// ---------------------------------------------------------------------------
static void panelLayout(ctl::Rect* out) {
    float x = ctl::BTN_X0;
    for (int i = 0; i < ctl::COUNT; i++) {
        float w = textWidth(ctl::PANEL[i].label, 2) + 16;
        out[i] = { x, ctl::BTN_Y, w, ctl::BTN_H };
        x += w + ctl::BTN_GAP;
    }
}
static int panelHit(int mx, int my) {
    ctl::Rect rc[ctl::COUNT];
    panelLayout(rc);
    for (int i = 0; i < ctl::COUNT; i++) {
        if (mx >= rc[i].x && mx <= rc[i].x + rc[i].w &&
            my >= rc[i].y && my <= rc[i].y + rc[i].h)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Audio (8-bit square wave; buzzer is active-low -> buzzerOn means "beep now")
// ---------------------------------------------------------------------------
static SDL_AudioStream* audioStream = nullptr;

static void SDLCALL audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    (void)userdata; (void)total_amount;
    if (additional_amount <= 0) return;
    int samples = additional_amount;            // 8-bit format: 1 byte per sample
    std::vector<int8_t> buf(samples, 0);
    if (buzzerOn && buzzerFreq > 0) {
        static long long smp = 0;
        int half = 44100 / buzzerFreq / 2;      // samples per half period
        if (half < 1) half = 1;
        for (int i = 0; i < samples; i++) {
            buf[i] = ((smp / half) & 1) ? -80 : 80;
            smp++;
        }
    }
    SDL_PutAudioStreamData(stream, buf.data(), additional_amount);
}

// ---------------------------------------------------------------------------
// Host UI click (short beep so button presses give audible feedback)
// ---------------------------------------------------------------------------
void hostUiClick() {
    if (!audioStream) return;
    const int ms = 26;                 // short blip
    const double freq = 1568.0;        // ~G6, clearly audible
    int n = 44100 * ms / 1000;
    std::vector<int8_t> buf(n);
    double period = 44100.0 / freq;
    for (int i = 0; i < n; i++) {
        double env = 1.0 - (double)i / n;      // linear fade -> no harsh edge
        double amp = 55.0 * env * env;
        int sq = (((int)(i / period)) & 1) ? -1 : 1;
        buf[i] = (int8_t)(sq * amp);
    }
    SDL_PutAudioStreamData(audioStream, buf.data(), n);
}

static void playClick() { hostUiClick(); }

TamagotchiDisplay::TamagotchiDisplay() : scale(16), initialized(false), sdlAvailable(false) {}

TamagotchiDisplay::~TamagotchiDisplay() {
    close();
}

bool TamagotchiDisplay::init(const std::string& title, int pixelScale) {
    scale = pixelScale;
    int w = (int)ctl::WIN_W;
    int h = (int)ctl::WIN_H;

#if defined(USE_SDL3)
    if (SDL_Init(SDL_INIT_VIDEO)) {
        window = SDL_CreateWindow(title.c_str(), w, h, 0);
        if (window) {
            renderer = SDL_CreateRenderer(window, nullptr);
            if (renderer) {
                sdlAvailable = true;
                initialized = true;
                SDL_RaiseWindow(window);
                SDL_Init(SDL_INIT_AUDIO);
                SDL_AudioSpec spec{ SDL_AUDIO_S8, 1, 44100 };
                audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioCallback, nullptr);
                if (audioStream) SDL_ResumeAudioStreamDevice(audioStream);
                SDL_StartTextInput(window);   // OS: Tastatur -> Mailbox
                std::cout << "[Display] SDL3 graphical window initialized (" << w << "x" << h << ")\n";
                return true;
            }
        }
    }
#elif defined(USE_SDL2)
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  w, h, SDL_WINDOW_SHOWN);
        if (window) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (renderer) {
                sdlAvailable = true;
                initialized = true;
                SDL_RaiseWindow(window);
                SDL_StartTextInput(window);
                std::cout << "[Display] SDL2 display initialized (" << w << "x" << h << ")\n";
                return true;
            }
        }
    }
#endif

    // Fallback mode
    std::cout << "[Display] Running in Headless / Terminal ASCII Fallback mode.\n";
    initialized = true;
    sdlAvailable = false;
    return true;
}

bool TamagotchiDisplay::getPixel(const Byte dataRam[4096], int x, int y) const {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;

    // Correct physical SEG->column map from TamaLIB/mcugotchi hw.c.
    static const int col_to_seg[32] = {
         0, 1, 2, 3, 4, 5, 6, 7, 9,10,
        11,12,13,14,15,16,
        36,35,34,33,32,31,
        30,29,27,26,25,24,23,22,21,20
    };
    // Interleaved VRAM layout: each segment uses two nibbles per bank.
    //   bank 0 (0xE00-0xE4F): COM0-3 (first nibble), COM4-7 (second nibble)
    //   bank 1 (0xE80-0xECF): COM8-11 (first nibble), COM12-15 (second nibble)
    unsigned int seg = col_to_seg[x];
    unsigned int com = y;
    unsigned int bank = com / 8;
    unsigned int addr = (bank == 0 ? 0xE00 : 0xE80) + seg * 2 + ((com % 8) / 4);
    unsigned int bit = com % 4;

    return ((dataRam[addr & 0xFFF].v >> bit) & 1) != 0;
}

bool TamagotchiDisplay::getIcon(const Byte dataRam[4096], int index) const {
    if (index < 4) {
        // Icons 0..3: SEG 8, COM 0..3 -> VRAM 0xE10, bits 0..3
        return (dataRam[0xE10].v >> index) & 1;
    }
    // Icons 4..7: SEG 28, COM 12..15 -> VRAM 0xEB9, bits 0..3
    return (dataRam[0xEB9].v >> (index - 4)) & 1;
}

void TamagotchiDisplay::render(const Byte dataRam[4096]) {
    if (!initialized) return;

#if defined(USE_SDL3) || defined(USE_SDL2)
    if (sdlAvailable && renderer) {
        gR = renderer;

        // Background
        setColor(BG_R, BG_G, BG_B, 255);
        SDL_RenderClear(gR);

        const float offsetX = 32.0f;
        const float offsetY = 32.0f;
        const float lcdW = SCREEN_WIDTH * (float)scale;
        const float lcdH = SCREEN_HEIGHT * (float)scale;

        // Draw 32x16 LCD matrix (cell borders act as the pixel grid).
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                float px = offsetX + x * scale;
                float py = offsetY + y * scale;
                bool on = getPixel(dataRam, x, y);
                if (on) {
                    setColor(FG_R, FG_G, FG_B, 255);
                    fillRect(px, py, scale - 1, scale - 1);
                } else {
                    setColor(GRID_R, GRID_G, GRID_B, 100);
                    fillRect(px, py, scale - 1, 1);
                    fillRect(px + scale - 1, py, 1, scale - 1);
                    fillRect(px, py + scale - 1, scale - 1, 1);
                    fillRect(px, py, 1, scale - 1);
                }
            }
        }

        // Status icons: 0..3 vertical left of the LCD, 4..7 vertical right.
        const int iconScale = 3;
        const int iconCell = 8 * iconScale;        // 24 px
        const int iconPitch = iconCell + 8;        // 32 px
        const int iconY0 = (int)offsetY + ((int)lcdH - (4 * iconCell + 3 * 8)) / 2;
        for (int icon = 0; icon < ICON_COUNT; icon++) {
            bool on = getIcon(dataRam, icon);
            float ix = (icon < 4) ? 4.0f : (offsetX + lcdW + 4.0f);
            float iy = iconY0 + (icon % 4) * iconPitch;
            for (int py = 0; py < 8; py++) {
                for (int px = 0; px < 8; px++) {
                    if (!tama_icons[icon][py][px]) continue;
                    if (on) {
                        setColor(FG_R, FG_G, FG_B, 255);
                        fillRect(ix + px * iconScale, iy + py * iconScale, iconScale - 1, iconScale - 1);
                    } else {
                        setColor(GRID_R, GRID_G, GRID_B, 120);
                        fillRect(ix + px * iconScale, iy + py * iconScale, iconScale - 1, iconScale - 1);
                    }
                }
            }
        }

        // ---- Bottom control bar ----------------------------------------
        setColor(0x85, 0xA5, 0x72, 255);            // panel background
        fillRect(0, ctl::BAR_Y, ctl::WIN_W, ctl::WIN_H - ctl::BAR_Y);
        setColor(GRID_R, GRID_G, GRID_B, 160);      // separator line
        fillRect(0, ctl::BAR_Y, ctl::WIN_W, 1);

        // Status line (speed / pause / slot)
        char status[96];
        char spd[16];
        if (hostPaused) std::snprintf(spd, sizeof spd, "PAUSED");
        else            std::snprintf(spd, sizeof spd, "%.3gX", hostSpeed);
        std::snprintf(status, sizeof status, "SPEED %-7s  SLOT %d", spd, hostSaveSlot);
        drawText(status, 10, ctl::BAR_Y + 6, 2, FG_R, FG_G, FG_B, 255);
        if (hostStatus && hostStatus[0]) {
            drawText(hostStatus, ctl::WIN_W - textWidth(hostStatus, 2) - 10,
                     ctl::BAR_Y + 6, 2, FG_R, FG_G, FG_B, 255);
        }

        // Buttons
        ctl::Rect rc[ctl::COUNT];
        panelLayout(rc);
        for (int i = 0; i < ctl::COUNT; i++) {
            bool hover = (i == gHoverBtn);
            bool active = (ctl::PANEL[i].act == ctl::PAUSE && hostPaused);
            if (hover || active) {
                setColor(FG_R, FG_G, FG_B, 255);
                fillRect(rc[i].x, rc[i].y, rc[i].w, rc[i].h);
            } else {
                setColor(0xC3, 0xDD, 0xAA, 255);
                fillRect(rc[i].x, rc[i].y, rc[i].w, rc[i].h);
            }
            setColor(0x5E, 0x7A, 0x50, 255);
            outlineRect(rc[i].x, rc[i].y, rc[i].w, rc[i].h, 1);
            float tx = rc[i].x + (rc[i].w - textWidth(ctl::PANEL[i].label, 2)) / 2;
            float ty = rc[i].y + (rc[i].h - 7 * 2) / 2;
            if (hover || active) {
                drawText(ctl::PANEL[i].label, tx, ty, 2, BG_R, BG_G, BG_B, 255);
            } else {
                drawText(ctl::PANEL[i].label, tx, ty, 2, FG_R, FG_G, FG_B, 255);
            }
        }

        // Key help (small)
        const char* help = "F1 HATCH  F2 +5MIN  F3/F4 SPEED  F5 SAVE  F8 LOAD  SPACE PAUSE  R RESET  ESC QUIT";
        drawText(help, 8, ctl::BTN_Y + ctl::BTN_H + 6, 1, 0x2C, 0x41, 0x30, 220);

        SDL_RenderPresent(gR);
        return;
    }
#endif

    // Fallback: render in terminal
    renderAscii(dataRam);
}

void TamagotchiDisplay::renderAscii(const Byte dataRam[4096]) {
    std::cout << "\n[ICONS] ";
    for (int i = 0; i < ICON_COUNT; i++) std::cout << (getIcon(dataRam, i) ? '#' : '.');
    std::cout << "\n+--------------------------------+\n";
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        std::cout << "|";
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            bool on = getPixel(dataRam, x, y);
            std::cout << (on ? "##" : "  ");
        }
        std::cout << "|\n";
    }
    std::cout << "+--------------------------------+\n";
}

void TamagotchiDisplay::renderRawVram(const Byte dataRam[4096]) const {
    static const int group_base[4] = { 0xE00, 0xE28, 0xE80, 0xEA8 };

    std::cout << "\n[RAW VRAM 40x16 - SEG 0..39 left-to-right]\n";
    std::cout << "   ";
    for (int seg = 0; seg < 40; seg++) std::cout << (seg % 10);
    std::cout << "\n";

    for (int y = 0; y < 16; y++) {
        std::cout << (y < 10 ? " " : "") << y << " ";
        int group = y / 4;
        int base = group_base[group];
        int bit = y % 4;
        for (int seg = 0; seg < 40; seg++) {
            int addr = base + seg;
            bool on = ((dataRam[addr & 0xFFF].v >> bit) & 1) != 0;
            std::cout << (on ? '#' : '.');
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

bool TamagotchiDisplay::pollEvents(HostAction& action, bool& buttonA, bool& buttonB, bool& buttonC) {
    if (!sdlAvailable) return true;

#if defined(USE_SDL3)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) { action.quit = true; return false; }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.repeat) continue;
            if (event.key.key == SDLK_F1) { action.hatch = true; playClick(); }
            else if (event.key.key == SDLK_F2) { action.skipMin = true; playClick(); }
            else if (event.key.key == SDLK_F3) { action.speedDelta = -1; playClick(); }
            else if (event.key.key == SDLK_F4) { action.speedDelta = +1; playClick(); }
            else if (event.key.key == SDLK_F5) { action.saveState = true; playClick(); }
            else if (event.key.key == SDLK_F8) { action.loadState = true; playClick(); }
            else if (event.key.key == SDLK_SPACE || event.key.key == SDLK_P) { action.pauseToggle = true; playClick(); }
            else if (event.key.key == SDLK_R) { action.reset = true; playClick(); }
            else if (event.key.key == SDLK_ESCAPE) { action.quit = true; return false; }
            else if (event.key.key == SDLK_RETURN)  { if (os::keyEnabled) os::feedKey(10); }
            else if (event.key.key == SDLK_BACKSPACE){ if (os::keyEnabled) os::feedKey(8); }
            else if (event.key.key == SDLK_LEFT) { buttonA = true; playClick(); }
            else if (event.key.key == SDLK_DOWN) { buttonB = true; playClick(); }
            else if (event.key.key == SDLK_RIGHT) { buttonC = true; playClick(); }
        } else if (event.type == SDL_EVENT_KEY_UP) {
            if (event.key.key == SDLK_LEFT) buttonA = false;
            if (event.key.key == SDLK_DOWN) buttonB = false;
            if (event.key.key == SDLK_RIGHT) buttonC = false;
        } else if (event.type == SDL_EVENT_TEXT_INPUT) {
            if (os::keyEnabled) {
                for (const char* p = event.text.text; *p; p++) {
                    unsigned char ch = (unsigned char)*p;
                    if (ch < 128) os::feedKey(ch);   // ASCII-Zeichen
                }
            }
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            gHoverBtn = panelHit((int)event.motion.x, (int)event.motion.y);
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int i = panelHit((int)event.button.x, (int)event.button.y);
            if (i >= 0) {
                playClick();
                switch (ctl::PANEL[i].act) {
                    case ctl::SPD_DN: action.speedDelta = -1; break;
                    case ctl::SPD_UP: action.speedDelta = +1; break;
                    case ctl::PAUSE:  action.pauseToggle = true; break;
                    case ctl::SAVE:   action.saveState = true; break;
                    case ctl::LOAD:   action.loadState = true; break;
                    case ctl::SKIP:   action.skipMin = true; break;
                    case ctl::HATCH:  action.hatch = true; break;
                    case ctl::RESET:  action.reset = true; break;
                    default: break;
                }
            }
        }
    }
#elif defined(USE_SDL2)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) { action.quit = true; return false; }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.repeat) continue;
            if (event.key.keysym.sym == SDLK_F1) { action.hatch = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_F2) { action.skipMin = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_F3) { action.speedDelta = -1; playClick(); }
            else if (event.key.keysym.sym == SDLK_F4) { action.speedDelta = +1; playClick(); }
            else if (event.key.keysym.sym == SDLK_F5) { action.saveState = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_F8) { action.loadState = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_p) { action.pauseToggle = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_r) { action.reset = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_ESCAPE) { action.quit = true; return false; }
            else if (event.key.keysym.sym == SDLK_RETURN)  { if (os::keyEnabled) os::feedKey(10); }
            else if (event.key.keysym.sym == SDLK_BACKSPACE){ if (os::keyEnabled) os::feedKey(8); }
            else if (event.key.keysym.sym == SDLK_LEFT) { buttonA = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_DOWN) { buttonB = true; playClick(); }
            else if (event.key.keysym.sym == SDLK_RIGHT) { buttonC = true; playClick(); }
        } else if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_LEFT) buttonA = false;
            if (event.key.keysym.sym == SDLK_DOWN) buttonB = false;
            if (event.key.keysym.sym == SDLK_RIGHT) buttonC = false;
        } else if (event.type == SDL_MOUSEMOTION) {
            gHoverBtn = panelHit(event.motion.x, event.motion.y);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int i = panelHit(event.button.x, event.button.y);
            if (i >= 0) {
                playClick();
                switch (ctl::PANEL[i].act) {
                    case ctl::SPD_DN: action.speedDelta = -1; break;
                    case ctl::SPD_UP: action.speedDelta = +1; break;
                    case ctl::PAUSE:  action.pauseToggle = true; break;
                    case ctl::SAVE:   action.saveState = true; break;
                    case ctl::LOAD:   action.loadState = true; break;
                    case ctl::SKIP:   action.skipMin = true; break;
                    case ctl::HATCH:  action.hatch = true; break;
                    case ctl::RESET:  action.reset = true; break;
                    default: break;
                }
            }
        }
    }
#endif
    return true;
}

void TamagotchiDisplay::close() {
#if defined(USE_SDL3)
    if (audioStream) { SDL_DestroyAudioStream(audioStream); audioStream = nullptr; }
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window) { SDL_DestroyWindow(window); window = nullptr; }
    if (sdlAvailable) { SDL_Quit(); sdlAvailable = false; }
#elif defined(USE_SDL2)
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window) { SDL_DestroyWindow(window); window = nullptr; }
    if (sdlAvailable) { SDL_Quit(); sdlAvailable = false; }
#endif
    initialized = false;
}
