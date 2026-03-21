#!/usr/bin/env python3
"""
ev_shell_workbench.py — Blender Level Editor for Gehry-esque Room Shells
=========================================================================

The "level editor" for EV. Blender IS the editor — this script turns it into
a context-aware authoring environment for organic, deconstructivist architecture.

Shell = the visual mesh (curved walls, tilted ceilings, swooping forms)
Collision = invisible AABB boxes defined by the artist, exported as C code

Workflow:
    1. shell_setup("my_room", width=14, depth=12, height=5)
        → Creates reference grid, lighting, camera
    2. Model the shell in Blender (curves, sculpt, whatever)
        → Put visual geometry in 'Shell' collection
    3. collision_box(name, pos, size)
        → Place invisible collision volumes in 'Collision' collection
        → These appear as wireframe orange boxes — the walkable space
    4. shell_preview()
        → Renders the shell with collision overlay, prints stats
    5. shell_export("my_room")
        → Exports shell GLB + generates complete C scene code:
           add_shell() for the visual mesh
           add_collision_wall() for each collision box
           spawn point, lighting preset, the works

Usage (Blender MCP or Python console):
    exec(open('/path/to/ev_shell_workbench.py').read())
    shell_setup("gehry_lounge", width=16, depth=14, height=6)
    # ... model your shell in Blender ...
    collision_box("floor", (0, 0, 0), (16, 0.05, 14))
    collision_box("wall_left", (-8, 3, 0), (0.15, 6, 14))
    # ... add collision volumes ...
    shell_preview()
    shell_export("gehry_lounge")
"""

import bpy
import math
import os
import sys
import json

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__)) if '__file__' in dir() else os.getcwd()
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

# ─────────────────────────────────────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────────────────────────────────────

GRID_COLOR = (0.15, 0.15, 0.18, 1.0)
COLLISION_COLOR = (0.9, 0.4, 0.1, 1.0)    # orange wireframe
SPAWN_COLOR = (0.2, 0.8, 0.3, 1.0)        # green
VOID_BG = (0.01, 0.01, 0.02, 1.0)

# ─────────────────────────────────────────────────────────────────────────────
# STATE — current shell being edited
# ─────────────────────────────────────────────────────────────────────────────

_shell_state = {
    'name': None,
    'width': 14,
    'depth': 12,
    'height': 5,
    'spawn': (0, 1.6, 4),
    'collision_boxes': [],   # list of {'name': str, 'pos': tuple, 'size': tuple}
}


def _make_material(name, color, wireframe=False):
    mat = bpy.data.materials.get(name)
    if not mat:
        mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = mat.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = 0.9
    bsdf.inputs["Alpha"].default_value = 0.15 if wireframe else 0.8
    mat.blend_method = 'BLEND' if wireframe else 'OPAQUE'
    mat.shadow_method = 'NONE'
    return mat


def _ensure_collection(name):
    col = bpy.data.collections.get(name)
    if not col:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col


def _add_wireframe_box(name, pos, size, material, collection):
    bpy.ops.mesh.primitive_cube_add(size=1, location=pos)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    obj.display_type = 'WIRE'
    if obj.data.materials:
        obj.data.materials[0] = material
    else:
        obj.data.materials.append(material)
    for c in obj.users_collection:
        c.objects.unlink(obj)
    collection.objects.link(obj)
    return obj


# ─────────────────────────────────────────────────────────────────────────────
# PUBLIC API
# ─────────────────────────────────────────────────────────────────────────────

def shell_setup(name, width=14, depth=12, height=5, spawn=(0, 1.6, 4)):
    """
    Initialize a new shell editing session.

    Args:
        name: Shell identifier (used for export filename)
        width: Room X dimension in meters
        depth: Room Z dimension in meters
        height: Room Y dimension in meters
        spawn: Player spawn position (x, y, z)
    """
    global _shell_state
    _shell_state = {
        'name': name,
        'width': width,
        'depth': depth,
        'height': height,
        'spawn': spawn,
        'collision_boxes': [],
    }

    print(f"\n[EV SHELL] Setting up '{name}' ({width}x{depth}x{height}m)...")

    # Clear scene
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for mat in list(bpy.data.materials):
        if mat.users == 0:
            bpy.data.materials.remove(mat)
    for c in list(bpy.data.collections):
        bpy.data.collections.remove(c)

    # Collections
    ref_col = _ensure_collection("Reference")
    _ensure_collection("Shell")        # artist puts visual mesh here
    _ensure_collection("Collision")    # collision boxes go here
    _ensure_collection("Interactables")  # furniture/objects (optional)

    # Materials
    grid_mat = _make_material("REF_Grid", GRID_COLOR, wireframe=True)

    # Reference grid — 1m squares on the floor
    W, D, H = width, depth, height
    for x in range(int(-W/2), int(W/2) + 1):
        # X-axis lines
        bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0, 0))
        obj = bpy.context.active_object
        obj.name = f"grid_x_{x}"
        obj.scale = (0.005, 0.005, D)
        bpy.ops.object.transform_apply(scale=True)
        obj.display_type = 'WIRE'
        obj.data.materials.append(grid_mat)
        for c in obj.users_collection:
            c.objects.unlink(obj)
        ref_col.objects.link(obj)

    for z in range(int(-D/2), int(D/2) + 1):
        # Z-axis lines
        bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, z))
        obj = bpy.context.active_object
        obj.name = f"grid_z_{z}"
        obj.scale = (W, 0.005, 0.005)
        bpy.ops.object.transform_apply(scale=True)
        obj.display_type = 'WIRE'
        obj.data.materials.append(grid_mat)
        for c in obj.users_collection:
            c.objects.unlink(obj)
        ref_col.objects.link(obj)

    # Bounding box outline (room extents)
    _add_wireframe_box("REF_Bounds",
                       (0, H/2, 0), (W, H, D),
                       grid_mat, ref_col)

    # Spawn marker
    spawn_mat = _make_material("REF_Spawn", SPAWN_COLOR, wireframe=False)
    bpy.ops.mesh.primitive_cone_add(radius1=0.3, radius2=0, depth=0.5,
                                    location=(spawn[0], spawn[1] - 0.5, spawn[2]))
    cone = bpy.context.active_object
    cone.name = "REF_Spawn"
    cone.data.materials.append(spawn_mat)
    for c in cone.users_collection:
        c.objects.unlink(cone)
    ref_col.objects.link(cone)

    # Lighting — dramatic Gehry-style (strong directional + warm ambient)
    key = bpy.data.lights.new("Key_Dramatic", 'SUN')
    key.energy = 3.0
    key.color = (0.95, 0.85, 0.7)
    key_obj = bpy.data.objects.new("Key_Dramatic", key)
    ref_col.objects.link(key_obj)
    key_obj.rotation_euler = (math.radians(-45), math.radians(-20), 0)

    fill = bpy.data.lights.new("Fill_Cool", 'SUN')
    fill.energy = 0.6
    fill.color = (0.5, 0.6, 0.8)
    fill_obj = bpy.data.objects.new("Fill_Cool", fill)
    ref_col.objects.link(fill_obj)
    fill_obj.rotation_euler = (math.radians(30), math.radians(45), 0)

    # Camera
    cam = bpy.data.cameras.new("ShellCam")
    cam.lens = 28
    cam.clip_start = 0.05
    cam.clip_end = 300
    cam_obj = bpy.data.objects.new("ShellCam", cam)
    cam_obj.location = spawn
    cam_obj.rotation_euler = (math.radians(90), 0, math.radians(180))
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    # Render settings
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1200
    scene.render.resolution_percentage = 100
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    try:
        scene.cycles.device = 'GPU'
    except:
        pass
    scene.world = bpy.data.worlds.get("World") or bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = VOID_BG
        bg.inputs["Strength"].default_value = 0.3

    print(f"[EV SHELL] Ready:")
    print(f"  Room: {W}x{D}x{H}m")
    print(f"  Spawn: {spawn}")
    print(f"  Collections: Shell (visual), Collision (physics), Interactables (objects)")
    print(f"\n  Model your architecture in 'Shell' collection.")
    print(f"  Add collision volumes with collision_box().")
    print(f"  Preview with shell_preview(), export with shell_export().")


def collision_box(name, pos, size):
    """
    Add an invisible collision volume. Shows as orange wireframe in Blender.

    Args:
        name: Identifier (e.g. "floor", "wall_left", "curved_wall_1")
        pos: Center position (x, y, z) in meters
        size: Dimensions (w, h, d) in meters
    """
    col = _ensure_collection("Collision")
    mat = _make_material("COL_Wireframe", COLLISION_COLOR, wireframe=True)

    box_name = f"COL_{name}"
    _add_wireframe_box(box_name, pos, size, mat, col)

    _shell_state['collision_boxes'].append({
        'name': name,
        'pos': pos,
        'size': size,
    })

    print(f"[EV SHELL] Collision: '{name}' at {pos} size {size}")


def collision_floor(y=0, width=None, depth=None):
    """Shortcut: full-room floor collision at given Y height."""
    w = width or _shell_state['width']
    d = depth or _shell_state['depth']
    collision_box("floor", (0, y, 0), (w, 0.05, d))


def collision_walls(thickness=0.15):
    """Shortcut: add 4 walls at room boundaries."""
    W, D, H = _shell_state['width'], _shell_state['depth'], _shell_state['height']
    collision_box("wall_back",  (0, H/2, -D/2), (W, H, thickness))
    collision_box("wall_front", (0, H/2,  D/2), (W, H, thickness))
    collision_box("wall_left",  (-W/2, H/2, 0), (thickness, H, D))
    collision_box("wall_right", ( W/2, H/2, 0), (thickness, H, D))


def collision_ceiling(y=None):
    """Shortcut: full-room ceiling collision."""
    h = y or _shell_state['height']
    W, D = _shell_state['width'], _shell_state['depth']
    collision_box("ceiling", (0, h, 0), (W, 0.05, D))


def set_spawn(x, y, z):
    """Update spawn position."""
    _shell_state['spawn'] = (x, y, z)
    # Update visual marker
    cone = bpy.data.objects.get("REF_Spawn")
    if cone:
        cone.location = (x, y - 0.5, z)
    print(f"[EV SHELL] Spawn updated: ({x}, {y}, {z})")


def shell_preview(camera_angle='spawn'):
    """
    Render a preview of the shell with collision overlay visible.
    """
    output_dir = os.path.join(SCRIPT_DIR, '..', 'qa', 'shell_previews')
    os.makedirs(output_dir, exist_ok=True)

    name = _shell_state['name'] or 'shell_preview'
    filepath = os.path.join(output_dir, f"{name}_preview.png")

    # Count shell tris
    shell_col = bpy.data.collections.get("Shell")
    total_tris = 0
    if shell_col:
        for obj in shell_col.objects:
            if obj.type == 'MESH':
                total_tris += len(obj.data.polygons)

    bpy.context.scene.render.filepath = filepath
    bpy.ops.render.render(write_still=True)

    n_col = len(_shell_state['collision_boxes'])
    print(f"\n[EV SHELL] Preview: {filepath}")
    print(f"  Shell tris: {total_tris}")
    print(f"  Collision boxes: {n_col}")
    print(f"  Wall budget: {n_col + 1} / 2048 (shell model + collision)")

    if total_tris > 5000:
        print(f"  WARNING: {total_tris} tris is heavy for a room shell. Target <3000.")
    if n_col == 0:
        print(f"  WARNING: No collision boxes! Player will fall through everything.")

    return filepath


def shell_export(name=None, export_dir=None, material_id="MAT_CONCRETE",
                 color="(Color){255,255,255,255}", lighting_preset=None):
    """
    Export the shell GLB and generate complete C scene code.

    Args:
        name: Override shell name (default: from shell_setup)
        export_dir: GLB output directory (default: assets/)
        material_id: Engine material for the shell mesh
        color: Engine Color literal
        lighting_preset: Name of a LightingPreset_*() function (optional)
    """
    name = name or _shell_state['name']
    if not name:
        print("[EV SHELL] ERROR: No shell name. Call shell_setup() first.")
        return

    if not export_dir:
        export_dir = os.path.join(SCRIPT_DIR, '..', 'assets')

    # Hide Reference + Collision for clean export
    for col_name in ("Reference", "Collision"):
        col = bpy.data.collections.get(col_name)
        if col:
            for obj in col.objects:
                obj.hide_set(True)
                obj.hide_render = True

    # Select Shell collection objects
    bpy.ops.object.select_all(action='DESELECT')
    shell_col = bpy.data.collections.get("Shell")
    if shell_col:
        for obj in shell_col.objects:
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj

    # Export GLB
    filepath = os.path.join(export_dir, f"{name}.glb")
    bpy.ops.export_scene.gltf(
        filepath=filepath,
        export_format='GLB',
        export_yup=True,
        export_apply=True,
        use_selection=True,
    )

    # Unhide
    for col_name in ("Reference", "Collision"):
        col = bpy.data.collections.get(col_name)
        if col:
            for obj in col.objects:
                obj.hide_set(False)
                obj.hide_render = False

    # ── Generate C code ──
    spawn = _shell_state['spawn']
    boxes = _shell_state['collision_boxes']
    preset = lighting_preset or "SpaceSuite"

    print(f"\n[EV SHELL] Exported: {filepath}")
    print(f"\n// ═══════════════════════════════════════════════════════")
    print(f"// Shell scene: {name}")
    print(f"// Generated by ev_shell_workbench.py")
    print(f"// ═══════════════════════════════════════════════════════")
    print(f"")
    print(f"static void build_{name}(Scene *s) {{")
    print(f"    // ── Visual shell (no collision — mesh is decorative) ──")
    print(f"    add_shell(s, \"{name}\", 0, 0, 0, 1,1,1, 0, {material_id}, {color});")
    print(f"")
    print(f"    // ── Collision volumes (invisible AABB boxes) ──")
    for box in boxes:
        bx, by, bz = box['pos']
        bw, bh, bd = box['size']
        print(f"    add_collision_wall(s, {bx}f, {by}f, {bz}f, {bw}f, {bh}f, {bd}f);  // {box['name']}")
    print(f"")
    print(f"    // ── Spawn ──")
    print(f"    s->spawn = (Vector3){{{spawn[0]}f, {spawn[1]}f, {spawn[2]}f}};")
    print(f"}}")
    print(f"")
    print(f"// ── Scene file (scene_{name}.c) ──")
    print(f"// void {name}_load(void) {{")
    print(f"//     build_{name}(&g.scene);")
    print(f"//     init_player(&g.player, g.scene.spawn);")
    print(f"//     SetSceneLighting(&g.lighting, LightingPreset_{preset}());")
    print(f"//     SetPostFXGrain(&g.postfx, 0.3f);")
    print(f"// }}")
    print(f"")
    print(f"// Add a registry entry in src/model_registry.c:")
    print(f"// {{\"{name}\", \"assets/{name}.glb\", MODEL_KIND_SHELL, false, 5, true, MODEL_STATUS_ACTIVE}},")

    # Also save collision data as JSON for iteration
    json_path = os.path.join(SCRIPT_DIR, '..', 'qa', f"{name}_collision.json")
    os.makedirs(os.path.dirname(json_path), exist_ok=True)
    with open(json_path, 'w') as f:
        json.dump({
            'name': name,
            'dimensions': {
                'width': _shell_state['width'],
                'depth': _shell_state['depth'],
                'height': _shell_state['height'],
            },
            'spawn': list(spawn),
            'collision_boxes': [
                {'name': b['name'], 'pos': list(b['pos']), 'size': list(b['size'])}
                for b in boxes
            ],
        }, f, indent=2)
    print(f"// Canonical path: ./scripts/mcp_model.sh full scripts/model_{name}.py {name}")
    print(f"// Quick checks: ./scripts/glb_qa.sh assets/{name}.glb")
    print(f"// Registry gate: python3 scripts/validate_model_registry.py")
    print(f"// Shell acceptance: make qa-shell")
    print(f"\n// Collision data saved: {json_path}")

    return filepath


# ─────────────────────────────────────────────────────────────────────────────
# SHORTCUTS — common Gehry patterns
# ─────────────────────────────────────────────────────────────────────────────

def collision_ramp(name, start_pos, end_pos, width, steps=8):
    """
    Approximate a ramp/slope with stepped collision boxes.
    For tilted floors, curved walkways, etc.

    Args:
        name: Base name for the collision boxes
        start_pos: (x, y, z) of ramp start (lower end)
        end_pos: (x, y, z) of ramp end (higher end)
        width: Width of the ramp perpendicular to direction
        steps: Number of collision boxes (more = smoother)
    """
    sx, sy, sz = start_pos
    ex, ey, ez = end_pos

    for i in range(steps):
        t0 = i / steps
        t1 = (i + 1) / steps
        tm = (t0 + t1) / 2

        # Interpolate position
        cx = sx + (ex - sx) * tm
        cy = sy + (ey - sy) * tm
        cz = sz + (ez - sz) * tm

        # Step height
        step_h = abs(ey - sy) / steps + 0.1  # slight overlap

        # Step length along ramp direction
        dx = (ex - sx) / steps
        dz = (ez - sz) / steps
        step_len = math.sqrt(dx*dx + dz*dz)

        collision_box(f"{name}_{i}", (cx, cy, cz), (width, step_h, step_len))


def collision_curve(name, center, radius, height, arc_degrees=180,
                    start_angle=0, segments=12, thickness=0.15):
    """
    Approximate a curved wall with collision boxes.
    For Gehry swoops, curved corridors, organic walls.

    Args:
        name: Base name
        center: (x, y, z) center of curve
        radius: Distance from center to wall
        height: Wall height
        arc_degrees: How much of the circle to cover
        start_angle: Starting angle in degrees
        segments: Number of boxes (more = smoother)
        thickness: Wall thickness
    """
    cx, cy, cz = center
    arc_rad = math.radians(arc_degrees)
    start_rad = math.radians(start_angle)
    seg_angle = arc_rad / segments

    for i in range(segments):
        a = start_rad + seg_angle * (i + 0.5)
        # Position on arc
        bx = cx + math.cos(a) * radius
        bz = cz + math.sin(a) * radius

        # Size: width is chord length, depth is thickness
        chord = 2 * radius * math.sin(seg_angle / 2) + 0.02  # slight overlap

        collision_box(f"{name}_{i}",
                      (bx, cy + height/2, bz),
                      (chord, height, thickness))


# ─────────────────────────────────────────────────────────────────────────────
# INFO
# ─────────────────────────────────────────────────────────────────────────────

print("""
[EV SHELL WORKBENCH] Loaded.
  shell_setup(name, width, depth, height)  — start a new shell
  collision_box(name, pos, size)           — add collision volume
  collision_floor(y)                       — full-room floor
  collision_walls()                        — 4 boundary walls
  collision_ceiling(y)                     — full-room ceiling
  collision_ramp(name, start, end, width)  — stepped ramp collision
  collision_curve(name, center, r, h)      — curved wall collision
  shell_preview()                          — render + stats
  shell_export()                           — GLB + C code
""")
