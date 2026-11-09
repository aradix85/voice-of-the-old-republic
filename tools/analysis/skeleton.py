"""Skelet van een bronbestand: alles wat niet in een functie staat (typen,
constanten, gegevens, namespaces), zonder commentaar, met regelnummers; de
functies zelf als een regel met naam en regelbereik.

Gebruik: python skeleton.py BESTAND.cpp
"""
import re
import sys

import lizard

pad = sys.argv[1]
with open(pad, encoding="latin-1") as f:
    regels = f.read().splitlines()
funcs = sorted(lizard.analyze_file(pad).function_list, key=lambda fn: fn.start_line)
start = {fn.start_line: fn for fn in funcs}
i = 1
in_comment = False
while i <= len(regels):
    if i in start:
        fn = start[i]
        print(f"{fn.start_line}-{fn.end_line} FUNC {fn.name}")
        i = fn.end_line + 1
        continue
    r = regels[i - 1]
    code = r
    if in_comment:
        if "*/" in code:
            code = code.split("*/", 1)[1]
            in_comment = False
        else:
            code = ""
    if "/*" in code and "*/" not in code.split("/*", 1)[1]:
        code = code.split("/*", 1)[0]
        in_comment = True
    code = re.sub(r"//.*", "", code).rstrip()
    if code.strip():
        print(f"{i}: {code}")
    i += 1
