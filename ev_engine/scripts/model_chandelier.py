#!/usr/bin/env python3
"""Chandelier — Grand lobby hanging light.
6-arm brass fixture with glass globes. ~400 tris.
The warmth of the lobby lives here.
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
GLASS_WARM = mat("EV_GlassWarm", 240, 220, 180, 0, 0.08)
CHAIN = mat("EV_Chain", 160, 140, 95, 0.7, 0.45)

# ── Central hub — ornate brass sphere ──
bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=0.12, location=(0, 0, 0))
hub = bpy.context.active_object
hub.name = "Hub"
hub.data.materials.append(BRASS)

# ── Ceiling mount plate ──
bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=0.15, depth=0.03, location=(0, 0, 0.4))
plate = bpy.context.active_object
plate.name = "Plate"
plate.data.materials.append(BRASS)

# ── Chain link (simplified — single cylinder rod) ──
bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=0.01, depth=0.35, location=(0, 0, 0.2))
chain = bpy.context.active_object
chain.name = "Chain"
chain.data.materials.append(CHAIN)

# ── 6 Arms with glass globes ──
num_arms = 6
arm_radius = 0.4
globe_radius = 0.08

for i in range(num_arms):
    angle = i * (2 * math.pi / num_arms)
    ax = math.cos(angle) * arm_radius * 0.5
    ay = math.sin(angle) * arm_radius * 0.5

    # Arm — curved brass rod (approximated as angled cylinder)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=6, radius=0.012, depth=arm_radius,
        location=(ax, ay, -0.04)
    )
    arm = bpy.context.active_object
    arm.name = f"Arm_{i}"
    # Angle arm outward and slightly down
    arm.rotation_euler = (
        math.radians(15),  # tilt down
        0,
        angle + math.radians(90)  # point outward
    )
    bpy.ops.object.transform_apply(rotation=True)
    arm.data.materials.append(BRASS)

    # Glass globe at arm tip
    gx = math.cos(angle) * arm_radius
    gy = math.sin(angle) * arm_radius
    gz = -0.12

    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=10, ring_count=6, radius=globe_radius,
        location=(gx, gy, gz)
    )
    globe = bpy.context.active_object
    globe.name = f"Globe_{i}"
    # Squash slightly — not perfect sphere, more elegant
    globe.scale[2] = 0.8
    bpy.ops.object.transform_apply(scale=True)
    globe.data.materials.append(GLASS_WARM)

# ── Bottom finial — small brass teardrop ──
bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=6, radius=0.04, location=(0, 0, -0.15))
finial = bpy.context.active_object
finial.name = "Finial"
finial.scale[2] = 1.5
bpy.ops.object.transform_apply(scale=True)
finial.data.materials.append(BRASS)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = hub
bpy.ops.object.join()
hub.name = "Chandelier"

# Origin at top (mounting point)
bpy.context.scene.cursor.location = (0, 0, 0.42)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')

# ── Stats ──
tris = sum(len(p.vertices) - 2 for p in hub.data.polygons)
verts = len(hub.data.vertices)
mats = len(hub.data.materials)
print(f"Chandelier: {verts} verts, {tris} tris, {mats} mats")

# ── Export ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath="/tmp/chandelier.glb",
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_yup=True,
)
size = os.path.getsize("/tmp/chandelier.glb")
print(f"Exported: /tmp/chandelier.glb ({size} bytes, {size/1024:.1f} KB)")
