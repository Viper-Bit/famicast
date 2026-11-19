/* The PPU, DMG and CGB mode (Pan Docs "Rendering", "LCDC", "STAT", "Tile
 * Data", "Tile Maps", "OAM", "Palettes", "BG Map Attributes", "LCD Color
 * Palettes"): 456 dots a line, 154 lines; mode 2 (OAM scan) dots
 * 0-79, mode 3 (drawing) 172 dots and more for SCX, the window and
 * objects, then mode 0 (hblank); lines 144-153 mode 1 (vblank).
 * shortcut: a whole line is drawn when mode 3 starts, from the registers
 * at that moment; writes during mode 3 (mid-line effects) take effect on
 * the next line. A pixel FIFO when a game needs it. */
#include <string.h>

#include "gb.h"

/* The game uses the Color's features (not a DMG game on a Color: compatibility mode) */
static int cgb_game(const Gb *g) { return g->cgb && !g->compat; }

int ppu_mode(const Gb *g) { return g->lcdc & 0x80 ? g->mode : 0; }

/* The STAT interrupt line is the OR of its enabled sources; it fires on a rising edge. */
static int stat_line(const Gb *g, uint8_t stat) {
    int mode = ppu_mode(g);
    return (g->lcdc & 0x80) && (((stat & 0x40) && g->ly == g->lyc) || ((stat & 0x20) && mode == 2) ||
                                ((stat & 0x10) && mode == 1) || ((stat & 0x08) && mode == 0));
}
void ppu_stat_update(Gb *g) {
    uint8_t line = (uint8_t)stat_line(g, g->stat);
    if (line && !g->stat_line) g->ifl |= INT_STAT;
    g->stat_line = line;
}

/* DMG quirk: a write to STAT enables every source for a moment, so in
 * hblank, vblank or on LY = LYC it raises the interrupt (games rely on it,
 * Pan Docs "STAT"). Not on the CGB (a CGB game's STAT handler that writes
 * STAT would re-enter itself until the stack overran). */
void ppu_stat_write(Gb *g, uint8_t v) {
    if (!g->cgb && stat_line(g, 0x78) && !g->stat_line) g->ifl |= INT_STAT;
    g->stat = v & 0x78;
    ppu_stat_update(g);
}

void ppu_lcdc_write(Gb *g, uint8_t v) {
    if ((g->lcdc & 0x80) && !(v & 0x80)) {  /* LCD off: line 0, a blank screen */
        g->ly = 0, g->dot = 0, g->mode = 0;
        for (int i = 0; i < 160 * 144; i++) g->frame[i] = g->cgb ? 0x7FFF : 0;  /* white */
        g->frame_count++;
    }
    if (!(g->lcdc & 0x80) && (v & 0x80)) g->ly = 0, g->dot = 0, g->mode = 2, g->wy_hit = 0, g->win_line = 0;
    g->lcdc = v;
    ppu_stat_update(g);
}

/* CGB: colour ci of palette p, RGB555 */
static uint16_t cgb_colour(const uint8_t *pal, int p, int ci) {
    return (uint16_t)(pal[p * 8 + ci * 2] | (pal[p * 8 + ci * 2 + 1] & 0x7F) << 8);
}

/* The 2-bit colour of pixel x of the tile row at `row` (VRAM offset, bank 1 from $2000). */
static int tile_pixel(const Gb *g, unsigned row, int x) {
    return (g->vram[row] >> (7 - x) & 1) | (g->vram[row + 1] >> (7 - x) & 1) << 1;
}
static unsigned tile_row(const Gb *g, uint8_t tile, int y) {  /* BG/window: LCDC bit 4 picks the addressing */
    return g->lcdc & 0x10 ? tile * 16u + y * 2u : (unsigned)(0x1000 + (int8_t)tile * 16 + y * 2);
}

/* Up to 10 objects on this line, in OAM order (Pan Docs "OAM": the scan). */
static int scan_objects(const Gb *g, uint8_t out[10]) {
    int h = g->lcdc & 4 ? 16 : 8, n = 0;
    for (int i = 0; i < 40 && n < 10; i++) {
        int y = g->oam[i * 4] - 16;
        if (g->ly >= y && g->ly < y + h) out[n++] = (uint8_t)i;
    }
    return n;
}

static void draw_line(Gb *g, uint8_t objs[10], int nobj, int window) {
    uint8_t bg[160], bg_attr[160];
    uint16_t *out = g->fb + g->ly * 160;
    const uint8_t bgp = g->io[0x47];
    int wx = g->wx - 7;
    for (int x = 0; x < 160; x++) {
        int ci = 0, attr = 0;
        if (cgb_game(g) || (g->lcdc & 1)) {  /* DMG and compatibility mode: bit 0 off blanks background and window */
            unsigned map, tx, ty;
            if (window && x >= wx) map = g->lcdc & 0x40 ? 0x1C00 : 0x1800, tx = (unsigned)(x - wx), ty = g->win_line;
            else map = g->lcdc & 0x08 ? 0x1C00 : 0x1800, tx = (g->scx + x) & 255u, ty = (g->scy + g->ly) & 255u;
            unsigned at = map + (ty / 8) * 32 + tx / 8;
            uint8_t tile = g->vram[at];
            /* CGB: bank 1 holds each map entry's attributes: palette, tile bank, flips, priority */
            attr = cgb_game(g) ? g->vram[0x2000 + at] : 0;
            int y = (int)(ty & 7), px = (int)(tx & 7);
            if (attr & 0x40) y = 7 - y;
            if (attr & 0x20) px = 7 - px;
            ci = tile_pixel(g, tile_row(g, tile, y) + (attr & 8 ? 0x2000u : 0), px);
        }
        bg[x] = (uint8_t)ci, bg_attr[x] = (uint8_t)attr;
        if (!g->cgb) out[x] = (uint16_t)(bgp >> (ci * 2) & 3);
        else out[x] = g->compat ? cgb_colour(g->bg_pal, 0, bgp >> (ci * 2) & 3) : cgb_colour(g->bg_pal, attr & 7, ci);
    }
    if (!(g->lcdc & 2)) return;
    /* objects: DMG: the lowest X wins, then the lowest OAM index; CGB: the
     * lowest OAM index. The winner's priority bit decides against
     * background colours 1-3 (CGB: so does the map's; LCDC bit 0 off: the
     * objects are always in front) */
    int h = g->lcdc & 4 ? 16 : 8;
    int8_t win[160];
    memset(win, -1, sizeof win);
    uint8_t col[160];
    /* order by priority: stable sort on X, OAM order kept; drawn worst
     * first, so better ones overwrite */
    uint8_t order[10];
    memcpy(order, objs, (size_t)nobj);
    for (int i = 1; i < nobj && !(cgb_game(g) && !g->opri); i++)
        for (int j = i; j > 0 && g->oam[order[j] * 4 + 1] < g->oam[order[j - 1] * 4 + 1]; j--) {
            uint8_t t = order[j];
            order[j] = order[j - 1], order[j - 1] = t;
        }
    for (int k = nobj - 1; k >= 0; k--) {
        const uint8_t *o = g->oam + order[k] * 4;
        int y = g->ly - (o[0] - 16), sx = o[1] - 8;
        if (o[3] & 0x40) y = h - 1 - y;  /* Y flip */
        uint8_t tile = h == 16 ? (uint8_t)((o[2] & 0xFE) | (y >= 8)) : o[2];
        unsigned row = tile * 16u + (unsigned)(y & 7) * 2u + (cgb_game(g) && (o[3] & 8) ? 0x2000u : 0);
        for (int p = 0; p < 8; p++) {
            int x = sx + p;
            if (x < 0 || x >= 160) continue;
            int ci = tile_pixel(g, row, o[3] & 0x20 ? 7 - p : p);
            if (ci) win[x] = (int8_t)k, col[x] = (uint8_t)ci;
        }
    }
    for (int x = 0; x < 160; x++) {
        if (win[x] < 0) continue;
        const uint8_t *o = g->oam + order[(int)win[x]] * 4;
        if (g->cgb) {
            if ((g->lcdc & 1) && bg[x] && ((o[3] & 0x80) || (bg_attr[x] & 0x80))) continue;
            out[x] = g->compat ? cgb_colour(g->obj_pal, o[3] >> 4 & 1, g->io[o[3] & 0x10 ? 0x49 : 0x48] >> (col[x] * 2) & 3)
                               : cgb_colour(g->obj_pal, o[3] & 7, col[x]);
            continue;
        }
        if ((o[3] & 0x80) && bg[x]) continue;  /* behind background colours 1-3 */
        uint8_t pal = g->io[o[3] & 0x10 ? 0x49 : 0x48];
        out[x] = (uint8_t)(pal >> (col[x] * 2) & 3);
    }
}

/* Mode 3 starts: draw the line; its length (Pan Docs "Mode 3 length"):
 * 172 dots, + SCX mod 8, + 6 when the window starts, + 6-11 per object. */
static void mode3_start(Gb *g) {
    uint8_t objs[10];
    int nobj = g->lcdc & 2 ? scan_objects(g, objs) : 0;
    if (g->ly == g->wy) g->wy_hit = 1;
    int window = (g->lcdc & 0x20) && (cgb_game(g) || (g->lcdc & 1)) && g->wy_hit && g->wx <= 166;
    draw_line(g, objs, nobj, window);
    int len = 172 + (g->scx & 7) + (window ? 6 : 0);
    for (int i = 0; i < nobj; i++) {
        int x = g->oam[objs[i] * 4 + 1];
        int skew = (x + g->scx) & 7;
        len += 6 + (skew < 5 ? 5 - skew : 0);
    }
    g->mode3_end = 80 + len;
    if (window) g->win_line++;
}

void ppu_tick(Gb *g) {
    if (!(g->lcdc & 0x80)) return;
    g->dot += g->double_speed ? 2 : 4;  /* dots are real time: a double-speed cycle is 2 */
    if (g->dot >= 456) {
        g->dot -= 456;
        if (++g->ly == 154) g->ly = 0, g->wy_hit = 0, g->win_line = 0;
        if (g->ly == 144) {
            g->mode = 1;
            g->ifl |= INT_VBLANK;
            memcpy(g->frame, g->fb, sizeof g->frame);
            g->frame_count++;
        } else if (g->ly < 144) {
            g->mode = 2;
        }
    } else if (g->ly < 144) {
        if (g->mode == 2 && g->dot >= 80) g->mode = 3, mode3_start(g);
        else if (g->mode == 3 && g->dot >= g->mode3_end) g->mode = 0, g->hdma_due = g->hdma_on;
    }
    ppu_stat_update(g);
}
