#!/usr/bin/env python3
"""Armchair — The gut-punch pair. Two facing Earth.
Curved armrests are the reason to model this. ~400 tris.
At 1920x1200, the curves need to breathe.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("Armchair", tri_budget=400)

# ── Seat cushion — rounded, inviting (Rule 3: player stares at these) ──
kit.rounded_cube("Seat", (0, 0.38, 0), (0.5, 0.08, 0.5),
                 material=LEATHER_NAVY, subdivisions=2)

# ── Back cushion — angled, soft ──
kit.rounded_cube("Back", (0, 0.65, -0.2), (0.48, 0.45, 0.06),
                 material=LEATHER_NAVY, subdivisions=2)

# ── Frame: 4 legs (sharp, structural — Rule 3) ──
kit.four_legs("Leg",
              [(-0.2, -0.2), (0.2, -0.2), (-0.2, 0.2), (0.2, 0.2)],
              radius=0.02, height=0.35, material=WOOD_DARK)

# ── Armrests — the whole point of modeling this ──
for side in [-1, 1]:
    tag = "L" if side < 0 else "R"
    # Vertical support (sharp)
    kit.cylinder(f"ArmV_{tag}", (side * 0.25, 0.48, 0.15),
                 radius=0.018, depth=0.25, material=WOOD_DARK)
    # Horizontal pad (rounded — you rest your arm here)
    kit.rounded_cube(f"ArmH_{tag}", (side * 0.25, 0.6, 0), (0.04, 0.03, 0.4),
                     material=WOOD_DARK, subdivisions=2)

# ── Seat welt — subtle brass-colored piping around cushion edge ──
kit.cube("Welt_F", (0, 0.34, 0.25), (0.50, 0.01, 0.01), material=BRASS_DULL)
kit.cube("Welt_B", (0, 0.34, -0.25), (0.50, 0.01, 0.01), material=BRASS_DULL)

kit.export_obj("/Users/klaus/armchair.obj")
kit.preview_placement()
