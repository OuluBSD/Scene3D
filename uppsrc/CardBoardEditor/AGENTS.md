# CardBoardEditor Package AGENTS Guide

## Scope
- Applies to `uppsrc/CardBoardEditor` and its entire sub-tree of U++ packages.

## Purpose
- Card board editor for designing and editing cardboard construction projects
- Provides tools for creating cardboard models, layouts, and assembly instructions
- Supports multiple export formats for manufacturing and prototyping
- Integrates with other U++ packages for design and manufacturing workflows

## Key Areas
- **Cardboard Designer**: Main interface for creating cardboard models
- **Layout Editor**: 2D/3D layout design and manipulation
- **Assembly Tools**: Automatic generation of assembly instructions
- **Export Formats**: Support for various manufacturing formats (SVG, PDF, etc.)
- **Material Library**: Collection of cardboard materials and specifications
- **Preview System**: Real-time preview of cardboard models

## Extension Points
- Add new export formats by extending `ExportFormatBase`
- Implement custom cardboard materials via material handlers
- Extend layout controls with new constraints and snap settings
- Add new preview rendering modes
- Integrate with external design tools

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
- Draw: Image and drawing operations
- TheIDE: Integration with the IDE application
- Common: Shared utilities and base classes

## Where To Start
- Review `CardBoardEditor.h` for the main editor class interface
- Examine `CardBoardEditor.cpp` for editor implementation details
- Study `LayoutEditor` for layout design implementation
- Look at `ExportHandler` for format export implementations
- Check `MaterialLibrary` for material definitions

## Design Principles
- User-friendly interface for cardboard model design
- Real-time preview with accurate material representation
- Extensible architecture for new features
- Consistent design workflow for cardboard projects
- Comprehensive documentation for manufacturing specifications

