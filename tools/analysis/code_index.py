"""Generates docs/llm-docs/code-index from the sources. Run after changing code.

One Markdown page per source file: the file's own description (its leading
comment; for a .cpp without one, that of its header) and its declarations with
line numbers, each with the first sentence of the comment above it. The index
is derived, never edited by hand, so it cannot drift from the code.

    python tools/analysis/code_index.py        (needs: pip install lizard)
"""

from __future__ import annotations

import glob
import os
import re
import shutil

import lizard

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(REPO, "docs", "llm-docs", "code-index")
# (source folder, file patterns, subfolder of the index)
SETS = [
    ("patches/Accessibility", ["*.cpp", "*.h"], ""),
    ("tools/kdev", ["**/*.cs"], "kdev"),
    ("installer/KotorAccessibilityInstaller", ["**/*.cs"], "installer"),
]
NOTE_MAX = 220


def read_lines(path: str) -> list[str]:
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read().split("\n")


def comment_text(line: str) -> str | None:
    """The text of a comment-only line, or None for any other line."""
    s = line.strip()
    for marker in ("///", "//", "/*", "*/", "*"):
        if s.startswith(marker):
            text = s[len(marker):]
            return text.replace("*/", "").strip()
    return None


def description(lines: list[str]) -> str:
    """The leading comment block, before the first line of code."""
    block: list[str] = []
    for line in lines:
        s = line.strip()
        text = comment_text(line)
        if text is not None:
            block.append(text)
            continue
        if block:
            break
        if not s or s.startswith("#") or re.match(r"using [\w.]+;", s):
            continue
        break
    paragraphs, cur = [], []
    for t in block:
        if t:
            cur.append(t)
        elif cur:
            paragraphs.append(" ".join(cur))
            cur = []
    if cur:
        paragraphs.append(" ".join(cur))
    return "\n\n".join(paragraphs)


def note_above(lines: list[str], index: int) -> str:
    """First sentence of the comment directly above line `index` (0-based)."""
    texts: list[str] = []
    i = index - 1
    while i >= 0 and comment_text(lines[i]) is not None:
        texts.insert(0, comment_text(lines[i]) or "")
        i -= 1
    text = " ".join(t for t in texts if t and not set(t) <= set("-=*"))
    text = re.sub(r"\s+", " ", re.sub(r"</?\w+[^>]*>", " ", text)).strip()  # C# XML doc tags
    m = re.match(r"(.+?[.!?])(\s|$)", text)
    first = m.group(1) if m else text
    return first if len(first) <= NOTE_MAX else first[: NOTE_MAX - 3] + "..."


def strip_comments(lines: list[str]) -> list[str]:
    out, in_block = [], False
    for line in lines:
        s = line
        if in_block:
            end = s.find("*/")
            if end < 0:
                out.append("")
                continue
            s, in_block = s[end + 2:], False
        s = re.sub(r"/\*.*?\*/", "", s)
        start = s.find("/*")
        if start >= 0:
            s, in_block = s[:start], True
        s = re.sub(r'"(\\.|[^"\\])*"', '""', s)
        out.append(re.sub(r"//.*", "", s))
    return out


def header_declarations(lines: list[str]) -> tuple[list[tuple[int, str]], int]:
    """Namespace-scope functions, types and aliases of a header; and the number
    of constants and variables, which are counted, not listed."""
    code = strip_comments(lines)
    decls: list[tuple[int, str]] = []
    others = 0
    depth = 0          # braces that are not namespaces
    ns_depths: list[int] = []
    stmt, stmt_line = "", -1
    for n, line in enumerate(code):
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if depth == 0 and re.match(r"(inline\s+)?namespace\b[^;]*\{", s):
            ns_depths.append(0)
            continue
        for ch in line:
            if depth > 0:
                if ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
                continue
            if ch == "}":
                if ns_depths:
                    ns_depths.pop()
                stmt = ""
                continue
            if stmt == "" and ch.isspace():
                continue
            if stmt == "":
                stmt_line = n
            if ch in ";{":
                text = re.sub(r"\s+", " ", stmt).strip()
                if ch == "{":
                    depth = 1
                kind = classify(text)
                if kind:
                    decls.append((stmt_line, kind))
                elif text and not text.startswith(("extern \"C\"", "template")):
                    others += 1
                stmt = ""
                continue
            stmt += ch
        if stmt:
            stmt += " "
    return decls, others


def classify(text: str) -> str | None:
    if not text or text.startswith(("public:", "private:", "protected:")):
        return None
    t = re.sub(r"^template\s*<[^>]*>\s*", "", text)
    m = re.match(r"(?:enum\s+class|enum|struct|class|union)\s+(\w+)", t)
    if m and "(" not in t.split(m.group(1), 1)[0]:
        return t.split(":")[0].strip() if "=" not in t else None
    if t.startswith("using ") and "=" in t:
        return t.split("=")[0].strip()
    if t.startswith("typedef"):
        return t
    paren = t.find("(")
    eq = t.find("=")
    if paren > 0 and (eq < 0 or eq > paren):
        return t
    return None


CS_MODS = (r"(?:(?:public|internal|private|protected|static|sealed|abstract|partial|"
           r"readonly|async|override|virtual|extern|unsafe|new|required)\s+)")
CS_TYPE = re.compile(rf"^\s*{CS_MODS}*(class|record\s+struct|record|struct|enum|interface)\s+\w+")
CS_METHOD = re.compile(rf"^\s*{CS_MODS}+[\w<>\[\],.?]+(?:\s*<[^>]*>)?\s+\w+\s*\(")


def cs_declarations(lines: list[str]) -> list[tuple[int, str]]:
    """Types and methods of a C# file (lizard misreads C# statements)."""
    out = []
    for n, line in enumerate(strip_comments(lines)):
        if CS_TYPE.match(line) or CS_METHOD.match(line):
            text = re.split(r"\{|=>", line, maxsplit=1)[0]
            out.append((n, re.sub(r"\s+", " ", text).strip().rstrip(";")))
    return out


def source_declarations(path: str, lines: list[str]) -> list[tuple[int, str]]:
    if path.endswith(".cs"):
        return cs_declarations(lines)
    out = []
    for fn in lizard.analyze_file(path).function_list:
        first = fn.start_line - 1
        sig, i = [], first
        while i < len(lines) and i < first + 8:
            part = lines[i]
            if "{" in part:
                sig.append(part.split("{", 1)[0])
                break
            sig.append(part)
            i += 1
        out.append((first, re.sub(r"\s+", " ", " ".join(sig)).strip()))
    return sorted(out)


def page(path: str, rel: str, sibling_desc: str) -> str:
    lines = read_lines(path)
    desc = description(lines) or sibling_desc
    name = os.path.basename(path)
    out = [f"# {name} ({len(lines)} lines)", "", f"Source: `{rel}`", ""]
    if desc:
        out += [desc, ""]
    if name.endswith(".h"):
        decls, others = header_declarations(lines)
    else:
        decls, others = source_declarations(path, lines), 0
    if decls:
        out += ["## Declarations (in source order)", ""]
        for n, text in decls:
            note = note_above(lines, n)
            out.append(f"- L{n + 1} `{text}`" + (f": {note}" if note else ""))
        out.append("")
    if others:
        out += [f"Also {others} constants or variables at namespace scope.", ""]
    return "\n".join(out)


def main() -> None:
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    listed = []
    for folder, patterns, sub in SETS:
        base = os.path.join(REPO, folder)
        files = sorted({p for pat in patterns
                        for p in glob.glob(os.path.join(base, pat), recursive=True)
                        if not re.search(r"[\\/](bin|obj)[\\/]", p)})
        target = os.path.join(OUT, sub)
        os.makedirs(target, exist_ok=True)
        for p in files:
            rel = os.path.relpath(p, REPO).replace("\\", "/")
            sibling = ""
            if p.endswith(".cpp") and os.path.exists(p[:-4] + ".h"):
                d = description(read_lines(p[:-4] + ".h"))
                sibling = f"(From {os.path.basename(p[:-4])}.h.) {d}" if d else ""
            with open(os.path.join(target, os.path.basename(p) + ".md"), "w",
                      encoding="utf-8", newline="\r\n") as f:
                f.write(page(p, rel, sibling))
            listed.append(rel)
    with open(os.path.join(OUT, "_files.txt"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(listed) + "\n")
    print(f"{len(listed)} pages in {os.path.relpath(OUT, REPO)}")


if __name__ == "__main__":
    main()
