"""
Combined gallery: build all models from scratch + camera + render.
No file imports needed — everything procedural.
"""
import bpy
import math

# ── Clear ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

def make_mat(name, r, g, b, metallic=0.0, roughness=0.7):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = mat.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat

# Materials
m_brass = make_mat("EV_Brass", 184, 157, 107, 0.9, 0.3)
m_brass_d = make_mat("EV_Brass_Dull", 160, 140, 95, 0.7, 0.45)
m_marble = make_mat("EV_Marble_Dark", 60, 58, 55, 0.0, 0.2)
m_glass = make_mat("EV_Glass", 220, 225, 230, 0.0, 0.05)
m_glass_w = make_mat("EV_Glass_Warm", 220, 200, 180, 0.0, 0.08)
m_fab_c = make_mat("EV_Fabric_Cream", 215, 210, 200, 0.0, 0.85)
m_fab_g = make_mat("EV_Fabric_Grey", 160, 155, 148, 0.0, 0.88)
m_fab_w = make_mat("EV_Fabric_White", 245, 242, 235, 0.0, 0.9)
m_wood = make_mat("EV_Wood_Dark", 105, 78, 48, 0.0, 0.7)
m_leather = make_mat("EV_Leather_Dark", 45, 35, 25, 0.0, 0.75)
m_red = make_mat("EV_Godard_Red", 204, 20, 13, 0.0, 0.7)
m_floor = make_mat("Gallery_Floor", 30, 28, 25, 0.0, 0.95)
m_wall = make_mat("Gallery_Wall", 220, 218, 212, 0.0, 0.85)
m_ped = make_mat("Pedestal", 180, 178, 172, 0.0, 0.7)

# ── Gallery room ──
bpy.ops.mesh.primitive_plane_add(size=12, location=(0, 0, 0))
bpy.context.active_object.data.materials.append(m_floor)

bpy.ops.mesh.primitive_plane_add(size=12, location=(0, -3, 3))
bk = bpy.context.active_object
bk.rotation_euler = (math.radians(90), 0, 0)
bk.data.materials.append(m_wall)

def pedestal(x, y, h):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(x, y, h/2))
    p = bpy.context.active_object
    p.scale = (0.35, 0.25, h)
    bpy.ops.object.transform_apply(scale=True)
    p.data.materials.append(m_ped)

# ═══ Model 1: Floor Lamp (x=-3) ═══
ox = -3.0
# Base
bpy.ops.mesh.primitive_cylinder_add(radius=0.18, depth=0.03, vertices=12, location=(ox, -1.5, 0.015))
bpy.context.active_object.data.materials.append(m_marble)
# Stem
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=1.5, vertices=6, location=(ox, -1.5, 0.78))
bpy.context.active_object.data.materials.append(m_brass)
# Arc
bpy.ops.mesh.primitive_cylinder_add(radius=0.01, depth=0.4, vertices=6,
    location=(ox+0.1, -1.5, 1.65), rotation=(0, math.radians(25), 0))
bpy.context.active_object.data.materials.append(m_brass)
# Shade
bpy.ops.mesh.primitive_cone_add(radius1=0.15, radius2=0.08, depth=0.2, vertices=10,
    location=(ox+0.2, -1.5, 1.8))
bpy.context.active_object.data.materials.append(m_fab_c)

# ═══ Model 2: Wine Glass (x=-1.5) ═══
ox = -1.5
pedestal(ox, -1.5, 0.75)
gy = 0.75
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.005, vertices=10, location=(ox, -1.5, gy+0.0025))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.12, vertices=6, location=(ox, -1.5, gy+0.065))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cone_add(radius1=0.01, radius2=0.04, depth=0.10, vertices=10,
    location=(ox, -1.5, gy+0.175))
bpy.context.active_object.data.materials.append(m_glass)
# Lipstick
bpy.ops.mesh.primitive_cube_add(size=0.01, location=(ox+0.038, -1.5, gy+0.225))
lip = bpy.context.active_object
lip.scale = (0.5, 2.0, 1.5)
bpy.ops.object.transform_apply(scale=True)
lip.data.materials.append(m_red)

# ═══ Model 3: Photo Frame (x=-0.3) ═══
ox = -0.3
pedestal(ox, -1.5, 0.75)
bpy.ops.mesh.primitive_cube_add(size=1, location=(ox, -1.5, 0.75+0.08))
f = bpy.context.active_object
f.scale = (0.10, 0.005, 0.13)
bpy.ops.object.transform_apply(scale=True)
f.data.materials.append(m_brass_d)
bpy.ops.mesh.primitive_cube_add(size=1, location=(ox, -1.495, 0.75+0.08))
p = bpy.context.active_object
p.scale = (0.085, 0.002, 0.115)
bpy.ops.object.transform_apply(scale=True)
p.data.materials.append(m_glass_w)

# ═══ Model 4: Standing Ashtray (x=0.8) ═══
ox = 0.8
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=0.02, vertices=10, location=(ox, -1.5, 0.01))
bpy.context.active_object.data.materials.append(m_marble)
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.65, vertices=6, location=(ox, -1.5, 0.345))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cone_add(radius1=0.10, radius2=0.08, depth=0.04, vertices=10,
    location=(ox, -1.5, 0.695))
bpy.context.active_object.data.materials.append(m_brass)
# Cigarette
bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.07, vertices=4,
    location=(ox+0.03, -1.5, 0.70), rotation=(0, math.radians(75), math.radians(20)))
bpy.context.active_object.data.materials.append(m_fab_g)

# ═══ Model 5: Record Player (x=2.2) ═══
ox = 2.2
pedestal(ox, -1.5, 0.75)
gy = 0.75
bpy.ops.mesh.primitive_cube_add(size=1, location=(ox, -1.5, gy+0.06))
c = bpy.context.active_object
c.scale = (0.25, 0.20, 0.06)
bpy.ops.object.transform_apply(scale=True)
c.data.materials.append(m_wood)
# Platter
bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.005, vertices=16, location=(ox, -1.5, gy+0.0925))
bpy.context.active_object.data.materials.append(m_leather)
# Lid (open)
bpy.ops.mesh.primitive_cube_add(size=1, location=(ox, -1.595, gy+0.12))
lid = bpy.context.active_object
lid.scale = (0.24, 0.003, 0.09)
bpy.ops.object.transform_apply(scale=True)
lid.rotation_euler = (math.radians(-60), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
lid.data.materials.append(m_wood)

# ═══ Model 6: Room Service Tray (x=3.5) ═══
ox = 3.5
pedestal(ox, -1.5, 0.75)
gy = 0.75
bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=0.015, vertices=16, location=(ox, -1.5, gy+0.0075))
t = bpy.context.active_object
t.scale = (1.3, 1.0, 1.0)
bpy.ops.object.transform_apply(scale=True)
t.data.materials.append(m_brass)
# Dome
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.14, segments=12, ring_count=6, location=(ox, -1.5, gy+0.08))
d = bpy.context.active_object
d.scale = (1.0, 1.0, 0.7)
bpy.ops.object.transform_apply(scale=True)
d.data.materials.append(m_brass)
# Handle
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.018, segments=8, ring_count=4, location=(ox, -1.5, gy+0.2))
bpy.context.active_object.data.materials.append(m_brass)

# ── Lighting ──
bpy.ops.object.light_add(type='SUN', location=(3, 3, 5))
key = bpy.context.active_object
key.data.energy = 3.0
key.data.color = (1.0, 0.93, 0.82)
key.rotation_euler = (math.radians(-45), math.radians(20), 0)

bpy.ops.object.light_add(type='SUN', location=(-3, 2, 4))
fill = bpy.context.active_object
fill.data.energy = 1.2
fill.data.color = (0.82, 0.88, 1.0)
fill.rotation_euler = (math.radians(-60), math.radians(-30), 0)

# ── Camera ──
cam_data = bpy.data.cameras.new("GalleryCam")
cam_data.lens = 40
cam_obj = bpy.data.objects.new("GalleryCam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
cam_obj.location = (0.5, -6, 2.5)
cam_obj.rotation_euler = (math.radians(72), 0, 0)
bpy.context.scene.camera = cam_obj

# ── World ──
world = bpy.context.scene.world or bpy.data.worlds.new("World")
bpy.context.scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get("Background")
if bg:
    bg.inputs["Color"].default_value = (0.015, 0.015, 0.02, 1.0)
    bg.inputs["Strength"].default_value = 0.3

# ── Render ──
bpy.context.scene.render.engine = 'BLENDER_EEVEE'
bpy.context.scene.render.filepath = '/tmp/ev_gallery.png'
bpy.context.scene.render.resolution_x = 960
bpy.context.scene.render.resolution_y = 600
bpy.ops.render.render(write_still=True)

import os
size = os.path.getsize('/tmp/ev_gallery.png')
print(f"[EV] Gallery rendered: {size/1024:.0f} KB — 6 models on display")
