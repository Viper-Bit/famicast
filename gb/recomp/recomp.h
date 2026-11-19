/* gbrecomp: the Game Boy recompiler (recomp.c), on the instruction
 * templates (sm83.h). */
#ifndef GB_RECOMP_H
#define GB_RECOMP_H

#include <stdint.h>

#include "sm83.h"

typedef struct {
    uint16_t pc;
    uint8_t op, cb, size;  /* cb: op is the second byte of a CB instruction */
} Ins;

/* One ROM bank seen in one window: mem holds it at its addresses (the
 * rest of the 64 KB is zero and never decoded). */
typedef struct {
    int bank, window;
    uint8_t mem[0x10000];
    uint8_t entries[0x10000], instr[0x10000];
    uint8_t data[0x10000];  /* hints: never decoded as code */
} Region;

uint32_t fnv1a(const uint8_t *d, size_t n);  /* the hash of gb_rom_hash */
/* Decode from the entries, following static flow inside the window. */
void explore(Region *R);
/* regions[bank * 2 + window], NULL where nothing was found; embed: put the ROM in the program. */
void emit_program(Buf *o, Region **regions, int nbanks, const char *rom_name, const uint8_t *rom, size_t rom_size,
                  int embed);

#endif
