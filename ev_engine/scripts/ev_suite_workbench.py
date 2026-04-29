#!/usr/bin/env python3
"""
ev_suite_workbench.py — Blender Reference Scene for Space Suite Props
=====================================================================

Sets up a wireframe reference of the Space Suite (14x12x5m) with key furniture
zones marked, engine-matched lighting, and a 960x600 camera. Model props
*in context*, validate, render a preview, export — never touch `make run`.

Usage (Blender MCP or Python console):
    exec(open('/path/to/ev_suite_workbench.py').read())

    # Then model your prop:
    from ev_model_kit import *
    kit = EVModelKit()
    kit.begin("wine_glass", tri_budget=80)
    # ... build geometry ...
    # Preview placement:
    workbench_preview(kit, position=(−3.3, 0.39, 3.6), rotation=15)
    # Renders 960x600 screenshot, prints add_model() C code

Full workflow:
    1. workbench_setup()        — builds reference room + lighting + camera
    2. Model your prop with kit — use real-world dimensions
    3. workbench_preview(kit)   — places model, renders, validates
    4. workbench_export(kit)    — exports GLB + prints C placement code
"""

import bpy
import math
import os
import sys

# Add scripts dir to path so we can import ev_model_kit
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__)) if '__file__' in dir() else os.getcwd()
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

# ─────────────────────────────────────────────────────────────────────────────
# SUITE GEOMETRY REFERENCE (matches build_space_suite in scene.c)
# ─────────────────────────────────────────────────────────────────────────────

SUITE = {
    'width': 14.0,     # X axis
    'depth': 12.0,     # Z axis
    'height': 5.0,     # Y axis
    'spawn': (0, 1.6, 4),
}

# Key furniture positions (center, Y=floor unless noted)
FURNITURE = {
    'bed':           {'pos': (0, 0.48, -4.5),  'size': (3.2, 0.28, 2.0), 'label': 'BED'},
    'headboard':     {'pos': (0, 1.8, -5.55),  'size': (3.6, 2.8, 0.12), 'label': 'HEADBOARD'},
    'nightstand_L':  {'pos': (-2.5, 0.32, -4.8), 'size': (0.6, 0.6, 0.6), 'label': 'NS-L'},
    'nightstand_R':  {'pos': (2.5, 0.32, -4.8),  'size': (0.6, 0.6, 0.6), 'label': 'NS-R'},
    'sofa':          {'pos': (-3, 0.35, 2.0),  'size': (1.8, 0.7, 0.8), 'label': 'SOFA'},
    'coffee_table':  {'pos': (-3, 0.35, 3.5),  'size': (1.4, 0.04, 0.8), 'label': 'TABLE'},
    'desk':          {'pos': (5.8, 0.38, -1.5), 'size': (1.2, 0.76, 0.6), 'label': 'DESK'},
    'chair_L':       {'pos': (-5.2, 0, 0.5),   'size': (0.6, 0.8, 0.6), 'label': 'CHAIR'},
    'chair_R':       {'pos': (-5.2, 0, 2.5),   'size': (0.6, 0.8, 0.6), 'label': 'CHAIR'},
    'wardrobe':      {'pos': (6.5, 1.2, -0.2), 'size': (0.6, 2.4, 0.8), 'label': 'WARDROBE'},
    'window':        {'pos': (-7.0, 2.5, -0.5), 'size': (0.06, 4.2, 7.0), 'label': 'WINDOW'},
}

# Colors for reference geometry (muted, won't distract)
REF_COLOR = (0.3, 0.3, 0.35, 1.0)       # furniture zones
WALL_COLOR = (0.15, 0.15, 0.18, 1.0)     # walls
FLOOR_COLOR = (0.25, 0.22, 0.18, 1.0)    # floor
WINDOW_COLOR = (0.1, 0.2, 0.4, 1.0)      # window (blue hint)


def _make_ref_material(name, color, wireframe=False):
    """Create a simple reference material — muted, non-distracting."""
    mat = bpy.data.materials.get(name)
    if not mat:
        mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = mat.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = 0.9
    bsdf.inputs["Alpha"].default_value = 0.15 if wireframe else 0.6
    mat.blend_method = 'BLEND' if wireframe else 'OPAQUE'
    mat.shadow_method = 'NONE'
    return mat


def _add_ref_box(name, pos, size, material, collection):
    """Add a wireframe reference box."""
    bpy.ops.mesh.primitive_cube_add(size=1, location=pos)
    obj = bpy.context.active_object
    obj.name = f"REF_{name}"
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    obj.display_type = 'WIRE'
    if obj.data.materials:
        obj.data.materials[0] = material
    else:
        obj.data.materials.append(material)

    # Move to reference collection
    for c in obj.users_collection:
        c.objects.unlink(obj)
    collection.objects.link(obj)
    return obj


def workbench_setup():
    """
    Build the suite reference scene. Call once at session start.
    Creates wireframe room + furniture zones + lighting + camera.
    Your models go in the 'Props' collection.
    """
    print("\n[EV WORKBENCH] Setting up Space Suite reference scene...")

    # ── Clear scene ──
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for mat in list(bpy.data.materials):
        if mat.users == 0:
            bpy.data.materials.remove(mat)
    # Clean collections
    for c in list(bpy.data.collections):
        bpy.data.collections.remove(c)

    # ── Collections ──
    ref_col = bpy.data.collections.new("Reference")
    bpy.context.scene.collection.children.link(ref_col)
    props_col = bpy.data.collections.new("Props")
    bpy.context.scene.collection.children.link(props_col)

    # ── Materials ──
    wall_mat = _make_ref_material("REF_Wall", WALL_COLOR, wireframe=True)
    floor_mat = _make_ref_material("REF_Floor", FLOOR_COLOR)
    furn_mat = _make_ref_material("REF_Furniture", REF_COLOR, wireframe=True)
    win_mat = _make_ref_material("REF_Window", WINDOW_COLOR)

    # ── Room shell (wireframe) ──
    W, D, H = SUITE['width'], SUITE['depth'], SUITE['height']
    # Floor
    _add_ref_box("Floor", (0, -0.025, 0), (W, 0.05, D), floor_mat, ref_col)
    # Ceiling
    _add_ref_box("Ceiling", (0, H, 0), (W, 0.05, D), wall_mat, ref_col)
    # Walls
    _add_ref_box("Wall_Back", (0, H/2, -D/2), (W, H, 0.15), wall_mat, ref_col)
    _add_ref_box("Wall_Front", (0, H/2, D/2), (W, H, 0.15), wall_mat, ref_col)
    _add_ref_box("Wall_Left", (-W/2, H/2, 0), (0.15, H, D), wall_mat, ref_col)
    _add_ref_box("Wall_Right", (W/2, H/2, 0), (0.15, H, D), wall_mat, ref_col)

    # ── Furniture zones (wireframe boxes) ──
    for name, info in FURNITURE.items():
        mat = win_mat if name == 'window' else furn_mat
        _add_ref_box(name, info['pos'], info['size'], mat, ref_col)

    # ── Lighting (matches LightingPreset_SpaceSuite) ──
    # Earth blue key light from window direction
    key = bpy.data.lights.new("Key_Earth", 'SUN')
    key.energy = 2.0
    key.color = (0.4, 0.6, 0.9)
    key_obj = bpy.data.objects.new("Key_Earth", key)
    ref_col.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(-50), math.radians(-30), 0)

    # Warm amber fill — floor bounce
    fill = bpy.data.lights.new("Fill_Amber", 'SUN')
    fill.energy = 0.8
    fill.color = (0.9, 0.7, 0.4)
    fill_obj = bpy.data.objects.new("Fill_Amber", fill)
    ref_col.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(30), math.radians(20), 0)

    # Point lights matching engine preset
    point_data = [
        ("Ceiling_Bed", (0, 4.8, -4.0), 12.0, (0.9, 0.7, 0.4), 80),
        ("Bedside_L", (-2.5, 0.85, -4.8), 5.0, (0.85, 0.65, 0.4), 30),
        ("Bedside_R", (2.5, 0.85, -4.8), 5.0, (0.85, 0.65, 0.4), 30),
        ("Earth_Glow", (-6.5, 2.5, -0.5), 14.0, (0.3, 0.55, 0.9), 60),
        ("Pendant", (-3, 4.0, 3.5), 6.0, (0.85, 0.65, 0.35), 40),
        ("Entry_Sconce", (3, 2.2, 5.5), 4.0, (0.8, 0.6, 0.3), 15),
    ]
    for name, pos, radius, color, energy in point_data:
        light = bpy.data.lights.new(f"PT_{name}", 'POINT')
        light.energy = energy
        light.color = color
        light.shadow_soft_size = radius * 0.1
        light.use_shadow = True
        obj = bpy.data.objects.new(f"PT_{name}", light)
        obj.location = pos
        ref_col.objects.link(obj)

    # ── Camera (player spawn view) ──
    cam = bpy.data.cameras.new("SuiteCam")
    cam.lens = 28  # ~70 FOV horizontal at 960x600 aspect
    cam.clip_start = 0.05
    cam.clip_end = 300
    cam_obj = bpy.data.objects.new("SuiteCam", cam)
    cam_obj.location = SUITE['spawn']
    cam_obj.rotation_euler = (math.radians(90), 0, math.radians(180))
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    # ── Render settings (match engine 1920x1200) ──
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1200
    scene.render.resolution_percentage = 100
    scene.cycles.samples = 64  # fast preview
    scene.cycles.use_denoising = True
    try:
        scene.cycles.device = 'GPU'
    except:
        pass
    # Dark background (void)
    scene.world = bpy.data.worlds.get("World") or bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.01, 0.01, 0.02, 1.0)
        bg.inputs["Strength"].default_value = 0.5

    # ── Lock reference collection (non-selectable) ──
    view_layer = bpy.context.view_layer
    ref_lc = None
    for lc in view_layer.layer_collection.children:
        if lc.name == "Reference":
            ref_lc = lc
            break
    # Can't lock selectability via Python easily, but we can exclude from render
    # Reference geometry should NOT appear in final renders of the prop
    # (we'll hide it in workbench_export)

    print(f"[EV WORKBENCH] Suite reference built:")
    print(f"  Room: {W}x{D}x{H}m")
    print(f"  {len(FURNITURE)} furniture zones (wireframe)")
    print(f"  6 point lights + key/fill (engine-matched)")
    print(f"  Camera at spawn: {SUITE['spawn']}")
    print(f"  Render: 1920x1200 Cycles 64spp")
    print(f"\n  Your models go in the 'Props' collection.")
    print(f"  Use workbench_preview(kit) to render + validate.")
    print(f"  Use workbench_export(kit, path) to export GLB + get C code.")


def workbench_place(kit, position=(0, 0, 0), rotation_deg=0):
    """
    Place the current kit model at a position in the suite.
    Moves the active object into the Props collection.
    """
    obj = bpy.context.active_object
    if not obj:
        print("[EV WORKBENCH] No active object to place!")
        return

    obj.location = position
    obj.rotation_euler = (0, math.radians(rotation_deg), 0)

    # Move to Props collection
    props_col = bpy.data.collections.get("Props")
    if props_col:
        for c in obj.users_collection:
            c.objects.unlink(obj)
        props_col.objects.link(obj)

    print(f"[EV WORKBENCH] Placed '{obj.name}' at {position}, rot={rotation_deg}")


def workbench_preview(kit=None, position=None, rotation_deg=0, camera_angle='spawn'):
    """
    Render a 960x600 preview of the prop in the suite context.
    Also runs kit.validate() if a kit is provided.

    camera_angle options:
        'spawn'   — player spawn view (default)
        'top'     — orthographic top-down
        'close'   — close-up of the prop
    """
    if kit:
        passed, report = kit.validate()

    # Set up camera
    cam_obj = bpy.data.objects.get("SuiteCam")
    if cam_obj:
        if camera_angle == 'spawn':
            cam_obj.location = SUITE['spawn']
            cam_obj.rotation_euler = (math.radians(90), 0, math.radians(180))
        elif camera_angle == 'close' and position:
            # Look at the prop from 2m away
            cx, cy, cz = position
            cam_obj.location = (cx + 1.5, cy + 1.0, cz + 1.5)
            # Point at prop
            direction = (cx - cam_obj.location[0],
                        cy - cam_obj.location[1],
                        cz - cam_obj.location[2])
            import mathutils
            rot = mathutils.Vector(direction).to_track_quat('-Z', 'Y')
            cam_obj.rotation_euler = rot.to_euler()
        elif camera_angle == 'top':
            cam_obj.location = (0, 12, 0)
            cam_obj.rotation_euler = (0, 0, 0)

    # Render preview
    output_dir = os.path.join(SCRIPT_DIR, '..', 'qa', 'blender_previews')
    os.makedirs(output_dir, exist_ok=True)

    name = kit.model_name if kit else "preview"
    filepath = os.path.join(output_dir, f"{name}_preview.png")
    bpy.context.scene.render.filepath = filepath
    bpy.ops.render.render(write_still=True)

    print(f"\n[EV WORKBENCH] Preview rendered: {filepath}")
    if position:
        print(f"  Position: {position}")
        print(f"  Rotation: {rotation_deg}")

    return filepath


def workbench_export(kit, export_dir=None, position=(0, 0, 0), rotation_deg=0,
                     material_id="MAT_CONCRETE", color="(Color){255,255,255,255}"):
    """
    Export the prop as GLB and print the engine placement code.

    Args:
        kit: EVModelKit instance
        export_dir: directory for GLB (default: assets/)
        position: (x, y, z) for engine placement
        rotation_deg: Y rotation in degrees
        material_id: engine material constant (e.g. MAT_BRASS)
        color: engine Color literal
    """
    if not export_dir:
        export_dir = os.path.join(SCRIPT_DIR, '..', 'assets')

    name = kit.model_name.lower()
    filepath = os.path.join(export_dir, f"{name}.glb")

    # Hide reference collection for clean export
    ref_col = bpy.data.collections.get("Reference")
    ref_was_visible = True
    if ref_col:
        # Hide all reference objects
        for obj in ref_col.objects:
            obj.hide_set(True)
            obj.hide_render = True

    # Select only Props collection objects
    bpy.ops.object.select_all(action='DESELECT')
    props_col = bpy.data.collections.get("Props")
    if props_col:
        for obj in props_col.objects:
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj

    # Export
    bpy.ops.export_scene.gltf(
        filepath=filepath,
        export_format='GLB',
        export_yup=True,
        export_apply=True,
        use_selection=True,
    )

    # Unhide reference
    if ref_col:
        for obj in ref_col.objects:
            obj.hide_set(False)
            obj.hide_render = False

    # Print engine placement code
    x, y, z = position
    print(f"\n[EV WORKBENCH] Exported: {filepath}")
    print(f"\n// ── Paste into build_space_suite() in scene.c ──")
    print(f"{{")
    print(f"    int {name}_mdl = find_model_asset(\"{name}\");")
    print(f"    if ({name}_mdl >= 0) {{")
    print(f"        add_model(s, {x}f, {y}f, {z}f, 1,1,1, {rotation_deg}, {name}_mdl, {material_id}, {color});")
    print(f"    }}")
    print(f"}}")
    print(f"\n// Also add a registry entry in src/model_registry.c:")
    print(f"{{\"{name}\", \"assets/{name}.glb\", MODEL_KIND_PROP, false, 3, true, MODEL_STATUS_ACTIVE}},")
    print(f"// Canonical path: ./scripts/mcp_model.sh full scripts/model_{name}.py {name}")
    print(f"// Quick checks: ./scripts/glb_qa.sh assets/{name}.glb")
    print(f"// Registry gate: python3 scripts/validate_model_registry.py")

    return filepath


# ─────────────────────────────────────────────────────────────────────────────
# AUTO-SETUP: Run workbench_setup() when this file is executed
# ─────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__" or 'workbench_setup' not in dir():
    workbench_setup()
