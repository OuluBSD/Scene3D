# AnimEdit Package AGENTS Guide

## Scope
- Applies to `uppsrc/AnimEdit` and its entire sub-tree of U++ packages.

## Purpose
- Animation editing and keyframe manipulation
- Provides tools for creating, editing, and previewing animations
- Supports multiple animation formats and interpolation methods
- Integrates with other U++ packages for animation workflows

## Key Areas
- **Animation Editor**: Main interface for keyframe manipulation
- **Timeline Control**: Time-based editing and playback controls
- **Interpolation Methods**: Linear, Bezier, Catmull-Rom, and custom curves
- **Animation Formats**: Support for various animation data formats
- **Preview System**: Real-time animation preview and rendering

## Extension Points
- Add new interpolation methods by extending `InterpolationBase`
- Implement custom animation formats via format handlers
- Extend timeline controls with new markers and annotations
- Add new preview rendering modes
- Integrate with external animation tools

## Conventions
- Follow `/CODESTYLE.md` for code style and documentation
- Use proper U++ naming conventions (PascalCase for classes, camelCase for methods)
- Include `AGENTS.md` as first file in `.upp` manifest
- Document all public APIs with Topic++ (`.tpp`) style comments
- Use `Separate` and `ReadOnly` options in `.upp` for logical grouping
- Keep package header (`AGENTS.md`) up to date with any structural changes
- Use proper acronym casing for shared components
- Prefer shared implementations over duplicated code patterns
- Follow existing patterns in the codebase

- Follow repo-wide naming rules: `ai/rules/naming-conventions.md`

## Dependencies
- CtrlLib: Base control infrastructure
- Draw: Image and drawing operations
- Core: Basic U++ functionality
- TheIDE: Integration with the IDE application

## Where To Start
- Review `AnimEdit.h` for the main editor class interface
- Examine `AnimEdit.cpp` for editor implementation details
- Study `TimelineCtrl` for timeline control implementation
- Look at `Interpolation` for interpolation methods
- Check `PreviewCtrl` for preview rendering

## Design Principles
- User-friendly interface for animation editing
- Real-time preview with minimal latency
- Extensible architecture for new animation features
- Consistent keyframe manipulation behavior
- Comprehensive undo/redo support

