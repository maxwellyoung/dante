#!/usr/bin/env python3
"""Grand piano — Lobby centrepiece.
Through-wall muffled piano comes from here. ~250 tris.
Glossy black body, ivory keys, brass pedals, burgundy velvet bench.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *
import math

kit = EVModelKit()
kit.begin("Piano", tri_budget=300)

# Piano body is glossy black — use LEATHER_DARK for the deep black + low roughness
PIANO_BLACK = EVMaterial("EV_Piano_Black", 20, 18, 15, 0.0, 0.30, 2, "Glossy piano body")

# ── Main case (subdiv 1 — grand piano curve matters) ──
kit.rounded_cube("Body", (0, 0.5, 0), (1.5, 0.3, 1.0),
                 material=PIANO_BLACK, subdivisions=1)

# ── Curved tail (wider at back) ──
kit.cube("Tail", (-0.3, 0.5, 0.4), (0.8, 0.28, 0.3), material=PIANO_BLACK)

# ── 3 legs (sharp, structural) ──
for i, (lx, lz) in enumerate([(-0.55, -0.35), (0.55, -0.35), (-0.2, 0.5)]):
    kit.cylinder(f"Leg_{i}", (lx, 0.18, lz), radius=0.03, depth=0.36,
                 material=PIANO_BLACK, vertices=8)
    # Brass caster at bottom
    kit.sphere(f"Caster_{i}", (lx, 0.02, lz), radius=0.02,
               material=BRASS, segments=6, rings=4)

# ── Lid — propped open ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.85, 0.15))
lid = bpy.context.active_object
lid.name = "Piano_Lid"
lid.scale = (1.4, 0.02, 0.8)
bpy.ops.object.transform_apply(scale=True)
lid.rotation_euler[0] = math.radians(-25)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(lid, PIANO_BLACK)
kit._track(lid, PIANO_BLACK)

# ── Lid prop stick ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.2, vertices=4,
                                     location=(0.3, 0.75, 0.1))
stick = bpy.context.active_object
stick.name = "Piano_Stick"
stick.rotation_euler[0] = math.radians(-15)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(stick, BRASS)
kit._track(stick, BRASS)

# ── Keyboard ──
kit.cube("KeysWhite", (0, 0.65, -0.45), (1.2, 0.02, 0.12), material=MARBLE_WHITE)
kit.cube("KeysBlack", (0, 0.67, -0.42), (1.1, 0.02, 0.05), material=LEATHER_DARK)

# ── Music stand ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.78, -0.38))
stand = bpy.context.active_object
stand.name = "Piano_MusicStand"
stand.scale = (0.6, 0.15, 0.01)
bpy.ops.object.transform_apply(scale=True)
stand.rotation_euler[0] = math.radians(-10)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(stand, PIANO_BLACK)
kit._track(stand, PIANO_BLACK)

# ── 3 pedals ──
for px in [-0.06, 0, 0.06]:
    kit.cube(f"Pedal_{px}", (px, 0.02, -0.4), (0.03, 0.005, 0.08), material=BRASS)

# ── Bench — burgundy velvet top ──
kit.cube("BenchTop", (0, 0.35, -0.75), (0.8, 0.04, 0.3), material=VELVET_RED)

kit.four_legs("BenchLeg",
              [(-0.35, -0.85), (0.35, -0.85), (-0.35, -0.65), (0.35, -0.65)],
              radius=0.015, height=0.3, material=PIANO_BLACK)

kit.export_obj("/Users/klaus/piano.obj")
kit.preview_placement()
