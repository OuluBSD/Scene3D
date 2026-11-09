# ModelerApp GUI Automation Tests

These tests run via ByteVM automation bindings in ModelerApp:

```
bin/ModelerApp --test uppsrc/ModelerApp/tests/<path>.py
```

Conventions:
- Tests should be small and focused (single feature/flow).
- Prefer robust discovery via `find("<menu/path>")` or `find("<label>")`.
- Use `wait_time()` after clicks where the UI needs to refresh.
- Exit with non-zero on failure.

Folders:
- `tree_props/` – scene tree + properties panel
- `timeline/` – playback and keyframe UI
- `viewports/` – viewport tools, gizmo, camera controls
- `materials/` – material + texture UI

Utilities:
- Keep test scripts standalone (ByteVM supports `import sys`).
