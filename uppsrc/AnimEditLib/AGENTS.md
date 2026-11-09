# AnimEditLib Package AGENTS Guide

## Scope
- Applies to `uppsrc/AnimEditLib` and its entire sub-tree of U++ packages.

## Purpose
- Library for animation editing and keyframe manipulation
- Provides core animation data structures and algorithms
- Supports multiple animation formats and interpolation methods
- Used by AnimEdit and other animation-related packages

## Key Areas
- **Animation Data Structures**: Core types for keyframes, curves, and tracks
- **Interpolation Algorithms**: Linear, Bezier, Catmull-Rom, and custom methods
- **Animation Formats**: Support for various animation data formats
- **Utilities**: Helper functions for animation operations
- **Serialization**: Animation data persistence and loading

## Extension Points
- Add new interpolation algorithms by extending `InterpolationBase`
- Implement custom animation formats via format handlers
- Extend data structures with new keyframe types
- Add new utility functions for animation operations
- Integrate with external animation libraries

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
- Core: Basic U++ functionality
- Common: Shared utilities and base classes
- TheIDE: Integration with the IDE application

## Where To Start
- Review `AnimEditLib.h` for the main library interface
- Examine `AnimEditLib.cpp` for library implementation details
- Study `AnimationData` for data structure definitions
- Look at `Interpolation` for interpolation algorithms
- Check `AnimationFormat` for format handlers

## Design Principles
- Clean separation between data and algorithms
- Efficient memory management for animation data
- Extensible architecture for new animation features
- Consistent API design for animation operations
- Comprehensive error handling

