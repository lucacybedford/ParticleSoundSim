import bpy
import bmesh

# Same tolerances as Plane::Plane in src/Plane.cpp, so the exporter and the
# loader agree on which faces are valid.
PLANAR_TOL = 1e-5
CONVEX_TOL = 1e-9


def is_valid_plane(face):
    """True if the loader would accept this face as-is (planar and convex)."""
    if len(face.verts) == 3:
        return True
    n = face.normal
    centre = face.calc_center_median()
    if any(abs((v.co - centre).dot(n)) > PLANAR_TOL for v in face.verts):
        return False
    co = [v.co for v in face.verts]
    k = len(co)
    for i in range(k):
        e0 = co[(i + 1) % k] - co[i]
        e1 = co[(i + 2) % k] - co[(i + 1) % k]
        if e0.cross(e1).dot(n) < -CONVEX_TOL * e0.length * e1.length:
            return False
    return True


with open(
    "/Users/lucabedford/Code/Important/ParticleSoundSim/scenes/scene.pss", "w"
) as f:
    f.write("pss 1\n")

    emitters, receivers = [], []
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            # work on a world-space copy so the user's mesh is untouched
            bm = bmesh.new()
            bm.from_mesh(obj.data)
            bm.transform(obj.matrix_world)
            bm.normal_update()

            # split only the faces the loader would reject; others stay as quads/ngons
            bad = [face for face in bm.faces if not is_valid_plane(face)]
            if bad:
                bmesh.ops.triangulate(bm, faces=bad)

            for face in bm.faces:
                verts = [v.co for v in face.verts]
                mat = obj.material_slots[face.material_index].name
                f.write(f"plane {mat} {len(verts)}")
                f.writelines(f" {vert.x} {vert.y} {vert.z}" for vert in reversed(verts))
                f.write("\n")
            bm.free()
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
