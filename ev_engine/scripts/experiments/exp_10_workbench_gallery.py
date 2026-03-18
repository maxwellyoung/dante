"""
Gallery v3: Workbench renderer (solid mode) — guaranteed visible.
Focus on silhouettes which is what matters at 960x600.
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
    # Also set viewport display color for Workbench
    m.diffuse_color = (r/255, g/255, b/255, 1.0)
    return m

m_brass = mat("Brass", 184, 157, 107, 0.9, 0.3)
m_brass_d = mat("BrassD", 160, 140, 95, 0.7, 0.45)
m_marble = mat("Marble", 80, 75, 70, 0.0, 0.2)
m_glass = mat("Glass", 200, 210, 220, 0.0, 0.05)
m_glass_w = mat("GlassW", 220, 200, 180, 0.0, 0.08)
m_fab_c = mat("FabC", 215, 210, 200, 0.0, 0.85)
m_fab_g = mat("FabG", 160, 155, 148, 0.0, 0.88)
m_wood = mat("Wood", 130, 95, 60, 0.0, 0.7)
m_leather = mat("Leather", 65, 50, 35, 0.0, 0.75)
m_red = mat("Red", 204, 20, 13, 0.0, 0.7)
m_floor = mat("Floor", 80, 75, 70, 0.0, 0.9)
m_ped = mat("Ped", 180, 175, 168, 0.0, 0.7)

# ── Floor ──
bpy.ops.mesh.primitive_plane_add(size=10, location=(0, 0, 0))
bpy.context.active_object.data.materials.append(m_floor)

positions = [-2, -0.8, 0.4, 1.6, 2.8, 4.0]

def ped(x, h):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0, h/2))
    p = bpy.context.active_object
    p.scale = (0.35, 0.3, h)
    bpy.ops.object.transform_apply(scale=True)
    p.data.materials.append(m_ped)

# 1: Floor Lamp
x = positions[0]
bpy.ops.mesh.primitive_cylinder_add(radius=0.18, depth=0.03, vertices=12, location=(x, 0, 0.015))
bpy.context.active_object.data.materials.append(m_marble)
bpy.ops.mesh.primitive_cylinder_add(radius=0.015, depth=1.5, vertices=6, location=(x, 0, 0.78))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cylinder_add(radius=0.01, depth=0.4, vertices=6,
    location=(x+0.12, 0, 1.65), rotation=(0, math.radians(25), 0))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cone_add(radius1=0.15, radius2=0.08, depth=0.2, vertices=10,
    location=(x+0.2, 0, 1.8))
bpy.context.active_object.data.materials.append(m_fab_c)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.02, segments=8, ring_count=6, location=(x, 0, 1.53))
bpy.context.active_object.data.materials.append(m_brass)

# 2: Wine Glass
x = positions[1]
ped(x, 0.75)
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.005, vertices=10, location=(x, 0, 0.755))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.12, vertices=6, location=(x, 0, 0.815))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cone_add(radius1=0.01, radius2=0.04, depth=0.10, vertices=10,
    location=(x, 0, 0.925))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_torus_add(major_radius=0.04, minor_radius=0.003, major_segments=12,
    minor_segments=4, location=(x, 0, 0.975))
bpy.context.active_object.data.materials.append(m_glass)
bpy.ops.mesh.primitive_cube_add(size=0.015, location=(x+0.038, 0, 0.975))
lip = bpy.context.active_object
lip.scale = (0.5, 2.0, 1.5)
bpy.ops.object.transform_apply(scale=True)
lip.data.materials.append(m_red)

# 3: Photo Frame
x = positions[2]
ped(x, 0.75)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0, 0.83))
f = bpy.context.active_object
f.scale = (0.10, 0.005, 0.13)
bpy.ops.object.transform_apply(scale=True)
f.data.materials.append(m_brass_d)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0.004, 0.83))
p = bpy.context.active_object
p.scale = (0.085, 0.002, 0.115)
bpy.ops.object.transform_apply(scale=True)
p.data.materials.append(m_glass_w)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.03, 0.79))
s = bpy.context.active_object
s.scale = (0.008, 0.04, 0.08)
bpy.ops.object.transform_apply(scale=True)
s.rotation_euler = (math.radians(15), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
s.data.materials.append(m_brass_d)

# 4: Standing Ashtray
x = positions[3]
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=0.02, vertices=10, location=(x, 0, 0.01))
bpy.context.active_object.data.materials.append(m_marble)
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.65, vertices=6, location=(x, 0, 0.345))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cone_add(radius1=0.10, radius2=0.08, depth=0.04, vertices=10,
    location=(x, 0, 0.695))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_torus_add(major_radius=0.10, minor_radius=0.005, major_segments=12,
    minor_segments=4, location=(x, 0, 0.715))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.07, vertices=4,
    location=(x+0.03, 0, 0.70), rotation=(0, math.radians(75), math.radians(20)))
bpy.context.active_object.data.materials.append(m_fab_g)

# 5: Record Player
x = positions[4]
ped(x, 0.75)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0, 0.81))
c = bpy.context.active_object
c.scale = (0.25, 0.20, 0.06)
bpy.ops.object.transform_apply(scale=True)
c.data.materials.append(m_wood)
bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.005, vertices=16, location=(x, 0, 0.845))
bpy.context.active_object.data.materials.append(m_leather)
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.02, vertices=6, location=(x, 0, 0.86))
bpy.context.active_object.data.materials.append(m_brass)
bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -0.095, 0.87))
lid = bpy.context.active_object
lid.scale = (0.24, 0.003, 0.09)
bpy.ops.object.transform_apply(scale=True)
lid.rotation_euler = (math.radians(-60), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
lid.data.materials.append(m_wood)
bpy.ops.mesh.primitive_cylinder_add(radius=0.003, depth=0.12, vertices=4,
    location=(x+0.04, -0.02, 0.86), rotation=(0, math.radians(80), math.radians(-30)))
bpy.context.active_object.data.materials.append(m_brass)

# 6: Room Service Tray
x = positions[5]
ped(x, 0.75)
bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=0.015, vertices=16, location=(x, 0, 0.76))
t = bpy.context.active_object
t.scale = (1.2, 1.0, 1.0)
bpy.ops.object.transform_apply(scale=True)
t.data.materials.append(m_brass)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, segments=12, ring_count=6, location=(x, 0, 0.84))
d = bpy.context.active_object
d.scale = (1.0, 1.0, 0.7)
bpy.ops.object.transform_apply(scale=True)
d.data.materials.append(m_brass)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.018, segments=8, ring_count=4, location=(x, 0, 0.94))
bpy.context.active_object.data.materials.append(m_brass)
# Napkin
bpy.ops.mesh.primitive_cube_add(size=0.1, location=(x+0.28, 0, 0.775))
n = bpy.context.active_object
n.scale = (1.2, 0.8, 0.15)
bpy.ops.object.transform_apply(scale=True)
n.data.materials.append(m_fab_c)

# ── Camera (slightly above, looking down at row) ──
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 35
cam_obj = bpy.data.objects.new("Cam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
cam_obj.location = (1.0, -5, 2.0)
cam_obj.rotation_euler = (math.radians(75), 0, 0)
bpy.context.scene.camera = cam_obj

# ── Render with Workbench (solid shading — always visible) ──
bpy.context.scene.render.engine = 'BLENDER_WORKBENCH'
bpy.context.scene.display.shading.light = 'STUDIO'
bpy.context.scene.display.shading.color_type = 'MATERIAL'
bpy.context.scene.display.shading.studio_light = 'studio.sl'
bpy.context.scene.render.filepath = '/tmp/ev_gallery3.png'
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 1080

bpy.ops.render.render(write_still=True)

import os
size = os.path.getsize('/tmp/ev_gallery3.png')
print(f"[EV] Gallery v3 (Workbench): {size/1024:.0f} KB")
