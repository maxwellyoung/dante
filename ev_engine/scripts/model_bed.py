#!/usr/bin/env python3
"""Hotel bed — The emotional endpoint.
60+ seconds of staring during the bed ritual. ~700 tris.
Two pillows. Booked for two. Player is one.

At 1920x1200, curves need to read as curves. Duvet, pillows,
and headboard get subdivision 2-3. Frame stays sharp.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("Bed", tri_budget=700)

# ── Frame (sharp, structural — Rule 3: no subdivision) ──
kit.cube("Frame", (0, 0.18, 0), (3.4, 0.36, 2.2), material=WOOD_DARK)

# ── Legs — tapered wood, visible from most angles ──
kit.four_legs("FrameLegs",
              [(-1.6, -1.0), (1.6, -1.0), (-1.6, 1.0), (1.6, 1.0)],
              radius=0.04, height=0.18, material=WOOD_DARK)

# ── Mattress (subdiv 2 — soft sides catch light) ──
kit.rounded_cube("Mattress", (0, 0.48, 0), (3.2, 0.28, 2.0),
                 material=FABRIC_WHITE, subdivisions=2)

# ── Duvet (subdiv 3 — hero surface, soft billowy form) ──
kit.rounded_cube("Duvet", (0, 0.66, 0.15), (3.0, 0.08, 1.4),
                 material=FABRIC_CREAM, subdivisions=3)

# ── Folded edge (sharp — crisp hotel fold) ──
kit.cube("Fold", (0, 0.70, -0.6), (3.0, 0.04, 0.3), material=FABRIC_CREAM)

# ── Two pillows (subdiv 3 — soft, emotional weight, player stares) ──
for side in [-0.6, 0.6]:
    tag = "L" if side < 0 else "R"
    kit.rounded_cube(f"Pillow_{tag}", (side, 0.70, -1.0), (0.65, 0.20, 0.42),
                     material=FABRIC_WHITE, subdivisions=3)

# ── Headboard — tall navy velvet panel (subdiv 2 — curve at top reads) ──
kit.rounded_cube("Headboard", (0, 1.8, -1.35), (3.6, 2.8, 0.12),
                 material=VELVET_NAVY, subdivisions=2)

# ── Brass cap on headboard (sharp, manufactured) ──
kit.cube("Cap", (0, 3.25, -1.32), (3.7, 0.06, 0.08), material=BRASS)

# ── Subtle bedskirt (hides frame-mattress gap) ──
kit.cube("Skirt_L", (-1.7, 0.30, 0), (0.04, 0.24, 2.1), material=FABRIC_CREAM)
kit.cube("Skirt_R", (1.7, 0.30, 0), (0.04, 0.24, 2.1), material=FABRIC_CREAM)
kit.cube("Skirt_F", (0, 0.30, 1.1), (3.3, 0.24, 0.04), material=FABRIC_CREAM)

kit.export_obj("/Users/klaus/bed.obj")
kit.preview_placement()
