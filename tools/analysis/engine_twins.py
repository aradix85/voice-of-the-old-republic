"""Checks that each KOTOR 2 address is the twin of the KOTOR 1 address beside it.

    python tools/analysis/engine_twins.py

`acc::addr::Pick(k1, k2)` claims that two functions are the same method in
both games. A wrong twin that takes the same number of arguments passes
call_convention.py and then does something else at run time. This check
compares what the two functions use:

  - calls: where the KOTOR 1 function calls a function that has a known twin,
    the KOTOR 2 function must call that twin;
  - globals: the same for the engine globals of `PickGlobal(k1, k2)`;
  - strings: text literals the KOTOR 1 function refers to should turn up in
    the KOTOR 2 function.

A call that agrees speaks for both ends: for the caller's pair and for the
pair of the function called.

KOTOR 1 is optimised and KOTOR 2 is not. KOTOR 2 calling more than KOTOR 1
is therefore normal, and what KOTOR 1 has inlined KOTOR 2 reaches through a
call: only what KOTOR 1 uses and KOTOR 2 does not reach within two calls
counts. Where both functions only forward to one other function, that pair
is compared instead. A pair with nothing to compare is listed as unproven,
not as wrong; one where some points agree and others do not as mixed, which
can also mean that the pair of the callee is the wrong one.

Pairs that were read by hand and found right although the check objects are
in engine_twins_reviewed.txt, one `kAddrName: reason` per line.

Needs both images of call_convention.py (see there) and capstone. The
report goes to build/analysis/engine_twins.txt.
"""
import os
import re
import struct
import sys

import capstone
from call_convention import IMAGE_BASE, OUT, SRC, strip_comments

PAIR = re.compile(r"(?:\b(k\w+)\s*=\s*)?acc::addr::(Pick|PickGlobal)\(\s*(0x[0-9A-Fa-f]+)\s*,"
                  r"\s*(0x[0-9A-Fa-f]+)\s*\)")
HEX = re.compile(r"0x[0-9a-f]+")
JUMP_TABLE = re.compile(r"dword ptr \[e\w\w\*4 \+ (0x[0-9a-f]+)\]")
REVIEWED = os.path.join(os.path.dirname(__file__), "engine_twins_reviewed.txt")


class Engine:
    def __init__(self, game: str):
        path = os.path.join(OUT, f"engine_image_{game}.bin")
        if not os.path.exists(path):
            sys.exit(f"no image at {path}; make one with call_convention.py")
        with open(path, "rb") as f:
            self.data = f.read()
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        # The first section is .text: its virtual size and address.
        text_size, text_at = struct.unpack_from("<2I", self.data, pe + 24 + optional_size + 8)
        self.text = range(IMAGE_BASE + text_at, IMAGE_BASE + text_at + text_size)
        self.cache: dict[int, tuple[set[int], set[int], set[str]]] = {}

    def literal(self, addr: int) -> str | None:
        """The text at `addr` when it is a string of four or more characters."""
        off = addr - IMAGE_BASE
        if addr in self.text or not 0 < off < len(self.data) - 4:
            return None
        end = self.data.find(b"\0", off, off + 200)
        raw = self.data[off:end]
        if end < 0 or len(raw) < 4 or not all(32 <= c < 127 for c in raw):
            return None
        return raw.decode("ascii")

    def after_padding(self, addr: int) -> bool:
        return self.data[addr - IMAGE_BASE - 1] in (0x90, 0xCC)

    def starts_function(self, addr: int) -> bool:
        """True right behind alignment padding or the previous function's ret."""
        off = addr - IMAGE_BASE
        return self.after_padding(addr) or self.data[off - 1] == 0xC3 or self.data[off - 3] == 0xC2

    def reach(self, entry: int, depth: int = 2) -> tuple[set[int], set[int], set[str]]:
        """What the function uses, together with the functions it calls."""
        own = self.uses(entry)
        calls, refs, strings = set(own[0]), set(own[1]), set(own[2])
        if depth:
            for callee in own[0]:
                if callee in self.text:
                    more = self.reach(callee, depth - 1)
                    calls |= more[0]
                    refs |= more[1]
                    strings |= more[2]
        return calls, refs, strings

    def uses(self, entry: int) -> tuple[set[int], set[int], set[str]]:
        """Direct call targets, absolute addresses and strings of one function."""
        if entry in self.cache:
            return self.cache[entry]
        calls: set[int] = set()
        refs: set[int] = set()
        strings: set[str] = set()
        seen: set[int] = set()
        todo = [entry]
        while todo:
            addr = todo.pop()
            while addr not in seen and addr in self.text:
                seen.add(addr)
                off = addr - IMAGE_BASE
                ins = next(self.cs.disasm(self.data[off:off + 16], addr, 1), None)
                if ins is None or ins.mnemonic == "int3":
                    break
                direct = int(ins.op_str, 16) if HEX.fullmatch(ins.op_str) else None
                if ins.mnemonic == "ret":
                    break
                if ins.mnemonic == "call" and direct is not None:
                    calls.add(direct)
                elif ins.mnemonic == "jmp":
                    table = JUMP_TABLE.fullmatch(ins.op_str)
                    if direct is not None:
                        inside = entry <= direct < entry + 0x6000
                        if inside and not self.after_padding(direct):
                            todo.append(direct)
                        else:
                            calls.add(direct)  # a tail call
                    elif table:  # a switch: its cases belong to the function
                        at = int(table.group(1), 16) - IMAGE_BASE
                        for case in struct.unpack_from("<64I", self.data, at):
                            if not entry <= case < entry + 0x6000:
                                break
                            todo.append(case)
                    break
                elif ins.mnemonic.startswith("j") and direct is not None:
                    todo.append(direct)
                else:
                    for m in HEX.finditer(ins.op_str):
                        value = int(m.group(0), 16)
                        if IMAGE_BASE < value < IMAGE_BASE + len(self.data):
                            refs.add(value)
                            text = self.literal(value)
                            if text:
                                strings.add(text)
                addr += ins.size
        self.cache[entry] = (calls, refs, strings)
        return self.cache[entry]


def read_pairs() -> tuple[dict[int, tuple[int, str]], dict[int, int]]:
    """Function pairs as k1 -> (k2, where), and global pairs as k1 -> k2."""
    functions: dict[int, tuple[int, str]] = {}
    data: dict[int, int] = {}
    for name in sorted(os.listdir(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue
        with open(os.path.join(SRC, name), encoding="latin-1") as f:
            text = strip_comments(f.read())
        for m in PAIR.finditer(text):
            k1, k2 = int(m.group(3), 16), int(m.group(4), 16)
            if not k1 or not k2:
                continue
            line = text.count("\n", 0, m.start()) + 1
            if m.group(2) == "PickGlobal":
                data[k1] = k2
            else:
                functions[k1] = (k2, f"{name}:{line}: {m.group(1) or 'Pick'}")
    return functions, data


def compare(k1: Engine, k2: Engine, a1: int, a2: int, functions: dict[int, tuple[int, str]],
            data: dict[int, int], called: set[int], hops: int = 3) -> tuple[list[str], int]:
    """What KOTOR 1's function uses and KOTOR 2's does not reach, and the agreeing points."""
    calls1, refs1, strings1 = k1.uses(a1)
    own2 = k2.uses(a2)[0]
    calls2, refs2, strings2 = k2.reach(a2)
    missing, agreeing = [], 0
    for callee in sorted(calls1):
        if callee in functions:
            twin = functions[callee][0]
            if twin in calls2:
                agreeing += 1
                called.add(callee)  # the same point speaks for the callee's pair
            else:
                missing.append(f"calls 0x{callee:08x}, its twin 0x{twin:08x} is not called")
    for ref in sorted(refs1):
        if ref in data:
            if data[ref] in refs2:
                agreeing += 1
            else:
                missing.append(f"uses global 0x{ref:08x}, its twin 0x{data[ref]:08x} is not used")
    shared = strings1 & strings2
    agreeing += len(shared)
    if strings1 and not shared:
        missing.append(f"none of its strings is used: {sorted(strings1)[:3]}")
    # KOTOR 1 only forwards: what it forwards to is the twin of KOTOR 2's
    # function itself, or of the one function that forwards to in turn.
    if hops and not missing and not agreeing and len(calls1) == 1:
        target = next(iter(calls1))
        tries = [compare(k1, k2, target, a2, functions, data, called, 0)]
        if len(own2) == 1:
            tries.append(compare(k1, k2, target, next(iter(own2)), functions, data, called,
                                 hops - 1))
        return min(tries, key=lambda t: (len(t[0]), -t[1]))
    return missing, agreeing


def main() -> int:
    k1, k2 = Engine("k1"), Engine("k2")
    functions, data = read_pairs()
    with open(REVIEWED, encoding="utf-8") as f:
        reviewed = {line.split(":")[0].strip() for line in f if ":" in line}
    wrong, mixed, proven, inside = [], [], 0, 0
    unproven: list[tuple[int, str]] = []
    called: set[int] = set()
    for a1, (a2, where) in sorted(functions.items(), key=lambda kv: kv[1][1]):
        label = f"{where} 0x{a1:08x} / 0x{a2:08x}"
        if a1 not in k1.text or a2 not in k2.text:
            continue  # a vtable or another constant, not a function
        if not k1.starts_function(a1) or not k2.starts_function(a2):
            inside += 1  # a hook site inside a function
            continue
        missing, agreeing = compare(k1, k2, a1, a2, functions, data, called)
        if where.split()[-1] in reviewed:
            proven += 1
        elif missing and agreeing:
            mixed.append(f"{label}: " + "; ".join(missing) + f" ({agreeing} points agree)")
        elif missing:
            wrong.append(f"{label}: " + "; ".join(missing))
        elif agreeing:
            proven += 1
        else:
            unproven.append((a1, label))
    by_caller = sum(1 for a1, _ in unproven if a1 in called)
    proven += by_caller
    unproven_rows = [label for a1, label in unproven if a1 not in called]

    os.makedirs(OUT, exist_ok=True)
    report = os.path.join(OUT, "engine_twins.txt")
    with open(report, "w", encoding="utf-8") as f:
        for title, rows in (("DISAGREE", wrong), ("MIXED", mixed),
                            ("UNPROVEN (nothing to compare)", unproven_rows)):
            f.write(f"== {title}: {len(rows)}\n")
            f.writelines(row + "\n" for row in rows)
    for row in wrong:
        print("DISAGREE", row)
    for row in mixed:
        print("MIXED", row)
    print(f"{proven + len(wrong) + len(mixed) + len(unproven_rows)} function pairs: "
          f"{proven} agree ({by_caller} of them through a caller), "
          f"{len(wrong)} disagree, {len(mixed)} mixed, {len(unproven_rows)} unproven; "
          f"{inside} addresses inside a function skipped -> {report}")
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main())
