"""
Gallery v2: Better lighting, closer camera, point lights per model.
"""
import bpy
import math

# ── Clear ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

def mat(name, r, g, b, metallic=0.0, roughness=0.7):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = m.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return m

m_brass = mat("Brass", 184, 157, 107, 0.9, 0.3)
m_brass_d = mat("BrassD", 160, 140, 95, 0.7, 0.45)
m_marble = mat("Marble", 60, 58, 55, 0.0, 0.2)
m_glass = mat("Glass", 220, 225, 230, 0.0, 0.05)
m_glass_w = mat("GlassW", 220, 200, 180, 0.0, 0.08)
m_fab_c = mat("FabC", 215, 210, 200, 0.0, 0.85)
m_fab_g = mat("FabG", 160, 155, 148, 0.0, 0.88)
m_wood = mat("Wood", 105, 78, 48, 0.0, 0.7)
m_leather = mat("Leather", 45, 35, 25, 0.0, 0.75)
m_red = mat("Red", 204, 20, 13, 0.0, 0.7)
m_floor = mat("Floor", 60, 55, 50, 0.0, 0.9)
m_wall = mat("Wall", 200, 195, 188, 0.0, 0.85)
m_ped = mat("Ped", 170, 165, 158, 0.0, 0.7)

# ── Room ──
bpy.ops.mesh.primitive_plane_add(size=8, location=(0, 0, 0))
bpy.context.active_object.data.materials.append(m_floor)
bpy.ops.mesh.primitive_plane_add(size=8, location=(0, -1.5, 2))
bk = bpy.context.active_object
bk.rotation_euler = (math.radians(90), 0, 0)
bk.data.materials.append(m_wall)

# Spacing: models at x = -2, -1, 0, 1, 2, 3
positions = [-2, -1, 0, 1, 2, 3]

def ped(x, h):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.5, h/2))
    p = bpy.context.active_object
    p.scale = (0.3, 0.25, h)
    bpy.ops.object.transform_apply(scale=True)
    p.data.materials.append(m_ped)

# ═══ 1: Floor Lamp ═══
x = -2
bpy.ops.mesh.primitive_cylinder_add(radius=0.18, depth=0.03, vertices=12, location=(x, -0.5, 0.015))
bpy.context.active_object.data.materials.append(m_marble)
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=1.5, vertices=6, location=(x, -0.5, 0.78))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cone_add(radius1=0.15, radius2=0.08, depth=0.2, vertices=10,
    location=(x+0.15, -0.5, 1.8))
bpy.context.active_object.data.materials.append(m_fab_c)

# ═══ 2: Wine Glass ═══
x = -1
ped(x, 0.75)
y = 0.75
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.005, vertices=10, location=(x, -0.5, y))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.12, vertices=6, location=(x, -0.5, y+0.06))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cone_add(radius1=0.01, radius2=0.04, depth=0.10, vertices=10,
    location=(x, -0.5, y+0.17))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cube_add(size=0.015, location=(x+0.038, -0.5, y+0.22))
bpy.context.active_object.data.materials.append(m_red)

# ═══ 3: Photo Frame ═══
x = 0
ped(x, 0.75)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.5, 0.83))
f = bpy.context.active_object
f.scale = (0.10, 0.005, 0.13)
bpy.ops.object.transform_apply(scale=True)
f.data.materials.append(m_brass_d)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.495, 0.83))
p = bpy.context.active_object
p.scale = (0.085, 0.002, 0.115)
bpy.ops.object.transform_apply(scale=True)
p.data.materials.append(m_glass_w)

# ═══ 4: Standing Ashtray ═══
x = 1
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=0.02, vertices=10, location=(x, -0.5, 0.01))
bpy.context.active_object.data.materials.append(m_marble)
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.65, vertices=6, location=(x, -0.5, 0.345))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cone_add(radius1=0.10, radius2=0.08, depth=0.04, vertices=10,
    location=(x, -0.5, 0.695))
bpy.context.active_object.data.materials.append(m_brass)

# ═══ 5: Record Player ═══
x = 2
ped(x, 0.75)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.5, 0.81))
c = bpy.context.active_object
c.scale = (0.25, 0.20, 0.06)
bpy.ops.object.transform_apply(scale=True)
c.data.materials.append(m_wood)
bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.005, vertices=16, location=(x, -0.5, 0.845))
bpy.context.active_object.data.materials.append(m_leather)

# ═══ 6: Room Service Tray ═══
x = 3
ped(x, 0.75)
bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=0.015, vertices=16, location=(x, -0.5, 0.76))
t = bpy.context.active_object
t.scale = (1.2, 1.0, 1.0)
bpy.ops.object.transform_apply(scale=True)
t.data.materials.append(m_brass)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, segments=12, ring_count=6, location=(x, -0.5, 0.84))
d = bpy.context.active_object
d.scale = (1.0, 1.0, 0.7)
bpy.ops.object.transform_apply(scale=True)
d.data.materials.append(m_brass)

# ── Point lights over each model ──
for i, px in enumerate(positions):
    bpy.ops.object.light_add(type='POINT', location=(px, -0.2, 2.5))
    lt = bpy.context.active_object
    lt.name = f"Spot_{i}"
    lt.data.energy = 50
    lt.data.color = (1.0, 0.95, 0.88)
    lt.data.shadow_soft_size = 0.5

# Key sun
bpy.ops.object.light_add(type='SUN', location=(2, 2, 5))
key = bpy.context.active_object
key.data.energy = 2.0
key.data.color = (1.0, 0.93, 0.82)
key.rotation_euler = (math.radians(-50), math.radians(15), 0)

# ── Camera ──
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 50
cam_obj = bpy.data.objects.new("Cam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
cam_obj.location = (0.5, -4, 1.8)
cam_obj.rotation_euler = (math.radians(78), 0, 0)
bpy.context.scene.camera = cam_obj

# ── World ──
world = bpy.context.scene.world or bpy.data.worlds.new("World")
bpy.context.scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get("Background")
if bg:
    bg.inputs["Color"].default_value = (0.03, 0.03, 0.035, 1.0)
    bg.inputs["Strength"].default_value = 1.0

# ── Render ──
bpy.context.scene.render.engine = 'BLENDER_EEVEE'
bpy.context.scene.render.filepath = '/tmp/ev_gallery2.png'
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 1080
bpy.ops.render.render(write_still=True)

import os
size = os.path.getsize('/tmp/ev_gallery2.png')
print(f"[EV] Gallery v2 rendered: {size/1024:.0f} KB")
