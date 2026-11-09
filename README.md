# Scene3D

A 3D scene editing, skeletal animation, Octree map IO, and 3D modeling application suite built natively in C++ using the Ultimate++ (U++) framework.

## Packages

### 1. `Scene3D`
Multi-tier 3D scene graph and execution engine:
- `Scene3D/Core`: Core transforms, nodes, and spatial scene hierarchy
- `Scene3D/IO`: Asset deserialization and scene streaming
- `Scene3D/Render`: Modern rendering backend
- `Scene3D/Anim`: Keyframe, skeletal, and morph target animation
- `Scene3D/Remote`: Remote scene streaming and synchronization
- `Scene3D/Exec`: Runtime scene execution logic

### 2. `Rt3D`
6-directional octree engine ported from Tesseract-Sauerbraten:
- Byte-exact map IO (`.ogz`)
- Skeletal mesh loader (`SkelModel`, `SkelMesh`, `Iqm`)
- Sub-octrees and raycast spatial queries

### 3. `AnimEdit` & `AnimEditLib`
2D/3D sprite, timeline, collision, and state machine editor suite.

### 4. `CardBoardEditor`
Visual card board layout editor and structural renderer.

### 5. `ModelerApp`
3D modeling and geometry projection editor.

---

## Building

Build with `uppbuilder`:

```bash
uppbuilder tests/SceneCheck -m CLANG --bazaarcore <path_to_BazaarCore> --uppgeometry <path_to_UppGeometry> -v
```

## CI Matrix
Scene3D is continuously verified on all 4 tier-1 platforms:
- Ubuntu Linux with Clang
- Ubuntu Linux with GCC
- macOS Apple Silicon (arm64) with Clang
- Windows x64 with MSVC
