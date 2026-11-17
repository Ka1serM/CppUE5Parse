import bpy, math, sys, os
out = sys.argv[sys.argv.index("--") + 1]

def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def mat(name, color):
    m = bpy.data.materials.new(name)
    m.diffuse_color = color
    return m

def add_uv2_and_colors(obj):
    me = obj.data
    me.uv_layers.new(name="Lightmap")
    attr = me.color_attributes.new("Col", 'BYTE_COLOR', 'CORNER')
    for i, d in enumerate(attr.data):
        d.color = ((i % 7) / 6.0, (i % 5) / 4.0, (i % 3) / 2.0, 1.0)

def static_mesh():
    reset()
    bpy.ops.mesh.primitive_cube_add(size=100)
    cube = bpy.context.active_object
    bpy.ops.object.modifier_add(type='BEVEL')
    cube.modifiers[0].width = 8
    cube.modifiers[0].segments = 2
    bpy.ops.object.modifier_apply(modifier=cube.modifiers[0].name)
    bpy.ops.mesh.primitive_cylinder_add(radius=25, depth=120, location=(0, 0, 110))
    cyl = bpy.context.active_object
    cube.data.materials.append(mat("M_Body", (0.8, 0.2, 0.2, 1)))
    cyl.data.materials.append(mat("M_Top", (0.2, 0.2, 0.8, 1)))
    bpy.ops.object.select_all(action='DESELECT')
    cube.select_set(True); cyl.select_set(True)
    bpy.context.view_layer.objects.active = cube
    bpy.ops.object.join()
    cube.name = "SM_Ref"
    add_uv2_and_colors(cube)
    bpy.ops.mesh.primitive_cube_add(size=104, location=(0, 0, 0))
    col = bpy.context.active_object
    col.name = "UCX_SM_Ref_00"
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, "SM_Ref.fbx"), use_selection=True, global_scale=1.0,
                             apply_unit_scale=True, mesh_smooth_type='FACE', add_leaf_bones=False)

def skeletal_mesh():
    reset()
    bpy.ops.object.armature_add(location=(0, 0, 0))
    arm = bpy.context.active_object
    arm.name = "Armature"
    bpy.ops.object.mode_set(mode='EDIT')
    eb = arm.data.edit_bones
    root = eb[0]; root.name = "root"; root.head = (0, 0, 0); root.tail = (0, 0, 0.5)
    prev = root
    for i in range(1, 4):
        b = eb.new("bone_%d" % i)
        b.head = (0, 0, 0.5 * i); b.tail = (0, 0, 0.5 * (i + 1)); b.parent = prev
        prev = b
    sock = eb.new("socket_tip"); sock.head = prev.tail; sock.tail = (0, 0.1, prev.tail[2]); sock.parent = prev
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=2.0, location=(0, 0, 1.0), vertices=16)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.loopcut_slide(MESH_OT_loopcut={"number_cuts": 7, "edge_index": 1})
    bpy.ops.object.mode_set(mode='OBJECT')
    body = bpy.context.active_object
    body.name = "SK_Ref"
    body.data.materials.append(mat("M_Skin", (0.9, 0.7, 0.5, 1)))
    body.data.materials.append(mat("M_Cloth", (0.1, 0.5, 0.1, 1)))
    for p in body.data.polygons:
        p.material_index = 1 if p.center.z > 1.2 else 0
    add_uv2_and_colors(body)
    body.shape_key_add(name="Basis")
    sk = body.shape_key_add(name="Bulge")
    for i, v in enumerate(body.data.vertices):
        if 0.8 < v.co.z < 1.4:
            sk.data[i].co = (v.co.x * 1.6, v.co.y * 1.6, v.co.z)
    body.select_set(True); arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.parent_set(type='ARMATURE_AUTO')
    def action(name, axis):
        arm.animation_data_create()
        act = bpy.data.actions.new(name)
        arm.animation_data.action = act
        bpy.ops.object.mode_set(mode='POSE')
        for f, ang in ((1, 0), (15, 40), (30, -40), (45, 0)):
            for b in arm.pose.bones[1:4]:
                b.rotation_mode = 'XYZ'
                r = [0, 0, 0]; r[axis] = math.radians(ang)
                b.rotation_euler = r
                b.keyframe_insert("rotation_euler", frame=f)
        bpy.ops.object.mode_set(mode='OBJECT')
        return act
    bpy.context.scene.frame_start = 1; bpy.context.scene.frame_end = 45
    bpy.context.scene.render.fps = 30
    action("Bend", 0)
    bpy.ops.object.select_all(action='SELECT')
    common = dict(use_selection=True, add_leaf_bones=False, global_scale=100.0, apply_unit_scale=True,
                  mesh_smooth_type='FACE', use_armature_deform_only=False, bake_anim_use_all_actions=False,
                  bake_anim_use_nla_strips=False)
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, "SK_Ref.fbx"), **common)
    action("Twist", 2)
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, "A_Twist.fbx"), object_types={'ARMATURE'}, **common)

static_mesh()
skeletal_mesh()
