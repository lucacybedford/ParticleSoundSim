import bpy

with open(bpy.path.abspath("//scene.pss"), "w") as f:
    f.write("pss 1\n")

    emitters, receivers = [], []
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            mw = obj.matrix_world
            for poly in obj.data.polygons:
                verts = [mw @ obj.data.vertices[i].co for i in poly.vertices]
                mat = obj.material_slots[poly.material_index].name
                f.write(f"plane {mat} {len(verts)}")
                f.writelines(f" {vert.x} {vert.y} {vert.z}" for vert in reversed(verts))
                f.write("\n")
        elif obj.type == "EMPTY" and obj.name.startswith("emitter"):
            emitters.append(obj)
        elif obj.type == "EMPTY" and obj.name.startswith("receiver"):
            receivers.append(obj)

    for obj in sorted(emitters, key=lambda o: o.name):
        pos = obj.matrix_world.translation
        f.write(f"emitter {pos.x} {pos.y} {pos.z}")
        if "power_db" in obj:
            f.write(" power_db " + " ".join(str(v) for v in obj["power_db"]))
        if "h_ang" in obj:
            f.write(" h_ang " + " ".join(str(a) for a in obj["h_ang"]))
        if "v_ang" in obj:
            f.write(" v_ang " + " ".join(str(a) for a in obj["v_ang"]))
        f.write("\n")

    for obj in sorted(receivers, key=lambda o: o.name):
        pos = obj.matrix_world.translation
        r = obj.empty_display_size * max(obj.scale)
        f.write(f"receiver {pos.x} {pos.y} {pos.z} {r}")
        f.write("\n")
