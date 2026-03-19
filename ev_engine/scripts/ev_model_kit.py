#!/usr/bin/env python3
"""
ev_model_kit.py — Endearing Void Model Production Kit
=====================================================

Import this at the top of EVERY model script. It provides:
  1. Locked material palette (maps directly to engine materialIds)
  2. Form primitives (rounded cubes, tapered legs, soft edges)
  3. Validation gates (tri budget, scale, material compliance)
  4. One-call export with QA report

Usage:
    from ev_model_kit import *
    kit = EVModelKit()
    kit.begin("Armchair", tri_budget=200)

    # Use kit materials (never define your own)
    seat = kit.rounded_cube("Seat", (0, 0.38, 0), (0.5, 0.08, 0.5),
                            material=kit.LEATHER_NAVY, subdivisions=1)
    leg = kit.cylinder("Leg", (0.2, 0.175, 0.2), radius=0.02, depth=0.35,
                       material=kit.WOOD_DARK)

    kit.export_obj("/Users/klaus/armchair.obj")

THE 12 RULES OF FORM (Endearing Void)
======================================
 1. SILHOUETTE FIRST — If the outline reads at 1920x1200, the model works.
 2. 3-PIXEL RULE — Nothing smaller than 3px at render resolution (1920x1200). Scale up or delete.
 3. SUBDIVISION = EMOTION — Only soften surfaces the player stares at or touches.
    Structural elements (legs, frames, rails) stay sharp.
 4. MATERIAL HONESTY — Wood is wood, brass is brass. Never fake a material
    with color alone. Use the correct MAT_ constant.
 5. WARM NEUTRALS, ONE ACCENT — Base palette is cream/navy/wood/brass.
    At most ONE saturated accent per model (Godard red, Earth blue).
 6. SCALE IS SACRED — 1 unit = 1 meter. Chair seat = 0.45m. Door = 2.1m.
    Table height = 0.75m. Bed width = 1.8m (queen). Check before export.
 7. JOIN EVERYTHING — One object per model. Multi-material via material slots.
 8. VERTEX COLORS CARRY — OBJ exports with vertex colors. These tint the
    engine's procedural shader. Get them right in Blender.
 9. Y-UP EXPORT — GLB exports Y-up automatically. OBJ must rotate -90 X in engine.
10. ORIGIN AT FLOOR CENTER — Model origin at bottom-center so placement is intuitive.
    add_model(s, x, 0, z, ...) puts the model ON the floor.
11. NO HIDDEN GEOMETRY — No internal faces, no coincident vertices.
    Every triangle must be visible from at least one player viewing angle.
12. LESS IS MORE — If a box says it better, keep the box. Model only when
    curvature or emotional weight demands it.
"""

import bpy
import math
from collections import OrderedDict

# ─────────────────────────────────────────────────────────────────────────────
# LOCKED MATERIAL PALETTE
# Maps to engine's lighting.c materialId system (0-13)
# Format: (name, R, G, B, metallic, roughness, engine_material_id)
# RGB in 0-255 for readability, converted to 0-1 internally
# ─────────────────────────────────────────────────────────────────────────────

class EVMaterial:
    """A locked material definition that maps to an engine materialId."""
    def __init__(self, name, r, g, b, metallic, roughness, engine_id, description=""):
        self.name = name
        self.r, self.g, self.b = r, g, b
        self.metallic = metallic
        self.roughness = roughness
        self.engine_id = engine_id  # MAT_* constant in ev_types.h
        self.description = description
        self._blender_mat = None

    def get_blender_material(self):
        """Get or create the Blender material. Re-validates cache each call."""
        try:
            if self._blender_mat and self._blender_mat.name in bpy.data.materials:
                return self._blender_mat
        except ReferenceError:
            self._blender_mat = None  # stale StructRNA reference
        mat = bpy.data.materials.get(self.name)
        if not mat:
            mat = bpy.data.materials.new(self.name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        if not bsdf:
            bsdf = mat.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
        bsdf.inputs["Base Color"].default_value = (self.r/255, self.g/255, self.b/255, 1.0)
        bsdf.inputs["Metallic"].default_value = self.metallic
        bsdf.inputs["Roughness"].default_value = self.roughness
        self._blender_mat = mat
        return mat


# ── Architectural ──
CONCRETE      = EVMaterial("EV_Concrete",      180, 175, 168, 0.0, 0.85, 0,  "Walls, floors, brutalist")
MARBLE_WHITE  = EVMaterial("EV_Marble_White",   240, 238, 232, 0.0, 0.25, 1,  "Lobby floors, countertops")
MARBLE_DARK   = EVMaterial("EV_Marble_Dark",    60,  58,  55,  0.0, 0.20, 1,  "Accent trim, dark stone")
WOOD_DARK     = EVMaterial("EV_Wood_Dark",      105, 78,  48,  0.0, 0.70, 2,  "Furniture frames, panels")
WOOD_LIGHT    = EVMaterial("EV_Wood_Light",     165, 130, 90,  0.0, 0.65, 2,  "Lighter wood accents")
CARPET_NAVY   = EVMaterial("EV_Carpet_Navy",    25,  25,  50,  0.0, 0.95, 3,  "Hotel corridor carpet")
WALLPAPER     = EVMaterial("EV_Wallpaper",      195, 190, 180, 0.0, 0.80, 4,  "Hotel room walls")
TILE_WHITE    = EVMaterial("EV_Tile_White",     235, 232, 225, 0.0, 0.30, 5,  "Bathroom tile")

# ── Metal ──
BRASS         = EVMaterial("EV_Brass",          184, 157, 107, 0.90, 0.30, 6, "Fixtures, handles, buttons")
BRASS_DULL    = EVMaterial("EV_Brass_Dull",     160, 140, 95,  0.70, 0.45, 6, "Aged brass, worn fixtures")
BRASS_GOLD    = EVMaterial("EV_Brass_Gold",     210, 180, 120, 0.85, 0.25, 6, "Polished gold accents")

# ── Glass ──
GLASS         = EVMaterial("EV_Glass",          220, 225, 230, 0.0, 0.05, 7,  "Windows, glasses, mirrors")
GLASS_WARM    = EVMaterial("EV_Glass_Warm",     220, 200, 180, 0.0, 0.08, 7,  "Champagne, warm glass")

# ── Soft ──
LEATHER_DARK  = EVMaterial("EV_Leather_Dark",   45,  35,  25,  0.0, 0.75, 8,  "Dark leather chairs, bags")
LEATHER_NAVY  = EVMaterial("EV_Leather_Navy",   25,  25,  80,  0.0, 0.75, 8,  "Navy leather upholstery")
LEATHER_TAN   = EVMaterial("EV_Leather_Tan",    140, 110, 75,  0.0, 0.70, 8,  "Tan leather, suitcases")
FABRIC_WHITE  = EVMaterial("EV_Fabric_White",   245, 242, 235, 0.0, 0.90, 9,  "Bedsheets, curtains")
FABRIC_CREAM  = EVMaterial("EV_Fabric_Cream",   215, 210, 200, 0.0, 0.85, 9,  "Duvet, tablecloth")
FABRIC_GREY   = EVMaterial("EV_Fabric_Grey",    160, 155, 148, 0.0, 0.88, 9,  "Cushions, towels")
VELVET_NAVY   = EVMaterial("EV_Velvet_Navy",    25,  25,  80,  0.0, 0.85, 13, "Headboard, curtains, luxury")
VELVET_RED    = EVMaterial("EV_Velvet_Red",     180, 30,  25,  0.0, 0.82, 13, "Godard accent (use sparingly)")

# ── Floor Patterns ──
CHECKER       = EVMaterial("EV_Checker",        200, 195, 185, 0.0, 0.35, 10, "Checkerboard floor")
HERRINGBONE   = EVMaterial("EV_Herringbone",    140, 105, 65,  0.0, 0.60, 11, "Herringbone parquet")
PARQUET       = EVMaterial("EV_Parquet",        130, 100, 60,  0.0, 0.55, 12, "Parquet flooring")

# ── Gibbons Character ──
GIBBONS_HEAD     = EVMaterial("EV_Gibbons_Head",     242, 237, 230, 0.0, 0.15, 0, "Porcelain, SSS")
GIBBONS_JACKET   = EVMaterial("EV_Gibbons_Jacket",   184, 71,  46,  0.0, 0.75, 9, "Coral/terracotta bellhop")
GIBBONS_TROUSERS = EVMaterial("EV_Gibbons_Trousers", 20,  26,  46,  0.0, 0.80, 9, "Navy")
GIBBONS_BUTTONS  = EVMaterial("EV_Gibbons_Buttons",  217, 179, 77,  0.7, 0.45, 6, "Dull gold")
GIBBONS_HALO     = EVMaterial("EV_Gibbons_Halo",     230, 191, 64,  0.85, 0.30, 6, "Gold ring")
GIBBONS_EYES     = EVMaterial("EV_Gibbons_Eyes",     115, 204, 166, 0.0, 0.10, 7, "Mint green, smooth")

# ── Accent (max ONE per model) ──
GODARD_RED    = EVMaterial("EV_Godard_Red",     204, 20,  13,  0.0, 0.70, 9, "THE red. Use once or never.")
EARTH_BLUE    = EVMaterial("EV_Earth_Blue",     51,  102, 191, 0.0, 0.60, 7, "Earth glow accent")

# Complete registry for validation
ALL_MATERIALS = [
    CONCRETE, MARBLE_WHITE, MARBLE_DARK, WOOD_DARK, WOOD_LIGHT,
    CARPET_NAVY, WALLPAPER, TILE_WHITE,
    BRASS, BRASS_DULL, BRASS_GOLD,
    GLASS, GLASS_WARM,
    LEATHER_DARK, LEATHER_NAVY, LEATHER_TAN,
    FABRIC_WHITE, FABRIC_CREAM, FABRIC_GREY,
    VELVET_NAVY, VELVET_RED,
    CHECKER, HERRINGBONE, PARQUET,
    GIBBONS_HEAD, GIBBONS_JACKET, GIBBONS_TROUSERS,
    GIBBONS_BUTTONS, GIBBONS_HALO, GIBBONS_EYES,
    GODARD_RED, EARTH_BLUE,
]

# ─────────────────────────────────────────────────────────────────────────────
# REAL-WORLD SCALE REFERENCE (1 unit = 1 meter)
# ─────────────────────────────────────────────────────────────────────────────

SCALE_REF = {
    'door_height':       2.10,
    'door_width':        0.90,
    'ceiling':           3.00,   # standard hotel
    'ceiling_grand':     6.50,   # lobby
    'table_height':      0.75,
    'desk_height':       0.76,
    'chair_seat':        0.45,
    'chair_back':        0.90,   # total height
    'armrest':           0.65,
    'bed_queen_w':       1.60,
    'bed_queen_l':       2.00,
    'bed_height':        0.55,   # mattress top
    'headboard':         1.40,   # above mattress
    'nightstand':        0.60,
    'bathtub_l':         1.70,
    'bathtub_w':         0.75,
    'bathtub_h':         0.60,
    'piano_grand_l':     1.80,
    'piano_grand_w':     1.50,
    'piano_height':      1.00,   # with lid
    'telephone':         0.20,   # base width
    'champagne_flute_h': 0.25,
    'suitcase_l':        0.65,
    'suitcase_h':        0.45,
    'telescope_h':       1.20,
    'person_height':     1.75,
    'gibbons_height':    1.30,   # shorter than player (vinyl toy)
}


# ─────────────────────────────────────────────────────────────────────────────
# FORM PRIMITIVES
# ─────────────────────────────────────────────────────────────────────────────

class EVModelKit:
    """
    Production toolkit for building EV engine models.

    Enforces the 12 Rules of Form, provides consistent primitives,
    validates output, and exports with QA report.
    """

    def __init__(self):
        self.model_name = None
        self.tri_budget = None
        self.parts = []
        self.materials_used = set()
        self.accent_count = 0
        self._started = False

    def begin(self, name, tri_budget=300):
        """Start a new model. Clears the scene."""
        self.model_name = name
        self.tri_budget = tri_budget
        self.parts = []
        self.materials_used = set()
        self.accent_count = 0
        self._started = True

        # Clear scene
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete()
        for mesh in list(bpy.data.meshes):
            if mesh.users == 0:
                bpy.data.meshes.remove(mesh)
        for mat in list(bpy.data.materials):
            if mat.users == 0:
                bpy.data.materials.remove(mat)
        # Invalidate all cached material references (StructRNA cleanup)
        for m in ALL_MATERIALS:
            m._blender_mat = None

        print(f"\n[EV] ── {name} ── budget: {tri_budget} tris")

    def _track(self, obj, material):
        """Track a part for validation."""
        self.parts.append(obj)
        if material:
            if material.name not in self.materials_used:
                if material in (GODARD_RED, VELVET_RED, EARTH_BLUE):
                    self.accent_count += 1
            self.materials_used.add(material.name)

    def _apply_material(self, obj, material):
        """Apply an EVMaterial to an object."""
        if material:
            bmat = material.get_blender_material()
            if obj.data.materials:
                obj.data.materials[0] = bmat
            else:
                obj.data.materials.append(bmat)

    # ── Primitive: Rounded Cube ──
    def rounded_cube(self, name, location, scale, material=None, subdivisions=1):
        """
        Cube with subdivision for soft edges. Use for:
        - Cushions (subdiv=2), mattresses (subdiv=1), padded surfaces
        Rule 3: Subdivision = Emotion. Only soften what matters.
        """
        bpy.ops.mesh.primitive_cube_add(size=1, location=location)
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        obj.scale = scale
        bpy.ops.object.transform_apply(scale=True)
        if subdivisions > 0:
            mod = obj.modifiers.new("Subsurf", "SUBSURF")
            mod.levels = subdivisions
            mod.render_levels = subdivisions
            bpy.ops.object.modifier_apply(modifier="Subsurf")
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Primitive: Sharp Cube ──
    def cube(self, name, location, scale, material=None):
        """
        Raw cube, no subdivision. Use for:
        - Structural elements (frames, rails, panels)
        - Anything that should read as manufactured/precise
        """
        return self.rounded_cube(name, location, scale, material, subdivisions=0)

    # ── Primitive: Cylinder ──
    def cylinder(self, name, location, radius, depth, material=None,
                 vertices=8, rotation=(0, 0, 0)):
        """
        Cylinder with controlled vertex count. Use for:
        - Legs (vertices=6-8), pipes, rails, columns
        - Set vertices low (6) for tiny details, higher (12) for hero elements
        """
        bpy.ops.mesh.primitive_cylinder_add(
            radius=radius, depth=depth, vertices=vertices,
            location=location, rotation=rotation
        )
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Primitive: Sphere ──
    def sphere(self, name, location, radius, material=None, segments=12, rings=8):
        """UV sphere. Use sparingly — spheres are expensive."""
        bpy.ops.mesh.primitive_uv_sphere_add(
            radius=radius, segments=segments, ring_count=rings,
            location=location
        )
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Primitive: Cone ──
    def cone(self, name, location, radius1, radius2, depth, material=None, vertices=8):
        """Truncated cone. Use for tapered legs, lampshades, etc."""
        bpy.ops.mesh.primitive_cone_add(
            radius1=radius1, radius2=radius2, depth=depth,
            vertices=vertices, location=location
        )
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Primitive: Torus ──
    def torus(self, name, location, major_radius, minor_radius, material=None,
              major_segments=24, minor_segments=8):
        """Torus for rings, handles, decorative elements."""
        bpy.ops.mesh.primitive_torus_add(
            major_radius=major_radius, minor_radius=minor_radius,
            major_segments=major_segments, minor_segments=minor_segments,
            location=location
        )
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Composite: Tapered Leg ──
    def tapered_leg(self, name, location, top_radius, bottom_radius, height,
                    material=None, vertices=6):
        """
        Furniture leg that tapers. Classic mid-century form.
        Top is wider (where it meets the seat), bottom narrows.
        """
        return self.cone(name, location, bottom_radius, top_radius, height,
                        material, vertices)

    # ── Composite: Leg Set ──
    def four_legs(self, prefix, positions, radius, height, material=None, vertices=6):
        """Place 4 identical legs at the given (x, z) positions. Y is auto-calculated."""
        legs = []
        for i, (x, z) in enumerate(positions):
            leg = self.cylinder(f"{prefix}_{i}", (x, height/2, z),
                               radius, height, material, vertices)
            legs.append(leg)
        return legs

    # ── Composite: Beveled Edge ──
    def beveled_cube(self, name, location, scale, material=None,
                     bevel_width=0.01, bevel_segments=2):
        """Cube with beveled edges — reads as manufactured/machined."""
        bpy.ops.mesh.primitive_cube_add(size=1, location=location)
        obj = bpy.context.active_object
        obj.name = f"{self.model_name}_{name}"
        obj.scale = scale
        bpy.ops.object.transform_apply(scale=True)
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel_width
        mod.segments = bevel_segments
        bpy.ops.object.modifier_apply(modifier="Bevel")
        self._apply_material(obj, material)
        self._track(obj, material)
        return obj

    # ── Join + Validate + Export ──

    def join_all(self):
        """Join all parts into one object. Sets origin to bottom-center (Rule 10)."""
        if not self.parts:
            print("[EV] WARNING: No parts to join!")
            return None

        bpy.ops.object.select_all(action='DESELECT')
        for p in self.parts:
            if p and p.name in bpy.data.objects:
                p.select_set(True)
        bpy.context.view_layer.objects.active = self.parts[0]
        bpy.ops.object.join()

        obj = bpy.context.active_object
        obj.name = self.model_name

        # Origin to bottom-center (Rule 10)
        bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
        # Now shift origin to bottom of bounding box
        bbox = [obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box]
        min_y = min(v.y for v in bbox)
        cursor_loc = bpy.context.scene.cursor.location.copy()
        center = obj.location.copy()
        bpy.context.scene.cursor.location = (center.x, min_y, center.z)
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        bpy.context.scene.cursor.location = cursor_loc

        return obj

    def validate(self):
        """
        Run all QA checks. Returns (passed: bool, report: str).
        Checks: tri count, scale, material compliance, accent count, hidden geometry.
        """
        obj = bpy.context.active_object
        if not obj or obj.type != 'MESH':
            return False, "[EV] ERROR: No active mesh object"

        report = []
        passed = True

        # Tri count
        tris = sum(max(0, len(p.vertices) - 2) for p in obj.data.polygons)
        verts = len(obj.data.vertices)
        if tris > self.tri_budget:
            report.append(f"  OVER BUDGET: {tris} tris (budget: {self.tri_budget})")
            passed = False
        else:
            headroom = self.tri_budget - tris
            report.append(f"  Tris: {tris}/{self.tri_budget} ({headroom} headroom)")

        report.append(f"  Verts: {verts}")

        # Bounding box scale check (Rule 6)
        bbox = [obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box]
        dims = obj.dimensions
        report.append(f"  Dimensions: {dims.x:.2f} x {dims.y:.2f} x {dims.z:.2f}m")

        if max(dims) > 5.0:
            report.append(f"  WARNING: Largest dimension {max(dims):.2f}m — seems too big?")
        if max(dims) < 0.05:
            report.append(f"  WARNING: Largest dimension {max(dims):.3f}m — too small at 960x600?")

        # Material compliance
        mat_count = len(obj.data.materials)
        report.append(f"  Materials: {mat_count} slots ({len(self.materials_used)} unique)")
        for mat_slot in obj.data.materials:
            if mat_slot and not mat_slot.name.startswith("EV_"):
                report.append(f"  WARNING: Non-EV material '{mat_slot.name}' — use kit materials")
                passed = False

        # Accent check (Rule 5)
        if self.accent_count > 1:
            report.append(f"  WARNING: {self.accent_count} accent materials (max 1)")

        # Loose vertices check
        import bmesh
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        loose = sum(1 for v in bm.verts if not v.link_edges)
        if loose > 0:
            report.append(f"  WARNING: {loose} loose vertices (hidden geometry)")
        bm.free()

        header = f"\n[EV] ── QA: {self.model_name} ──"
        status = "PASS" if passed else "FAIL"
        footer = f"  Result: {status}"

        full_report = "\n".join([header] + report + [footer])
        print(full_report)
        return passed, full_report

    def export_obj(self, filepath):
        """Export as OBJ with validation gate."""
        obj = self.join_all()
        if not obj:
            return

        passed, report = self.validate()

        if not passed:
            print(f"\n[EV] WARNING: validation issues (see above)")
            print(f"[EV] Exporting anyway — check the warnings")

        self._do_export_obj(filepath)

    def force_export_obj(self, filepath):
        """Export OBJ even if validation fails. Use sparingly."""
        self._do_export_obj(filepath)
        print("[EV] FORCE EXPORTED (validation warnings ignored)")

    def _do_export_obj(self, filepath):
        """Internal OBJ export."""
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.wm.obj_export(
            filepath=filepath,
            export_selected_objects=True,
            apply_modifiers=True,
            export_normals=True,
            export_colors=True,
        )
        print(f"[EV] Exported: {filepath}")

    def export_glb(self, filepath):
        """Export as GLB (for rigged/animated models) with validation."""
        obj = self.join_all()
        if not obj:
            return

        passed, report = self.validate()

        if not passed:
            print(f"\n[EV] EXPORT BLOCKED — fix warnings above")
            return

        bpy.ops.export_scene.gltf(
            filepath=filepath,
            export_format='GLB',
            export_yup=True,
            export_apply=True,
            export_animations=True,
        )
        print(f"[EV] Exported: {filepath}")

    def preview_placement(self):
        """
        Print the C code needed to place this model in a scene.
        Reads materials used and suggests the primary engine materialId.
        """
        # Find the most-used material's engine ID
        primary_engine_id = 0
        for mat in ALL_MATERIALS:
            if mat.name in self.materials_used:
                primary_engine_id = mat.engine_id
                break

        mat_names = {
            0: "MAT_CONCRETE", 1: "MAT_MARBLE", 2: "MAT_WOOD",
            3: "MAT_CARPET", 4: "MAT_WALLPAPER", 5: "MAT_TILE",
            6: "MAT_BRASS", 7: "MAT_GLASS", 8: "MAT_LEATHER",
            9: "MAT_FABRIC", 10: "MAT_CHECKERBOARD", 11: "MAT_HERRINGBONE",
            12: "MAT_PARQUET", 13: "MAT_VELVET",
        }
        mat_name = mat_names.get(primary_engine_id, "MAT_CONCRETE")
        name_lower = self.model_name.lower()

        print(f"""
[EV] Placement code:
    int {name_lower} = find_model_asset("{name_lower}");
    if ({name_lower} >= 0) {{
        add_model(s, 0, 0, 0,    // position (adjust)
                  1, 1, 1,        // scale
                  0,              // rotation degrees
                  {name_lower},   // model_index
                  {mat_name},     // primary material
                  WHITE);         // preserve Blender colors
    }}
""")


# ─────────────────────────────────────────────────────────────────────────────
# CONVENIENCE: import * gives you the kit + all materials
# ─────────────────────────────────────────────────────────────────────────────

import mathutils  # needed for join_all origin calculation
