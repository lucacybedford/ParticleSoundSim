# Scene file (`.pss`) → `Scene`

Working notes for loading scenes from a text file, so that Blender-exported rooms can be simulated directly. Follows on from `temp-polygon.md` (polygon `Plane` is done and validated). Delete when done.

## Status going in

- Polygon `Plane` is working. The offline 4×7×3 wood room (200k particles) matches the pre-change run within noise:

  | | Total energy | T20 | T30 |
  |---|---|---|---|
  | Before (committed `24d3fe8`) | 2.7239e-05 | 0.4211 s | 0.4345 s |
  | After (polygon) | 2.7259e-05 | 0.4219 s | 0.4357 s |

- Still open from `temp-polygon.md`: the **triangulated shoebox test** (12 triangles instead of 6 rectangles; total energy and T30 should match the table above within ~1%). This tests seams between coplanar faces, which Blender meshes will be full of. It's easy to do once the loader exists: write the triangles in a `.pss` file.

## Design decisions

- **Extension `.pss`.** The loader rejects anything else.
- **One record per line, keyword first, no sections.** Each line is self-contained, so the parser needs no state, error messages can name the line, and the exporter can write objects in any order. Lines can still be grouped for readability.
- **Plain text, not JSON.** Thousands of faces stay readable and easy to diff in git, and there's no new dependency.
- **Version record first**: `pss 1`. Reject a missing or unknown version with a clear message, because the format will change (directional receivers, transmission).
- **Materials by name.** Stock materials come from `Materials.hpp` (`stone`, `concrete`, `carpet`, `glass`, `plaster`, `plasterboard`, `wood`, `absorber`). `material` lines add to or override them through `materials::make()`, so impedance is calibrated the same way. A `material` line must come before any plane that uses it.
- **Explicit vertex count on planes**, so a hand-edited line that's wrong gets reported instead of misread.
- **Emitter options match `Emitter`.** `power_db` is optional because `source_power` is a `std::optional`. Angle ranges are optional, default to the constructor's defaults, and are written in **degrees** (as in Blender's UI), converted to radians in the loader.
- **Emitter/receiver order = index.** Histograms are indexed by `emitter_id`, and `app_offline` uses `emitters[0]`/`[1]`. Document this at the top of the loader.
- **No run settings in the file.** Particle count, `dt` and `max_time` stay in `SimConfig`.
- **Coordinates are Z-up, in metres**, the same as Blender and the existing factories.

## Format

```
# ParticleSoundSim scene
pss 1                                   # format version, must be the first record

# optional custom materials; stock ones (stone, concrete, wood, ...) are built in
material velvet  absorption 0.05 0.12 0.35 0.45 0.38 0.36 0.36 0.36  scattering 0.3

# plane <material> <vertex count> then x y z per vertex, CCW viewed from inside
plane wood 4   -2 -3.5 0   2 -3.5 0   2 3.5 0   -2 3.5 0

# emitter x y z [power_db 8 values] [h_ang start end] [v_ang start end]   (degrees)
emitter -0.4 2.1 1.6  power_db 90 92 94 95 95 93 90 86
emitter  0.4 -2.1 1.6 h_ang 0 180

# receiver x y z radius
receiver -0.24 -0.56 1.2 0.5
```

| Record | Fields |
|---|---|
| `pss` | version (int). Must be the first record. |
| `material` | name, then `absorption` + 8 values (63 Hz–8 kHz), then `scattering` + 1 value |
| `plane` | material name, vertex count k ≥ 3, then 3k coordinates |
| `emitter` | x y z, then optional `power_db` + 8 values (L_W re 1 pW), `h_ang` start end, `v_ang` start end |
| `receiver` | x y z radius |

`#` starts a comment anywhere on a line. Blank lines are ignored.

## Steps

### 1. Material lookup
- [x] in `Materials.hpp`, a way to find a stock material by name (e.g. a function that returns the stock materials as a `std::map<std::string, Material>`)

### 2. Loader
- [x] `Scene load_scene(const std::string &path);` declared in `Scene.hpp`. It throws `std::runtime_error("file:line: ...")`.
- [x] reject paths that don't end in `.pss`
- [x] read with `std::getline`; strip everything after `#`; skip blank lines
- [x] read the keyword with `std::istringstream`, then dispatch
- [x] require `pss 1` as the first record
- [x] `material`: check the `absorption` / `scattering` labels, then `materials::make(name, absorption, scattering)`, then insert into the map (overriding a stock entry is allowed)
- [x] `plane`: look up the material (unknown name → error), read k, read 3k numbers, then construct a `Plane`. **Catch `std::invalid_argument` from `Plane` and rethrow it with the line number.**
- [x] `emitter`: position, then loop over the optional keywords; `power_db` → `set_power_dB`; convert the angles from degrees
- [x] `receiver`: position + radius (radius > 0)
- [x] after each record, **check for leftover tokens** (`iss >> extra` succeeding means an error). This catches typos like `power_dB`.
- [x] every numeric read is checked (`if (!(iss >> x))` → error naming the field)

### 3. Saver
- [x] `void save_scene(const Scene &, const std::string &path);`, about 20 lines: `pss 1`, a `material` line for any material that isn't stock, a `plane` line from `corners` + `material.name`, then emitters and receivers
- [x] for `power_db`: `source_power` is stored in W, so either convert back to dB (`10·log10(W / 1e-12)`) or also store the dB values on `Emitter`
- [x] write with enough precision to round-trip (`std::setprecision(17)` for doubles)

### 4. `app_offline`
- [x] scene path on the command line, e.g. `ParticleSoundSimOffline [scene.pss] [dry.wav]` (chosen by extension) or a `--scene` flag. `argv[1]` is currently the dry WAV.
- [x] the hardcoded `set_power_dB` at `app_offline.cpp:132-133` applies only to the factory scenes, or to emitters with no `source_power`; a file's power wins

### 5. Tests
- [x] save `make_room(4, 7, 3, wood)` → load → run. Total energy and T30 should match the status table above within noise.
- [x] write the triangulated shoebox by hand (12 triangles) and run it: the seam-leak test carried over from `temp-polygon.md`
- [x] save every factory scene to a `.pss` file, as reference files for the Blender exporter
- [x] error cases: unknown material, wrong vertex count, non-planar face, missing version, leftover token. Each gives a message with the line number.

### 6. Blender exporter (`blend_exporter.py`)
- [ ] write the same keywords as the format above (lowercase)
- [ ] use `obj.matrix_world` for vertices and for Empty positions
- [ ] Blender normals point outward: reverse the vertex order (or use "Recalculate Inside" in Blender)
- [ ] triangulate concave ngons and non-planar faces (`bmesh.ops.triangulate`) before writing
- [ ] emitter Empties: custom properties `power_db` (8 values), and optionally `h_ang` / `v_ang`
- [ ] receiver Empties: display type Sphere, radius = `empty_display_size * max(obj.scale)`
- [ ] write emitters and receivers in a stable order (sort by object name), so indices don't change between exports
