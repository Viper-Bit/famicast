# Game Boy and Game Boy Color verification log

What was checked, how, and the result. Newest first. A claim without a
reproducible check is not "verified".

Checks of shared parts: [docs/verification-log.md](../../docs/verification-log.md).

## 2026-11-19 · Game Boy: timer edge, serial phase, MBC1 multicart, speed switch time
- mooneye `timer/rapid_toggle` (a TAC write that enables a bit that fell
  in that same cycle counts as a falling edge), `serial/boot_sclk_align`
  (the first bit goes at DIV's next bit-8 fall, 7 more follow at 128 cycles;
  the constant -1 is the one value of -140..140 that passes) and
  `emulator-only/mbc1/multicart_rom_8Mb` (bank bits shifted by 4) pass.
- The CGB speed switch takes 2050 M-cycles in the old speed, then DIV is
  reset (reset before the wait fails AGE spsw-div; waits of 2030-2068
  and 4100 all pass it, so the length rests on Pan Docs). AGE spsw-div shows
  "TEST PASSED!"; gambatte start_stop1/2 inc_1/inc_2 show their names' 00
  and 01 (`tests/gb/test_roms.py` TestSpeedSwitch). AGE's tima, mode0,
  ch2-lc-delay and stop-prefetch ROMs fail with or without the wait.
- 77 sampled games, 1800 frames with `--play`, before and after: 74 the
  same picture, 3 Color games differ by timing only (still in play or menu).
  Lockstep with `--diff`, 1800 frames with `--play`: the 3 Color games
  that differ (all switch speed) and 3 Game Boy games, no divergence.
- The GB suites pass.

## 2026-11-19 · Game Boy: stereo sound
- 30 s of one game recorded with `gb_headless --wav` (now stereo): the mean
  of left and right has, per 5 s, exactly the RMS the mono output had in
  the G4 entry (0.028, 0.085, 0.071, 0.062, 0.068, 0.064). In the first
  10 s left and right differ (the title music pans, |L-R| RMS 0.024-0.034),
  after that they are equal.
- The GB suites (ROMs, acid2, states, the recompiled cpu_instrs in
  lockstep) pass.

## 2026-11-19 · Game Boy G6: Game Boy Color
- **cgb-acid2**: 0 of 23,040 pixels differ from the reference
  (`tests/gb/test_ppu.py`, the paletted PNG decoded there; 5-bit
  components as `(c << 3) | (c >> 2)`, the test's own formula). dmg-acid2
  still exact.
- **blargg cgb_sound**: the 12 singles (now in `tests/gb/test_roms.py`: 129
  ROMs must pass) and the combined ROM pass; the 117 earlier ones still
  pass (cpu_instrs now runs in CGB mode: its header allows it).
- **Games**: every 40th licensed title of the GBC set (36 games), 30 s of
  scripted play, on a colour contact sheet: all 36 reach a title, menu or
  play screen. Found on the way, each fixed and the game checked again:
  the speed switch must skip the byte after STOP (a game has $E3 there and
  locked up); CGB writes to STAT raise no interrupt (a STAT handler re-entered
  itself until the stack ran into HRAM); a DMA from WRAM leaves the
  cartridge bus free on the CGB (a STAT handler in ROM read the DMA's
  bytes as its JP target). Black, white and faded tiles were transitions
  (frames at 5-55 s show logos, credits and play).
- **Lockstep**: every 80th licensed GBC title (18), traced and recompiled,
  1,800 frames against the interpreter: 18 of 18 identical; coverage 83-99.97%,
  Gex 40%; in the three lowest no ROM code was interpreted (code in RAM).
  Three DMG games again in lockstep for 3,600 frames, with the coverage
  they had before (the DMG paths unchanged).
- **Speed**: a double-speed game, 3,600 frames in 2.6 s (Release, MSVC).
- **Not checked**: the CGB boot DIV value (mooneye's CGB boot tests are DMG
  games on a CGB, a mode not emulated), HDMA timing against a test ROM.

## 2026-11-19 · Game Boy G5: recompiler
- **Lockstep**: every 80th licensed title of the GB set (21 games), each
  traced (6,000 frames of scripted play) and recompiled, then run 1,800
  frames with the same input against an interpreter-only Game Boy
  (`gb_headless --diff`: CPU after every step, every field of the machine
  after every frame): 21 of 21 identical. Also three more games for 3,600
  frames and blargg cpu_instrs (passes, recompiled and in lockstep).
- **Coverage** (`--coverage`, share of CPU cycles in recompiled code): 19
  games 94.8-99.8%; Hook 29.4%, TMNT 3 74.1%. In both, no ROM code was
  interpreted (`--misses` on the recompiled build lists nothing): the rest
  runs from WRAM. HRAM DMA routines, interrupt entry and halted cycles are
  never recompiled.
- **Chaining**: MSVC has no guaranteed tail call, so blocks return to
  `gb_step` there; with MinGW gcc 15.2 the generated code for one game has
  5,709 tail calls between blocks, and two games plus cpu_instrs are in
  lockstep with chaining on (the diff no longer turns it off).
- **Speed**: 3,600 frames of one game, Release: recompiled 1.46 s,
  interpreter 1.46 s (gcc; MSVC 1.56 s vs 1.49 s). The CPU's decoding is
  a small part; every memory access steps the PPU, APU and timer.
- **Correction (G4)**: the "11x real time" there was a Debug build; in
  Release the interpreter runs about 40x.
- **Tests**: `tests/gb/test_recomp.py`: block starts, chaining, the bank
  check and a trace-made bank region from a small made-up ROM;
  cpu_instrs recompiled passes and is in lockstep.

## 2026-11-19 · Game Boy G4: sound
- **blargg dmg_sound**: the 12 singles pass (`tests/gb/test_roms.py`: 117
  ROMs must pass, 20 are listed open; the "103" in the G3 entry
  miscounted, the suite then had 105), and so does the combined
  `dmg_sound.gb`. The wave tests (09, 10, 12) pass with the first sample
  read 3 cycles (2 MHz) after the period, SameBoy's value; 1, 2, 4, 5
  and 6 fail all three.
- **Games**: 30 s with scripted input from three games, recorded with
  `gb_headless --wav`: sound throughout (RMS 0.03-0.10 per 5 s), peak
  0.64, nothing clipped, exactly 48,000 samples per emulated second. The
  strongest pitch 24 s into one game's title music is 656 Hz, its first
  note being E5 (659 Hz). Not listened to.
- **Speed**: 60 emulated seconds in 5.3 s (11x real time) with sound.
- **Window program**: `gb` with a game runs 200 frames with the audio
  device open and exits 0; the GB suites (CPU, ROMs, dmg-acid2, states)
  pass.

## 2026-11-19 · Game Boy G3: PPU and the window program
- **dmg-acid2**: 0 of 23,040 pixels differ from the reference screenshot
  (`tests/gb/test_ppu.py`, a 2-bit PNG decoded in the test).
- **Games**: every 40th licensed title (no Proto/Beta/Demo/Unl/Pirate) of a
  1,919-archive set, 30 s with scripted input: 41 of 41 load and show a
  title, menu or play screen on a contact sheet (ROM only, MBC1, MBC2,
  MBC3); the one near-white frame (Tokyo Disneyland) was a fade, frames
  at 5-45 s show the welcome screen and the park map.
- **Save states**: 600 frames straight equal 300, save, load, 300 more
  (and differ from 300 alone). Battery: Pocket Monsters Pikachu (MBC3 +
  RAM) writes a 32 KB `ROM.sav`. Not checked: the clock part of `.sav`
  (no MBC3-clock game in the sample).
- **mooneye**: `ppu/hblank_ly_scx_timing-GS` now passes (mode 3 grows with
  SCX); the 102 earlier ROMs still pass; 8 PPU timing tests open.
- **Window program**: `gb` with a game runs 200 frames and exits cleanly,
  without one prints its usage and exits 2; a fresh `gb.ini` has the
  frontend's keys and `palette`.

## 2026-11-19 · Game Boy G2: memory map, timer, interrupts, cartridges
- **Test ROMs**: the c-sp/game-boy-test-roms v7.0 zip (SHA-256 pinned);
  `gb_headless` reads the result from the serial port (blargg text,
  mooneye's 3 5 8 13 21 34 / six $42) or blargg's cartridge-RAM protocol.
  `tests/gb/test_roms.py`: 102 pass; 21 open, and the test checks they
  still fail (9 boot tests of other models, 9 PPU timing, serial boot
  phase, `timer/rapid_toggle`, the MBC1 multicart).
- **Found on the way** (each by its test):
  - I/O was blocked during OAM DMA; only the DMA's bus is. With the bus
    conflict (that bus reads the byte in flight) mooneye's instruction
    timing tests, which measure through it, pass.
  - The DMA starts 2 M-cycles after the write and holds the bus for 160
    (`oam_dma_start`: the next cycle is free; `oam_dma_timing`).
  - A halted CPU keeps reading the opcode at PC; with IME off it fetches
    the next instruction in the cycle after the interrupt shows up
    (`halt_ime0_nointr_timing`, traced to the cycle: 17,535 M-cycles
    between the DIV reset and read). An extra cycle on the IME-on exit
    (as Pan Docs puts it) broke `halt_ime1_timing2-GS` and `di_timing-GS`.
  - DIV after boot: $ABC8 (with the clocks stepped before each access) is
    the only value from $AB00-$AC00 that passes `boot_div-dmgABCmgb`.
  - `halt_bug`'s header says MBC1+RAM with RAM size 0: such types get 8 KB.
- **Open, tried**: `rapid_toggle` takes the timer interrupt one loop late
  (BC $FFD8, want $FFD9); evaluating TAC writes at the cycle's start did
  not change it, resetting DIV at the cycle's start broke every timer test.
- **Mutations**: no timer interrupt on reload: 4 ROMs fail; MBC1 bank 0
  not read as 1: 6 ROMs fail.

## 2026-11-19 · Game Boy G1: SM83 CPU
- **What**: `gb/recomp/sm83.c` writes each opcode's C from its bit fields
  with the PC markers; `gbrecomp --cpu-switch` makes the interpreter's
  switch (`gb/runtime/cpu.c`). The template helpers (lines, markers, their
  expansion) moved to `core/recomp/template.c`; nesrecomp uses them too.
- **SingleStepTests sm83** (`python tests/fetch_test_roms.py --sst-sm83`,
  `tests/gb/test_cpu.py`): all 500 files, 1,000 tests each, pass on
  registers, IME and the EI delay, RAM, and every M-cycle (kind, address,
  value); HALT and STOP on registers and RAM only.
- **Mutations**: ADC without the carry in the half-carry fails ADC A,(HL)
  (8E: "f = 0, want 20"); PUSH writing low before high fails C5 (RAM);
  LD HL,SP+e without its internal cycle fails F8 ("2 cycles, want 3").
- **NES unchanged**: nesrecomp from main and from this branch, same
  arguments, every 50th NES archive and every 200th FDS disk: 83 of 83
  identical (72 programs, 11 refused alike); `cpu_switch.inc` identical.
- **Not checked yet**: interrupt entry (`gb_cpu_interrupt`) and HALT
  waking need the system (G2).

