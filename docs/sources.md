# Sources and credits

Every external source we use goes here: what it is, what we used it for,
and its license if we take code or data from it. If code is copied or adapted,
the file header must also say where it came from.

Status: **read** = we used it directly; **listed** = found, not read in
detail yet.

## Hardware documentation
| Source | Used for | Status |
|--------|----------|--------|
| [nesdev wiki](https://www.nesdev.org/wiki/Nesdev_Wiki) (CPU, PPU, APU, mapper pages) | Main hardware reference for everything in 03/04 | read (in part) |
| [nesdev wiki · Emulator tests](https://www.nesdev.org/wiki/Emulator_tests) | Test ROM list; nestest automation mode (start at $C000, compare to log made with Nintendulator) | read |
| nesdev wiki: PPU registers, PPU scrolling (loopy v/t/x/w), PPU rendering, PPU sprite evaluation, PPU frame timing, Open bus behavior, CPU interrupts, CPU memory map, NTSC video | PPU, bus and interrupt design in `nes/runtime/`; behaviour then checked by the test ROMs | from memory, not re-read this session |
| Bisqwit's NTSC palette model (as described on nesdev "NTSC video") | `nes/runtime/palette.c` signal levels and method | from memory |
| Mesen2's delayed rendering-enable flag | idea for the odd-frame skip timing (confirmed by blargg 10-even_odd_timing) | from memory |
| mmc3_test_2 (readme read), MMC1_A12 from the same mirror (freely distributed test ROMs) | MMC3 IRQ and A12 checks (M7) | read |
| nesdev wiki: MMC1, UxROM, CNROM, MMC3 (IRQ counter, A12 filter, "cycle 260"), AxROM, Color Dreams, GxROM, PPU sprite fetches | `nes/runtime/cart.c`, sprite fetch timing | from memory |
| nesdev wiki: MMC5 (PRG/CHR modes, $5105 nametable map, ExRAM modes, scanline IRQ and in-frame flag, multiplier), Namco 163 (registers, 15-bit IRQ, $4800/$F800 RAM port) | `nes/runtime/cart.c`, `nes/runtime/ppu.c` hooks (M7e) | from memory; CHR set A/B use with 8x8 sprites as Mesen2 does it (last set written), from memory |
| nesdev wiki: VRC2 and VRC4 (register map, board wirings per submapper, VRC2a CHR shift), VRC IRQ (prescaler 341 - 3 per cycle, cycle mode, ack) | `nes/runtime/cart.c` (M7d) | from memory; checked against the games in docs/06 |
| nesdev wiki: MMC2, MMC4 (latch trigger addresses, latch set after the fetch), INES Mapper 018 (Jaleco SS88006: nibble registers, 4/8/12/16-bit IRQ counter), Sunsoft FME-7 (commands 0-F, ROM/RAM at $6000, IRQ on wrap from $0000), INES Mapper 210 (Namco 175/340, battery = 175 without a submapper) | `nes/runtime/cart.c` (M7f) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: Colour emphasis, NTSC video ("Color Tint Bits": attenuation 0.816328 of the absolute voltage while the emphasized phase is high, not in columns $xE/$xF), PPU registers (PPUMASK bits 5-7), Colour-emphasis games | `nes/runtime/palette.c`, `nes/runtime/ppu.c` | read (public domain per the wiki's disclaimer) |
| nesdev wiki: Cycle reference chart (NTSC/PAL/Dendy clocks, frame and vblank lengths, emphasis order, OAM write window), APU Frame Counter, APU Noise, APU DMC (PAL tables, Dendy on NTSC's), PPU frame timing; NES 2.0 (byte 12 region); Overscan (PAL pixel aspect 1.3862, from memory) | `nes/runtime/nes.c`, `ppu.c`, `apu.c`, frontend (M10) | read this session (Dendy page: 404); not checked against a PAL console |
| nesdev wiki: VRC7 (85: A3/A4 submappers, registers, $E000 bit 6), VRC7 audio (ports, register map, the 15-patch dump, multiplier and feedback tables, 49716 * fnum * 2^block / 2^19); the YM2413/OPL operator structure as emu2413 (Okazaki) models it (log-sin/exp tables, envelope rate counters and attack curve, KSL table, LFO depths and rates), from memory: not on the wiki | `nes/runtime/cart.c`, `nes/runtime/apu.c` (M9e) | wiki read; operator internals from memory of the OPL/OPLL literature, checked by unit ROMs and Lagrange Point's tuning, not against hardware |
| nesdev wiki: Family Computer Disk System (RAM adapter registers $4020-$4033, memory map), FDS disk format (blocks 1-4, gaps, start mark, CRC), FDS audio (wavetable, envelopes, modulator, master volume, $408A = $E8 from the BIOS, "roughly 2.4x the maximum volume of the APU square", same polarity, ~2 kHz one-pole low-pass) | `nes/runtime/fds.c`, FDS sound in `nes/runtime/apu.c` | read (FDS audio page, Nov 2026 revision) |
| nesdev wiki: VRC6 (24/26 A0/A1 swap, $F003 decoding, PRG/CHR registers, $B003 modes; the games only use mode 0), VRC6 audio (16-step duty D + 1, mode bit, 12-bit dividers, $9003 shift/halt, saw accumulator every second clock and cleared on the 14th, linear 6-bit DAC, pulses as loud as the APU's), VRC IRQ | `nes/runtime/cart.c`, `nes/runtime/apu.c` (M9d) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: Sunsoft 5B audio (registers, YM2149F with SEL low: Clock / (32 * period) tone and noise, Clock / (16 * period) envelope steps, 17-bit LFSR taps 16 and 13, 1.5 dB per 5-bit level, linear sum, $C000 high nibble disables writes); nesdev forum "VRC7 and 5B amplifier investigation" (t=17745: a 5B channel ~8 dB over an APU square, signals from 0 to negative, compression near full volume) | `nes/runtime/apu.c` (M9c) | read; the forum figures are estimates |
| nesdev wiki: MMC5 audio (pulses without sweep or low-period muting, envelope/length at M2 / 7424, $5010/$5011 PCM with the $00 IRQ, $5015, inverted polarity, volume as the APU's) | `nes/runtime/apu.c` (M9b) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: Namco 163 audio (RAM layout, 15 cycles per channel, phase/output model, low nibble first, averaging; dB over the APU per game), INES Mapper 019 ($E000 bit 6 halt; submappers 2-5 as the mixing resistor) | `nes/runtime/cart.c`, `nes/runtime/apu.c` (M9a) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: INES Mapper 033 (TC0190: $A003 mask, $8000 bit 6 mirroring, 2 KB CHR numbered in 2 KB units), INES Mapper 048 (TC0690: $E000 mirroring, MMC3 IRQ with the latch XOR $FF and about 4 CPU cycles later), Taito X1-005 (80: $7EF0-$7EFF with A7 ignored, $A3 RAM enable, 128 bytes mirrored at $7F80), INES Mapper 207 (CHR A17 to CIRAM A10) | `nes/runtime/cart.c` (M7k) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: INES Mapper 032 (Irem G-101: PRG mode bit, mirroring, $B000-$B007 CHR), NES 2.0 submappers (032: 1, Major League: $9000 not connected, CIRAM A10 tied high) | `nes/runtime/cart.c` (M7j) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: INES Mapper 013, 034 (BNROM/NINA-001 by submapper), 037, 041, 047, 064 (RAMBO-1: R8/R9/RF, $C001 cycle mode), 065 (Irem H3001), 067 (Sunsoft-3), 068 (Sunsoft-4), 070, 072, 073 (VRC3), 076, 077, 078 (submappers 1/3), 082 (Taito X1-017), 086, 088, 089, 092, 093, 094, 095, 097, 113, 119 (TQROM), 140, 152, 154, 155, 158, 180, 184, 188, 228, 232 | `nes/runtime/cart.c` (M7l, "more boards") | from memory; RAMBO-1's reload quirk (latch + 1 when 2 or more) and Sunsoft-4's sub-cartridge bank as Mesen2 does them, from memory; checked by unit ROMs and the dumps in docs/06 |
| nesdev wiki: VRC1 (75: PRG selects at $8000/$A000/$C000, $9000 mirroring and CHR bit 4, CHR low bits at $E000/$F000, mirroring ignored on four-screen boards) | `nes/runtime/cart.c` (M7i) | read (public domain per the wiki's disclaimer) |
| nesdev wiki: TxSROM / INES Mapper 118 (CHR bank bit 7 drives CIRAM A10: R0/R1 for the nametable pairs, R2-R5 with CHR A12 inversion; mirroring register unconnected) | `nes/runtime/cart.c` (M7h) | from memory; checked by a unit ROM and the games in docs/06 |
| nesdev wiki: Bandai FCG board, INES Mapper 016 (submappers 4/5: FCG-1/2 at $6000 with a direct counter, LZ93D50 at $8000 with a latch), 153, 157, 159 ($800D EEPROM lines, $6000 bit 4 SDA, X24C01 MSB-first) | `nes/runtime/cart.c` (M7g) | read (public domain per the wiki's disclaimer); the I2C protocol (start/stop, acknowledge, 24C02 device byte, page writes of 8 and 4 bytes) from the 24C02/X24C01 datasheets and the IRQ at the clock that finds 0 as Mesen2 does it, from memory |
| nesdev wiki: NES 2.0 (mapper bits 8-11, submapper), INES Mapper 071, NINA-003-006 (79), INES Mapper 087, INES Mapper 185 (submappers 4-7), Namco 108 (206) | `nes/runtime/cart.c` (M7c) | from memory; mapper 185 without a submapper uses the usual emulator guess (CHR on if latch & $0F and latch != $13), from memory |
| Mesen2 APU reset behaviour (keep $4017 mode, clear IRQ inhibit; pulse/noise length counters zeroed and unhalted, triangle unaffected) | `apu_reset()` | from memory || nesdev wiki "DMA" (halt, dummy and alignment cycles, get/put, DMC during OAM DMA) and Mesen2's DMA loop | `run_dma` in `nes/runtime/nes.c` | from memory |
| Test sources from the pinned nes-test-roms mirror: cpu_interrupts_v2/source/4-irq_and_dma.s, apu_reset/source/*.s, dmc_dma_during_read4/source/*.s | expected outputs and what each test measures | read |
| [nesdev forum, "Help with dmc_dma [SOLVED]"](https://nesdev.nes.science/f3/t7547.xhtml) | sprdma_and_dmc_dma_512 rows $0A/$0B = 526/527 | read |
| nesdev wiki "INES" (DiskDude! junk in bytes 7-15) | header check in `nes/runtime/cart.c`, used by `nesrecomp` too | from memory |
| Inigo Quilez, "2D distance functions" (rounded box SDF) | `core/frontend/tv_frame.c` shapes | from memory |
| George Marsaglia, "Xorshift RNGs" (J. Stat. Software, 2003), the 13/17/5 xorshift32 | `nes_headless --play` input generator | from memory |
| nesdev wiki: APU, APU Frame Counter, Length Counter, Envelope, Sweep, Triangle, Noise, DMC, Mixer (output filters), CPU interrupts (branch polling, hijacking), DMA | `nes/runtime/apu.c`, interrupt details | from memory |

## Test suites (downloaded by `tests/fetch_test_roms.py`, not committed)
| Source | Used for | License | Status |
|--------|----------|---------|--------|
| nestest.nes + nestest.log (Kevin Horton), from [qmtpro.com](https://www.qmtpro.com/~nes/misc/) (original) and the [christopherpow/nes-test-roms](https://github.com/christopherpow/nes-test-roms) mirror | Decoder test (M1); CPU trace comparison (M2/M5) | freely distributed test ROM | read |
| blargg and other test ROMs via [christopherpow/nes-test-roms](https://github.com/christopherpow/nes-test-roms), pinned at commit `95d8f621`: ppu_vbl_nmi, vbl_nmi_timing, sprite_hit_tests_2005, sprite_overflow_tests, blargg_ppu_tests_2005, ppu_open_bus, oam_read, oam_stress, instr_test-v5 (readmes and the 2005 test sources read for result protocols) | PPU/system checks (M3) | freely distributed test ROMs; check per archive before redistributing | read |
| More test ROMs from the same mirror: apu_test, blargg_apu_2005.07.30 (readme with frame-counter timing tables and sources read), apu_reset (readme read), cpu_interrupts_v2 (readme read), cpu_dummy_writes, sprdma_and_dmc_dma, dmc_dma_during_read4 | APU, interrupts, DMA (M6) | freely distributed test ROMs | read |
| [SingleStepTests/sm83 `v1`](https://github.com/SingleStepTests/sm83), pinned at commit `f9c30210` | 1,000 tests per Game Boy opcode with the bus state of every M-cycle; SM83 interpreter check (G1) | MIT | read |
| [c-sp/game-boy-test-roms](https://github.com/c-sp/game-boy-test-roms) release v7.0 (one zip, SHA-256 pinned): blargg's GB tests, mooneye-test-suite (acceptance, emulator-only), and more for later milestones | Game Boy system checks (G2); test sources read for `oam_dma_start`, `halt_ime0_nointr_timing`, `rapid_toggle` | MIT (collection); mooneye MIT; blargg's ROMs freely distributed | read |
| Pan Docs "Memory Map", "Timer and Divider Registers", "Timer obscure behaviour", "Interrupts", "HALT", "OAM DMA Transfer", "The Cartridge Header", "MBC1"-"MBC5", "Power Up Sequence" | `gb/runtime/gb.c`, `gb/runtime/cart.c` | CC0 / docs | from memory, checked against the test ROMs |
| Pan Docs "CPU instruction set", gbdev "Opcode table" (sizes, M-cycle order, flags) | `gb/recomp/sm83.c`, `gb/runtime/cpu_ops.h` | CC0 / docs | from memory, checked against SingleStepTests sm83 |
| [SingleStepTests/65x02 `nes6502/v1`](https://github.com/SingleStepTests/65x02/tree/main/nes6502), pinned at commit `b9cfa8b2` | 10,000 tests per opcode with the bus access of every cycle; interpreter check (M2) | MIT | read |

## Prior art
| Source | What we took | License | Status |
|--------|--------------|---------|--------|
| [Andrew Kelley, "Statically Recompiling NES Games into Native Executables with LLVM and Go"](https://andrewkelley.me/post/jamulator.html), [jamulator repo](https://github.com/andrewrk/jamulator) | Why pure static recompilation fails → interpreter fallback | — (ideas only) | search summary |
| [Alyssa Rosenzweig, "Solving BIT Magic"](https://rosenzweig.io/blog/solving-bit-magic.html) | Overlapping-instruction decoding | — (ideas only) | search summary |
| [NESRecomp (mstan)](https://github.com/mstan/nesrecomp) and ports, e.g. [Dr. Mario](https://raw.githubusercontent.com/mstan/DrMarioNesRecomp/HEAD/README.md) | 6502 → C + runner library; per-game `game.toml` | check before reuse | search summary |
| [NESgen (Xenomega)](https://www.github.com/Xenomega/NESgen) | Another NES → C recompiler | check | listed |
| [SuperMarioBros-C](https://github.com/MitchellSternke/SuperMarioBros-C) | Example of a game as readable C | check | listed |
| [N64Recomp](https://github.com/N64Recomp/N64Recomp) | Emit-C recompiler design on another system | check | listed |
| [FOSDEM 2026 talk, "The joys and horrors of NES dynamic recompilation"](https://fosdem.org/schedule/event/ETLVUA-the_joys_and_horrors_of_nes_dynamic_recompilation) | Context on dynarec pitfalls | — | listed |
| [RustyNES](https://github.com/doublegate/RustyNES): [testing-strategy.md](https://github.com/doublegate/RustyNES/blob/main/docs/testing-strategy.md), [scheduler.md](https://github.com/doublegate/RustyNES/blob/main/docs/scheduler.md) | Test harness protocol (nestest, blargg $6000), timing model | GPL-3.0-or-later | summary read |
| radare2 6502 + 65816 tables (`libr/arch/p/6502/6502dis.inc.c`, `libr/arch/p/snes/optable.h`) | Independent opcode table; extracted to `tests/nes/data/r2_6502.json` | LGPL-3.0 | read |
| [Mesen2](https://github.com/SourMesen/Mesen2) (the program) | Second opinion when a game looks wrong (its Lua scripting, state for state against ours) and the speed comparison in 06 | GPL-3.0 | used |
| Mesen2 `Core/NES/Mappers/FDS` (Fds.cpp, FdsAudio.cpp, ModChannel.h, BaseFdsChannel.h) and `Core/NES/Loaders/FdsLoader.cpp` | The drive model followed in `nes/runtime/fds.c` (gaps and fake CRCs, 50000-cycle spin-up, a byte every 149 cycles, gap detection, $4030-$4032 bits), the automatic disk swap ($4032 polls, $E445 header match), the FDS sound details in `nes/runtime/apu.c` | GPL-3.0 (same as this program) | read |
| FCEUX | Second reference | GPL (check version) | listed |
| [SameBoy](https://github.com/LIJI32/SameBoy) `BootROMs/cgb_boot.asm` (the title checksum list, the 4th-letter list, the palette per checksum, the palette combinations and the 30 palettes; not its extra palettes or key combinations) | `gb/runtime/cgb_compat.c`: the colours of a Game Boy game on a Game Boy Color (compatibility mode). The tables are copied, the lookup follows the boot ROM's routine. No Nintendo boot ROM is used | Expat (MIT), (c) 2015-2026 Lior Halphon; checked in the repository's LICENSE: all files but the iOS and HexFiend directories | read |

## Libraries
| Library | Used for | License |
|---------|----------|---------|
| [SDL3](https://libsdl.org), tag `release-3.4.20` via CMake FetchContent | Window, audio, keyboard, gamepad | zlib |
