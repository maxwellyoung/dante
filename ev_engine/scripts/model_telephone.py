#!/usr/bin/env python3
"""Vintage rotary telephone — brass body, dark handset.
The phone that rings unanswered. ~120 tris.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *
import math

kit = EVModelKit()
kit.begin("Telephone", tri_budget=150)

# ── Body — squashed sphere (rotary phone base) ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.08, segments=10, ring_count=6,
                                      location=(0, 0, 0.04))
body = bpy.context.active_object
body.name = "Telephone_Body"
body.scale = (1.0, 1.0, 0.5)
bpy.ops.object.transform_apply(scale=True)
kit._apply_material(body, BRASS)
kit._track(body, BRASS)

# ── Rotary dial ──
kit.cylinder("Dial", (0, 0, 0.065), radius=0.045, depth=0.008,
             material=CONCRETE, vertices=10)

# ── Dial finger stop ──
kit.cylinder("Stop", (0.035, 0, 0.07), radius=0.005, depth=0.012,
             material=BRASS)

# ── Handset cradle prongs ──
for side in [-1, 1]:
    tag = "L" if side < 0 else "R"
    kit.cylinder(f"Prong_{tag}", (side * 0.04, 0, 0.075),
                 radius=0.006, depth=0.03, material=BRASS)

# ── Handset (resting in cradle) ──
kit.cylinder("Earpiece", (-0.04, 0, 0.1), radius=0.018, depth=0.025,
             material=LEATHER_DARK, vertices=8)

bpy.ops.mesh.primitive_cylinder_add(radius=0.008, depth=0.08, vertices=6,
                                     location=(0, 0, 0.1))
handle = bpy.context.active_object
handle.name = "Telephone_Handle"
handle.rotation_euler[1] = math.radians(90)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(handle, LEATHER_DARK)
kit._track(handle, LEATHER_DARK)

kit.cylinder("Mouthpiece", (0.04, 0, 0.1), radius=0.016, depth=0.025,
             material=LEATHER_DARK, vertices=8)

# ── Cord ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.003, depth=0.06, vertices=4,
                                     location=(-0.05, 0.02, 0.04))
cord = bpy.context.active_object
cord.name = "Telephone_Cord"
cord.rotation_euler[0] = math.radians(60)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(cord, LEATHER_DARK)
kit._track(cord, LEATHER_DARK)

kit.export_obj("/Users/klaus/telephone.obj")
kit.preview_placement()
