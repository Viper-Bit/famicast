"""Game Boy recompiler (gb/recomp): the blocks gbrecomp makes from a small\nmade-up ROM, and blargg's cpu_instrs recompiled (gb/CMakeLists.txt\ngbtest_cpu_instrs): it must pass and match the interpreter in lockstep."""
import os
import re
import subprocess
import tempfile
import unittest

from tests.fetch_test_roms import fetch_gb_test_roms
from tests.gb.test_roms import BUILD
from tests.games import built

EXE = ".exe" if os.name == "nt" else ""
GBRECOMP = os.path.join(BUILD, "gbrecomp" + EXE)
CPU_INSTRS = os.path.join(BUILD, "gbtest_cpu_instrs_headless" + EXE)


def make_rom():
    """64 KB, MBC1. Vectors: RET. $0100: NOP, JP $0150.\n    $0150 LD A,2 / $0152 LD ($2000),A / $0155 CALL $4000 / $0158 JR NZ,$0155 / $015A JR $015A\n    bank 1, $4000: INC A / RET"""
    rom = bytearray(0x10000)
    rom[0:0x100] = b"\xC9" * 0x100
    rom[0x100:0x104] = bytes([0x00, 0xC3, 0x50, 0x01])
    rom[0x147], rom[0x148] = 0x01, 0x01
    rom[0x150:0x15C] = bytes([0x3E, 0x02, 0xEA, 0x00, 0x20, 0xCD, 0x00, 0x40, 0x20, 0xFB, 0x18, 0xFE])
    rom[0x4000:0x4002] = bytes([0x3C, 0xC9])
    return bytes(rom)


def recompile(rom, misses="", hints=None):
    """(gbrecomp's exit code, stderr, the C it wrote); misses: a trace's text, hints: a hint file's text"""
    with tempfile.TemporaryDirectory() as tmp:
        paths = {n: os.path.join(tmp, n) for n in ("t.gb", "t.trace", "t.toml", "t.c")}
        for name, data in (("t.gb", rom), ("t.trace", misses.encode()), ("t.toml", (hints or "").encode())):
            with open(paths[name], "wb") as f:
                f.write(data)
        args = [GBRECOMP, paths["t.gb"], "-o", paths["t.c"], "--misses", paths["t.trace"]]
        if hints is not None:
            args += ["--hints", paths["t.toml"]]
        r = subprocess.run(args, capture_output=True, text=True)
        c = ""
        if r.returncode == 0:
            with open(paths["t.c"]) as f:
                c = f.read()
        return r.returncode, r.stderr, c


def blocks(c_source):
    """{block name: its C}"""
    parts = re.split(r"^static void (b_\w+)\(GbCpu \*c\) \{$", c_source, flags=re.M)
    return {parts[i]: parts[i + 1].split("\n}\n", 1)[0] for i in range(1, len(parts), 2)}


@built(GBRECOMP, "build gbrecomp first")
class TestBlocks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as tmp:
            rom, trace, out = (os.path.join(tmp, n) for n in ("t.gb", "t.trace", "t.c"))
            with open(rom, "wb") as f:
                f.write(make_rom())
            with open(trace, "w") as f:
                f.write("MISS $4000 bank 1\n")
            subprocess.run([GBRECOMP, rom, "-o", out, "--misses", trace], check=True, capture_output=True)
            with open(out) as f:
                cls.c = f.read()
        cls.b = blocks(cls.c)

    def test_block_starts(self):
        mine = sorted(n for n in self.b if n.startswith("b_000_01"))
        # $0155 starts a block (JR target), so $0150's block ends before it; after CALL, $0158
        self.assertEqual(mine, ["b_000_0100", "b_000_0150", "b_000_0155", "b_000_0158", "b_000_015A"])

    def test_bank_switch_checked(self):
        self.assertIn("if (gb_cart_bank(&g->cart, 0x0000) != 0) return;", self.b["b_000_0150"])

    def test_chaining_stays_in_the_window(self):
        self.assertIn("RECOMP_CHAIN(b_000_0155)", self.b["b_000_0150"])  # falls into it
        self.assertNotIn("RECOMP_CHAIN", self.b["b_000_0155"])           # CALL $4000: bank unknown here
        for t in ("b_000_0155", "b_000_015A"):
            self.assertIn("RECOMP_CHAIN(%s)" % t, self.b["b_000_0158"])

    def test_banked_entry_from_trace(self):
        self.assertIn("b_001_4000", self.b)
        self.assertIn("[1][1] = r_001_1", self.c)
        self.assertNotIn("[1][0]", self.c)


@built(GBRECOMP, "build gbrecomp first")
class TestHints(unittest.TestCase):
    """Hint files (gb/games/NAME.toml) and the misses file of a program's own play."""

    def rom(self):  # bank 1 holds INC A / RET at $4000 and $4100; bank 0 at $0200: NOP / RET (not reachable)
        rom = bytearray(make_rom())
        rom[0x4100:0x4102] = bytes([0x3C, 0xC9])
        rom[0x0200:0x0202] = bytes([0x00, 0xC9])
        return bytes(rom)

    def names(self, **kw):
        code, err, c = recompile(self.rom(), **kw)
        self.assertEqual(code, 0, err)
        return set(blocks(c)), err

    def test_nothing_without_hints(self):
        names, _ = self.names()
        self.assertTrue(names.isdisjoint({"b_001_4000", "b_001_4100", "b_000_0200"}))

    def test_hint_entries(self):
        hints = "# a comment\nentries = [0x0200]  # bank 0\nbanked = [\n  [1, 0x4000], [1, 0x4100],\n]\n"
        names, err = self.names(hints=hints)
        self.assertTrue({"b_001_4000", "b_001_4100", "b_000_0200"} <= names)
        self.assertEqual(err, "")

    def test_misses_file_of_play_and_hints_together(self):
        names, _ = self.names(misses="MISS $4100 bank 1\nMISS $4100 bank 1\n", hints="entries = [0x200]\n")
        self.assertTrue({"b_001_4100", "b_000_0200"} <= names)
        self.assertNotIn("b_001_4000", names)

    def test_data_ranges_are_not_decoded(self):
        hints = "entries = [0x0200]\nbanked = [[1, 0x4000]]\ndata = [[0, 0x200, 0x202], [1, 0x4000, 0x4001]]\n"
        names, _ = self.names(hints=hints)
        self.assertTrue(names.isdisjoint({"b_000_0200", "b_001_4000"}))

    def test_other_dump_warns(self):
        _, err = self.names(hints='rom_fnv1a = "00000000"\nentries = [0x0200]\n')
        self.assertIn("another dump", err)

    def test_matching_dump_does_not_warn(self):
        import functools
        h = functools.reduce(lambda h, b: (h ^ b) * 16777619 & 0xFFFFFFFF, self.rom(), 2166136261)
        _, err = self.names(hints='rom_fnv1a = "%08X"\n' % h)
        self.assertEqual(err, "")

    def test_bad_hints_are_refused(self):
        for hints in ("banked = [[1]]\n", "banked = [[9, 0x4000]]\n", "entries = [0x4000]\n", "data = [[0, 5, 4]]\n",
                      "entries = 5\n", "entries = [0x200\n"):
            code, err, _ = recompile(self.rom(), hints=hints)
            self.assertEqual(code, 2, hints)
            self.assertIn("t.toml", err)

    def test_missing_hint_file_is_fine(self):
        with tempfile.TemporaryDirectory() as tmp:
            rom, out = os.path.join(tmp, "t.gb"), os.path.join(tmp, "t.c")
            with open(rom, "wb") as f:
                f.write(self.rom())
            subprocess.run([GBRECOMP, rom, "-o", out, "--hints", os.path.join(tmp, "none.toml")], check=True, capture_output=True)


@built(CPU_INSTRS, "fetch the test ROMs, configure CMake, then build")
class TestCpuInstrsRecompiled(unittest.TestCase):
    ROM = os.path.join(fetch_gb_test_roms(), "blargg", "cpu_instrs", "cpu_instrs.gb") if os.path.exists(CPU_INSTRS) else ""

    def test_passes(self):
        r = subprocess.run([CPU_INSTRS, self.ROM, "--frames", "4000", "--coverage"], capture_output=True, text=True)
        self.assertIn("result: passed", r.stdout)
        self.assertRegex(r.stdout, r"COVERAGE [1-9]")  # some of it ran recompiled (most runs from RAM)

    def test_lockstep_with_the_interpreter(self):
        r = subprocess.run([CPU_INSTRS, self.ROM, "--frames", "4000", "--diff"], capture_output=True, text=True)
        self.assertIn("DIFF OK", r.stdout, r.stdout[-1500:])


if __name__ == "__main__":
    unittest.main()
