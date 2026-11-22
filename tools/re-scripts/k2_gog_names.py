"""Find a KOTOR 2 engine function by NAME and give its Steam address to check.

Kotor Patch Manager's kotor2_gog_aspyr.db names some ten thousand functions of
the GOG build. The GOG build is the Steam compile re-linked: the code is the
same, the object files moved. A GOG address therefore maps to Steam through a
distance that holds within one object file, and this script finds it two ways:

  name lookup   the nearest known Steam/GOG pairs on either side, from the
                table the mod carried while it supported GOG (read from git);
  --class       the class's own vtable: every (Steam vtable slot, GOG named
                function) pair proposes a distance, and the one that puts the
                most named functions on a Steam function start wins.

What comes out is a CANDIDATE. Distances drift by a few bytes inside an object
file (two neighbours 0x20 apart were seen to disagree), and a class with few
functions can match by chance (CSWGuiExamine did). Use an address only when it
is a function start AND its decompile shows what the name says:

    KDEV_GHIDRA_PROJ=kotor2 KDEV_GHIDRA_PROGRAM=swkotor2.exe \\
        tools/ghidra-scripts/decomp.sh 0xADDR

The database is not in the repo (docs/tools.md says where to get it).

Usage:
    k2_gog_names.py <db> <Class::Function or part of a function name> [...]
    k2_gog_names.py <db> --class <Class> [...]
    k2_gog_names.py <db> --offsets <part of a class name> [...]
"""
import bisect
import collections
import csv
import os
import re
import sqlite3
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
K2 = os.path.join(REPO, "docs", "llm-docs", "re", "k2")
# The last commit that still had the Steam -> GOG table for KOTOR 2.
PAIRS_BLOB = "54b3b0b7^:patches/Accessibility/engine_rebase_k2_table.inc"
PAIR = re.compile(r"\{\s*0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+)\s*\},?\s*//\s*(.*)")
FUNC = re.compile(r"0x([0-9a-fA-F]+),(\d+)")


def function_starts():
    starts = {}
    with open(os.path.join(K2, "k2-functions.csv")) as f:
        for line in f:
            m = FUNC.match(line)
            if m:
                starts[int(m.group(1), 16)] = int(m.group(2))
    return starts


def known_pairs():
    """[(gog, steam, label)] sorted by GOG address."""
    text = subprocess.run(["git", "-C", REPO, "show", PAIRS_BLOB], check=True,
                          capture_output=True, text=True, errors="replace").stdout
    pairs = [(int(m.group(2), 16), int(m.group(1), 16), m.group(3).strip())
             for m in map(PAIR.search, text.splitlines()) if m]
    return sorted(pairs)


def mark(addr, starts):
    return f"0x{addr:08X} " + (f"start, {starts[addr]} bytes" if addr in starts
                               else "NOT a function start")


def by_name(con, queries, starts):
    pairs = known_pairs()
    gogs = [p[0] for p in pairs]
    for q in queries:
        cls, sep, fn = q.partition("::")
        where, args = (("class_name like ? and function_name like ?", (cls, f"%{fn}%"))
                       if sep else ("function_name like ?", (f"%{q}%",)))
        rows = con.execute("select class_name, function_name, address, param_size_bytes "
                           f"from functions where {where} order by address", args).fetchall()
        print(f"## {q}: {len(rows)} named")
        for c, f, gog, psz in rows[:20]:
            print(f"  {c}::{f}  GOG 0x{gog:08X}  argument bytes {psz}")
            i = bisect.bisect_left(gogs, gog)
            for j in (i - 1, i):
                if 0 <= j < len(pairs):
                    g, s, label = pairs[j]
                    print(f"      Steam {mark(gog - g + s, starts)}"
                          f"  (via {label[:44]}, {abs(gog - g):#x} away)")


def by_class(con, classes, starts):
    slots = collections.defaultdict(list)
    with open(os.path.join(K2, "k2-vtable-slots.csv"), newline="") as f:
        for row in csv.reader(f):
            if len(row) >= 4 and row[3].startswith("0x"):
                slots[row[0]].append(int(row[3], 16))
    for cls in classes:
        fns = con.execute("select function_name, address, param_size_bytes from functions "
                          "where class_name = ? order by address", (cls,)).fetchall()
        print(f"## {cls}: {len(fns)} named, {len(slots.get(cls, []))} vtable slots")
        deltas = {k2 - gog for k2 in slots.get(cls, []) for _, gog, _ in fns}
        if not deltas:
            print("   no vtable to anchor on; try the name lookup")
            continue
        best = max(deltas, key=lambda d: sum(gog + d in starts for _, gog, _ in fns))
        hits = sum(gog + best in starts for _, gog, _ in fns)
        print(f"   {hits} of {len(fns)} land on a function start")
        for name, gog, psz in fns:
            print(f"   {name}  Steam {mark(gog + best, starts)}  argument bytes {psz}")


def offsets(con, queries):
    for q in queries:
        for cls, mem, off in con.execute(
                "select class_name, member_name, offset from offsets "
                "where class_name like ? order by class_name, offset", (f"%{q}%",)):
            print(f"{cls}.{mem} +0x{off:x}")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    con = sqlite3.connect(f"file:{sys.argv[1]}?mode=ro", uri=True)
    args = sys.argv[2:]
    if args[0] == "--offsets":
        offsets(con, args[1:])
    elif args[0] == "--class":
        by_class(con, args[1:], function_starts())
    else:
        by_name(con, args, function_starts())


if __name__ == "__main__":
    main()
