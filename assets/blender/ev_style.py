"""
ev_style.py — Endearing Void Blender Style Bible
=================================================
Run this FIRST in every Claude Code / MCP session before touching anything.
Defines all locked aesthetic constants. Do not drift from these values.

Usage (Blender Python console or MCP):
    exec(open('ev_style.py').read())
    # or via MCP: run_script('ev_style.py')

After running, call:
    setup_gibbons_material(obj)
    setup_scene_lighting(scene_name)
    setup_render()
"""

import bpy
import mathutils

# ─────────────────────────────────────────────────────────────────────────────
# RENDER SETTINGS — 480x300, lo-fi, consistent with ev_engine
# ─────────────────────────────────────────────────────────────────────────────

RENDER_W = 1920
RENDER_H = 1200
RENDER_SCALE = 1.0          # native resolution — no scaling needed at 1920x1200
RENDER_SAMPLES = 128        # fast but not noisy for stills; 64 for animation previews
RENDER_ENGINE = 'CYCLES'    # Cycles for material consistency; Eevee for quick previews

def setup_render():
    scene = bpy.context.scene
    scene.render.engine = RENDER_ENGINE
    scene.render.resolution_x = int(RENDER_W * RENDER_SCALE)
    scene.render.resolution_y = int(RENDER_H * RENDER_SCALE)
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    if RENDER_ENGINE == 'CYCLES':
        scene.cycles.samples = RENDER_SAMPLES
        scene.cycles.use_denoising = True
        scene.cycles.device = 'GPU'
    print(f"[EV] Render: {scene.render.resolution_x}x{scene.render.resolution_y} @ {RENDER_SAMPLES} samples")


# ─────────────────────────────────────────────────────────────────────────────
# COLOR PALETTE — derived from ev_engine lighting presets
# All values in linear sRGB (Blender's native space)
# ─────────────────────────────────────────────────────────────────────────────

PALETTE = {
    # Scene atmosphere colors (match lighting.c presets)
    'earth_blue':       (0.20, 0.40, 0.75, 1.0),   # Earth glow through window
    'amber_lamp':       (1.10, 0.80, 0.45, 1.0),   # Bedside lamp warm
    'chandelier_gold':  (0.85, 0.65, 0.40, 1.0),   # Lobby chandelier
    'paris_grey':       (0.55, 0.52, 0.48, 1.0),   # Paris dream walls
    'void_black':       (0.02, 0.02, 0.03, 1.0),   # The void

    # Godard accent — one saturated color per scene, everything else desaturated
    'godard_red':       (0.80, 0.08, 0.05, 1.0),   # Suite accent (pillow, book)
    'paris_red':        (0.75, 0.06, 0.04, 1.0),   # Paris dream (scarf/flower)

    # Gibbons character palette
    'gibbons_head':     (0.95, 0.93, 0.90, 1.0),   # Porcelain white, slight warmth
    'gibbons_jacket':   (0.72, 0.28, 0.18, 1.0),   # Coral/terracotta bellhop
    'gibbons_trousers': (0.08, 0.10, 0.18, 1.0),   # Navy blue, near-black
    'gibbons_buttons':  (0.85, 0.70, 0.30, 1.0),   # Dull gold
    'gibbons_halo':     (0.90, 0.75, 0.25, 1.0),   # Gold halo ring
    'gibbons_eyes':     (0.45, 0.80, 0.65, 1.0),   # Mint green, no pupils
}

# ─────────────────────────────────────────────────────────────────────────────
# GIBBONS MATERIALS
# ─────────────────────────────────────────────────────────────────────────────

def make_material(name, base_color, roughness=0.6, metallic=0.0, subsurface=0.0):
    """Create or update a Principled BSDF material with locked values."""
    mat = bpy.data.materials.get(name)
    if not mat:
        mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    if not bsdf:
        bsdf = mat.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = base_color
    bsdf.inputs['Roughness'].default_value = roughness
    bsdf.inputs['Metallic'].default_value = metallic
    if 'Subsurface Weight' in bsdf.inputs:
        bsdf.inputs['Subsurface Weight'].default_value = subsurface
    elif 'Subsurface' in bsdf.inputs:
        bsdf.inputs['Subsurface'].default_value = subsurface
    return mat

def setup_gibbons_materials():
    """
    Apply all Gibbons materials. Call after importing/linking Gibbons mesh.
    Expects mesh objects named: Gibbons.Head, Gibbons.Body, Gibbons.Trousers,
    Gibbons.Halo, Gibbons.Eyes, Gibbons.Buttons
    Adjust object names to match your actual mesh.
    """
    mats = {
        # name: (base_color, roughness, metallic, subsurface)
        'EV_Gibbons_Head':     (PALETTE['gibbons_head'],     0.15, 0.0, 0.12),  # porcelain SSS
        'EV_Gibbons_Jacket':   (PALETTE['gibbons_jacket'],   0.75, 0.0, 0.0),
        'EV_Gibbons_Trousers': (PALETTE['gibbons_trousers'], 0.80, 0.0, 0.0),
        'EV_Gibbons_Buttons':  (PALETTE['gibbons_buttons'],  0.45, 0.7, 0.0),   # slightly metallic
        'EV_Gibbons_Halo':     (PALETTE['gibbons_halo'],     0.30, 0.85, 0.0),  # gold metal
        'EV_Gibbons_Eyes':     (PALETTE['gibbons_eyes'],     0.10, 0.0, 0.0),   # smooth, no SSS
    }
    created = []
    for name, (color, roughness, metallic, subsurface) in mats.items():
        mat = make_material(name, color, roughness, metallic, subsurface)
        created.append(name)
    print(f"[EV] Gibbons materials: {', '.join(created)}")
    return mats


# ─────────────────────────────────────────────────────────────────────────────
# LIGHTING PRESETS — mirror lighting.c presets for Blender consistency
# Each returns a dict describing the light rig to build
# ─────────────────────────────────────────────────────────────────────────────

LIGHTING_PRESETS = {

    'space_suite': {
        # Hotel Chevalier in orbit: golden lamplight, Earth blue from window
        # Intimate, warm pools, dark corners
        'key': {
            'direction': (-0.5, -0.5, -0.2),    # diagonal from window
            'color': (0.55, 0.68, 0.90),         # Earth blue key
            'energy': 3.0,
        },
        'fill': {
            'direction': (0.3, 0.4, 0.2),
            'color': (0.25, 0.20, 0.12),         # amber floor bounce
            'energy': 1.0,
        },
        'ambient': (0.10, 0.08, 0.06),           # warm ambient
        'points': [
            {'pos': (0, 4.8, -4.0),   'color': (1.2, 0.90, 0.50), 'energy': 30},  # ceiling
            {'pos': (-2.5, 0.85, -4.8), 'color': (1.1, 0.80, 0.45), 'energy': 20},  # bedside L
            {'pos': (2.5, 0.85, -4.8),  'color': (1.1, 0.80, 0.45), 'energy': 20},  # bedside R
            {'pos': (-6, 2, 0),        'color': (0.20, 0.40, 0.75), 'energy': 15},  # Earth glow
        ],
    },

    'elevator': {
        # Warm amber + wood paneling, intimate compression
        # Hotel Chevalier energy — two people in a small space
        'key': {
            'direction': (0.0, -0.9, -0.2),
            'color': (0.95, 0.80, 0.55),         # warm overhead
            'energy': 4.0,
        },
        'fill': {
            'direction': (0.1, 0.3, 0.5),
            'color': (0.20, 0.15, 0.08),
            'energy': 0.8,
        },
        'ambient': (0.12, 0.10, 0.07),
        'points': [
            {'pos': (0, 2.8, 0),  'color': (1.1, 0.85, 0.50), 'energy': 25},  # ceiling strip
            {'pos': (0, 0.8, 1.2), 'color': (0.8, 0.65, 0.35), 'energy': 10},  # panel glow
        ],
    },

    'space_lobby': {
        # Massive open plan, Earth glow, chandelier drama
        # Bioshock Infinite arrival energy — vast and lonely
        'key': {
            'direction': (-0.3, -0.8, -0.4),     # steep overhead chandelier
            'color': (0.55, 0.65, 0.85),         # Earth blue tint
            'energy': 5.0,
        },
        'fill': {
            'direction': (0.2, 0.5, 0.3),
            'color': (0.12, 0.18, 0.28),         # cool blue bounce
            'energy': 0.6,
        },
        'ambient': (0.08, 0.09, 0.12),           # LOW — dramatic pools
        'points': [
            {'pos': (0, 6.4, -3.0),  'color': (0.85, 0.65, 0.40), 'energy': 40},  # chandelier
            {'pos': (-8, 3, 0),      'color': (0.3, 0.45, 0.65),  'energy': 20},
            {'pos': (8, 3, 0),       'color': (0.3, 0.45, 0.65),  'energy': 20},
            {'pos': (0, 0.5, -6.0),  'color': (0.4, 0.6, 0.9),   'energy': 25},  # Earth floor
        ],
    },

    'paris_dream': {
        # B&W with one Godard red — Parisian hotel room, memory register
        # Everything desaturated except one color object
        'key': {
            'direction': (0.3, -0.6, -0.5),
            'color': (0.85, 0.82, 0.78),         # warm grey overcast — Paris window
            'energy': 2.5,
        },
        'fill': {
            'direction': (-0.2, 0.4, 0.3),
            'color': (0.60, 0.58, 0.55),
            'energy': 1.2,
        },
        'ambient': (0.35, 0.33, 0.30),           # higher ambient — B&W needs flatter light
        'points': [
            {'pos': (0, 2.5, -2),  'color': (0.9, 0.85, 0.75), 'energy': 15},  # table lamp
        ],
        'note': 'Apply EV_BW post-process. One object gets PALETTE[paris_red] — everything else desaturated.',
    },

    'taxi': {
        # Auckland dawn, wet streets, city light through glass
        # Last grounded moment — warmest scene in the game
        'key': {
            'direction': (0.5, -0.4, -0.6),
            'color': (1.0, 0.75, 0.45),          # dawn golden
            'energy': 3.5,
        },
        'fill': {
            'direction': (-0.3, 0.2, 0.4),
            'color': (0.4, 0.5, 0.7),            # cool sky bounce
            'energy': 1.5,
        },
        'ambient': (0.18, 0.16, 0.14),
        'points': [
            {'pos': (-1.5, 1.2, 0.5), 'color': (1.0, 0.8, 0.5), 'energy': 10},  # streetlight through window
        ],
    },

    'void': {
        # The ending. Nothing.
        'key': {
            'direction': (0.0, -1.0, 0.0),
            'color': (0.3, 0.3, 0.35),
            'energy': 1.0,
        },
        'fill': {
            'direction': (0.0, 1.0, 0.0),
            'color': (0.1, 0.1, 0.12),
            'energy': 0.3,
        },
        'ambient': (0.04, 0.04, 0.05),           # near-black
        'points': [],
    },
}


def setup_scene_lighting(scene_name):
    """
    Build a light rig for the named scene preset.
    Removes all existing lights first. scene_name must match LIGHTING_PRESETS key.
    """
    if scene_name not in LIGHTING_PRESETS:
        print(f"[EV] Unknown scene: {scene_name}. Options: {list(LIGHTING_PRESETS.keys())}")
        return

    # Remove existing lights
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            bpy.data.objects.remove(obj, do_unlink=True)

    preset = LIGHTING_PRESETS[scene_name]

    # Key light (Sun)
    bpy.ops.object.light_add(type='SUN')
    key = bpy.context.object
    key.name = f'EV_Key_{scene_name}'
    key.data.color = preset['key']['color']
    key.data.energy = preset['key']['energy']
    d = preset['key']['direction']
    key.rotation_euler = mathutils.Vector(d).to_track_quat('-Z', 'Y').to_euler()

    # Fill light (Sun, dimmer)
    bpy.ops.object.light_add(type='SUN')
    fill = bpy.context.object
    fill.name = f'EV_Fill_{scene_name}'
    fill.data.color = preset['fill']['color']
    fill.data.energy = preset['fill']['energy']
    d = preset['fill']['direction']
    fill.rotation_euler = mathutils.Vector(d).to_track_quat('-Z', 'Y').to_euler()

    # Point lights
    for i, pt in enumerate(preset.get('points', [])):
        bpy.ops.object.light_add(type='POINT', location=pt['pos'])
        point = bpy.context.object
        point.name = f'EV_Point_{scene_name}_{i}'
        point.data.color = pt['color']
        point.data.energy = pt['energy']
        point.data.shadow_soft_size = 0.5

    # World ambient
    world = bpy.context.scene.world
    if not world:
        world = bpy.data.worlds.new('EV_World')
        bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    if bg:
        amb = preset['ambient']
        bg.inputs['Color'].default_value = (*amb, 1.0)
        bg.inputs['Strength'].default_value = 1.0

    if 'note' in preset:
        print(f"[EV] Note: {preset['note']}")

    print(f"[EV] Lighting set: {scene_name} ({len(preset.get('points',[]))} point lights)")


# ─────────────────────────────────────────────────────────────────────────────
# CAMERA — match ev_engine FOV and aspect
# ─────────────────────────────────────────────────────────────────────────────

CAMERA_FOV_DEFAULT = 75.0   # degrees, matches player.c default
CAMERA_FOV_SPRINT  = 90.0   # FOV zoom on sprint
CAMERA_FOV_BED     = 60.0   # FOV narrows in bed ritual (agency surrender)

def setup_camera(fov=CAMERA_FOV_DEFAULT):
    cam = bpy.context.scene.camera
    if not cam:
        bpy.ops.object.camera_add()
        cam = bpy.context.object
        bpy.context.scene.camera = cam
        cam.name = 'EV_Camera'
    cam.data.lens_unit = 'FOV'
    cam.data.angle = fov * (3.14159 / 180.0)
    # Match 480x300 aspect (8:5)
    bpy.context.scene.render.pixel_aspect_x = 1.0
    bpy.context.scene.render.pixel_aspect_y = 1.0
    print(f"[EV] Camera FOV: {fov}°")


# ─────────────────────────────────────────────────────────────────────────────
# GIBBONS POSE CONSTANTS
# ─────────────────────────────────────────────────────────────────────────────

GIBBONS_POSES = {
    'idle':        'gibbons_idle.json',       # formal standing, slight lean
    'walk':        'gibbons_walk.json',       # purposeful, slightly formal gait
    'bow':         'gibbons_bow.json',        # at suite door
    'glance':      'gibbons_glance.json',     # looks behind player once, lobby
    'tie_adjust':  'gibbons_tie.json',        # nervous tell in elevator
    'taxi_home':   'gibbons_taxi.json',       # ending montage, going home
}

# Head-to-body ratio: approximately 1:2 (vinyl toy proportions)
# Eyes: spherical, mint green, no pupils, smooth
# Halo: thin gold ring, radius = head_radius * 1.15, height = eye_level + 0.1


# ─────────────────────────────────────────────────────────────────────────────
# QUICK SETUP — run all of the above
# ─────────────────────────────────────────────────────────────────────────────

def ev_setup(scene='space_suite', fov=CAMERA_FOV_DEFAULT):
    """Full scene setup. Call at start of every session."""
    setup_render()
    setup_gibbons_materials()
    setup_scene_lighting(scene)
    setup_camera(fov)
    print(f"\n[EV] Style bible loaded. Scene: {scene}. Ready.")
    print("[EV] Available scenes:", list(LIGHTING_PRESETS.keys()))


# ─────────────────────────────────────────────────────────────────────────────
# MODEL KIT INTEGRATION
# When modeling props, use ev_model_kit.py (in ev_engine/scripts/) instead
# of defining ad-hoc materials. ev_model_kit provides:
#   - Locked material palette mapping to engine materialIds
#   - Form primitives (rounded_cube, cylinder, tapered_leg, etc.)
#   - Validation gates (tri budget, scale, material compliance)
#   - Auto-export with QA report
#
# This file (ev_style.py) handles SCENE SETUP (lighting, render, camera).
# ev_model_kit.py handles MODEL BUILDING (geometry, materials, export).
# ─────────────────────────────────────────────────────────────────────────────


# Auto-run if executed directly
if __name__ == '__main__' or True:
    print("\n[EV] ev_style.py loaded. Call ev_setup('scene_name') to initialize.")
    print("[EV] Scenes:", list(LIGHTING_PRESETS.keys()))
    print("[EV] Materials: run setup_gibbons_materials()")
    print("[EV] For model building: use ev_model_kit.py (ev_engine/scripts/)")
