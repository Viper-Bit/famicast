/* Colours for a Game Boy game on a Game Boy Color (CGB compatibility mode):
 * the palette the Color's boot ROM picks from the cartridge header, by the
 * title's checksum (and the 4th letter of the title for the checksums that
 * several games share).
 * The tables and the lookup are from SameBoy's BootROMs/cgb_boot.asm
 * (https://github.com/LIJI32/SameBoy, Expat licence, (c) 2015-2026 Lior
 * Halphon; see docs/sources.md). Not taken: the key-combination palettes,
 * SameBoy's own extra palettes, the DMG logo tilemap some games need.
 *
 * Expat License, Copyright (c) 2015-2026 Lior Halphon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <string.h>

#include "gb.h"

enum { N_CHECKSUMS = 94, FIRST_DUP = 65 };
static const uint8_t CHECKSUM[N_CHECKSUMS] = {
    0x00, 0x88, 0x16, 0x36, 0xD1, 0xDB, 0xF2, 0x3C, 0x8C, 0x92, 0x3D, 0x5C,
    0x58, 0xC9, 0x3E, 0x70, 0x1D, 0x59, 0x69, 0x19, 0x35, 0xA8, 0x14, 0xAA,
    0x75, 0x95, 0x99, 0x34, 0x6F, 0x15, 0xFF, 0x97, 0x4B, 0x90, 0x17, 0x10,
    0x39, 0xF7, 0xF6, 0xA2, 0x49, 0x4E, 0x43, 0x68, 0xE0, 0x8B, 0xF0, 0xCE,
    0x0C, 0x29, 0xE8, 0xB7, 0x86, 0x9A, 0x52, 0x01, 0x9D, 0x71, 0x9C, 0xBD,
    0x5D, 0x6D, 0x67, 0x3F, 0x6B, 0xB3, 0x46, 0x28, 0xA5, 0xC6, 0xD3, 0x27,
    0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4, 0xB3, 0x46, 0x28, 0xA5, 0xC6,
    0xD3, 0x27, 0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4, 0xB3,
};
/* the combination each checksum picks */
static const uint8_t COMBO[N_CHECKSUMS] = {
    0, 4, 5, 35, 34, 3, 31, 15, 10, 5, 19, 36, 7, 37, 30, 44,
    21, 32, 31, 20, 5, 33, 13, 14, 5, 29, 5, 18, 9, 3, 2, 26,
    25, 25, 41, 42, 26, 45, 42, 45, 36, 38, 26, 42, 30, 41, 34, 34,
    5, 42, 6, 5, 33, 25, 42, 42, 40, 2, 16, 25, 42, 42, 5, 0,
    39, 36, 22, 25, 6, 32, 12, 36, 11, 39, 18, 39, 24, 31, 50, 17,
    46, 6, 27, 0, 47, 41, 41, 0, 0, 19, 34, 23, 18, 29,
};
/* from FIRST_DUP: the 4th letter of the title that tells the games apart */
static const char LETTER4[] = "BEFAARBEKEK R-URAR INAILICE R";
/* combinations: where in PALETTE the 4 colours of object palette 0, 1 and the background start (in colours) */
static const uint8_t COMBINATION[51][3] = {
    {16, 16, 116}, {72, 72, 72}, {80, 80, 80}, {96, 96, 96},
    {36, 36, 36}, {0, 0, 0}, {108, 108, 108}, {20, 20, 20},
    {48, 48, 48}, {104, 104, 104}, {64, 32, 32}, {16, 112, 112},
    {16, 8, 8}, {12, 16, 16}, {16, 116, 116}, {112, 16, 112},
    {8, 68, 8}, {64, 64, 32}, {16, 16, 28}, {16, 16, 72},
    {16, 16, 80}, {76, 76, 36}, {15, 15, 44}, {68, 68, 8},
    {16, 16, 8}, {16, 16, 12}, {112, 112, 0}, {12, 12, 0},
    {0, 0, 4}, {72, 88, 72}, {80, 88, 80}, {96, 88, 96},
    {64, 88, 32}, {68, 16, 52}, {111, 0, 56}, {111, 16, 60},
    {76, 91, 36}, {64, 112, 40}, {16, 92, 112}, {68, 88, 8},
    {16, 0, 8}, {16, 112, 12}, {112, 12, 0}, {12, 112, 16},
    {84, 112, 16}, {12, 112, 0}, {100, 12, 112}, {0, 112, 32},
    {16, 12, 112}, {112, 12, 24}, {16, 112, 116},
};
static const uint16_t PALETTE[30 * 4] = {
    0x7FFF, 0x32BF, 0x00D0, 0x0000,
    0x639F, 0x4279, 0x15B0, 0x04CB,
    0x7FFF, 0x6E31, 0x454A, 0x0000,
    0x7FFF, 0x1BEF, 0x0200, 0x0000,
    0x7FFF, 0x421F, 0x1CF2, 0x0000,
    0x7FFF, 0x5294, 0x294A, 0x0000,
    0x7FFF, 0x03FF, 0x012F, 0x0000,
    0x7FFF, 0x03EF, 0x01D6, 0x0000,
    0x7FFF, 0x42B5, 0x3DC8, 0x0000,
    0x7E74, 0x03FF, 0x0180, 0x0000,
    0x67FF, 0x77AC, 0x1A13, 0x2D6B,
    0x7ED6, 0x4BFF, 0x2175, 0x0000,
    0x53FF, 0x4A5F, 0x7E52, 0x0000,
    0x4FFF, 0x7ED2, 0x3A4C, 0x1CE0,
    0x03ED, 0x7FFF, 0x255F, 0x0000,
    0x036A, 0x021F, 0x03FF, 0x7FFF,
    0x7FFF, 0x01DF, 0x0112, 0x0000,
    0x231F, 0x035F, 0x00F2, 0x0009,
    0x7FFF, 0x03EA, 0x011F, 0x0000,
    0x299F, 0x001A, 0x000C, 0x0000,
    0x7FFF, 0x027F, 0x001F, 0x0000,
    0x7FFF, 0x03E0, 0x0206, 0x0120,
    0x7FFF, 0x7EEB, 0x001F, 0x7C00,
    0x7FFF, 0x3FFF, 0x7E00, 0x001F,
    0x7FFF, 0x03FF, 0x001F, 0x0000,
    0x03FF, 0x001F, 0x000C, 0x0000,
    0x7FFF, 0x033F, 0x0193, 0x0000,
    0x0000, 0x4200, 0x037F, 0x7FFF,
    0x7FFF, 0x7E8C, 0x7C00, 0x0000,
    0x7FFF, 0x1BEF, 0x6180, 0x0000,
};

/* Load the palettes into palette RAM; returns the title checksum the boot ROM leaves in B (0: not a Nintendo licensee). */
uint8_t gb_compat_palettes(Gb *g) {
    uint8_t sum = 0;
    const uint8_t *h = g->cart.rom + 0x134;  /* title $134-$143 */
    int combo = 0;
    if ((g->cart.rom[0x14B] == 0x33 && g->cart.rom[0x144] == '0' && g->cart.rom[0x145] == '1') || g->cart.rom[0x14B] == 0x01) {
        for (int i = 0; i < 16; i++) sum = (uint8_t)(sum + h[i]);
        for (int i = 0; i < N_CHECKSUMS; i++)
            if (CHECKSUM[i] == sum && (i < FIRST_DUP || LETTER4[i - FIRST_DUP] == h[3])) {
                combo = COMBO[i] & 0x7F;  /* bit 7: needs the DMG logo tilemap */
                break;
            }
    }
    for (int k = 0; k < 3; k++) {
        uint8_t *dst = k == 2 ? g->bg_pal : k == 0 ? g->obj_pal : g->obj_pal + 8;
        for (int i = 0; i < 4; i++) {
            uint16_t c = PALETTE[COMBINATION[combo][k] + i];
            dst[i * 2] = (uint8_t)c, dst[i * 2 + 1] = (uint8_t)(c >> 8);
        }
    }
    return sum;
}
