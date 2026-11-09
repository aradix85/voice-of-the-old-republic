"""Splitsplan voor een bronbestand: welke functies elkaar aanroepen en welke
gedeelde gegevens (variabelen op bestandsniveau) ze gebruiken.

Gebruik: python split_plan.py BESTAND.cpp [--groepen] [--kern N]

Uitvoer, per functie (in bronvolgorde): regels, lengte, welke functies uit
hetzelfde bestand ze aanroept, en welke variabelen op bestandsniveau ze
leest of schrijft. Met --groepen ook de samenhangende groepen: functies die
via aanroepen of gedeelde variabelen aan elkaar vastzitten. Een groep die
los staat van de rest kan zonder meer een eigen bestand worden; wat tussen
groepen gedeeld wordt, komt in een interne kop (<module>_internal.h).

Met --kern N tellen functies en variabelen die door N of meer functies
gebruikt worden als gedeelde kern: ze verbinden de groepen niet meer en
worden apart genoemd. Dat is wat in de interne kop komt.
"""
import argparse
import collections
import re
import sys

import lizard


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bestand")
    ap.add_argument("--groepen", action="store_true")
    ap.add_argument("--kern", type=int, default=0)
    args = ap.parse_args()

    with open(args.bestand, encoding="latin-1") as f:
        tekst = f.read()
    regels = tekst.splitlines()
    info = lizard.analyze_file(args.bestand)
    funcs = sorted(info.function_list, key=lambda fn: fn.start_line)
    kort = lambda fn: fn.name.split("::")[-1]  # noqa: E731

    # Variables at file scope: declarations outside every function body and
    # outside struct/class/enum bodies (only namespace braces may be open).
    binnen = set()
    for fn in funcs:
        binnen.update(range(fn.start_line, fn.end_line + 1))
    var_re = re.compile(r"^\s*(?:static\s+|constexpr\s+|const\s+|inline\s+)*[\w:<>,\s\*&]+?[\s\*&](\w+)\s*(?:\[[^\]]*\])?\s*(?:=[^=]|\{|;)")
    variabelen = {}
    stapel = []  # per open brace: True when it opened a namespace
    for i, r in enumerate(regels, start=1):
        code = re.sub(r"//.*", "", r)
        alleen_namespaces = all(stapel)
        if (i not in binnen and alleen_namespaces
                and not code.lstrip().startswith(("#", "namespace", "using", "struct", "class", "enum", "}", "return"))):
            m = var_re.match(code)
            if m and "(" not in code.split("=")[0]:
                variabelen[m.group(1)] = i
        is_ns = bool(re.match(r"\s*namespace\b[^;]*\{", code))
        for ch in code:
            if ch == "{":
                stapel.append(is_ns)
                is_ns = False
            elif ch == "}" and stapel:
                stapel.pop()

    namen = {kort(fn) for fn in funcs}
    roept = {}
    gebruikt = {}
    for fn in funcs:
        body = "\n".join(regels[fn.start_line - 1:fn.end_line])
        tokens = set(re.findall(r"\b\w+\b", body))
        roept[fn] = sorted((tokens & namen) - {kort(fn)})
        gebruikt[fn] = sorted(tokens & set(variabelen))

    print(f"{args.bestand}: {len(regels)} regels, {len(funcs)} functies, {len(variabelen)} variabelen op bestandsniveau")
    print("variabelen:", ", ".join(f"{v} (r{n})" for v, n in sorted(variabelen.items(), key=lambda kv: kv[1])))
    print()
    for fn in funcs:
        print(f"{fn.start_line}-{fn.end_line} ({fn.nloc} nloc, ccn {fn.cyclomatic_complexity}) {fn.name}")
        if roept[fn]:
            print("    roept:", ", ".join(roept[fn]))
        if gebruikt[fn]:
            print("    gegevens:", ", ".join(gebruikt[fn]))

    if args.groepen:
        # Union-find over functions linked by a call or a shared variable.
        ouder = {kort(fn): kort(fn) for fn in funcs}

        def vind(x):
            while ouder[x] != x:
                ouder[x] = ouder[ouder[x]]
                x = ouder[x]
            return x

        def koppel(a, b):
            ouder[vind(a)] = vind(b)

        per_var = collections.defaultdict(list)
        aanroepers = collections.Counter()
        for fn in funcs:
            for c in roept[fn]:
                aanroepers[c] += 1
            for v in gebruikt[fn]:
                per_var[v].append(kort(fn))
        kern_f = {f for f, n in aanroepers.items() if args.kern and n >= args.kern}
        kern_v = {v for v, fs in per_var.items() if args.kern and len(fs) >= args.kern}
        if args.kern:
            print(f"\nkern (gebruikt door {args.kern} of meer functies):")
            print("  functies:", ", ".join(sorted(kern_f)) or "-")
            print("  gegevens:", ", ".join(sorted(kern_v)) or "-")
        for fn in funcs:
            if kort(fn) in kern_f:
                continue
            for c in roept[fn]:
                if c not in kern_f:
                    koppel(kort(fn), c)
        for v, fs in per_var.items():
            if v in kern_v:
                continue
            fs = [f2 for f2 in fs if f2 not in kern_f]
            for f2 in fs[1:]:
                koppel(fs[0], f2)
        groepen = collections.defaultdict(list)
        for fn in funcs:
            groepen[vind(kort(fn))].append(fn)
        print(f"\nsamenhangende groepen: {len(groepen)}")
        for leden in sorted(groepen.values(), key=lambda g: -sum(fn.nloc for fn in g)):
            print(f"- {sum(fn.nloc for fn in leden)} nloc, {len(leden)} functies: "
                  + ", ".join(kort(fn) for fn in leden[:12]) + (" ..." if len(leden) > 12 else ""))


if __name__ == "__main__":
    sys.exit(main())
