#!/usr/bin/env python3
"""
Gibbons — The bellhop. Gravity Bone chunky proportions.

A single joined OBJ with multiple material slots.
Rounded geometry replaces raw cubes — head is a sphere-ish blob,
limbs are subdivided cubes, cap is a cylinder with brim.

The engine's cube-person code handles animation procedurally.
This model is for future use when per-part model drawing is implemented.

~350 tris total. Vinyl toy proportions.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("Gibbons", tri_budget=400)

# ── Reference heights (1 unit = 1 meter) ──
# Gibbons is 1.5m tall (shorter than player, vinyl toy proportions)
# Head is ~25% of height. Limbs are chunky tubes.
shoe_top = 0.10
shin_top = shoe_top + 0.22
knee = shin_top
thigh_top = knee + 0.22
hip = thigh_top
torso_mid = hip + 0.23
shoulder = hip + 0.46
neck_top = shoulder + 0.10
head_center = neck_top + 0.20
cap_top = neck_top + 0.42

ls = 0.17  # leg spread from center

# ═══ SHOES — dark leather, chunky ═══
kit.rounded_cube("ShoeL", (-ls, 0.05, 0), (0.26, 0.10, 0.32),
                 material=LEATHER_DARK, subdivisions=1)
kit.rounded_cube("ShoeR", (ls, 0.05, 0), (0.26, 0.10, 0.32),
                 material=LEATHER_DARK, subdivisions=1)

# ═══ SHINS — navy trousers ═══
kit.rounded_cube("ShinL", (-ls, shoe_top + 0.11, 0), (0.24, 0.22, 0.24),
                 material=GIBBONS_TROUSERS, subdivisions=1)
kit.rounded_cube("ShinR", (ls, shoe_top + 0.11, 0), (0.24, 0.22, 0.24),
                 material=GIBBONS_TROUSERS, subdivisions=1)

# ═══ THIGHS — navy, slightly wider ═══
kit.rounded_cube("ThighL", (-ls, knee + 0.11, 0), (0.26, 0.22, 0.26),
                 material=GIBBONS_TROUSERS, subdivisions=1)
kit.rounded_cube("ThighR", (ls, knee + 0.11, 0), (0.26, 0.22, 0.26),
                 material=GIBBONS_TROUSERS, subdivisions=1)

# ═══ BELT ═══
kit.cube("Belt", (0, hip + 0.03, 0), (0.56, 0.06, 0.36),
         material=LEATHER_DARK)
kit.cube("Buckle", (0, hip + 0.03, -0.19), (0.06, 0.05, 0.02),
         material=BRASS)

# ═══ TORSO — bellhop jacket, coral/terracotta ═══
kit.rounded_cube("Jacket", (0, torso_mid, 0), (0.54, 0.46, 0.34),
                 material=GIBBONS_JACKET, subdivisions=1)
kit.cube("Shoulders", (0, shoulder, 0), (0.66, 0.10, 0.34),
         material=GIBBONS_JACKET)

# ═══ TIE — red ═══
kit.cube("Tie", (0, torso_mid + 0.06, -0.175), (0.14, 0.30, 0.02),
         material=GODARD_RED)

# ═══ BUTTONS — brass, 4 total ═══
for bx, by in [(0.08, torso_mid+0.14), (0.08, torso_mid-0.06),
               (-0.08, torso_mid+0.14), (-0.08, torso_mid-0.06)]:
    kit.cube(f"Btn_{bx:.0f}_{by:.0f}", (bx, by, -0.175), (0.045, 0.045, 0.02),
             material=GIBBONS_BUTTONS)

# ═══ UPPER ARMS — chunky ═══
arm_x = 0.43
kit.rounded_cube("ArmUL", (-arm_x, shoulder - 0.10, 0), (0.20, 0.20, 0.20),
                 material=GIBBONS_JACKET, subdivisions=1)
kit.rounded_cube("ArmUR", (arm_x, shoulder - 0.10, 0), (0.20, 0.20, 0.20),
                 material=GIBBONS_JACKET, subdivisions=1)

# ═══ FOREARMS — slightly lighter cuff ═══
CUFF = EVMaterial("EV_Cuff", 45, 50, 72, 0.0, 0.75, 9, "Shirt cuff")
kit.rounded_cube("ArmLL", (-arm_x, shoulder - 0.29, 0), (0.18, 0.18, 0.18),
                 material=CUFF, subdivisions=1)
kit.rounded_cube("ArmLR", (arm_x, shoulder - 0.29, 0), (0.18, 0.18, 0.18),
                 material=CUFF, subdivisions=1)

# ═══ HANDS — skin-colored ═══
kit.rounded_cube("HandL", (-arm_x, shoulder - 0.43, 0), (0.12, 0.12, 0.12),
                 material=GIBBONS_HEAD, subdivisions=1)
kit.rounded_cube("HandR", (arm_x, shoulder - 0.43, 0), (0.12, 0.12, 0.12),
                 material=GIBBONS_HEAD, subdivisions=1)

# ═══ BRIEFCASE — left hand ═══
CASE_BROWN = EVMaterial("EV_Briefcase", 160, 110, 50, 0.0, 0.70, 8, "Briefcase")
kit.rounded_cube("Briefcase", (-arm_x, shoulder - 0.62, 0), (0.14, 0.26, 0.36),
                 material=CASE_BROWN, subdivisions=1)
kit.cube("Latch", (-arm_x, shoulder - 0.52, -0.19), (0.04, 0.05, 0.03),
         material=BRASS)

# ═══ NECK ═══
kit.cylinder("Neck", (0, neck_top - 0.025, 0), radius=0.09, depth=0.05,
             material=GIBBONS_HEAD, vertices=8)

# ═══ HEAD — the most important silhouette ═══
# Subdivided for roundness — player stares at this
kit.rounded_cube("Head", (0, head_center, 0), (0.38, 0.40, 0.34),
                 material=GIBBONS_HEAD, subdivisions=2)

# Nose
kit.cube("Nose", (0, head_center - 0.02, -0.18), (0.06, 0.06, 0.04),
         material=GIBBONS_HEAD)

# ═══ EYES — dark squares ═══
kit.cube("EyeL", (-0.09, head_center + 0.04, -0.16), (0.08, 0.06, 0.02),
         material=GIBBONS_EYES)
kit.cube("EyeR", (0.09, head_center + 0.04, -0.16), (0.08, 0.06, 0.02),
         material=GIBBONS_EYES)

# Mouth
MOUTH = EVMaterial("EV_Mouth", 140, 100, 75, 0.0, 0.60, 0, "Mouth")
kit.cube("Mouth", (0, head_center - 0.08, -0.16), (0.10, 0.025, 0.02),
         material=MOUTH)

# ═══ CAP — the Godard red beacon ═══
kit.cylinder("CapCrown", (0, neck_top + 0.37, 0), radius=0.22, depth=0.12,
             material=GODARD_RED, vertices=12)
# Brim — forward extension
kit.cylinder("CapBrim", (0, neck_top + 0.32, -0.06), radius=0.27, depth=0.03,
             material=GODARD_RED, vertices=12)
# Gold band
kit.cylinder("CapBand", (0, neck_top + 0.34, 0), radius=0.225, depth=0.04,
             material=GIBBONS_BUTTONS, vertices=12)

kit.export_obj("/Users/klaus/gibbons_v2.obj")
kit.preview_placement()
