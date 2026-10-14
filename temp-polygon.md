# Plane → convex polygon

Working notes for generalising `Plane` from a rectangle (normal, anchor, length, height) to a convex polygon, so Blender-exported faces can be loaded directly. Delete when done.

## What depends on `Plane`

| Field | Used by | After the change |
|---|---|---|
| `n`, `p` | ray–plane intersection, `Particle.cpp:89` | Unchanged. `p` only needs to be *some* point on the plane (centroid). |
| `l`, `h` | finite-extent check, `Particle.cpp:94-97` | **Removed.** Replaced by `contains()`. |
| `h_tangent`, `v_tangent` | extent check, plus the Lambert basis at `Particle.cpp:148-150` | Kept, but only for the Lambert basis. Any orthonormal pair perpendicular to `n` works. |
| `corners` | bounding box and drawing in `app_visual.cpp:55, 110` | `std::vector<dvec3>`. The range-for loops compile unchanged. |

## Conventions

- **Winding defines the normal.** Vertices are CCW when viewed from the front (the side the normal points to, i.e. into the room). The existing rectangle corners already satisfy this: `cross(h_tangent, v_tangent) = n`.
- **Blender normals point outward** (CCW viewed from outside). Either use "Recalculate Inside" in Blender, or reverse the vertex order in the exporter.
- **Convex and planar only.** Concave ngons and non-planar quads are triangulated in the exporter (`bmesh.ops.triangulate`), not handled in C++.

## Steps

### 1. Header
- [ ] `corners` → `std::vector<dvec3>`
- [ ] remove `l`, `h`
- [ ] add `std::vector<dvec3> edge_normals`
- [ ] add the polygon constructor `Plane(std::vector<dvec3> vertices, const Material &)`
- [ ] add `bool contains(const dvec3 &x) const`

### 2. Polygon constructor (the primary one)
Order matters, because each step uses the previous one:

1. **Centroid** `p`: the average of the vertices. For a convex polygon this always lies inside it. The true area centroid isn't needed, because `p` only has to be a point on the plane.
2. **Normal** `n`: Newell's method, using vertices taken relative to `p` for precision far from the origin:
   `n = normalize( Σ cross(v[i] - p, v[i+1] - p) )`
   This is robust to collinear vertices and slight non-planarity.
3. **Tangents**: use the same formula as the old rectangle constructor, so the Lambert basis for existing scenes is bit-identical.
4. **Edge normals**: `m_i = normalize(cross(n, v[i+1] - v[i]))`. These point inward for CCW winding. Normalise them so that `eps` in `contains` is a distance in metres.

### 3. Validation (throw `std::invalid_argument`)
- [ ] at least 3 vertices
- [ ] Newell sum not ~zero (a degenerate, zero-area polygon)
- [ ] planar: `|dot(v - p, n)| < 1e-6` for every vertex
- [ ] convex and consistently wound: `dot(cross(e_i, e_{i+1}), n) > 0` for every consecutive edge pair

### 4. Rectangle constructor
- [ ] compute the 4 corners in a static helper, then delegate: `: Plane(rect_corners(...), material)`
- [ ] because the corners are CCW about the given normal, Newell gives back the same normal (possibly differing in the last bits)

### 5. `contains`
```
for each edge i:  if dot(x - v[i], m_i) < -eps → return false
return true
```
- Make it **inclusive** (`-eps`, not `0`). Adjacent faces must both accept a hit on their shared edge, or particles leak through the seams.
- `eps` ~ 1e-9 m. Nearest-`t` selection resolves the tie.

### 6. `Particle.cpp`
- [ ] replace lines 94–97 (the `h_dist`/`v_dist` check) with `if (!p.contains(x_hit)) continue;`

### 7. Regression checks
- [ ] **before starting**: run `./ParticleSoundSimExperiments edc` from `build/` and keep the CSVs
- [ ] after the change: rerun and diff. With the same seed they should match, or differ only in the last few digits.
- [ ] build the shoebox as 12 triangles instead of 6 rectangles. EDC and RT60 should match within stochastic noise. A faster decay means a seam leak: check `eps` and inclusivity.

## Cost
`contains` does one dot product per edge (4 for a quad, versus 2 before) and only runs for planes the ray actually crosses. It is not expected to show up in profiles.
