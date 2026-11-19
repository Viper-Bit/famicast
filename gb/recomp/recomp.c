/* The Game Boy recompiler: decoding, exploration and C emission (gbrecomp).
 *
 * Code is recompiled per region: one 16 KB ROM bank as seen in one window
 * ($0000-$3FFF or $4000-$7FFF). Exploration follows static control flow
 * inside the region only: the bank in the other window is not known at
 * build time, so flow into it is left to run time (the dispatcher looks the
 * block up by PC and the bank mapped there). Entries: the reset and
 * interrupt vectors in bank 0, and every (bank, address) an interpreter
 * trace ran (gb_headless --misses).
 *
 * One C function per basic block; each instruction is the interpreter's
 * own template (sm83.c) with the code bytes and PC as constants, so bus
 * traffic and timing are the interpreter's by construction. After each
 * instruction a block returns if gb_step would do something else next
 * (recomp_rt.h RECOMP_STOP); after a store that may switch banks it returns
 * if its own bank is no longer mapped. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recomp.h"

static int is_unused(int op) {
    return op == 0xD3 || op == 0xDB || op == 0xDD || op == 0xE3 || op == 0xE4 || op == 0xEB ||
           op == 0xEC || op == 0xED || op == 0xF4 || op == 0xFC || op == 0xFD;
}
static int is_jr_cc(int op) { return op == 0x20 || op == 0x28 || op == 0x30 || op == 0x38; }
static int is_jp_cc(int op) { return op == 0xC2 || op == 0xCA || op == 0xD2 || op == 0xDA; }
static int is_call_cc(int op) { return op == 0xC4 || op == 0xCC || op == 0xD4 || op == 0xDC; }
static int is_ret_cc(int op) { return op == 0xC0 || op == 0xC8 || op == 0xD0 || op == 0xD8; }
static int is_rst(int op) { return (op & 0xC7) == 0xC7; }

/* Does control go somewhere else than the next instruction (or nowhere)? */
static int is_transfer(const Ins *i) {
    int op = i->op;
    return !i->cb && (op == 0x18 || is_jr_cc(op) || op == 0xC3 || is_jp_cc(op) || op == 0xCD || is_call_cc(op) ||
                      op == 0xC9 || op == 0xD9 || is_ret_cc(op) || op == 0xE9 || is_rst(op) || is_unused(op));
}
/* Can the next instruction run after this one (CALL and RST: on return)? */
static int falls_through(const Ins *i) {
    int op = i->op;
    return i->cb || !(op == 0x18 || op == 0xC3 || op == 0xC9 || op == 0xD9 || op == 0xE9 || is_unused(op));
}
/* Last instruction of a block: transfers, and what gb_step must see next:
 * HALT and STOP (the halted CPU), EI (IME turns on before the next one). */
static int ends_block(const Ins *i) {
    return is_transfer(i) || (!i->cb && (i->op == 0x76 || i->op == 0x10 || i->op == 0xFB));
}
/* The static target of a jump, call or RST; -1 if none. */
static int target(const Ins *i, const uint8_t *mem) {
    int op = i->op;
    if (i->cb) return -1;
    if (op == 0x18 || is_jr_cc(op)) return (uint16_t)(i->pc + 2 + (int8_t)mem[(uint16_t)(i->pc + 1)]);
    if (op == 0xC3 || is_jp_cc(op) || op == 0xCD || is_call_cc(op))
        return mem[(uint16_t)(i->pc + 1)] | mem[(uint16_t)(i->pc + 2)] << 8;
    if (is_rst(op)) return op & 0x38;
    return -1;
}
/* Could it write to $0000-$7FFF (a bank switch)? Through a register pair,
 * or LD (nn),A / LD (nn),SP below $8000.
 * shortcut: stack writes (PUSH, CALL, RST) are taken never to reach ROM. */
static int may_switch(const Ins *i, const uint8_t *mem) {
    int op = i->op;
    if (i->cb) return (op & 7) == 6 && (op >> 6) != 1;  /* shifts, RES, SET on (HL); BIT only reads */
    if (op == 0xEA || op == 0x08) return (mem[(uint16_t)(i->pc + 2)] << 8 | mem[(uint16_t)(i->pc + 1)]) < 0x8000;
    return op == 0x02 || op == 0x12 || op == 0x22 || op == 0x32 || op == 0x34 || op == 0x35 || op == 0x36 ||
           (op >= 0x70 && op <= 0x77 && op != 0x76);
}

static int in_window(const Region *R, uint32_t a) { return a >> 14 == (uint32_t)R->window; }

/* The instruction at pc, if all its bytes are in the region's window. */
static int decode(const Region *R, uint16_t pc, Ins *out) {
    int op = R->mem[pc], cb = op == 0xCB;
    int size = cb ? 2 : sm83_size(op);
    if (!in_window(R, pc) || !in_window(R, (uint32_t)pc + size - 1)) return 0;
    for (int k = 0; k < size; k++)
        if (R->data[(uint16_t)(pc + k)]) return 0;
    out->pc = pc, out->size = (uint8_t)size, out->cb = (uint8_t)cb;
    out->op = (uint8_t)(cb ? R->mem[(uint16_t)(pc + 1)] : op);
    return 1;
}

void explore(Region *R) {
    static uint16_t work[0x10000];
    int n = 0;
    for (uint32_t a = 0; a < 0x10000; a++)
        if (R->entries[a]) work[n++] = (uint16_t)a;
    while (n) {
        uint16_t pc = work[--n];
        Ins i;
        if (R->instr[pc] || !decode(R, pc, &i)) continue;
        R->instr[pc] = 1;
        int t = target(&i, R->mem);
        if (t >= 0 && in_window(R, (uint32_t)t) && !R->instr[t]) work[n++] = (uint16_t)t;
        uint16_t next = (uint16_t)(pc + i.size);
        if (falls_through(&i) && in_window(R, next) && !R->instr[next]) work[n++] = next;
    }
}

/* Block starts: static targets, what follows a block's last instruction,
 * and entries that no other instruction falls into. */
static void block_starts(const Region *R, uint8_t *starts) {
    static uint8_t fallen_into[0x10000];
    memset(fallen_into, 0, sizeof fallen_into);
    memset(starts, 0, 0x10000);
    for (uint32_t a = 0; a < 0x10000; a++) {
        Ins i;
        if (!R->instr[a] || !decode(R, (uint16_t)a, &i)) continue;
        uint16_t next = (uint16_t)(a + i.size);
        int t = target(&i, R->mem);
        if (t >= 0 && R->instr[t]) starts[t] = 1;
        if (!falls_through(&i) || !R->instr[next]) continue;
        if (ends_block(&i)) starts[next] = 1;
        else fallen_into[next] = 1;
    }
    for (uint32_t a = 0; a < 0x10000; a++)
        if (R->instr[a] && R->entries[a] && !fallen_into[a]) starts[a] = 1;
}

/* Instructions of the block from `start`, in order. */
static int block_instrs(const Region *R, const uint8_t *starts, uint16_t start, Ins *out) {
    int n = 0;
    for (uint16_t pc = start;;) {
        decode(R, pc, &out[n]);
        const Ins *i = &out[n++];
        uint16_t next = (uint16_t)(pc + i->size);
        if (ends_block(i) || !R->instr[next] || starts[next] || !in_window(R, next)) return n;
        pc = next;
    }
}

static void block_name(char *out, size_t len, const Region *R, uint32_t a) {
    snprintf(out, len, "b_%03X_%04X", R->bank, a);
}

/* The interpreter's lines for the instruction, with constants. */
static void emit_ins(Buf *o, const Region *R, const Ins *i) {
    Lines l;
    char name[32];
    sm83_body(i->op, i->cb, &l, name, sizeof name);
    char raw[16] = "";
    for (int k = 0; k < i->size; k++)
        snprintf(raw + strlen(raw), sizeof raw - strlen(raw), k ? " %02X" : "%02X", R->mem[(uint16_t)(i->pc + k)]);
    bprintf(o, "i_%04X: /* %04X  %-8s  %s */\n", i->pc, i->pc, raw, name);
    bprintf(o, "    fetch_k(c, 0x%04X, 0x%02X);\n", i->pc, R->mem[i->pc]);
    if (i->cb) bprintf(o, "    fetch_k(c, 0x%04X, 0x%02X);\n", (uint16_t)(i->pc + 1), i->op);
    bprintf(o, "    {\n        c->pc = 0x%04X;\n", (uint16_t)(i->pc + i->size));
    uint32_t pc = (uint16_t)(i->pc + (i->cb ? 2 : 1));  /* the first operand byte */
    unsigned fetched = 0;
    int dynamic = 0;
    for (int k = 0; k < l.n; k++) {
        if (dynamic) { template_plain(o, "        ", l.s[k]); continue; }
        bprintf(o, "        ");
        template_expand(o, l.s[k], &pc, R->mem, 0xFFFF, i->pc, &fetched, &dynamic);
        bprintf(o, "\n");
    }
    bprintf(o, "    }\n");
    /* every operand byte fetched, nothing more: else the template and
     * sm83_size disagree, and the build stops rather than run wrong code */
    unsigned need = i->cb ? 0 : ((1u << i->size) - 1) & ~1u;
    if (fetched != need) {
        fprintf(stderr, "gbrecomp: $%04X %s: the template fetches offsets %X (bits), the instruction has %d bytes\n",
                i->pc, name, fetched, i->size);
        exit(2);
    }
}

static void emit_block(Buf *o, const Region *R, const uint8_t *starts, uint16_t start, Ins *ins, int n) {
    char name[24], next_name[24];
    block_name(name, sizeof name, R, start);
    int switches = 0;
    for (int k = 0; k < n; k++) switches |= may_switch(&ins[k], R->mem);
    bprintf(o, "static void %s(GbCpu *c) {\n    Gb *const g = c->bus;\n", name);
    if (!switches) bprintf(o, "    (void)g;\n");
    bprintf(o, "    switch (c->pc) {\n");
    for (int k = 0; k < n; k++) bprintf(o, "    case 0x%04X: goto i_%04X;\n", ins[k].pc, ins[k].pc);
    bprintf(o, "    default: return;\n    }\n");
    for (int k = 0; k < n; k++) {
        const Ins *i = &ins[k];
        emit_ins(o, R, i);
        if (may_switch(i, R->mem))
            bprintf(o, "    if (gb_cart_bank(&g->cart, 0x%04X) != %d) return;  /* bank switched */\n",
                    R->window * 0x4000, R->bank);
        if (k < n - 1) bprintf(o, "    if (RECOMP_STOP(g)) return;\n");
    }
    /* chaining: to a block of this region known from here */
    const Ins *end = &ins[n - 1];
    int next[2], nnext = 0, t = target(end, R->mem);
    uint16_t after = (uint16_t)(end->pc + end->size);
    int op = end->cb ? -1 : end->op;
    if (op == 0x76 || op == 0x10 || op == 0xFB || (op >= 0 && is_unused(op))) nnext = 0;  /* gb_step's turn */
    else if (t >= 0) {
        next[nnext++] = t;
        if (is_jr_cc(op) || is_jp_cc(op)) next[nnext++] = after;
    } else if (is_ret_cc(op) || !is_transfer(end)) next[nnext++] = after;
    int chained = 0;
    for (int k = 0; k < nnext; k++) {
        if (!in_window(R, (uint32_t)next[k]) || !R->instr[next[k]] || !starts[next[k]]) continue;
        if (k == 1 && next[1] == next[0]) continue;
        if (!chained++) bprintf(o, "    if (RECOMP_STOP(g)) return;\n    switch (c->pc) {\n");
        block_name(next_name, sizeof next_name, R, (uint32_t)next[k]);
        bprintf(o, "    case 0x%04X: RECOMP_CHAIN(%s);\n", next[k], next_name);
    }
    if (chained) bprintf(o, "    }\n");
    bprintf(o, "}\n\n");
}

uint32_t fnv1a(const uint8_t *d, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) h = (h ^ d[i]) * 16777619u;
    return h;
}

void emit_program(Buf *o, Region **regions, int nbanks, const char *rom_name, const uint8_t *rom, size_t rom_size,
                  int embed) {
    static uint8_t starts[0x10000];
    static Ins ins[0x4000];
    long nblocks = 0, ninstrs = 0;
    for (int r = 0; r < nbanks * 2; r++) {
        if (!regions[r]) continue;
        block_starts(regions[r], starts);
        for (uint32_t a = 0; a < 0x10000; a++) nblocks += starts[a], ninstrs += regions[r]->instr[a];
    }
    bprintf(o, "/* Generated by gbrecomp from %s. Do not edit.\n * %ld blocks, %ld instructions. */\n", rom_name,
            nblocks, ninstrs);
    bprintf(o, "#include \"cpu_ops.h\"\n#include \"gb.h\"\n#include \"recomp_rt.h\"\n\n");
    char name[24];
    for (int r = 0; r < nbanks * 2; r++) {  /* prototypes: blocks chain to each other */
        if (!regions[r]) continue;
        block_starts(regions[r], starts);
        for (uint32_t a = 0; a < 0x10000; a++)
            if (starts[a]) block_name(name, sizeof name, regions[r], a), bprintf(o, "static void %s(GbCpu *c);\n", name);
    }
    bprintf(o, "\n");
    for (int r = 0; r < nbanks * 2; r++) {
        Region *R = regions[r];
        if (!R) continue;
        block_starts(R, starts);
        static uint16_t table[0x4000];  /* window offset -> block start + 1 */
        memset(table, 0, sizeof table);
        for (uint32_t s = 0; s < 0x10000; s++) {
            if (!starts[s]) continue;
            int n = block_instrs(R, starts, (uint16_t)s, ins);
            emit_block(o, R, starts, (uint16_t)s, ins, n);
            for (int k = 0; k < n; k++)
                if (!table[ins[k].pc & 0x3FFF]) table[ins[k].pc & 0x3FFF] = (uint16_t)(s + 1);
        }
        for (uint32_t a = 0; a < 0x10000; a++)
            if (R->instr[a] && !table[a & 0x3FFF]) {  /* every instruction must be in a block */
                fprintf(stderr, "gbrecomp: bank %d $%04X is in no block\n", R->bank, a);
                exit(1);
            }
        bprintf(o, "static const GbRecompFn r_%03X_%d[0x4000] = {\n", R->bank, R->window);
        for (int off = 0; off < 0x4000; off++)
            if (table[off]) block_name(name, sizeof name, R, table[off] - 1u), bprintf(o, "    [0x%04X] = %s,\n", off, name);
        bprintf(o, "};\n\n");
    }
    bprintf(o, "static const GbRecompFn *const regions[%d][2] = {\n", nbanks);
    for (int r = 0; r < nbanks * 2; r++)
        if (regions[r]) bprintf(o, "    [%d][%d] = r_%03X_%d,\n", r / 2, r % 2, r / 2, r % 2);
    bprintf(o, "};\n\n");
    bprintf(o, "static GbRecompFn lookup(uint16_t pc, uint32_t bank) {\n");
    bprintf(o, "    if (pc >= 0x8000 || bank >= %d) return 0;\n", nbanks);
    bprintf(o, "    const GbRecompFn *r = regions[bank][pc >> 14];\n");
    bprintf(o, "    return r ? r[pc & 0x3FFF] : 0;\n}\n\n");
    if (embed) {
        bprintf(o, "static const uint8_t rom_image[%zu] = {", rom_size);
        for (size_t i = 0; i < rom_size; i++) bprintf(o, i % 16 ? "0x%02X," : "\n    0x%02X,", rom[i]);
        bprintf(o, "\n};\n\n");
    }
    bprintf(o, "static const GbRecompProgram program = {\n    .rom_hash = 0x%08Xu,\n    .rom_name = \"", fnv1a(rom, rom_size));
    for (const char *s = rom_name; *s; s++) bprintf(o, (*s == '"' || *s == '\\') ? "\\%c" : "%c", *s);
    bprintf(o, "\",\n    .lookup = lookup,\n");
    if (embed) bprintf(o, "    .rom = rom_image,\n    .rom_size = sizeof rom_image,\n");
    bprintf(o, "};\nconst GbRecompProgram *const gb_recomp_program = &program;\n");
}
