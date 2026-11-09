# Rt3D

Tesseract-Sauerbraten engine code ported to U++ value types and U++ package
structure. Phase `013-tesseract-integration` of the SweetSandy plan.

Reference source: `~/Dev/Tesseract-Sauerbraten/src/` (read-only reference;
nothing is compiled from it directly).

## Rules for this package

- **Every ported symbol is prefixed `Rt3`/uppercase `RT3_`.** Everything in
  this repo shares the flat `Upp` namespace (BLITZ merges packages), and
  Tesseract's names are extremely generic - `cube`, `vec`, `octa`,
  `material`, `light`, `node`. An unprefixed `cube` or `vec` would be an
  ODR collision waiting to happen, the same class of bug as `WorldOctree`
  (SweetSandy) and `VegQuat` (Vegetation phase). `LMID_*`/`LAYER_*`/
  `MAXFACEVERTS`/`O_LEFT` are kept unprefixed for now because they are
  enum *constants*, not types, and so cannot collide across translation
  units - revisit if that ever stops being true. It stops being true the
  moment a *system* header defines the same name as a macro, which is
  exactly what happened to upstream's `Dimension`/`DimCoord`/`Opposite`:
  X11's `X.h` has `#define Opposite 4`, so the ported `Opposite` only
  compiled in the headless test and broke the moment SweetSandy (a GUI
  build, which includes X11) pulled Rt3D in. They are `Rt3Dimension` /
  `Rt3DimCoord` / `Rt3Opposite` now.
- `Rt3Skeleton::CalcTags(bdata, scale, out)` is skelmodel.h's
  `skeleton::calctags`: the world transform of every tag, composed
  `pose * base * tagmatrix` with the **tag matrix last** (a tag is expressed
  in its bone's space, so a bone that turns takes it along), and with the
  translation scaled by the model's own scale while the basis is not. Two
  things to know before using it: a null `bdata` gives the bone's
  *parent-relative* base rather than a world transform - that is upstream's
  own behaviour, and the reason a caller who wants a world position must pass
  a real pose; and a tag naming a bone that is not there yields the identity
  rather than a default-constructed (all-zero) matrix, because zero would
  flatten everything into the origin. Upstream cannot reach that case at all
  (its `settag` refuses the bone), so it is this port's own edge, and the
  deviation is deliberate.
- **Struct layout is byte-exact with upstream.** Phase 013 Task 2 loads real
  `.ogz` maps through `Rt3Cube`/`Rt3CubeExt`/`Rt3VertInfo`, so field order,
  types (`word` = `ushort`, `byte` = `uchar`, `dword` = `uint`) and the
  `edges[12]`/`faces[3]` union must stay exactly as upstream has them. Do
  not "improve" these into U++-idiomatic shapes.
- **Faithful ports, quirks included.** Where upstream does something odd
  (e.g. `Rt3SurfaceInfo::Brighten` assigns fields and then immediately
  overwrites them with `Clear()`), port it verbatim and pin the behavior
  with a comment plus a test. Do not silently "fix" upstream.
- **When upstream copies an operand, so must the port.** geom.h's arithmetic
  writes its result in place, so `d.mul(p, d)` is wrong unless the right
  operand is copied out first - and upstream copies it
  (`mul(parent, dualquat(d.pose))`) precisely because of that. Two bugs of this
  shape shipped and were both found by tests, not by reading: `InitPitchDeps`
  (3A) and the IQM bone table's `base.mul(parent.base, base)` (3C, where the
  second bug is invisible for a root at the origin, so only a real model file
  finds it). Whenever an upstream expression passes the destination as an
  operand, write `Mul(p, Rt3Xxx(dest))` and leave a comment saying why.
- **A failed load must leave nothing behind.** `Rt3LoadOgz`/`Rt3LoadMapData`
  return false, set the error string, free whatever partial tree was built and
  leave `Rt3MapData::root` null. Callers must be able to check the bool and
  never see a half-loaded map.
- Only `Core` is used so far (including `GZDecompress` for the `.ogz` gzip
  framing). GL/renderer/model code lands in later tasks and will add
  `GLCtrl`/`Draw` then - not before.

## Current contents

- `Math.h` - `Rt3IVec` (int vector), `Rt3IVec4`, `Rt3Vec3`, `Rt3Quat`,
  `Rt3DualQuat` and `Rt3Matrix4x3`, from `shared/geom.h`. Only the members the
  ports need; `Rt3Cross` is a free function because geom.h's
  `ivec::cross(a, b)` overwrites its receiver and never reads it.
  `Rt3Vec4` is **not** ported: upstream's `quat` derives from it, so the four
  components are members of `Rt3Quat` and the inherited mixins became explicit
  members. Two consequences to keep in mind when editing:
  - the row arithmetic (`Mul`, `Madd`, `Msub`) is **in place**, so a row of a
    const matrix has to be copied before scaling it, and `Mul(p, o)` may not be
    passed the destination as `o` (see the rule above);
  - `Rt3Quat::Mul(quat)` is the quaternion *product*, while geom.h's
    `vec4::mul(vec4)` it hides is *componentwise*. The one place that needs
    the componentwise form (`matrix4x3(dualquat)`) spells the components out.
- `Octree.h` - data format from `engine/octa.h`: `Rt3Cube`, `Rt3CubeExt`,
  `Rt3VertInfo`, `Rt3SurfaceInfo`, `Rt3ElementSet`, `Rt3MaterialSurface`,
  `Rt3TJoint`, plus the edge/octant bit helpers (upstream macros ported to
  inline functions), `Rt3GenFaceVerts`/`RT3_FACE_COORDS`, `Rt3ValidateCubes`
  and `Rt3InsideWorld`.
- `Octree.cpp` - allocation, face corners and the validate pass from
  `engine/octa.cpp`: `Rt3NewCubes`, `Rt3FreeOcta`, `Rt3DiscardChildren`,
  `Rt3NewCubeExt`, `Rt3FamilySize`, `Rt3Get/SetCubeVector`,
  `Rt3Get/SetOctantVector`, the `D`/`R`/`C` octant axis tables, the shared
  empty cube, and the `allocnodes` counter.
- `MapIO.h`/`MapIO.cpp` - `.ogz` loading from `engine/worldio.cpp`: the
  `Rt3OctaHeader`/`Rt3MapHeader`/`Rt3Entity`/`Rt3VSlot` file structs, a
  little-endian reader, and `Rt3LoadOgz`/`Rt3LoadMapData` which fill an
  `Rt3MapData` (header, vars, entities, vslots, octree).
- `SubOctree.h`/`SubOctree.cpp` - `Rt3SubOctree`: a loaded map placed in
  SweetSandy world space (origin, yaw, meters-per-unit), with `LeafAt`,
  `SolidAt`, `GetStats` and a `Rt3SubOctreeVisitor` leaf traversal. This is
  the integration seam for the renderer/collision tasks - they consume the
  visitor, not the raw cube tree.
- `AnimModel.h`/`AnimModel.cpp` - the animation sampling layer from
  `engine/animmodel.h` and `shared/ents.h`: the `RT3_ANIM_*` flags,
  `Rt3AnimInfo`, `Rt3AnimSpec`, `Rt3AnimPos` (with `SetFrames`, this fork's
  whole clip sampling) and `Rt3AnimState`. **Must be included before
  `SkelModel.h`** in `Rt3D.h`: `InterpBones` reads `RT3_ANIM_NOPITCH`, and
  sub-headers may not include each other.
- `SkelModel.h`/`SkelModel.cpp` - the skeletal animation **data** core from
  `engine/skelmodel.h`: `Rt3Tag`, `Rt3SkelAnimSpec`, `Rt3BoneInfo`,
  `Rt3BlendCombo`, `Rt3Antipode`, `Rt3PitchDep`, `Rt3PitchTarget`,
  `Rt3PitchCorrect` and `Rt3Skeleton` (bone table, keyframe `framebones`,
  anim specs, tags, blend combos, antipodes, pitch tables) with
  `FindBone`/`FindTag`/`AddTag`, `FindSkelAnim`/`AddSkelAnim`,
  `LinkChildren`, `CalcAntipodes`, `AddPitchDep`/`InitPitchDeps`/
  `CalcPitchCorrects`, `CalcDeviation`, the bone-mask expansion
  (`ApplyBoneMask`/`ExpandBoneMask`), and the two passes that turn a skeleton
  into a pose: `RemapBones` (compact bone indexes, grouping constraints,
  antipodes) and `InterpBones` (the blended, pitch-corrected pose).
  Pure data, no GL. Upstream's `char *name` +
  `newstring`/`DELETEA` is a `String`, so the NULL-name guards are gone (a real
  bone always has a name). Also holds the two pieces the model loader needs and
  that already existed upstream: `Rt3Skeleton::AddBlendCombo`
  (`skelmeshgroup::addblendcombo`, which also assigns the entry's
  `interpindex`) and `Rt3BlendCombo::SortCmp` (`blendcombo::sortcmp`).
- `SkelMesh.h`/`SkelMesh.cpp` - the skinning/emission half of `skelmodel.h`:
  `Rt3SkelVert` (upstream's `vert` - which `render.h` declares, but which both
  the IQM reader and `skelmesh` need, so it lives here), `Rt3SkelTri`,
  `Rt3SkelMesh` (`GenVbo` + `InterpVerts`), `Rt3SkelCacheEntry` (upstream's
  `skelcacheentry`: the pose array plus the version it is good for) and
  `Rt3SkelModel` (`GenVbo` + `Animate`). `Rt3FixQuatTangent` - upstream's
  `fixqtangent` - is defined here because both the tangent frame construction
  (`Iqm.cpp`) and the skinning pass need it and it must be one function.
  **Must be included after `SkelModel.h` and before `Iqm.h`** in `Rt3D.h`.
  Three deliberate departures from upstream, each because there is no renderer
  here yet:
  - the shared vertex array is one `Vector<Rt3SkelVert>` instead of a byte
    buffer with a `fillverts` copy, and `GenVbo` fills it with the bind pose so
    an unanimated model still draws. Upstream leaves the positions undefined
    until the first `interpverts`;
  - `Rt3SkelCacheEntry` stamps its own version counter where upstream stamps
    `Shader::uniformlocversion()` (a GL counter), and the entry answers
    `Matches` for itself because there is no `shaderparamskey` to ask;
  - the element list is `Vector<int>`, not `GLuint` in a buffer, and the
    per-mesh mesh-group bookkeeping (`vblends`, `vweights`, the blend cache) is
    on the skeleton, as 3B put it.
- `Iqm.h`/`Iqm.cpp` - the Inter-Quake Model reader from `engine/iqm.h`
  (`loadiqm`, `loadiqmmeshes`, `loadiqmanims` with the
  `skelloader`/`skelmeshgroup` plumbing inlined): `Rt3IqmHeader` and the
  decoded `iqm*` structs, `Rt3IqmMeshPart` (where each mesh landed in the flat
  vertex/triangle lists), and
  `Rt3IqmModel` with `Load` (parse and validate the whole file),
  `BuildSkeleton` (joints -> bones with the rest-pose `base`/`invbase`),
  `BuildMesh` (skinned vertices and rebased triangles, combinations into
  `skel.blendcombos`), `SortBlendCombos`, `BuildSkelMeshes` (the flat lists
  split back into one `Rt3SkelMesh` per file mesh, each with its name and
  material) and `AppendAnims` (the file's animations appended to the
  skeleton's keyframe poses, named `"<path>:<anim>"`). Every field is read through an explicit little-endian
  accessor, so struct padding never touches the file layout, and every array
  offset is bounds-checked against the header's declared size - upstream trusts
  it and reads out of bounds on a file that lies.

## Coordinate systems

- Tesseract map space: integer **cube units**, world spans `[0, worldsize)` per
  axis, `+x` east, `+y` north, `+z` up, and 1 unit == 1 Sauerbraten game unit
  (`physics.cpp`'s `octacollide` passes game coordinates straight through as
  octree coordinates). The root's children have edge `worldsize/2` and together
  span the world.
- `Rt3VertInfo` coordinates are in **1/8 of a cube unit** (upstream's
  `ivec(co).mask(0xFFF).shl(3)` vertex origin). Multiply by 8 and divide by 8 to
  get cube units. The test asserts every loaded vertex stays within
  `[0, worldsize * 8]`.
- SweetSandy world space: meters, `+x` east, `+y` up, `+z` south (see
  `game/SweetSandy/Terrain.cpp`'s heightfield grid). `Rt3SubOctree::ToMeters`
  applies yaw, then the axis swap, then the scale. Positive yaw rotates the
  map clockwise seen from above. The default scale is
  `DEFAULT_METERS_PER_UNIT = 0.25`, so a 4096-unit map is 1024 m across.

## Deliberately out of scope so far

- `Rt3LoadOgz` supports map version 33 (`MAPVERSION`) and the `OCTA` header,
  which is what every map in `packages/map` uses. The pre-33 "old map" reader
  (`tryoldmap`/`loadc_old`/`convertoldsurfaces`, ~1000 lines of a different cube
  encoding) is **not** ported and a load fails with a clear message.
- No PVS, lightmap, blendmap, lighting or material work: the reader stops after
  the octree and reports what the header said. `Rt3MapData::octreeOffset` is
  kept for diagnostics.
- Entities are read verbatim (they are game data, and reading them needs a
  loaded game to interpret). The per-entity extra bytes are skipped using the
  file's own `eif`, which is what upstream's entity-only reader `loadents` does.
- `vtxarray`/`octaentities` are **declared in `Rt3CubeExt` but never
  populated** by the loader - the renderer task fills them.
- `validatec`'s subdivision branch (leaves larger than 0x1000 cube units) is not
  ported, because it needs `subdividecube` + `clipplanes`. It is unreachable
  for `worldsize <= 8192`, and `Rt3ValidateCubes` asserts rather than silently
  skipping a repair it cannot do.
- Ray tracing, `neighbourcube`, mip/fixtex (`getmippedtexture`), editing
  (`octaedit.cpp`) - ported only as far as a task needs them.
- From `geom.h`: `vec4`, `matrix4` (the 4x4/projection one), `matrix3`,
  `vec2`, `hvec`/`hvec2`/`hvec4` and `plane`. `Rt3Matrix4x3` is therefore a
  **reduced** port - construction, `Identity`, `Mul`, translation/scale,
  `Lerp`, `Accumulate`, the three transpose forms, `Invert` and the
  transform accessors, but not the projection or GL matrix members.
- `InterpBones` returns the pose buffer instead of writing into upstream's
  `skelcacheentry` itself; `Rt3SkelCacheEntry` (`SkelMesh.h`) is that entry, and
  `Rt3SkelModel::Animate` is what fills it. `InterpBones` asserts that
  every anim state's frames are inside the keyframe array (upstream seeds
  `fr1`/`fr2` with -1 in the cache constructor to catch the same bug; U++
  value types cannot). `blendcombos` is a flat list rather than one list per
  mesh group - the grouping is GL-side, and only the iteration order matters.
- From `skelmodel.h`: everything that needs a GL context or a loaded model -
  `animmodel`'s `skin`/`mesh`/`meshgroup`/`part`/`shaderparams`, the VBO/VAO
  half of `skelmesh`'s emission (the skinning pass is ported, the GL upload is
  not), the blend/VBO caches' GL halves, `remapbones` (it walks blend combos)
  and the ragdoll (`ragdoll.h`). The antipode scheduling that
  `remapbones` ends with is ported separately as `Rt3Skeleton::CalcAntipodes`,
  because that part is pure data. `availgpubones`/`gpuaccelerate` are not
  ported: they query GL limits this port does not have.
- **No MD5/MD3/MD2 readers.** This fork ships no `.md5` file at all
  (`src/engine/rendermodel.cpp` still registers `MDL_MD5`, but there is nothing
  for it to read), its `.md3` props are `vertloader`s with no bone table, and
  its old ogre/monster models are MD2, which is not skeletal. The only
  skeletal asset in the fork is `packages/models/ogro2/*.iqm`, which is what
  `Iqm.h` reads, and it is also what the pedestrians need (the ogre is
  Tesseract's own character). Port a reader when a task needs a file to test it
  against - an untestable reader is how the 3A algebra bugs shipped.
- The ragdoll (`ragdoll.h`) is not ported, and neither is
  `skeladjustment`, so `AppendAnims` applies no bone adjustments to a pose.
  Two different reasons, and they are worth keeping apart: `ragdoll.h`'s
  `ragdollskel` + `ragdolldata` are geometry and constraint solving that is
  perfectly portable, but `init`/`move`/`constrain` and the whole `dynent`
  side need ODE, which this package deliberately does not depend on. And
  `skeladjustment` has **no data anywhere in the fork** - a grep for
  `skeladjust` over `packages/` returns nothing, because
  `packages/models/ogro2/ragdoll.cfg` is not its configuration: that file
  holds the ragdoll's own geometry (`rdvert`/`rdtri`/`rdjoint`/
  `rdlimitdist`/`rdlimitrot`), read by `ragdoll.h`. An earlier note here
  claimed the opposite, and the grep is what caught it.

## Verification

Headless self-test: `upptst/Rt3DTest` (`bin/build -m CLANG upptst/Rt3DTest`,
then run `bin/CLANG/Rt3DTest`). Pass `--noogz` to skip the map-loading part
when no Tesseract checkout is at hand; by default the maps are required.

The test project must NOT be named `Rt3D` - `umk` resolves `uses` by package
name and would resolve a test package named `Rt3D` to itself, silently never
building the library.

It pins:

- edge-nibble packing, the octant bit math against hand-computed values, all
  eight single-axis `OctaBoxOverlap` masks, `Rt3SurfaceInfo`'s vertex-count
  rules and the `Brighten` quirk, and the cube-tree allocation/discard
  accounting (including that `Rt3FreeOcta` releases every level and every
  cubeext, via the global `allocnodes` counter returning to its baseline).
- `Rt3GenFaceVerts` for all six orientations against the hand-expanded
  `GENFACEVERTS` winding order, and that zeroing one edge only moves the one
  corner that reads it.
- `Rt3ValidateCubes`'s two degeneracy tests, the forced-solid-at-size-1 path,
  and that `F_SOLID`'s own nibbles are not degenerate.
- `Rt3SubOctree::ToMeters` at yaw 0 and yaw 90.
- Real map loading for `aard3c` (4096), `abyss` (1024) and `abbey` (2048):
  header fields, entity/vslot/var counts, leaf/cube/surface/vertex counts
  against reference numbers produced by an **independent Python reader** of the
  same record stream, cube-block accounting, the validate pass being a no-op,
  all vertices inside the world, the visitor reaching every leaf once,
  reload determinism, and that a garbage or truncated payload fails loudly
  with no tree left behind.
- The skeletal data core: quaternion axis/angle and `u -> v` construction,
  `Rotate`/`InvertedRotate`, the product's operand order, `RestoreW`'s
  negative root and its 0 clamp, and `CalcAngleAxis` **dividing by the
  squared vector length** (geom.h's own result, pinned so it stays).
  Dualquaternion `GetTranslation`/`Transform`/`TransformNormal`,
  `pose * inverse(pose) == identity`, and the antipodal flip in `Lerp` and
  `Accumulate`. `matrix4x3` identity/transpose/`Transform`. On a hand-built
  five-bone skeleton: bone lookup, the `children`/`next` wiring, anim-spec
  lookup with and without the `:` group separator, `AddTag` with and without
  `testTags`, `CalcAntipodes`' scheduling order (including the starvation
  branch that pulls a blocked conflict into the work list), `InitPitchDeps`'
  parent chain and its `parent * own` pose composition, `CalcPitchCorrects`'
  two-stage clamp (the correct's own limits, then the target's) and its
  non-accumulation on a second pass, the bone-mask subtree expansion with and
  without the NOT bit, and `CalcDeviation`'s degrees and sign.

- The animation layer: `SetFrames` for a single-frame animation, looping with
  its wrap, non-looping holding its last frame, a reversed pair,
  `RT3_ANIM_SETTIME` from the request's own `basetime`, and a different
  `speed`; `Rt3AnimInfo`/`Rt3AnimState` equality; `Rt3BlendCombo`'s sorted
  insert, four-influence cap, 1e-3 drop and normalize; `RemapBones` numbering
  bones in the order the combos mention them, the negated `group` constraint
  for a parent/child pair and the resulting antipode; `InterpBones` averaging
  two keyframes, honouring the previous position's `(1 - interp)` weight while
  blending, applying a pitch correct and cancelling it with
  `RT3_ANIM_NOPITCH`.
- The IQM reader, in two layers. **Files the test writes itself** (a two-bone
  skinned mesh and a two-frame animation, written little-endian so the byte
  layout is the format's and not the reader's): the header counts, the text
  block **read by byte offset** (a name is an offset into the block, not an
  index into it), the y-flip on positions and the x/z flip plus normalize on
  orientations, `base * invbase`, triangle rebasing onto the mesh's first
  vertex, three vertices deduplicating to two combinations with the shared
  one's `uses == 2`, the sort order and every vertex following its own
  combination, the frame channel order **including the size-channel skip that
  `mask & 0x380` implies** (the test puts a 65535 there, which no other reading
  of the stream can produce), a pose stored relative to its bone's rest
  transform, an animation named `<path>:<anim>` and found by path, reloading
  adding nothing, a second animation appending after the first, the unskinned
  and dropped-empty-mesh cases, and six files that must be rejected with
  nothing left behind (bad magic, wrong version, declared size longer than the
  file, a section offset past the end, a triangle naming a vertex that does not
  exist, a weight naming a bone that does not exist).
  **The fork's real model** (`RunRealOgroTests`, skipped with a `LOG` when no
  checkout is at hand, `$RT3_MODEL_DIR` overrides the search): `ogro.iqm`'s 29
  joints / 539 vertices / 670 triangles / 252 text strings / 27 distinct
  combinations / 21 weighted bones / 22 interpolated bones, the first vertex's
  five attributes, the composition of a child bone's rest transform with its
  parent's inverse, and then 20 consecutive `SetFrames` + `InterpBones`
  samples through `idle.iqm`'s 40 frames with all 1160 keyframe poses and every
  interpolated bone asserted finite. The expected numbers come from an
  **independent Python reader** of the same files.

- **The tags the fork's own config declares** (`RunCalctagsTests` plus the
  tail of `RunSkelMeshTests`): the three from
  `packages/models/ogro2/iqm.cfg` - `tag_weapon` on `Tag.L`, `tag_shield` on
  `Forearm.R`, `tag_powerup` on `Chest`, the latter two built through
  upstream's `settag` euler formula (a config file is not code, so the test
  reproduces the formula rather than calling ported code). Pinned: the
  composition order, the parent-relative rest path and the scale on a
  hand-worked five-bone skeleton; the missing-bone identity; and on the real
  ogre that all three rest tags land within a quarter of the mesh's bounds,
  and that a tag travels up to 1.376 units over the idle clip - i.e. it really
  rides the bone it is bound to.
- The emission half, **the real model laid out and animated**
  (`RunSkelMeshTests`, skipped with a `LOG` when no checkout is at hand): the
  real ogre laid out in a `Rt3SkelModel`'s shared buffers - the element list is
  the triangles in order, every index inside the
`minvert`/`maxvert` range a draw call would be given, and the shared vertex
array holds the bind pose. Then, driven by the real `idle.iqm`: a
hand-built rest pose reproduces the 539 bind-pose vertices **exactly**
(the one pose for which that is true - frame 0 of the clip is *not* the rest
pose, it is the rest pose plus the idle sway, 0.005 units, which is why that
number is in the log); the whole clip keeps every vertex finite and within the
model's own diagonal of its bind position while moving most of them; the
cache's version is bumped per evaluation, its pose buffer is reused in place,
and `Matches` answers for the state it holds (and for nothing else, a
different pitch included); `interp` 1 ignores the previous position and
`interp` 0 is the previous position alone. It also logs that a half blend
puts **no** vertex on the exact midpoint of the two - dual-quaternion
blending is linear in the translation, not in the rotation.

Maps are searched in `$RT3_MAP_DIR`, `packages/map`,
`Tesseract-Sauerbraten/packages/map` and `~/Dev/Tesseract-Sauerbraten/packages/map`.
Models are searched in `$RT3_MODEL_DIR`, `packages/models/ogro2`,
`Tesseract-Sauerbraten/packages/models/ogro2` and
`~/Dev/Tesseract-Sauerbraten/packages/models/ogro2`.
