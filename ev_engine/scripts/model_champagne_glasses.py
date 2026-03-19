#!/usr/bin/env python3
"""Two champagne flutes on a brass tray + wine glass with lipstick.
One half-full (yours), one empty (hers — untouched). The washing line moment.
~200 tris. Lipstick is the single Godard red accent.

At 1920x1200, glass bowls need 12+ vertices to read as round.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *
import math

kit = EVModelKit()
kit.begin("ChampagneGlasses", tri_budget=300)

# ── Tray — brass with rim ──
kit.cylinder("Tray", (0, 0, 0.01), radius=0.25, depth=0.02,
             material=BRASS, vertices=16)

kit.torus("TrayRim", (0, 0, 0.02), major_radius=0.25, minor_radius=0.008,
          material=BRASS, major_segments=16, minor_segments=6)


# ── Champagne flute helper ──
def make_flute(name, x, y, has_liquid=False, tilt_deg=0):
    """Build a flute from kit primitives."""
    # Base
    kit.cylinder(f"{name}_Base", (x, y, 0.025), radius=0.025, depth=0.003,
                 material=GLASS, vertices=12)
    # Stem
    kit.cylinder(f"{name}_Stem", (x, y, 0.065), radius=0.004, depth=0.08,
                 material=GLASS)
    # Bowl (cone frustum — more verts for glass clarity)
    kit.cone(f"{name}_Bowl", (x, y, 0.135), radius1=0.008, radius2=0.025,
             depth=0.06, material=GLASS, vertices=12)
    # Liquid
    if has_liquid:
        GOLD_LIQUID = EVMaterial("EV_Gold_Liquid", 218, 175, 95, 0.0, 0.20, 7,
                                "Champagne liquid")
        kit.cone(f"{name}_Liquid", (x, y, 0.12), radius1=0.006, radius2=0.018,
                 depth=0.035, material=GOLD_LIQUID, vertices=12)


# ── Glass 1: half-full (yours) ──
make_flute("FluteFull", -0.06, 0, has_liquid=True)

# ── Glass 2: empty, slight tilt (hers — untouched) ──
make_flute("FluteEmpty", 0.06, 0.02, has_liquid=False)

# ── Wine glass with lipstick mark ──
kit.cylinder("Wine_Base", (0.12, -0.06, 0.025), radius=0.025, depth=0.003,
             material=GLASS, vertices=12)
kit.cylinder("Wine_Stem", (0.12, -0.06, 0.055), radius=0.004, depth=0.06,
             material=GLASS)
kit.sphere("Wine_Bowl", (0.12, -0.06, 0.11), radius=0.03,
           material=GLASS, segments=12, rings=8)
# Squash bowl to half-sphere
bpy.context.active_object.scale[2] = 0.7
bpy.ops.object.transform_apply(scale=True)

# ── Lipstick mark — the ONE accent (Godard red) ──
kit.cube("Wine_Lipstick", (0.14, -0.06, 0.135), (0.015, 0.005, 0.003),
         material=GODARD_RED)

kit.export_obj("/Users/klaus/champagne_glasses.obj")
kit.preview_placement()
