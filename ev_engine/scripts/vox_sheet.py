#!/usr/bin/env python3
"""vox_sheet.py — recording sheet for voice acting.

Parses assets/dialogue/*.rules into a CSV: one row per line variant with a
stable vox ID (<rule>_v<n>). This is the Valve/Dota workflow: the same sheet
drives the engine (subtitles today) and the recording booth (VO later).
Stable IDs mean recorded takes map 1:1 onto rules with no code changes.

Usage: python3 scripts/vox_sheet.py > qa/vox_sheet.csv
"""
import csv, glob, re, sys

w = csv.writer(sys.stdout)
w.writerow(["vox_id", "speaker", "concept", "line", "context", "file"])
for path in sorted(glob.glob("assets/dialogue/*.rules")):
    rule = who = concept = None
    variant = 0
    context = []
    for raw in open(path):
        line = raw.split("#", 1)[0].strip() if not raw.strip().startswith("#") else ""
        comment = raw.strip()[1:].strip() if raw.strip().startswith("#") else None
        if comment:
            context.append(comment)
            continue
        if not line:
            continue
        tok = line.split(None, 1)
        kw = tok[0]
        if kw == "rule":
            rule, who, concept, variant = tok[1].strip(), None, None, 0
        elif kw == "who":
            who = tok[1].strip()
        elif kw == "criteria":
            m = re.search(r"concept=(\S+)", tok[1])
            if m: concept = m.group(1)
        elif kw == "say":
            text = tok[1].strip().strip('"')
            w.writerow([f"{rule}_v{variant}", who or "?", concept or "?",
                        text, " / ".join(context[-2:]), path])
            variant += 1
        elif kw == "end":
            context = []
