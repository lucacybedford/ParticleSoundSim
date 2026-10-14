for obj in bpy.context.scene.objects:
    if obj.type == 'MESH':
        mw = obj.matrix_world
        for poly in obj.data.polygons:
            verts = [mw @ obj.data.vertices[i].co for i in poly.vertices]
            mat = obj.material_slots[poly.material_index].name
            # write: PLANE <mat> <n verts> x y z ...
    elif obj.type == 'EMPTY' and obj.name.startswith("Emitter"):
        # write: EMITTER x y z <obj["power_dB"]...> ...
    elif obj.type == 'EMPTY' and obj.name.startswith("Receiver"):
        # write: RECEIVER x y z <obj.empty_display_size * max(obj.scale)>
