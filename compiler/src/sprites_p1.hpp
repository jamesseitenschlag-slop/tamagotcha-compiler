// sprites_p1.hpp
// created: Jan 29, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// sprites_p1.hpp - 1:1 Tamagotchi P1 ROM extracted 16x16 bitmaps
#pragma once
#include <string>
#include <map>
#include <vector>

struct Sprite16 {
    unsigned char hi[16];
    unsigned char lo[16];
};

extern const int COL_TO_SEG[32];
extern const std::map<std::string, Sprite16> SPRITES_P1;
