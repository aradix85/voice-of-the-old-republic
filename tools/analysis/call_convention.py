"""Checks every engine call's argument bytes against the engine's own `ret N`.

    python tools/analysis/call_convention.py --game k2 --from-exe swkotor2.exe
    python tools/analysis/call_convention.py --game k1 --from-dump GAME.dmp
    python tools/analysis/call_convention.py [--game k1|k2] [--image FILE]

A function-pointer typedef that pushes more or fewer bytes than the engine
function pops leaves the stack off by that much. The caller's epilogue then
restores its saved registers from the wrong slots and returns normally, and
the function above it carries on with garbage: a crash or a wrong value far
from the cause.

The check reads the game's code as it sits in memory, from
build/analysis/engine_image_<game>.bin. `--from-exe` makes that file from
the executable on disk. Steam's KOTOR 1 executable is encrypted there (a
.bind section). Three ways around that: `kdev dump-text` with the game
running (pass its swkotor-image.bin with `--image`), a copy unpacked with
Steamless, or `--from-dump`, which cuts the image out of a full process dump
(Task Manager, "Create dump file", or a crash dump under
%LOCALAPPDATA%\\CrashDumps). Findings go to build/analysis/call_convention.txt.

Needs capstone; --from-dump also needs minidump (pip install capstone minidump).
"""
import argparse
import bisect
import collections
import os
import re
import struct
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(REPO, "patches", "Accessibility")
OUT = os.path.join(REPO, "build", "analysis")
IMAGE_BASE = 0x400000
EXE = {"k1": "swkotor.exe", "k2": "swkotor2.exe"}

TYPE = re.compile(
    r"(?:typedef\s+(?P<ret1>[^;(){}]+?)\s*\(\s*__(?P<cc1>\w+)\s*\*\s*(?P<name1>\w+)\s*\)"
    r"|using\s+(?P<name2>\w+)\s*=\s*(?P<ret2>[^;(){}=]+?)\s*\(\s*__(?P<cc2>\w+)\s*\*\s*\))"
    r"\s*\((?P<params>[^;{}]*?)\)\s*;", re.DOTALL)
CAST = re.compile(r"reinterpret_cast<\s*(\w+)\s*>\s*\(")
CONST = re.compile(r"\b(k\w+)\s*=\s*acc::addr::(Pick|R|Kotor1Only|Kotor2Only)\(\s*"
                   r"(0x[0-9A-Fa-f]+)\s*(?:,\s*(0x[0-9A-Fa-f]+)\s*)?\)")
INCLUDE = re.compile(r'#include\s+"([\w.]+)"')
BY_VALUE = {"Vector": 12, "CExoString": 8, "CResRef": 16, "double": 8,
            "int64_t": 8, "uint64_t": 8, "__int64": 8}
FOUR_BYTES = {
    "int", "unsigned", "long", "short", "char", "bool", "float", "signed", "size_t",
    "uintptr_t", "intptr_t", "uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t",
    "int32_t", "DWORD", "WORD", "BYTE", "BOOL", "HRESULT", "UINT", "LONG", "ULONG"}

Fn = collections.namedtuple("Fn", "cc ret params")
Use = collections.namedtuple("Use", "file line type fn where")


def strip_comments(text: str) -> str:
    """Comments become spaces, so offsets and line numbers stay the same."""
    def blank(m: re.Match) -> str:
        return re.sub(r"[^\n]", " ", m.group(0))
    return re.sub(r"//[^\n]*|/\*.*?\*/", blank, text, flags=re.DOTALL)


def split_params(params: str) -> list[str]:
    parts = [p.strip() for p in params.split(",")]
    return [p for p in parts if p and p != "void"]


def size_of(decl: str) -> int | None:
    """Stack bytes of one parameter; None when the type is not known here."""
    if "*" in decl or "&" in decl:
        return 4
    words = [w for w in re.findall(r"[A-Za-z_]\w*", decl) if w not in ("const", "struct")]
    if not words:
        return None
    for w in words:
        if w in BY_VALUE:
            return BY_VALUE[w]
    return 4 if words[0] in FOUR_BYTES else None


def pushed_bytes(fn: Fn) -> tuple[int | None, str]:
    """What the callee must pop for this typedef, and a note on any guess."""
    raw = [size_of(p) for p in fn.params]
    note = "unknown parameter type taken as 4 bytes" if None in raw else ""
    sizes = [4 if s is None else s for s in raw]
    hidden = 4 if size_of(fn.ret) not in (4, None) and "double" not in fn.ret else 0
    if fn.cc == "cdecl":
        return 0, note
    if fn.cc == "thiscall":
        if not sizes:
            return None, "thiscall without a this parameter"
        return sum(sizes[1:]) + hidden, note
    if fn.cc == "fastcall":
        return sum(sizes[2:]) + hidden, note
    return sum(sizes) + hidden, note


def close_paren(text: str, start: int) -> int:
    depth = 1
    i = start
    while depth and i < len(text):
        depth += {"(": 1, ")": -1}.get(text[i], 0)
        i += 1
    return i - 1


def address_for(game: str, m: re.Match) -> int | None:
    """The constant's address in this game; None when it is the other game's."""
    kind, first, second = m.group(2), m.group(3), m.group(4)
    if kind == "Pick":
        return int(first if game == "k1" else second, 16)
    return int(first, 16) if (kind == "Kotor2Only") == (game == "k2") else None


def reachable(name: str, includes: dict[str, list[str]]) -> list[str]:
    """The file itself, then the headers it includes, nearest first."""
    order, todo = [name], list(includes.get(name, []))
    while todo:
        header = todo.pop(0)
        if header in includes and header not in order:
            order.append(header)
            todo.extend(includes[header])
    return order


def scan_sources(game: str) -> tuple[list[Use], list[str]]:
    texts = {}
    for name in sorted(os.listdir(SRC)):
        if name.endswith((".cpp", ".h", ".inc")):
            with open(os.path.join(SRC, name), encoding="latin-1") as f:
                texts[name] = strip_comments(f.read())
    includes = {name: INCLUDE.findall(text) for name, text in texts.items()}

    consts: dict[str, dict[str, int | None]] = {}
    types: dict[str, list[tuple[int, str, Fn]]] = {}
    for name, text in texts.items():
        consts[name] = {m.group(1): address_for(game, m) for m in CONST.finditer(text)}
        types[name] = [
            (m.start(), m.group("name1") or m.group("name2"),
             Fn(m.group("cc1") or m.group("cc2"),
                (m.group("ret1") or m.group("ret2")).strip(),
                split_params(m.group("params"))))
            for m in TYPE.finditer(text)]

    other_game = "K2" if game == "k1" else "K1"
    uses: list[Use] = []
    skipped: list[str] = []
    for name, text in texts.items():
        files = reachable(name, includes)
        starts = [pos for pos, _, _ in types[name]]
        for m in CAST.finditer(text):
            type_name = m.group(1)
            # The nearest definition above the cast (short names are reused),
            # else the one in an included header.
            above = types[name][:bisect.bisect_right(starts, m.start())]
            candidates = list(reversed(above)) + [t for f in files[1:] for t in types[f]]
            fn = next((tfn for _, tname, tfn in candidates if tname == type_name), None)
            if fn is None or other_game in type_name:
                continue  # not a function-pointer type, or the other game's call
            line = text.count("\n", 0, m.start()) + 1
            expr = text[m.end():close_paren(text, m.end())]
            known = {k: v for f in reversed(files) for k, v in consts[f].items()}
            idents = [w for w in re.findall(r"\b[A-Za-z_]\w*", expr) if w in known]
            literal = re.search(r"\b0x00[4-9A-Fa-f][0-9A-Fa-f]{5}\b", expr)
            if idents:
                if known[idents[0]] is not None:
                    uses.append(Use(name, line, type_name, fn,
                                    (idents[0], known[idents[0]])))
            elif literal:
                uses.append(Use(name, line, type_name, fn,
                                (literal.group(0), int(literal.group(0), 16))))
            else:
                skipped.append(f"{name}:{line}: {type_name}({' '.join(expr.split())[:60]})")
    # Where the games differ the code branches on the game, with a typedef
    # named for KOTOR 2 beside the plain one: the plain one is KOTOR 1's call.
    split = {(u.file, u.where[0]) for u in uses if "K2" in u.type}
    if game == "k2":
        uses = [u for u in uses if "K2" in u.type or (u.file, u.where[0]) not in split]
    return uses, skipped


class Image:
    def __init__(self, path: str):
        import capstone
        with open(path, "rb") as f:
            self.data = f.read()
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.cs.detail = False
        self.cache: dict[int, tuple[frozenset, str]] = {}

    def rets(self, entry: int, depth: int = 0) -> tuple[frozenset, str]:
        """The `ret N` values reachable from `entry`, following its own jumps."""
        if entry in self.cache:
            return self.cache[entry]
        found: set[int] = set()
        note = ""
        seen: set[int] = set()
        todo = [entry]
        limit = entry + 0x6000
        while todo:
            addr = todo.pop()
            while addr not in seen and IMAGE_BASE <= addr < IMAGE_BASE + len(self.data) - 16:
                seen.add(addr)
                off = addr - IMAGE_BASE
                ins = next(self.cs.disasm(self.data[off:off + 16], addr, 1), None)
                if ins is None:
                    note = "undecodable instruction"
                    break
                m = ins.mnemonic
                target = int(ins.op_str, 16) if re.fullmatch(r"0x[0-9a-f]+", ins.op_str) else None
                if m == "ret":
                    found.add(int(ins.op_str, 16) if ins.op_str else 0)
                    break
                if m == "jmp":
                    if target is None:
                        note = note or "indirect jump"
                    elif entry <= target < limit:
                        todo.append(target)
                    elif not IMAGE_BASE <= target < IMAGE_BASE + len(self.data):
                        # One of the mod's own detours in a running game: its
                        # wrapper comes back right behind the overwritten bytes.
                        addr += ins.size
                        continue
                    elif depth < 4:
                        sub, sub_note = self.rets(target, depth + 1)
                        found |= sub
                        note = note or sub_note
                    break
                if m.startswith("j") and target is not None:
                    todo.append(target)
                if m == "int3":
                    break
                addr += ins.size
        self.cache[entry] = (frozenset(found), note)
        return self.cache[entry]


def image_from_exe(exe: str, out_path: str) -> None:
    """Lays the executable's sections out as the loader does."""
    with open(exe, "rb") as f:
        raw = f.read()
    pe = struct.unpack_from("<I", raw, 0x3C)[0]
    sections, optional_size = struct.unpack_from("<H12xH", raw, pe + 6)
    optional = pe + 24
    base, image_size = (struct.unpack_from("<I", raw, optional + off)[0] for off in (28, 56))
    if base != IMAGE_BASE:
        sys.exit(f"{exe}: image base 0x{base:x}, expected 0x{IMAGE_BASE:x}")
    image = bytearray(image_size)
    image[:0x400] = raw[:0x400]
    for i in range(sections):
        header = optional + optional_size + 40 * i
        name = raw[header:header + 8].rstrip(b"\0")
        _, address, raw_size, raw_at = struct.unpack_from("<4I", raw, header + 8)
        if name == b".bind":
            sys.exit(f"{exe} is encrypted (.bind section); see the top of this script")
        image[address:address + raw_size] = raw[raw_at:raw_at + raw_size]
    os.makedirs(OUT, exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(image)
    print(f"image of {os.path.basename(exe)}: {sections} sections -> {out_path}")


def image_from_dump(dump: str, game: str, out_path: str) -> None:
    from minidump.minidumpfile import MinidumpFile
    md = MinidumpFile.parse(dump)
    module = next((m for m in md.modules.modules
                   if m.name.lower().endswith("\\" + EXE[game])), None)
    if module is None:
        sys.exit(f"{EXE[game]} is not a module of this dump")
    reader = md.get_reader()
    pages = []
    for addr in range(module.baseaddress, module.baseaddress + module.size, 0x1000):
        try:
            pages.append(reader.read(addr, 0x1000))
        except Exception:  # noqa: BLE001 - a page the dump does not hold
            pages.append(bytes(0x1000))
    os.makedirs(OUT, exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(b"".join(pages))
    print(f"image of {EXE[game]}: {len(pages)} pages -> {out_path}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--game", choices=("k1", "k2"), default="k1")
    ap.add_argument("--image", help="memory image of the game's executable")
    ap.add_argument("--from-exe", help="unencrypted executable to make the image from")
    ap.add_argument("--from-dump", help="full process dump to cut the image from")
    args = ap.parse_args()

    image_path = args.image or os.path.join(OUT, f"engine_image_{args.game}.bin")
    if args.from_exe:
        image_from_exe(args.from_exe, image_path)
    if args.from_dump:
        image_from_dump(args.from_dump, args.game, image_path)
    if not os.path.exists(image_path):
        sys.exit(f"no image at {image_path}; make one with --from-exe or --from-dump")
    image = Image(image_path)

    uses, skipped = scan_sources(args.game)
    wrong, unsure, ok = [], [], 0
    for u in uses:
        name, addr = u.where
        want, note = pushed_bytes(u.fn)
        got, got_note = image.rets(addr)
        where = f"{u.file}:{u.line}: {u.type} -> {name} @0x{addr:08x}"
        if want is None or not got:
            unsure.append(f"{where}: {note or got_note or 'no ret found'}")
        elif got == {want}:
            ok += 1
            if note:
                unsure.append(f"{where}: fits, but {note}")
        elif len(got) > 1 and want in got:
            unsure.append(f"{where}: pushes {want}, engine has ret {sorted(got)} {got_note}")
        else:
            wrong.append(f"{where}: {u.fn.cc} pushes {want} bytes, "
                         f"engine pops {sorted(got)} {note}".rstrip())

    os.makedirs(OUT, exist_ok=True)
    report = os.path.join(OUT, "call_convention.txt")
    with open(report, "w", encoding="utf-8") as f:
        for title, rows in (("MISMATCH", wrong), ("UNSURE", unsure),
                            ("NOT CHECKED (address is not a constant)", skipped)):
            f.write(f"== {title}: {len(rows)}\n")
            f.writelines(row + "\n" for row in sorted(set(rows)))
    for row in sorted(set(wrong)):
        print(row)
    print(f"{args.game}: {len(uses)} calls, {ok} fit, {len(set(wrong))} mismatches, "
          f"{len(set(unsure))} unsure, {len(skipped)} not checked -> {report}")
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main())
