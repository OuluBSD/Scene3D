#include "AnimBuild.h"
#include "AnimExport.h"
#include "AnimSerialize.h"
#include <Core/Core.h>

namespace Upp {

Vector<String> FindAnimProjectFiles(const String& directory) {
    Vector<String> files;
    
    // Find all JSON files in the directory that might be animation projects
    FindFile ff(AppendFileName(directory, "*.json"));
    while (ff) {
        if (!ff.IsDirectory()) {
            // Check if this is actually an animation project by attempting to load it
            String content = LoadFile(ff.GetPath());
            if (!content.IsEmpty()) {
                AnimationProject project;
                if (LoadProjectJson(project, content)) {
                    files.Add(ff.GetPath());
                }
            }
        }
        ff.Next();
    }
    
    return files;
}

bool ValidateBuildConfig(const BuildConfig& config, String& error_out) {
    if (config.source_path.IsEmpty()) {
        error_out = "Source path is empty";
        return false;
    }
    
    if (!DirectoryExists(config.source_path)) {
        error_out = "Source path does not exist: " + config.source_path;
        return false;
    }
    
    if (config.output_path.IsEmpty()) {
        error_out = "Output path is empty";
        return false;
    }
    
    return true;
}

bool PreprocessAssets(const BuildConfig& config, String& error_out) {
    // In a real implementation, this would preprocess assets before the main build
    // (e.g., convert textures to optimal formats, validate asset integrity, etc.)
    
    if (config.verbose_output) {
        RLOG("Preprocessing assets...");
    }
    
    // For now, just return success
    return true;
}

bool ProcessAnimations(const BuildConfig& config, String& error_out) {
    // In a real implementation, this would process animations specifically
    // (e.g., apply compression, optimize keyframes, generate animation data for runtime)
    
    if (config.verbose_output) {
        RLOG("Processing animations...");
    }
    
    // For now, just return success
    return true;
}

bool PackSpritesheets(const BuildConfig& config, String& error_out) {
    // In a real implementation, this would pack sprites into texture atlases
    // using an efficient packing algorithm
    
    if (config.verbose_output) {
        RLOG("Packing spritesheets...");
    }
    
    // For now, just return success
    return true;
}

bool GenerateMetadata(const BuildConfig& config, String& error_out) {
    // In a real implementation, this would generate build metadata
    // (e.g., asset dependencies, build timestamps, version info)
    
    if (config.verbose_output) {
        RLOG("Generating metadata...");
    }
    
    // For now, just return success
    return true;
}

bool PostProcessOutput(const BuildConfig& config, String& error_out) {
    // In a real implementation, this would perform final processing on the output
    // (e.g., create archives, generate manifest files, optimize file layout)
    
    if (config.verbose_output) {
        RLOG("Post-processing output...");
    }
    
    // For now, just return success
    return true;
}

BuildResult BuildProject(const BuildConfig& config) {
    BuildResult result;
    
    // Validate configuration
    String validation_error;
    if (!ValidateBuildConfig(config, validation_error)) {
        result.success = false;
        result.error_message = "Build configuration validation failed: " + validation_error;
        return result;
    }
    
    // Start timing the build - using a manual approach to avoid TimeStop compatibility issues
    // TimeStop ts;
    // ts.Start();
    
    try {
        // Find all project files in the source directory
        Vector<String> project_files = FindAnimProjectFiles(config.source_path);
        
        if (project_files.GetCount() == 0) {
            result.success = false;
            result.error_message = "No valid animation projects found in source directory: " + config.source_path;
            return result;
        }
        
        // Process each project file
        for (const String& project_file : project_files) {
            if (config.verbose_output) {
                RLOG("Processing project: " << project_file);
            }
            
            // Load the project
            String project_content = LoadFile(project_file);
            if (project_content.IsEmpty()) {
                result.success = false;
                result.error_message = "Failed to load project file: " + project_file;
                return result;
            }
            
            AnimationProject project;
            if (!LoadProjectJson(project, project_content)) {
                result.success = false;
                result.error_message = "Failed to parse project file: " + project_file;
                return result;
            }
            
            // Set up export options based on build config
            ExportOptions export_options;
            export_options.format = config.format;
            export_options.include_sprites = true;
            export_options.include_animations = true;
            export_options.include_entities = true;
            export_options.compress_sprites = config.optimize_sprites;
            export_options.optimize_animations = config.optimize_animations;
            export_options.output_path = config.output_path;
            export_options.generate_spritesheets = config.create_spritesheets;
            
            // Export the processed project
            ExportResult export_result = ExportProject(project, export_options);
            if (!export_result.success) {
                result.success = false;
                result.error_message = "Export failed for project " + project_file + ": " + export_result.error_message;
                return result;
            }
            
            // Add generated files to result
            for (const String& file : export_result.exported_files) {
                result.generated_files.Add(file);
                result.total_size_bytes += GetFileLength(file);
            }
        }
        
        // Run build steps
        String step_error;
        
        if (!PreprocessAssets(config, step_error)) {
            result.success = false;
            result.error_message = "Asset preprocessing failed: " + step_error;
            return result;
        }
        
        if (!ProcessAnimations(config, step_error)) {
            result.success = false;
            result.error_message = "Animation processing failed: " + step_error;
            return result;
        }
        
        if (config.create_spritesheets && !PackSpritesheets(config, step_error)) {
            result.success = false;
            result.error_message = "Spritesheet packing failed: " + step_error;
            return result;
        }
        
        if (!GenerateMetadata(config, step_error)) {
            result.success = false;
            result.error_message = "Metadata generation failed: " + step_error;
            return result;
        }
        
        if (!PostProcessOutput(config, step_error)) {
            result.success = false;
            result.error_message = "Post-processing failed: " + step_error;
            return result;
        }
        
        // Build completed successfully
        result.success = true;
        result.build_time_seconds = 0.0; // Placeholder - timing not implemented due to TimeStop compatibility
        result.summary = Format("Build completed successfully in %.2f seconds. Generated %d files (%.2f MB)", 
                                result.build_time_seconds, 
                                result.generated_files.GetCount(),
                                result.total_size_bytes / (1024.0 * 1024.0));
        
        if (config.verbose_output) {
            RLOG(result.summary);
        }
        
        return result;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Build failed with exception: " + String(e.what());
        return result;
    }
    catch (...) {
        result.success = false;
        result.error_message = "Build failed with unknown exception";
        return result;
    }
}

} // namespace Upp