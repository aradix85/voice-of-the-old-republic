"""Runs clang-tidy over patches/Accessibility and reports the distinct findings.

    python tools/analysis/clang_tidy.py                 every .cpp
    python tools/analysis/clang_tidy.py a.cpp b.cpp     these files only

The checks are in .clang-tidy at the repo root. Findings go to
build/analysis/clang_tidy.txt; the console gets the count per check.
Needs LLVM (clang-tidy) and the Visual Studio C++ build tools.
"""
import collections
import concurrent.futures
import json
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(REPO, "patches", "Accessibility")
COMMON = os.path.join(REPO, "third_party", "Kotor-Patch-Manager", "Patches", "Common")
LIB = os.path.join(REPO, "third_party", "Kotor-Patch-Manager", "lib")
OUT = os.path.join(REPO, "build", "analysis")
LLVM = os.path.join(os.environ.get("ProgramFiles", r"C:\Program Files"), "LLVM", "bin")

FINDING = re.compile(r"^(?P<where>.+?:\d+:\d+): warning: (?P<msg>.*) \[(?P<check>[\w.,-]+)\]$")


def msvc_environment() -> dict[str, str]:
    """The x86 compiler environment, so clang finds the MSVC and SDK headers."""
    vswhere = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio",
                           "Installer", "vswhere.exe")
    vs = subprocess.run(
        [vswhere, "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        capture_output=True, text=True, check=True).stdout.strip()
    vcvars = os.path.join(vs, "VC", "Auxiliary", "Build", "vcvars32.bat")
    dump = subprocess.run(f'cmd /c ""{vcvars}" >nul 2>&1 && set"', capture_output=True,
                          text=True, check=True).stdout
    return dict(line.split("=", 1) for line in dump.splitlines() if "=" in line)


def write_compile_commands(files: list[str]) -> None:
    entries = [{
        "directory": SRC,
        "file": path,
        "command": (f'clang-cl -m32 /nologo /c /EHsc /std:c++17 /MD /W4 '
                    f'"/I{SRC}" "/I{COMMON}" "/I{LIB}" "{path}"'),
    } for path in files]
    with open(os.path.join(OUT, "compile_commands.json"), "w", encoding="utf-8") as f:
        json.dump(entries, f, indent=1)


def run_one(path: str, env: dict[str, str]) -> list[str]:
    result = subprocess.run(
        [os.path.join(LLVM, "clang-tidy.exe"), "-p", OUT, "--quiet", path],
        capture_output=True, text=True, env=env, cwd=REPO, errors="replace", check=False)
    return result.stdout.splitlines()


def main() -> int:
    os.makedirs(OUT, exist_ok=True)
    names = sys.argv[1:] or sorted(n for n in os.listdir(SRC) if n.endswith(".cpp"))
    files = [os.path.join(SRC, n) for n in names]
    write_compile_commands(files)
    env = msvc_environment()

    findings: set[tuple[str, str, str]] = set()
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as pool:
        for lines in pool.map(lambda p: run_one(p, env), files):
            for line in lines:
                m = FINDING.match(line)
                # A finding in a header repeats for every file that includes it.
                if m and SRC.lower() in m["where"].lower():
                    findings.add((m["where"][len(REPO) + 1:], m["check"], m["msg"]))

    report = os.path.join(OUT, "clang_tidy.txt")
    with open(report, "w", encoding="utf-8") as f:
        f.writelines(f"{where}: {msg} [{check}]\n" for where, check, msg in sorted(findings))

    print(f"{len(files)} files, {len(findings)} distinct findings -> {report}")
    per_check = collections.Counter(check for _, check, _ in findings)
    for check, count in per_check.most_common():
        print(f"{count:5}  {check}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
