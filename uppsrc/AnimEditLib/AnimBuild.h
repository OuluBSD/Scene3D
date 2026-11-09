#ifndef _AnimEditLib_AnimBuild_h_
#define _AnimEditLib_AnimBuild_h_

#include <Core/Core.h>
#include "AnimCore.h"
#include "AnimExport.h"

namespace Upp {

// Build configuration options
struct BuildConfig {
    String source_path;           // Path to source project files
    String output_path;           // Path for built project output
    ExportFormat format = ExportFormat::GAME_READY;  // Output format
    bool optimize_sprites = true;      // Whether to optimize sprites (compression, etc.)
    bool optimize_animations = true;   // Whether to optimize animations (keyframe reduction, etc.)
    bool generate_mipmaps = false;     // Whether to generate mipmaps for textures
    bool strip_debug_info = true;      // Whether to remove debug information from output
    String target_platform;            // Target platform (e.g., "desktop", "mobile", "web")
    bool create_spritesheets = false;  // Whether to pack sprites into atlases
    int spritesheet_width = 2048;      // Width of generated spritesheets
    int spritesheet_height = 2048;     // Height of generated spritesheets
    bool verbose_output = false;       // Whether to output detailed build information
    
    BuildConfig() = default;
};

// Build result structure
struct BuildResult {
    bool success = false;
    String error_message;
    Vector<String> generated_files;
    int64 total_size_bytes = 0;
    double build_time_seconds = 0.0;
    String summary;
    
    BuildResult() = default;
};

// Main build function
BuildResult BuildProject(const BuildConfig& config);

// Build step functions
bool PreprocessAssets(const BuildConfig& config, String& error_out);
bool ProcessAnimations(const BuildConfig& config, String& error_out);
bool PackSpritesheets(const BuildConfig& config, String& error_out);
bool GenerateMetadata(const BuildConfig& config, String& error_out);
bool PostProcessOutput(const BuildConfig& config, String& error_out);

// Utility functions
Vector<String> FindAnimProjectFiles(const String& directory);
bool ValidateBuildConfig(const BuildConfig& config, String& error_out);

} // namespace Upp

#endif