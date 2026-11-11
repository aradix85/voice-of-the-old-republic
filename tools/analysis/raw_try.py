"""Raw __try blocks outside engine_reads.

The one SEH layer is engine_reads; a module that only reads or writes engine
memory goes through it. This counts the __try blocks elsewhere and picks out
the ones without a call inside: those are candidates for TryRead* / Read*Or0.
A block that calls the engine keeps its own guard.

    python tools/analysis/raw_try.py [REPO] [--list]

--list prints every candidate with its statements on one line.
"""
import os
import re
import sys

NOT_CALLS = {"reinterpret_cast", "static_cast", "const_cast", "sizeof", "if", "for",
             "while", "return"}


def blocks(text):
    """(start offset, body) of every __try block."""
    for m in re.finditer(r"__try\s*\{", text):
        depth, i = 1, m.end()
        while depth and i < len(text):
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        yield m.start(), text[m.end():i - 1]


def is_plain(body):
    code = re.sub(r"//[^\n]*", "", body)
    statements = [s for s in code.split(";") if s.strip()]
    calls = [c for c in re.findall(r"\b([A-Za-z_][\w:]*)\s*\(", code) if c not in NOT_CALLS]
    through_pointer = "reinterpret_cast<PFN" in code
    return not calls and not through_pointer and len(statements) <= 4


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    src = os.path.join(args[0] if args else ".", "patches", "Accessibility")
    total, plain, per_file = 0, [], {}
    for name in sorted(os.listdir(src)):
        if not name.endswith((".cpp", ".h")) or name.startswith("engine_reads"):
            continue
        with open(os.path.join(src, name), encoding="latin-1") as fh:
            text = fh.read()
        for start, body in blocks(text):
            total += 1
            if is_plain(body):
                line = text.count("\n", 0, start) + 1
                plain.append((name, line, " ".join(re.sub(r"//[^\n]*", "", body).split())))
                per_file[name] = per_file.get(name, 0) + 1
    print(f"raw __try blocks outside engine_reads: {total}")
    print(f"without a call inside (candidates): {len(plain)}")
    for name, n in sorted(per_file.items(), key=lambda kv: (-kv[1], kv[0])):
        print(f"  {n:3d}  {name}")
    if "--list" in sys.argv:
        for name, line, body in plain:
            print(f"{name}:{line}: {body[:160]}")


if __name__ == "__main__":
    main()
