"""Splits one C++ source file into files by job (move only, no logic change).

    python tools/analysis/split_file.py CONFIG.json

CONFIG names the source file, the module and, per new file, its description
and the functions it gets:

    {"source": "patches/Accessibility/room_topology.cpp",
     "module": "room_topology",          ("namespace" if not acc::<module>)
     "files": {"":      {"desc": null, "functions": ["Reset", ...]},
               "doors": {"desc": "Door snapshots ...", "functions": [...]}}}

File key "" is <module>.cpp (desc null keeps the original top comment); any
other key k is <module>_k.cpp. Every free function must be listed once.
Code after the module namespace (extern "C" hooks) is moved whole to the
file named by "tail" (default ""), after its namespace; its functions are
not listed.

What goes where, decided from who uses what:
- A function used only in its own file stays local to it (anonymous
  namespace); one used from another file gets a prototype in
  <module>_internal.h. Functions outside the original anonymous namespaces
  are public (declared in the module's own header) and stay so.
- Constants, types and variables at file scope go to the one file that uses
  them; when several files do, constants and types go to the internal header
  and variables are declared extern there and defined in <module>_state.cpp.
- A comment block standing on its own travels with what follows it.
Afterwards the lines of the new files are compared with the original: only
includes, namespace lines, prototypes, extern lines and file comments may
differ. The original file is overwritten by the core file.
"""

from __future__ import annotations

import collections
import json
import os
import re
import sys

import lizard

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
IDENT = re.compile(r"[A-Za-z_]\w*")


def strip_code(lines: list[str]) -> list[str]:
    """Each line without comments, string and character literals."""
    out, in_block = [], False
    for line in lines:
        s = line
        if in_block:
            end = s.find("*/")
            if end < 0:
                out.append("")
                continue
            s, in_block = s[end + 2:], False
        s = re.sub(r'"(\\.|[^"\\])*"', '""', s)
        s = re.sub(r"'(\\.|[^'\\])'", "''", s)
        s = re.sub(r"/\*.*?\*/", " ", s)
        start = s.find("/*")
        if start >= 0:
            s, in_block = s[:start], True
        out.append(re.sub(r"//.*", "", s))
    return out


def is_comment(line: str) -> bool:
    s = line.strip()
    return s.startswith(("//", "/*", "*")) and not s.startswith("*/") or s.startswith("*/")


class Chunk:
    def __init__(self, lines: list[int], region: str):
        self.lines = lines          # 1-based line numbers, in order
        self.region = region        # "anon" or "named"
        self.stmts: list[dict] = []
        self.names: set[str] = set()
        self.uses: set[str] = set()
        self.dest = ""
        self.lead: list[int] = []   # comment blocks that stood before it

    def code_only(self, code):
        return [n for n in self.lines if code[n - 1].strip()]


def main() -> None:
    cfg = json.load(open(sys.argv[1], encoding="utf-8"))
    src = os.path.join(REPO, cfg["source"])
    folder = os.path.dirname(src)
    module = cfg["module"]
    raw = open(src, encoding="latin-1", newline="").read()
    nl = "\r\n" if "\r\n" in raw else "\n"
    lines = [x.rstrip("\r") for x in raw.split("\n")]
    code = strip_code(lines)
    L = lambda n: lines[n - 1]  # noqa: E731
    C = lambda n: code[n - 1]   # noqa: E731

    target = {}
    for key, spec in cfg["files"].items():
        for f in spec["functions"]:
            if f in target:
                sys.exit(f"{f} staat twee keer in de config")
            target[f] = key

    # Function ranges (with the comment block directly above).
    fn_at = {}
    for fn in lizard.analyze_file(src).function_list:
        fn_at[fn.start_line] = fn

    # One pass: namespaces, free functions, chunks at namespace scope.
    ns_stack: list[str] = []
    depth = 0          # braces opened inside chunks (struct bodies etc.)
    chunks: list[Chunk] = []
    elements: list[tuple] = []   # ("chunk", Chunk) | ("func", name, start, end, region)
    cur: list[int] = []
    preamble: list[int] = []
    funcs: dict[str, dict] = {}

    def region() -> str:
        return "anon" if "anon" in ns_stack else "named"

    def close_chunk():
        nonlocal cur
        if cur:
            ch = Chunk(cur, region())
            chunks.append(ch)
            elements.append(("chunk", ch))
            cur = []

    n = 1
    while n <= len(lines):
        c = C(n).strip()
        if not ns_stack:
            m = re.match(r"namespace\s+([\w:]+)\s*\{$", c)
            if m:
                ns_stack.append("named")
                close_chunk()
            else:
                preamble.append(n)
            n += 1
            continue
        if depth == 0 and n in fn_at:
            fn = fn_at[n]
            # Name relative to the module namespace; "Census::Reset" is a member
            # function defined at namespace scope.
            name = fn.name
            ns_prefix = cfg.get("namespace", "acc::" + module) + "::"
            if name.startswith(ns_prefix):
                name = name[len(ns_prefix):]
            start = n
            if cur and cur[-1] == start - 1 and re.match(r"\s*template\s*<", L(cur[-1])):
                start = cur.pop()
            # take back the comment lines directly above from the open chunk
            while cur and is_comment(L(cur[-1])) and cur[-1] == start - 1:
                start = cur.pop()
            close_chunk()
            if name not in target:
                sys.exit(f"geen bestemming voor functie {name} (regel {n})")
            key, i = name, 2   # overloads: Speak, Speak#2, ... (same file)
            while key in funcs:
                key, i = f"{name}#{i}", i + 1
            funcs[key] = {"start": start, "sig": fn.start_line, "end": fn.end_line,
                          "region": region(), "file": target[name],
                          "method": "::" in name}
            elements.append(("func", key))
            n = fn.end_line + 1
            continue
        if depth == 0 and re.match(r"namespace\s*\{$", c):
            close_chunk()
            ns_stack.append("anon")
            n += 1
            continue
        if depth == 0 and c.startswith("}") and re.fullmatch(r"\}\s*", c):
            close_chunk()
            ns_stack.pop()
            n += 1
            continue
        if not L(n).strip():
            if depth == 0:
                close_chunk()
            n += 1
            continue
        cur.append(n)
        depth += C(n).count("{") - C(n).count("}")
        n += 1
    close_chunk()
    missing = set(target) - set(funcs)
    if missing:
        sys.exit(f"in de config maar niet gevonden: {sorted(missing)}")

    # A comment-only chunk travels with the next element of its region.
    merged: list[tuple] = []
    pending: list[int] = []
    for el in elements:
        if el[0] == "chunk" and all(is_comment(L(k)) for k in el[1].lines):
            pending += el[1].lines
            continue
        if pending:
            if el[0] == "chunk":
                el[1].lines = pending + el[1].lines
                el[1].lead = pending
            elif all(not L(k).strip() for k in range(pending[-1] + 1, funcs[el[1]]["start"])):
                funcs[el[1]]["start"] = pending[0]
                funcs[el[1]]["lead"] = pending
            else:
                # A namespace line stands between the comment and the
                # function: carry the comment, not the lines in between.
                funcs[el[1]]["extra_lead"] = pending
            pending = []
        merged.append(el)
    if pending:
        print(f"LET OP: losse commentaarregels aan het eind vervallen: {pending[0]}-{pending[-1]}")
    elements = merged
    chunks = [el[1] for el in elements if el[0] == "chunk"]

    # Statements and declared names per chunk.
    for ch in chunks:
        stmt, d, pp = [], 0, False
        for k in ch.lines:
            s = C(k).strip()
            if not s and not pp:
                continue
            stmt.append(k)
            if pp or s.startswith("#"):
                # A preprocessor line, with its backslash continuations.
                pp = s.endswith("\\")
                if not pp:
                    ch.stmts.append({"lines": stmt}); stmt = []  # noqa: E702
                continue
            d += s.count("{") - s.count("}")
            if d == 0 and (s.endswith(";") or s.endswith("}")):
                ch.stmts.append({"lines": stmt}); stmt = []  # noqa: E702
        if stmt:
            sys.exit(f"onvolledige declaratie in regels {stmt[0]}-{stmt[-1]}")
        for st in ch.stmts:
            text = " ".join(C(k).strip() for k in st["lines"])
            st["text"] = text
            t = re.sub(r"^(static|inline)\s+", "", text)
            t = re.sub(r"^template\s*<[^>]*>\s*", "", t)
            m = re.match(r"(struct|class|union|enum(?:\s+class)?)\s+(\w+)", t)
            if t.startswith("#define"):
                st["kind"], st["name"] = "const", t.split()[1].split("(")[0]
            elif re.match(r"enum\s*(:\s*\w+\s*)?\{", t):  # anonymous enum
                st["kind"], st["name"] = "type", None
                body = t[t.index("{") + 1:t.rindex("}")]
                st["enumerators"] = [IDENT.match(e.strip()).group(0)
                                     for e in body.split(",") if e.strip()]
            elif m and (t.endswith("};") or t.endswith(";")) and "(" not in t.split(m.group(2))[0]:
                st["kind"], st["name"] = "type", m.group(2)
                if m.group(1).startswith("enum") and "{" in t:
                    body = t[t.index("{") + 1:t.rindex("}")]
                    st["enumerators"] = [IDENT.match(e.strip()).group(0)
                                         for e in body.split(",") if e.strip()]
            elif t.startswith("using namespace"):
                st["kind"], st["name"] = "type", None
            elif t.startswith("using "):
                mm = re.match(r"using\s+(\w+)\s*=", t) or re.match(r"using\s+[\w:]*::(\w+)\s*;", t)
                st["kind"], st["name"] = "type", mm.group(1)
            elif t.startswith("typedef"):
                mm = re.search(r"\(\s*(?:\w+\s*)?\*\s*(\w+)\s*\)", t) or re.search(r"(\w+)\s*;$", t)
                st["kind"], st["name"] = "type", mm.group(1)
            else:
                paren, eq = t.find("("), t.find("=")
                head = re.split(r"[=\[{;(]", t, maxsplit=1)[0].strip()
                name = IDENT.findall(head)[-1] if IDENT.findall(head) else None
                if paren > 0 and (eq < 0 or eq > paren) and name in funcs:
                    st["kind"], st["name"] = "proto", name
                else:
                    const = re.match(r"(static\s+)?(constexpr|const)\b", text) is not None
                    st["kind"], st["name"] = ("const" if const else "var"), name
            if st["name"]:
                ch.names.add(st["name"])
            ch.names.update(st.get("enumerators", []))
        body = " ".join(C(k) for k in ch.lines)
        ch.uses = set(IDENT.findall(body)) - ch.names
        kinds = {st["kind"] for st in ch.stmts}
        if "proto" in kinds and kinds != {"proto"}:
            sys.exit(f"voorwaartse declaratie gemengd met andere code, regel {ch.lines[0]}")

    fbody = {f: set(IDENT.findall(" ".join(C(k) for k in range(v["sig"], v["end"] + 1))))
             for f, v in funcs.items()}

    # Where each chunk goes. Forward declarations follow their function.
    owner = {}
    for ch in chunks:
        for nm in ch.names:
            owner[nm] = ch
    exports = cfg.get("export", {})   # constant name -> file that defines it
    for ch in chunks:
        ch.export = bool(ch.names & set(exports))
        if ch.export:
            ch.dest = exports[next(iter(ch.names & set(exports)))]
            continue
        if ch.stmts and ch.stmts[0]["kind"] == "proto":
            ch.dest = funcs[ch.stmts[0]["name"]]["file"]
            continue
        users = {funcs[f]["file"] for f in funcs if fbody[f] & ch.names}
        ch.dest = users.pop() if len(users) == 1 else ("H" if users else "?")
    changed = True
    while changed:
        changed = False
        for ch in chunks:
            if ch.dest in ("H", "?") or ch.export or (ch.stmts and ch.stmts[0]["kind"] == "proto"):
                continue
            users = {o.dest for o in chunks if o is not ch and o.uses & ch.names}
            users |= {funcs[f]["file"] for f in funcs if fbody[f] & ch.names}
            if len(users - {ch.dest}) > 0:
                ch.dest, changed = "H", True
        for ch in chunks:
            if ch.dest == "H":
                for nm in ch.uses:
                    o = owner.get(nm)
                    if o and o.dest not in ("H",) and not o.export and not (o.stmts and o.stmts[0]["kind"] == "proto"):
                        o.dest, changed = "H", True
    for ch in chunks:
        if ch.dest == "?":
            print(f"LET OP: regels {ch.lines[0]}-{ch.lines[-1]} door niemand gebruikt: naar het kernbestand")
            ch.dest = ""

    # Which functions are shared (used from another file or a header chunk).
    header_uses = set().union(*[ch.uses for ch in chunks if ch.dest == "H"]) if chunks else set()
    protos_text = {ch.stmts[0]["name"]: ch.stmts[0]["text"]
                   for ch in chunks if ch.stmts and ch.stmts[0]["kind"] == "proto"}
    pub_path = os.path.join(folder, f"{module}.h")
    public_text = open(pub_path, encoding="latin-1").read() if os.path.exists(pub_path) else ""
    for f, v in funcs.items():
        b = f.split("#")[0]
        users = {funcs[g]["file"] for g in funcs if g != f and b in fbody[g]}
        users |= {ch.dest for ch in chunks if ch.dest not in ("H", "drop") and b in ch.uses}
        # Outside the anonymous namespaces but not in the module's header: it
        # still needs a prototype when another file calls it.
        declared = v["region"] == "named" and (v["method"] or
                                               re.search(rf"\b{b}\s*\(", public_text))
        v["shared"] = not declared and (bool(users - {v["file"]}) or b in header_uses)
    for i, el in enumerate(elements):  # a shared function's forward declaration becomes its prototype
        ch = el[1] if el[0] == "chunk" else None
        if ch and ch.stmts and ch.stmts[0]["kind"] == "proto" and funcs[ch.stmts[0]["name"]]["shared"]:
            ch.dest = "drop"
            # The comment blocks that stood before the declaration move on to
            # what follows it.
            if ch.lead and i + 1 < len(elements):
                nxt = elements[i + 1]
                if nxt[0] == "chunk":
                    nxt[1].lines = ch.lead + nxt[1].lines
                else:
                    funcs[nxt[1]]["extra_lead"] = ch.lead

    def signature(f: str) -> str:
        if f in protos_text:
            return protos_text[f]
        v, parts, k = funcs[f], [], funcs[f]["sig"]
        while True:
            s = C(k)
            if "{" in s:
                parts.append(s.split("{", 1)[0])
                break
            parts.append(s)
            k += 1
        # A static function another file calls loses its internal linkage.
        text = re.sub(r"^static\s+", "", re.sub(r"\s+", " ", " ".join(parts)).strip())
        if re.search(r"\([^)]*=", text):
            # Default arguments move to the prototype; the definition drops them.
            v["strip_defaults"] = k
        return text + ";"

    # Emit.
    out = collections.defaultdict(list)       # file key -> local section lines
    hdr_body, state_defs = [], []

    def emit_chunk(ch: Chunk, sink: list[str]) -> None:
        if sink and sink[-1] != "":
            sink.append("")
        if ch.dest != "H":
            sink += [L(k) for k in ch.lines]
            return
        var_first = {st["lines"][0]: st for st in ch.stmts if st["kind"] == "var"}
        var_lines = {k for st in var_first.values() for k in st["lines"]}
        for k in ch.lines:
            if k in var_first:
                st = var_first[k]
                decl = re.sub(r"^static\s+", "", st["text"])
                decl = re.split(r"\s*(=|\{)", decl, maxsplit=1)[0].rstrip(";").strip()
                trail = re.search(r"(//.*)$", L(k)) if len(st["lines"]) == 1 else None
                sink.append(f"extern {decl};" + (f"  {trail.group(1)}" if trail else ""))
                for j in st["lines"]:
                    d = re.sub(r"^(\s*)static\s+", r"\1", L(j))
                    if trail:
                        d = d[:d.index(trail.group(1))].rstrip()
                    state_defs.append(d)
            elif k not in var_lines:
                sink.append(L(k))

    exported = collections.defaultdict(list)   # file key -> exported definitions
    for el in elements:
        if el[0] == "chunk" and el[1].export:
            # Defined in its file (named namespace), declared extern in the header.
            emit_chunk(el[1], exported[el[1].dest])
            for st in el[1].stmts:
                m = re.match(r"(?:static\s+)?(?:constexpr|const)\s+(.+?)\s+(\w+)\s*=", st["text"])
                if m:
                    hdr_body.append(f"extern const {m.group(1)} {m.group(2)};")
        elif el[0] == "chunk" and el[1].dest == "H":
            emit_chunk(el[1], hdr_body)
        elif el[0] == "chunk" and el[1].dest not in ("drop",):
            emit_chunk(el[1], out[el[1].dest])

    def fn_lines(f: str) -> list[str]:
        v = funcs[f]
        out_lines = [L(k) for k in v.get("extra_lead", [])]
        if out_lines:
            out_lines.append("")
        for k in range(v["start"], v["end"] + 1):
            line = L(k)
            if "strip_defaults" in v and v["sig"] <= k <= v["strip_defaults"]:
                line = re.sub(r"\s*=\s*[^,()=]+(?=[,)])", "", line)
            if k == v["sig"] and v["shared"]:
                line = re.sub(r"^static\s+", "", line)
            out_lines.append(line)
        return out_lines

    # Everything before the first namespace from the first #include on (the
    # includes with their comments, file-wide using declarations) goes to the
    # internal header; the module's own header is included there anyway.
    head_end = next((k for k in range(1, len(lines) + 1)
                     if re.match(r"\s*namespace\s+[\w:]+\s*\{", C(k))), len(lines))
    pre = [k for k in preamble if k < head_end]
    first_inc = next((i for i, k in enumerate(pre) if L(k).startswith("#")), len(pre))
    includes = [L(k) for k in pre[first_inc:] if f'"{module}.h"' not in L(k)]
    while includes and not includes[-1].strip():
        includes.pop()
    top_comment = [L(k) for k in pre[:first_inc]]
    while top_comment and not top_comment[-1].strip():
        top_comment.pop()
    tail = [L(k) for k in preamble if k > head_end]
    while tail and not tail[0].strip():
        tail.pop(0)
    while tail and not tail[-1].strip():
        tail.pop()
    tail_key = cfg.get("tail", "")

    def fmt_desc(desc: str) -> list[str]:
        return ["// " + x if x else "//" for x in desc.split("\n")]

    for f in funcs:  # marks shared definitions whose default arguments move
        if funcs[f]["shared"]:
            signature(f)

    def is_inline_shared(f: str) -> bool:
        return funcs[f]["shared"] and C(funcs[f]["sig"]).lstrip().startswith("inline ")
    written = {}
    ns = cfg.get("namespace", "acc::" + module)
    for key, spec in cfg["files"].items():
        name = f"{module}.cpp" if key == "" else f"{module}_{key}.cpp"
        head = top_comment if spec.get("desc") is None else fmt_desc(spec["desc"])
        body = head + ["", f'#include "{module}_internal.h"', "", f"namespace {ns} {{", ""]
        def class_in_header(f: str) -> bool:
            o = owner.get(f.split("::")[0])
            if o is not None and o.dest not in ("H", funcs[f]["file"]):
                print(f"LET OP: {f} staat in een ander bestand dan zijn klasse")
            return o is not None and o.dest == "H"

        local_fns = [f for f in funcs if funcs[f]["file"] == key and funcs[f]["region"] == "anon"
                     and not funcs[f]["shared"]
                     and not (funcs[f]["method"] and class_in_header(f))]
        if out[key] or local_fns:
            body += ["namespace {", ""] + out[key] + [""]
            for f in sorted(local_fns, key=lambda g: funcs[g]["start"]):
                body += fn_lines(f) + [""]
            body += ["}  // namespace", ""]
        if exported[key]:
            body += exported[key] + [""]
        for f in sorted([f for f in funcs if funcs[f]["file"] == key and f not in local_fns
                         and not is_inline_shared(f)],
                        key=lambda g: funcs[g]["start"]):
            body += fn_lines(f) + [""]
        body += [f"}}  // namespace {ns}", ""]
        if key == tail_key and tail:
            body += tail + [""]
        written[name] = body

    hdr = [f"// Internal to the {module} module ({module}*.cpp); no other file includes",
           f"// it. The public interface is {module}.h. Here: what the files share.",
           "", "#pragma once", "", f'#include "{module}.h"', ""] + includes + \
          ["", f"namespace {ns} {{", ""] + hdr_body + ["", "// Helpers shared across the files, by the file that defines them."]
    for key in cfg["files"]:
        ps = []
        for f in sorted(funcs, key=lambda g: funcs[g]["start"]):
            if funcs[f]["file"] != key or not funcs[f]["shared"]:
                continue
            # An inline function other files call must be defined in the header.
            ps += ([""] + fn_lines(f)) if is_inline_shared(f) else [signature(f)]
        if ps:
            fname = f"{module}.cpp" if key == "" else f"{module}_{key}.cpp"
            hdr += ["", f"// {fname}"] + ps
    hdr += ["", f"}}  // namespace {ns}", ""]
    written[f"{module}_internal.h"] = hdr
    if state_defs:
        written[f"{module}_state.cpp"] = [
            f"// The state the {module} files share, defined in one place; each item is",
            f"// described where it is declared, in {module}_internal.h.", "",
            f'#include "{module}_internal.h"', "", f"namespace {ns} {{", ""] + state_defs + \
            ["", f"}}  // namespace {ns}", ""]

    for name, body in written.items():
        text = re.sub(r"\n{3,}", "\n\n", "\n".join(body))
        with open(os.path.join(folder, name), "w", encoding="latin-1", newline="") as f:
            f.write(text.replace("\n", nl))
        print(f"{name}: {text.count(chr(10))} regels")

    # Line comparison with the original.
    def bag(texts):
        return collections.Counter(t.strip() for t in texts if t.strip())
    a = bag(lines)
    b = bag(x for body in written.values() for x in body)
    print("--- alleen in het origineel:")
    for t, c in sorted((a - b).items()):
        print(f"  {c}x {t}")
    print("--- alleen in de nieuwe bestanden (verwacht: includes, namespaces, prototypes, extern):")
    for t, c in sorted((b - a).items()):
        if not (t.startswith(("#include", "namespace", "}  // namespace", "extern ", "#pragma"))
                or t.startswith("//")):
            print(f"  {c}x {t}")
    print("gedeeld:", sorted(f for f in funcs if funcs[f]["shared"]))


if __name__ == "__main__":
    main()
