#ifndef _AnimEditLib_AnimExport_h_
#define _AnimEditLib_AnimExport_h_

#include <Core/Core.h>
#include "AnimCore.h"

namespace Upp {

// Export format options
enum class ExportFormat {
    JSON,           // Standard JSON format
    BINARY,         // Binary format for runtime efficiency
    SPRITESHEET,    // Combined spritesheet with animation data
    GAME_READY      // Optimized format for specific game engine
};

// Export options structure
struct ExportOptions {
    ExportFormat format = ExportFormat::JSON;
    bool include_sprites = true;
    bool include_animations = true;
    bool include_entities = true;
    bool compress_sprites = false;      // Whether to compress sprite textures
    bool optimize_animations = true;    // Apply animation optimizations
    bool generate_spritesheets = false; // Pack sprites into atlases
    String output_path;                 // Where to export files
    String base_resource_path;          // Base path for resource references
    bool embed_resources = false;       // Whether to embed resources in output
    
    ExportOptions() = default;
};

// Export result structure
struct ExportResult {
    bool success = false;
    String error_message;
    Vector<String> exported_files;
    int64 file_size_bytes = 0;
    String summary;  // Human-readable summary of export
    
    ExportResult() = default;
};

// Main export function
ExportResult ExportProject(const AnimationProject& project, const ExportOptions& options);

// Specific export functions
ExportResult ExportEntities(const AnimationProject& project, const ExportOptions& options);

// Resource dependency tracking
struct ResourceDependency : public Moveable<ResourceDependency> {
    String resource_id;
    String resource_type;  // "sprite", "animation", "entity", etc.
    Vector<String> references;  // IDs of resources this depends on
    Vector<String> dependents;  // IDs of resources that depend on this one
    
    ResourceDependency() = default;
};

struct DependencyGraph : public Moveable<DependencyGraph> {
    Vector<ResourceDependency> dependencies;
    Vector<String> root_nodes;  // Nodes that nothing depends on
    Vector<String> leaf_nodes;   // Nodes that depend on nothing else
    
    DependencyGraph() = default;
};

DependencyGraph AnalyzeDependencies(const AnimationProject& project);
Vector<String> FindUnusedResources(const AnimationProject& project);
Vector<String> FindMissingDependencies(const AnimationProject& project, const String& base_path = "");
Vector<String> GetBuildOrder(const DependencyGraph& graph);

// Animation optimization and compression functions
void OptimizeAnimation(Animation& anim);
void OptimizeProject(AnimationProject& project);
void CompressAnimation(Animation& anim);
void CompressAnimations(Vector<Animation>& animations);
void CompressSprites(Vector<Sprite>& sprites, const ExportOptions& options);

// Version compatibility checking
struct VersionInfo {
    int major = 0;
    int minor = 0;
    int patch = 0;
    String prerelease;  // e.g., "alpha", "beta", "rc1"
    
    String ToString() const {
        String result = AsString(major) + "." + AsString(minor) + "." + AsString(patch);
        if (!prerelease.IsEmpty()) {
            result += "-" + prerelease;
        }
        return result;
    }
};

VersionInfo ParseVersion(const String& version_string);
bool IsVersionCompatible(const VersionInfo& project_version, const VersionInfo& target_version);
bool IsVersionCompatible(const String& project_version, const String& target_version);
bool ValidateForExport(const AnimationProject& project, String& error_out);

} // namespace Upp

#endif