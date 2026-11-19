# Game Boy and Game Boy Color

The Game Boy part of [retrocast](../README.md): `gb` plays a game in the
interpreter, `gb_headless` runs one without a window, `gbrecomp`
recompiles one (through `-DROMS`, see the [root README](../README.md#recompiling-a-game)).

## Status
- **CPU**: the SM83 interpreter passes SingleStepTests sm83 (every opcode,
  cycle by cycle); recompiled code is the same C per instruction.
- **System**: 147 blargg and mooneye test ROMs pass (cpu_instrs,
  instr_timing, mem_timing, halt_bug, dmg_sound, cgb_sound, mooneye's
  timer, interrupt, serial, DMA and MBC tests, MBC1 multicart, and its boot
  register, DIV and I/O tests for DMG0, MGB, SGB, SGB2, CGB and AGB); 8 are
  listed open in `tests/gb/test_roms.py`, all PPU timing. The Color's speed
  switch is checked by AGE's spsw-div and gambatte's start_stop ROMs.
- **Picture**: dmg-acid2 and cgb-acid2 match their reference screenshots
  pixel for pixel.
- **Games**: in samples of the licensed sets, 41 Game Boy and 36 Game Boy
  Color games reach a title, menu or play screen with scripted input.
- **Recompiled**: 21 Game Boy and 18 Game Boy Color sampled games ran in
  lockstep with the interpreter, 34 of the 39 with 94.8-99.97% of their CPU
  cycles recompiled (the rest run much code from RAM, which is always
  interpreted).
- **Sound**: stereo, as the console's headphone output: NR51 pans each
  channel, NR50 sets each side's volume.
- **Speed**: about 40x real time (23x for a double-speed Color game), the
  same recompiled: the picture and sound hardware, stepped every cycle,
  take most of the time.

## Models and cartridges
A game whose header asks for the Color runs in CGB mode, any other as on
the original Game Boy (`model = auto`). `model = cgb` runs a game that does
not ask for the Color in the Color's compatibility mode: the game behaves
as on a Game Boy, with the palette the Color's boot ROM picks from the
header (the title's checksum, and its 4th letter where games share one),
and the boot state of that mode. No Nintendo boot ROM is used; the
palette table is from SameBoy's boot ROM source ([sources](../docs/sources.md)).
`gb_headless --model` also takes `cgb0`, `agb`, `dmg0`, `mgb`, `sgb` and
`sgb2`: those start in the boot state of that model (registers, DIV, I/O);
nothing else differs, there are no Super Game Boy features.
Cartridges: ROM only, MBC1 (also the multicart wiring: an 8 Mbit ROM with the logo again at
$40104), MBC2, MBC3 with its
clock, MBC5 (rumble ignored). Battery RAM is kept in `ROM.sav` next to the
ROM; for MBC3 the clock is saved with it and moves on by the time the
program was closed.

## Recompiling
`.gb` and `.gbc` files go in `ROMS` (root README); the build traces each
game first (`gb_headless --play --misses`). Code in RAM (HRAM DMA routines,
some games' WRAM code) always runs in the interpreter. Not yet: hint files
and the misses of your own play, as the NES has.

`build/game_one_headless --play --diff` runs the interpreter in lockstep and
reports the first difference.

## Settings and the headless program
`palette = green` (the original screen's tint) or `gray`, in `gb.ini` or
the game's .ini; Color games bring their own colours. `model = auto`, `dmg`
or `cgb`, there too (see above).

`gb_headless ROM` options: `--frames N`, `--play` (scripted input),
`--pgm FILE` (DMG shades) or `--ppm FILE` (colour) for the last frame,
`--wav FILE` (the sound, stereo), `--state-at F FILE` / `--load-state FILE`,
`--misses FILE`, `--diff`, `--coverage`, `--model dmg|cgb|cgb0|agb|dmg0|mgb|sgb|sgb2`. Test ROMs that
report on the serial port or in cartridge RAM print `result: passed`.

## Not yet
- Compatibility mode: the palettes chosen by held buttons at power-on
  (the boot ROM's key combinations) and the logo tilemap some games need
  are not there.
- A line is drawn when its mode 3 starts: register writes in the middle of
  a line show on the next one (cgb-acid-hell needs that).
- Open test ROMs: 8 mooneye PPU timing tests. The boot DIV of a Color
  game is unchecked (the test ROMs that check it are Game Boy games, so
  only compatibility mode has it); the DMG0's start position in the frame
  is only known within its test's window; AGE's other speed switch ROMs
  (tima, mode0, ch2 length, stop prefetch) still fail: the TIMA, PPU and
  prefetch details after the switch are not modelled.

## Docs
| File | Content |
|------|---------|
| [architecture.md](docs/architecture.md) | Components, timing, models, recompiled regions |
| [roadmap.md](docs/roadmap.md) | Milestones G1-G6 |
| [verification-log.md](docs/verification-log.md) | What was checked, how, and the result |
