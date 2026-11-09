AGENTS

Scope
- Applies to `uppsrc/Scene3D` and subpackages.

Purpose
- Shared 3D scene/pointcloud/pose data package used by Edit3D and WmrTest shells.
- Defines the canonical `.scene3d` project format (scene, pose stream, pointcloud).

Notes
- Keep this package independent of `SoftHMD`; use adapters in consumers.
- Prefer modular subpackages if functionality grows (e.g., Scene3D/Core, Scene3D/Render, Scene3D/Anim).
- If adding a `.upp`, list `AGENTS.md` first in the `file` section.
