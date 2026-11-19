retrocast - static recompiler for NES, Famicom Disk System, Game Boy and Game Boy Color games
https://github.com/Viper-Bit/retrocast

No games are included. Use your own legally dumped games (.nes, .fds, .gb, .gbc).


WHAT IS IN HERE

  nes             Plays a game in the built-in 6502 interpreter.
                  Usage: nes path/to/game.nes (or .fds, see below)
  nes_headless    The same without a window, for scripts and checks
                  (--frames N, --record/--replay, --diff, --coverage, ...).
  nesrecomp       The recompiler: turns a ROM into C source.
  gb              Plays a Game Boy or Game Boy Color game in the built-in
                  interpreter. Usage: gb path/to/game.gb (or .gbc)
  gb_headless     The same without a window, for test ROMs (--frames N,
                  --wav FILE to record the sound).
  gbrecomp        The Game Boy recompiler: turns a ROM into C source.
  kit/            Everything needed to build a recompiled game into a
                  native program (see below), with SDL3 prebuilt.

On macOS, remove the download quarantine once before the first run:
  xattr -dr com.apple.quarantine retrocast-*


PLAYING

Keyboard: arrows, X = A, Z = B, (S = X, A = Y, Q = L, W = R for pads
that have them), Enter = Start, Right Shift = Select,
F5 = reset, F2 / F4 = save / load state, F6 = next disk side (FDS),
F10 = TV frame, F11 or Alt+Enter = fullscreen, Esc = quit.
Gamepads work too (first = player 1, second = player 2).
Settings are in nes.ini / gb.ini next to the program, created on first run.


RECOMPILING A GAME (the kit)

A recompiled game is C code compiled on your machine; it then runs the game
natively. You need CMake 3.20 or newer and a C compiler:
  Windows: Visual Studio or its Build Tools ("Desktop development with C++")
           - the prebuilt SDL3 is built with MSVC, so MinGW will not link it
  Linux:   gcc or clang (e.g. sudo apt install build-essential cmake)
  macOS:   Xcode command line tools (xcode-select --install) and CMake

From the folder this file is in:

  cmake -S kit -B build -DROMS="/path/to/My Game.nes"
  cmake --build build --config Release

The program is build/my_game (build/my_game.exe on Windows), plus
build/my_game_headless. Several games: separate the paths with ";".
A relative path is relative to the folder you run cmake from. The file's
extension picks the system: .nes, .fds, .gb, .gbc.

Code the recompiler cannot find on its own runs in the interpreter. Two
things recompile more of it:
  - the build plays each game for a while first (scripted, about 6 seconds)
    and recompiles everything that ran; -DAUTO_HINTS=OFF skips this;
  - your own play (NES games so far): the game notes code it had to
    interpret in build/my_game.misses, and the next
    "cmake --build build --config Release" recompiles it.

DO NOT SHARE RECOMPILED PROGRAMS: they contain the game.


NES AND FAMICOM DISK SYSTEM

Famicom Disk System: .fds images need the FDS BIOS (8 KB, not included):
put it next to nes as disksys.rom, or set fds_bios in nes.ini. Disk sides
are swapped by themselves when a game asks (F6 by hand). The kit
recompiles disk games like the others, given the BIOS:
  cmake -S kit -B build -DROMS="/path/to/Disk Game.fds" -DFDS_BIOS="/path/to/disksys.rom"
The program carries the BIOS too (and build/fds, which plays any .fds
image with only the BIOS recompiled). Do not share them either.

Supported mappers and more: nes/README.md on the project page.


GAME BOY AND GAME BOY COLOR

Color games run as on a Game Boy Color, the others as on the original
Game Boy (palette = green or gray in gb.ini; model = cgb runs those in the
Color's compatibility mode, with its colours). Battery saves are kept in
game.gb.sav next to the game, with the cartridge clock where there is one.
More: gb/README.md on the project page.


LICENSES

retrocast: GPL-3.0-or-later (LICENSE); the source is on the project page.
SDL3 (linked into nes, gb and the kit): zlib license (LICENSE-SDL3.txt).
