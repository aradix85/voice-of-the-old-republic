"""Every engine constant has one definition.

An engine address or struct offset (a constant initialised from acc::addr or
acc::off) that is written down in two files has to be corrected in two places,
and the copy that is forgotten keeps a wrong value without a build error.

    python tools/analysis/engine_constants.py          the check, exit 1 on a finding
    python tools/analysis/engine_constants.py --all    also list plain numeric
                                                        constants that share a name
                                                        across files, for review
"""
import collections
import pathlib
import re
import sys

SRC = pathlib.Path(__file__).resolve().parents[2] / "patches" / "Accessibility"

ENGINE = re.compile(
    r"^[ \t]*(?:static\s+|inline\s+)?(?:const|constexpr)\s+[\w:]+\s+(k\w+)\s*=\s*"
    r"(acc::(?:off|addr)::\w+\([^;]*\))\s*;", re.M)
PLAIN = re.compile(
    r"^(?:static\s+|inline\s+)?(?:const|constexpr)\s+[\w:]+(?:\s+\w+)?\s+(k\w+)\s*=\s*"
    r"([^;{]*?)\s*;", re.M)


def definitions(pattern):
    found = collections.defaultdict(list)
    for path in sorted(SRC.glob("*")):
        if path.suffix not in (".cpp", ".h"):
            continue
        text = path.read_bytes().decode("latin-1")
        for m in pattern.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            found[m.group(1)].append((path.name, line, re.sub(r"\s+", " ", m.group(2))))
    return found


def report(found, title):
    names = {n: d for n, d in found.items() if len(d) > 1}
    print(f"== {title}: {len(names)}")
    for name, defs in sorted(names.items()):
        values = {v for _, _, v in defs}
        print(name + ("" if len(values) == 1 else "   << the values differ"))
        for file, line, value in defs:
            print(f"    {file}:{line}  {value}")
    return len(names)


def main():
    engine = definitions(ENGINE)
    failures = report(engine, "engine constants defined more than once")
    if "--all" in sys.argv[1:]:
        plain = {n: d for n, d in definitions(PLAIN).items()
                 if n not in engine and len({f for f, _, _ in d}) > 1}
        report(plain, "other constants with one name in several files (review)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
