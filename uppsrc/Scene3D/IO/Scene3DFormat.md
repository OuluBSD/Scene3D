# Scene3D Project Format

Status: v2 (current)

## Goals
- Store scene graph, object transforms, pointcloud references, and HMD pose streams.
- Support live capture + offline replay.
- Keep core format engine-agnostic (no SoftHMD dependency).

## File Extension
- `.scene3d`

## Encodings
- **JSON**: human-readable for tests, review, and hand edits.
- **Binary**: compact format for large scenes and pointclouds.

Both encodings use the same field layout and are serialized via U++ `Vis`.

## Top-Level Document
```
{
  "version": 2,
  "name": "ProjectName",
  "project": { ... },
  "active_scene": 0,
  "focus": { ... },
  "program": { ... },
  "created_utc": "2026-01-30T00:00:00Z",
  "modified_utc": "2026-01-30T00:00:00Z",
  "data_dir": "data",
  "external_files": [ ... ],
  "meta": [ { "key": "source", "value": "ModelerApp" } ]
}
```

## Camera
```
GeomCamera {
  position: vec3,
  orientation: quat,
  distance: 10,
  fov: 120,
  scale: 1
}
```

## Project
```
project {
  scenes: [ Scene, ... ]
}
```

## Scene
```
Scene {
  GeomDirectory: { ... }
}
```

## Directory
```
GeomDirectory {
  name: "",
  subdir: { "dir_name": GeomDirectory, ... },
  objs: [ GeomObject, ... ]
}
```

## Object
```
GeomObject {
  name: "camera",
  type_i: 2,          // enum index
  asset_ref: "assets/headset.glb",
  pointcloud_ref: "pointclouds/session01.pcd",
  timeline: GeomTimeline,
  data: GeomObjectData
}
```

## Timeline
```
GeomTimeline {
  keypoints: [ GeomKeypoint, ... ]
}

GeomKeypoint {
  key: 0,
  value: GeomObjectData
}
```

## Object Data (current)
```
GeomObjectData {
  frame_id: 0,
  position: vec3,
  orientation: quat
}
```

## External Files
```
Scene3DExternalFile {
  id: "pc_session01",
  type: "pointcloud",
  path: "pointclouds/session01.pcd",
  note: "Stereo capture 01",
  created_utc: "...",
  modified_utc: "...",
  size: 12345678
}
```

## Metadata
```
Scene3DMetaEntry {
  key: "source",
  value: "ModelerApp"
}
```

## Notes
- JSON uses the same fields as binary; binary storage is recommended for large scenes.
- Store heavy assets (pointclouds, meshes, textures) in `data_dir` and list them in `external_files`.
- Versioning is mandatory; loaders should refuse unknown major versions.
- Additional fields should be introduced with backward-compatible defaults.
