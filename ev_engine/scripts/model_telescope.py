#!/usr/bin/env python3
"""Brass telescope on tripod — balcony prop.
Pointing at Earth. ~100 tris.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *
import math

kit = EVModelKit()
kit.begin("Telescope", tri_budget=120)

# ── Main tube (angled up toward Earth) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.6, vertices=8,
                                     location=(0, 1.0, 0))
tube = bpy.context.active_object
tube.name = "Telescope_Tube"
tube.rotation_euler[0] = math.radians(30)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(tube, BRASS)
kit._track(tube, BRASS)

# ── Eyepiece ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.02, depth=0.08, vertices=6,
                                     location=(0.12, 0.85, 0))
eyepiece = bpy.context.active_object
eyepiece.name = "Telescope_Eye"
eyepiece.rotation_euler[0] = math.radians(30)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(eyepiece, BRASS)
kit._track(eyepiece, BRASS)

# ── Objective lens ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.04, depth=0.015, vertices=8,
                                     location=(-0.15, 1.1, 0))
lens = bpy.context.active_object
lens.name = "Telescope_Lens"
lens.rotation_euler[0] = math.radians(30)
bpy.ops.object.transform_apply(rotation=True)
kit._apply_material(lens, GLASS)
kit._track(lens, GLASS)

# ── Tripod — 3 legs ──
for angle in [0, 120, 240]:
    rad = math.radians(angle)
    lx = math.sin(rad) * 0.2
    lz = math.cos(rad) * 0.2
    bpy.ops.mesh.primitive_cylinder_add(radius=0.01, depth=0.9, vertices=4,
                                         location=(lx, 0.45, lz))
    leg = bpy.context.active_object
    leg.name = f"Telescope_Leg_{angle}"
    leg.rotation_euler[0] = math.radians(10) * math.cos(rad)
    leg.rotation_euler[2] = math.radians(10) * math.sin(rad)
    bpy.ops.object.transform_apply(rotation=True)
    kit._apply_material(leg, BRASS)
    kit._track(leg, BRASS)

# ── Hub — where legs meet ──
kit.sphere("Hub", (0, 0.9, 0), radius=0.03, material=BRASS, segments=6, rings=4)

kit.export_obj("/Users/klaus/telescope.obj")
kit.preview_placement()
