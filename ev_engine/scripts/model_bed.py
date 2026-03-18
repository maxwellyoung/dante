#!/usr/bin/env python3
"""Hotel bed — The emotional endpoint.
60+ seconds of staring during the bed ritual. ~300 tris.
Two pillows. Booked for two. Player is one.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("Bed", tri_budget=350)

# ── Frame (sharp, structural) ──
kit.cube("Frame", (0, 0.18, 0), (3.4, 0.36, 2.2), material=WOOD_DARK)

# ── Mattress (subdiv 1 — slight softness, not the hero) ──
kit.rounded_cube("Mattress", (0, 0.48, 0), (3.2, 0.28, 2.0),
                 material=FABRIC_WHITE, subdivisions=1)

# ── Duvet (subdiv 2 — soft, the player stares at this) ──
kit.rounded_cube("Duvet", (0, 0.66, 0.15), (3.0, 0.06, 1.4),
                 material=FABRIC_CREAM, subdivisions=2)

# ── Folded edge (sharp — crisp hotel fold) ──
kit.cube("Fold", (0, 0.68, -0.6), (3.0, 0.04, 0.3), material=FABRIC_CREAM)

# ── Two pillows (subdiv 2 — soft, emotional weight) ──
for side in [-0.6, 0.6]:
    tag = "L" if side < 0 else "R"
    kit.rounded_cube(f"Pillow_{tag}", (side, 0.68, -1.0), (0.65, 0.18, 0.4),
                     material=FABRIC_WHITE, subdivisions=2)

# ── Headboard — tall navy velvet panel (subdiv 1 — slight curve on top) ──
kit.rounded_cube("Headboard", (0, 1.8, -1.35), (3.6, 2.8, 0.12),
                 material=VELVET_NAVY, subdivisions=1)

# ── Brass cap on headboard (sharp, manufactured) ──
kit.cube("Cap", (0, 3.25, -1.32), (3.7, 0.06, 0.08), material=BRASS)

kit.export_obj("/Users/klaus/bed.obj")
kit.preview_placement()
