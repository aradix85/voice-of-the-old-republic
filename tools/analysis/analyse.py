"""Meet de broncode van de mod: functies, complexiteit, duplicaten, includes, ongebruikte functies.

Gebruik: python analyse.py REPO_PAD UITVOERMAP
"""
import collections
import hashlib
import json
import os
import re
import subprocess
import sys

import lizard

repo, out = sys.argv[1], sys.argv[2]
src = os.path.join(repo, "patches", "Accessibility")
os.makedirs(out, exist_ok=True)

files = sorted(os.path.join(dp, f) for dp, _, fs in os.walk(src) for f in fs
               if f.endswith((".cpp", ".h")))
rel = lambda p: os.path.relpath(p, src).replace("\\", "/")

# Our own additions vs upstream (v0.9.1): lines added per file.
ours = {}
try:
    ns = subprocess.run(["git", "-C", repo, "diff", "--numstat", "v0.9.1", "HEAD", "--", "patches/Accessibility"],
                        capture_output=True, text=True).stdout
    for line in ns.splitlines():
        a, d, f = line.split("\t")
        ours[f.split("patches/Accessibility/")[-1]] = int(a) if a.isdigit() else 0
except Exception:
    pass

# --- lizard: functions
funcs = []
file_nloc = {}
for p in files:
    try:
        info = lizard.analyze_file(p)
    except Exception as e:
        print("lizard fout", p, e)
        continue
    file_nloc[rel(p)] = info.nloc
    for fn in info.function_list:
        funcs.append({"file": rel(p), "name": fn.name, "long": fn.long_name, "line": fn.start_line,
                      "nloc": fn.nloc, "ccn": fn.cyclomatic_complexity, "params": fn.parameter_count})

total_nloc = sum(file_nloc.values())
print(f"bestanden {len(files)}, nloc {total_nloc}, functies {len(funcs)}")
ccns = sorted(f["ccn"] for f in funcs)
nlocs = sorted(f["nloc"] for f in funcs)
med = lambda v: v[len(v) // 2] if v else 0
print(f"mediaan ccn {med(ccns)}, mediaan nloc {med(nlocs)}")
for lim in (15, 30, 50):
    print(f"functies ccn > {lim}: {sum(1 for c in ccns if c > lim)}")
for lim in (100, 200, 400):
    print(f"functies nloc > {lim}: {sum(1 for c in nlocs if c > lim)}")

print("\n== 25 zwaarste functies (ccn)")
for f in sorted(funcs, key=lambda f: -f["ccn"])[:25]:
    print(f"  {f['ccn']:4d} ccn {f['nloc']:5d} nloc  {f['file']}:{f['line']} {f['name']}")
print("\n== 15 langste functies (nloc)")
for f in sorted(funcs, key=lambda f: -f["nloc"])[:15]:
    print(f"  {f['nloc']:5d} nloc {f['ccn']:4d} ccn  {f['file']}:{f['line']} {f['name']}")
print("\n== 20 grootste bestanden (nloc), met onze toegevoegde regels")
for f, n in sorted(file_nloc.items(), key=lambda kv: -kv[1])[:20]:
    print(f"  {n:6d}  {f}" + (f"  (ons: +{ours[f]})" if ours.get(f) else ""))

# --- duplicates: normalized windows of N lines
N = 10
norm_lines = {}
def normalize(line):
    line = re.sub(r"//.*", "", line).strip()
    if not line or line in ("{", "}", "};", "} else {", "break;", "return;", "continue;") or line.startswith("#include"):
        return None
    line = re.sub(r"\s+", " ", line)
    return line

windows = collections.defaultdict(list)
for p in files:
    with open(p, encoding="latin-1") as fh:
        raw = fh.read().splitlines()
    seq = [(i + 1, normalize(l)) for i, l in enumerate(raw)]
    seq = [(i, l) for i, l in seq if l]
    norm_lines[rel(p)] = seq
    for k in range(len(seq) - N + 1):
        chunk = "\n".join(l for _, l in seq[k:k + N])
        if len(chunk) < 250:
            continue
        h = hashlib.md5(chunk.encode()).hexdigest()
        windows[h].append((rel(p), seq[k][0]))

# merge overlapping windows into clone groups (by file pair and start lines)
groups = collections.defaultdict(set)
for h, locs in windows.items():
    if len(locs) < 2:
        continue
    key = tuple(sorted(set(f for f, _ in locs)))
    for loc in locs:
        groups[(h,)].add(loc)
# collapse: keep groups whose location set is not adjacent continuation of a previous one
seen_starts = set()
clones = []
for h, locs in windows.items():
    if len(locs) < 2:
        continue
    locs = sorted(set(locs))
    sig = tuple((f, l // 15) for f, l in locs)
    if sig in seen_starts:
        continue
    seen_starts.add(sig)
    clones.append(locs)
cross = [c for c in clones if len(set(f for f, _ in c)) > 1]
dup_lines = collections.Counter()
for c in clones:
    for f, _ in c:
        dup_lines[f] += 1
print(f"\n== duplicaten: {len(clones)} blokken van >= {N} gelijke regels, {len(cross)} over bestanden heen")
print("bestanden met de meeste dubbele blokken:")
for f, n in dup_lines.most_common(15):
    print(f"  {n:4d}  {f}")
print("voorbeelden over bestanden heen:")
for c in sorted(cross, key=lambda c: -len(c))[:20]:
    print("  " + " | ".join(f"{f}:{l}" for f, l in c[:4]) + (f" (+{len(c)-4})" if len(c) > 4 else ""))

# --- includes
inc = {}
for p in files:
    with open(p, encoding="latin-1") as fh:
        inc[rel(p)] = re.findall(r'^\s*#\s*include\s+"([^"]+)"', fh.read(), re.M)
fanin = collections.Counter(h for v in inc.values() for h in v)
print("\n== includes: meest gebruikte headers (fan-in)")
for h, n in fanin.most_common(12):
    print(f"  {n:4d}  {h}")
print("meeste includes per bestand (fan-out):")
for f, v in sorted(inc.items(), key=lambda kv: -len(kv[1]))[:12]:
    print(f"  {len(v):4d}  {f}")
# header cycles (headers including headers)
hdr = {f: [h for h in v if h.endswith(".h")] for f, v in inc.items() if f.endswith(".h")}
cycles = set()
def dfs(n, stack, onstack):
    for m in hdr.get(n, []):
        if m in onstack:
            cyc = stack[stack.index(m):] + [m]
            cycles.add(" -> ".join(cyc))
        elif len(stack) < 12:
            dfs(m, stack + [m], onstack | {m})
for h in hdr:
    dfs(h, [h], {h})
print(f"include-cycli tussen headers: {len(cycles)}")
for c in list(cycles)[:8]:
    print("  " + c)

# --- possibly unused functions: name occurs once in the whole tree (+ hooks/exports)
tokens = collections.Counter()
extra = [os.path.join(src, f) for f in os.listdir(src) if f.endswith((".toml", ".def"))]
for p in files + extra:
    with open(p, encoding="latin-1") as fh:
        tokens.update(re.findall(r"[A-Za-z_][A-Za-z0-9_]*", fh.read()))
unused = []
for f in funcs:
    short = f["name"].split("::")[-1]
    if short in ("main", "DllMain") or short.startswith("operator") or short.startswith("~"):
        continue
    if tokens[short] <= 1:
        unused.append(f)
print(f"\n== mogelijk ongebruikte functies (naam komt maar 1 keer voor): {len(unused)}")
for f in unused[:40]:
    print(f"  {f['file']}:{f['line']} {f['name']} ({f['nloc']} nloc)")

# --- TODO / FIXME / HACK markers
marks = collections.Counter()
for p in files:
    with open(p, encoding="latin-1") as fh:
        for m in re.findall(r"\b(TODO|FIXME|HACK|XXX|WORKAROUND)\b", fh.read()):
            marks[m] += 1
print("\n== markeringen:", dict(marks))

json.dump({"funcs": funcs, "file_nloc": file_nloc, "clones": cross[:500], "unused": unused, "ours": ours},
          open(os.path.join(out, "analyse.json"), "w"), indent=1)
