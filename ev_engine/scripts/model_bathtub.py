#!/usr/bin/env python3
"""Freestanding bathtub — porcelain, curved.
The shape boxes can never achieve. ~200 tris.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *
import math

kit = EVModelKit()
kit.begin("Bathtub", tri_budget=250)

# ── Outer shell (subdiv 1 — smooth porcelain curve) ──
# Elongated cylinder as tub body
bpy.ops.mesh.primitive_cylinder_add(radius=0.4, depth=0.5, vertices=16,
                                     location=(0, 0.35, 0))
outer = bpy.context.active_object
outer.name = "Bathtub_Outer"
outer.scale = (1.8, 1, 1)
bpy.ops.object.transform_apply(scale=True)
mod = outer.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
kit._apply_material(outer, MARBLE_WHITE)
kit._track(outer, MARBLE_WHITE)

# ── Inner cavity (slightly smaller) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=0.42, vertices=16,
                                     location=(0, 0.38, 0))
inner = bpy.context.active_object
inner.name = "Bathtub_Inner"
inner.scale = (1.7, 1, 0.92)
bpy.ops.object.transform_apply(scale=True)
mod = inner.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
kit._apply_material(inner, TILE_WHITE)
kit._track(inner, TILE_WHITE)

# ── Rim — torus ring ──
kit.torus("Rim", (0, 0.6, 0), major_radius=0.4, minor_radius=0.025,
          material=MARBLE_WHITE, major_segments=16, minor_segments=6)
# Scale to match tub proportions
bpy.context.active_object.scale = (1.8, 1, 1)
bpy.ops.object.transform_apply(scale=True)

# ── Feet — 4 claw feet ──
for i, (fx, fz) in enumerate([(-0.5, -0.25), (0.5, -0.25),
                               (-0.5, 0.25), (0.5, 0.25)]):
    kit.sphere(f"Foot_{i}", (fx, 0.06, fz), radius=0.04,
               material=BRASS, segments=6, rings=4)

# ── Faucet ──
kit.cylinder("Faucet_V", (0, 0.68, -0.3), radius=0.015, depth=0.15,
             material=BRASS)
kit.cylinder("Spout", (0, 0.75, -0.28), radius=0.012, depth=0.08,
             material=BRASS, rotation=(math.radians(90), 0, 0))

# ── Handles ──
for side in [-0.04, 0.04]:
    kit.sphere(f"Knob_{'L' if side < 0 else 'R'}", (side, 0.72, -0.32),
               radius=0.015, material=BRASS, segments=6, rings=4)

kit.export_obj("/Users/klaus/bathtub.obj")
kit.preview_placement()
