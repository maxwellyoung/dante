#!/usr/bin/env python3
"""Reception Desk — The lobby centrepiece.
Curved front, marble top, brass trim, dark wood body.
First thing guests approach. ~500 tris.
"""
import bpy, bmesh, math, os

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
for m in list(bpy.data.meshes):
    if m.users == 0: bpy.data.meshes.remove(m)
for m in list(bpy.data.materials):
    if m.users == 0: bpy.data.materials.remove(m)

# ── Materials ──
def mat(name, r, g, b, metal=0, rough=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1)
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    return m

BRASS = mat("EV_Brass", 184, 157, 107, 0.9, 0.3)
WOOD_DARK = mat("EV_WoodDark", 105, 78, 48, 0, 0.7)
MARBLE = mat("EV_Marble", 240, 238, 232, 0, 0.25)

def half_cylinder(name, radius, depth, z_offset, material, verts=24):
    """Create a half-cylinder (front-facing arc)."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=verts, radius=radius, depth=depth,
        location=(0, 0, z_offset)
    )
    obj = bpy.context.active_object
    obj.name = name
    # Cut to half — remove back half (y > 0)
    bpy.ops.object.mode_set(mode='EDIT')
    bm = bmesh.from_edit_mesh(obj.data)
    result = bmesh.ops.bisect_plane(
        bm, geom=bm.verts[:]+bm.edges[:]+bm.faces[:],
        plane_co=(0, 0, 0), plane_no=(0, 1, 0),
        clear_inner=True
    )
    # Fill the flat back face
    boundary = [e for e in bm.edges if e.is_boundary]
    if boundary:
        try:
            bmesh.ops.edgeloop_fill(bm, edges=boundary)
        except:
            pass
    bmesh.update_edit_mesh(obj.data)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.materials.append(material)
    return obj

# ── Desk body — curved half-cylinder, dark wood ──
body = half_cylinder("Desk_Body", 1.8, 0.95, 0.475, WOOD_DARK)

# ── Marble countertop — slightly wider, thin ──
top = half_cylinder("Desk_Top", 1.9, 0.04, 0.97, MARBLE)

# ── Brass trim strip — decorative band at 60% height ──
trim = half_cylinder("Desk_Trim", 1.85, 0.03, 0.60, BRASS)

# ── Brass foot rail — low, for guests to rest feet ──
rail = half_cylinder("Desk_Rail", 1.7, 0.025, 0.12, BRASS)

# ── Back panel — flat staff side ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.02, 0.475))
back = bpy.context.active_object
back.name = "Desk_Back"
back.scale = (3.6, 0.04, 0.95)
bpy.ops.object.transform_apply(scale=True)
back.data.materials.append(WOOD_DARK)

# ── Shelf behind counter (staff side) ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.3, 0.45))
shelf = bpy.context.active_object
shelf.name = "Desk_Shelf"
shelf.scale = (3.2, 0.4, 0.03)
bpy.ops.object.transform_apply(scale=True)
shelf.data.materials.append(WOOD_DARK)

# ── Join everything ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()

# Origin to floor center
bpy.context.scene.cursor.location = (0, 0, 0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
body.name = "ReceptionDesk"

# ── Stats ──
tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
verts = len(body.data.vertices)
mats = len(body.data.materials)
print(f"ReceptionDesk: {verts} verts, {tris} tris, {mats} mats")

# ── Export ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath="/tmp/reception_desk.glb",
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_yup=True,
)
size = os.path.getsize("/tmp/reception_desk.glb")
print(f"Exported: /tmp/reception_desk.glb ({size} bytes, {size/1024:.1f} KB)")
