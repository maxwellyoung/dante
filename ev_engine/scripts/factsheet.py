#!/usr/bin/env python3
"""factsheet.py — the facts registry (the Delilah Brain, Remo's talk).

One table of every fact in the game: where it's SET (build_query, rules
`remember`) and where it's CHECKED (rules criteria). Run before inventing a
new fact name — accidental near-duplicates (`gibbons_knows_x` vs
`x_happened`) cause false negatives hours into a run.

Usage: python3 scripts/factsheet.py
"""
import re, collections, glob

facts = collections.defaultdict(lambda: {"set": set(), "checked": set()})

# engine-provided facts
for line in open("src/dialog_game.c"):
    m = re.search(r'dlg_query_add(?:_sym)?\(q?&?q?,?\s*"(\w+)"', line)
    if m: facts[m.group(1)]["set"].add("build_query")
# speak_ex extras
facts["object"]["set"].add("speak_ex"); facts["step"]["set"].add("speak_ex")

for path in glob.glob("assets/dialogue/*.rules"):
    rule = "?"
    for raw in open(path):
        t = raw.split("#")[0].strip()
        if t.startswith("rule "): rule = t.split()[1]
        elif t.startswith("criteria"):
            for tok in t.split()[1:]:
                key = re.split(r"[=<>]", tok)[0]
                facts[key]["checked"].add(rule)
        elif t.startswith("remember"):
            for tok in t.split()[1:]:
                key = re.split(r"[+=@]", tok)[0]
                facts[key]["set"].add(rule)

print(f"{'FACT':<22}{'SET BY':<34}CHECKED BY")
warn = []
for k in sorted(facts):
    f = facts[k]
    setby = ", ".join(sorted(f["set"])) or "NOTHING (dead check!)"
    chk = ", ".join(sorted(f["checked"])[:4]) or "nothing yet"
    if len(f["checked"]) > 4: chk += f" +{len(f['checked'])-4}"
    if not f["set"]: warn.append(k)
    print(f"{k:<22}{setby[:33]:<34}{chk}")
if warn:
    print(f"\nWARNING — checked but never set: {', '.join(warn)}")
