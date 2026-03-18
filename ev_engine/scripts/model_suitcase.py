#!/usr/bin/env python3
"""Vintage suitcase — leather, brass clasps, airline tags.
Packed for two. ~100 tris.
One Godard red tag (her luggage tag) — the single accent.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("Suitcase", tri_budget=120)

# ── Main body ──
kit.cube("Body", (0, 0.15, 0), (0.7, 0.3, 0.45), material=LEATHER_TAN)

# ── Brass edge trim (top and bottom seams) ──
kit.cube("TrimTop", (0, 0.305, 0), (0.72, 0.02, 0.47), material=BRASS)
kit.cube("TrimBot", (0, 0, 0), (0.72, 0.02, 0.47), material=BRASS)

# ── Clasps — 2 brass rectangles on front ──
for cx in [-0.2, 0.2]:
    tag = "L" if cx < 0 else "R"
    kit.cube(f"Clasp_{tag}", (cx, 0.16, 0.23), (0.06, 0.04, 0.02), material=BRASS)

# ── Handle ──
kit.cube("Handle", (0, 0.34, 0), (0.15, 0.03, 0.04), material=BRASS)

# ── Lid (propped slightly — half-packed) ──
kit.cube("Lid", (0, 0.34, -0.1), (0.68, 0.12, 0.02), material=LEATHER_TAN)

# ── Airline tag (fabric blue — not an accent, just a travel detail) ──
AIRLINE_BLUE = EVMaterial("EV_Airline_Blue", 55, 85, 175, 0.0, 0.60, 9, "Airline tag")
kit.cube("TagBlue", (0.12, 0.32, 0.05), (0.08, 0.05, 0.005), material=AIRLINE_BLUE)

# ── Red tag — Godard red, HER luggage tag (the ONE accent) ──
kit.cube("TagRed", (-0.1, 0.33, 0.04), (0.06, 0.04, 0.005), material=GODARD_RED)

# ── Shirt spilling out ──
kit.cube("Shirt", (0.15, 0.28, 0.2), (0.15, 0.03, 0.12), material=FABRIC_WHITE)

kit.export_obj("/Users/klaus/suitcase.obj")
kit.preview_placement()
