"""Tweede meting: hulpfuncties met dezelfde naam in meerdere bestanden, en
bijna-kopieen (namen en getallen genormaliseerd, vensters van 6 regels)."""
import collections, hashlib, json, os, re, sys

out = sys.argv[1]
data = json.load(open(os.path.join(out, "analyse.json")))
src = sys.argv[2]

# 1. Same short function name defined in several files (not methods of classes, not Tick/Init hooks).
by_name = collections.defaultdict(list)
for f in data["funcs"]:
    short = f["name"].split("::")[-1]
    by_name[short].append(f)
skip = {"Tick", "Init", "Reset", "Get", "Install", "Shutdown", "OnEnter", "OnExit", "Poll", "Update", "Run",
        "Clear", "Speak", "Announce", "Next", "IsActive", "Arm", "Disarm", "Begin", "End", "Draw", "OnTick"}
print("== zelfde functienaam in meerdere bestanden (mogelijke dubbele hulpfuncties)")
rows = []
for name, fs in by_name.items():
    files = sorted(set(f["file"] for f in fs))
    if len(files) < 2 or name in skip or name.startswith("On") or name.startswith("~"):
        continue
    if all(f["nloc"] <= 2 for f in fs):
        continue
    rows.append((len(files), name, files, sum(f["nloc"] for f in fs)))
for n, name, files, nloc in sorted(rows, key=lambda r: (-r[0], -r[3]))[:60]:
    print(f"  {name} ({n} bestanden, {nloc} nloc): {', '.join(files[:6])}{' ...' if len(files) > 6 else ''}")

# 2. Type-2 clones.
N = 6
KW = set("if else for while return const auto void int bool float char unsigned uint32_t uint8_t size_t static "
         "struct break continue switch case default nullptr true false sizeof __try __except std string vector "
         "do new delete".split())
def norm(line):
    line = re.sub(r"//.*", "", line).strip()
    if not line or line in ("{", "}", "};", "} else {") or line.startswith("#"):
        return None
    line = re.sub(r'"([^"\\]|\\.)*"', "S", line)
    line = re.sub(r"\b0x[0-9a-fA-F]+\b|\b\d+(\.\d+)?f?\b", "N", line)
    line = re.sub(r"[A-Za-z_][A-Za-z0-9_]*", lambda m: m.group(0) if m.group(0) in KW else "I", line)
    return re.sub(r"\s+", "", line)

win = collections.defaultdict(list)
for dp, _, fs in os.walk(src):
    for fn in fs:
        if not fn.endswith(".cpp") or fn.startswith("strings_") or "effect_names" in fn:
            continue
        p = os.path.join(dp, fn)
        lines = open(p, encoding="latin-1").read().splitlines()
        seq = [(i + 1, norm(l)) for i, l in enumerate(lines)]
        seq = [(i, l) for i, l in seq if l and len(l) > 6]
        for k in range(len(seq) - N + 1):
            chunk = "\n".join(l for _, l in seq[k:k + N])
            if len(chunk) < 160:
                continue
            win[hashlib.md5(chunk.encode()).hexdigest()].append((fn, seq[k][0]))
pairs = collections.Counter()
for locs in win.values():
    fsets = sorted(set(f for f, _ in locs))
    if len(fsets) < 2:
        continue
    for i in range(len(fsets)):
        for j in range(i + 1, len(fsets)):
            pairs[(fsets[i], fsets[j])] += 1
print("\n== bestandsparen met de meeste bijna-kopieen (vensters van 6 regels)")
for (a, b), n in pairs.most_common(40):
    print(f"  {n:4d}  {a} <-> {b}")
