#include "AnimExport.h"
#include "AnimSerialize.h"
#include "AnimUtils.h"
#include <Core/Core.h>

namespace Upp {

ExportResult ExportProject(const AnimationProject& project, const ExportOptions& options) {
    ExportResult result;
    
    // Validate the project for export
    String validation_error;
    if (!ValidateForExport(project, validation_error)) {
        result.success = false;
        result.error_message = "Project validation failed: " + validation_error;
        return result;
    }
    
    // Check if output path is valid
    if (options.output_path.IsEmpty()) {
        result.success = false;
        result.error_message = "Output path is empty";
        return result;
    }
    
    // Just check if output directory exists (don't try to create it)
    if (!DirectoryExists(options.output_path)) {
        result.success = false;
        result.error_message = "Output directory does not exist: " + options.output_path;
        return result;
    }
    
    try {
        // Use the original project directly to avoid copy issues
        // We can only apply modifications in-place if needed, but for export,
        // we'll use the original without modifications to avoid the copy issue
        const AnimationProject& optimized_project = project;  // Use reference to avoid copying
        
        // For a full implementation, optimization and compression would need to be implemented
        // in a way that doesn't require copying the whole project
        
        // Export based on format
        switch (options.format) {
            case ExportFormat::JSON: {
                String json = SaveProjectJson(optimized_project);
                String output_file = AppendFileName(options.output_path, project.id + ".json");
                
                if (!SaveFile(output_file, json)) {
                    result.success = false;
                    result.error_message = "Failed to save JSON export file: " + output_file;
                    return result;
                }
                
                result.exported_files.Add(output_file);
                result.file_size_bytes = json.GetLength();
                result.success = true;
                result.summary = Format("Exported project '%s' to JSON format (%d bytes)", 
                                       project.name, result.file_size_bytes);
                break;
            }
            
            case ExportFormat::BINARY: {
                // For now, just serialize as JSON since we don't have a binary serializer yet
                // In a real implementation, we would serialize to an efficient binary format
                String json = SaveProjectJson(optimized_project);
                String output_file = AppendFileName(options.output_path, project.id + ".bin");
                
                // Convert JSON string to raw bytes and save
                String raw_data = json; // In real implementation, we'd convert to binary format
                if (!SaveFile(output_file, raw_data)) {
                    result.success = false;
                    result.error_message = "Failed to save binary export file: " + output_file;
                    return result;
                }
                
                result.exported_files.Add(output_file);
                result.file_size_bytes = raw_data.GetLength();
                result.success = true;
                result.summary = Format("Exported project '%s' to binary format (%d bytes)", 
                                       project.name, result.file_size_bytes);
                break;
            }
            
            case ExportFormat::SPRITESHEET: {
                // Export as spritesheet format
                // For now we'll just export the project as JSON, but in a real implementation
                // we would pack sprites into texture atlases
                String json = SaveProjectJson(optimized_project);
                String output_file = AppendFileName(options.output_path, project.id + "_spritesheet.json");
                
                if (!SaveFile(output_file, json)) {
                    result.success = false;
                    result.error_message = "Failed to save spritesheet export file: " + output_file;
                    return result;
                }
                
                result.exported_files.Add(output_file);
                result.file_size_bytes = json.GetLength();
                result.success = true;
                result.summary = Format("Exported project '%s' to spritesheet format (%d bytes)", 
                                       project.name, result.file_size_bytes);
                break;
            }
            
            case ExportFormat::GAME_READY: {
                // Export in game-ready format
                // This would create optimized assets specifically for a particular game engine
                String json = SaveProjectJson(optimized_project);
                String output_file = AppendFileName(options.output_path, project.id + "_game.json");
                
                if (!SaveFile(output_file, json)) {
                    result.success = false;
                    result.error_message = "Failed to save game-ready export file: " + output_file;
                    return result;
                }
                
                result.exported_files.Add(output_file);
                result.file_size_bytes = json.GetLength();
                result.success = true;
                result.summary = Format("Exported project '%s' to game-ready format (%d bytes)", 
                                       project.name, result.file_size_bytes);
                break;
            }
        }
        
        return result;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Export failed with exception: " + String(e.what());
        return result;
    }
    catch (...) {
        result.success = false;
        result.error_message = "Export failed with unknown exception";
        return result;
    }
}

ExportResult ExportEntities(const AnimationProject& project, const ExportOptions& options) {
    ExportResult result;
    
    // Check if output path is valid
    if (options.output_path.IsEmpty()) {
        result.success = false;
        result.error_message = "Output path is empty";
        return result;
    }
    
    // Just check if output directory exists (don't try to create it)
    if (!DirectoryExists(options.output_path)) {
        result.success = false;
        result.error_message = "Output directory does not exist: " + options.output_path;
        return result;
    }
    
    try {
        // Export each entity to a separate file if requested
        for (const auto& entity : project.entities) {
            String entity_json = StoreAsJson(entity, true);
            String output_file = AppendFileName(options.output_path, entity.id + ".json");
            
            if (!SaveFile(output_file, entity_json)) {
                result.success = false;
                result.error_message = "Failed to save entity export file: " + output_file;
                return result;
            }
            
            result.exported_files.Add(output_file);
        }
        
        result.success = true;
        result.summary = Format("Exported %d entities to separate files", project.entities.GetCount());
        
        // Calculate total size
        for (const String& file_path : result.exported_files) {
            int64 file_size = GetFileLength(file_path);
            if (file_size > 0) {
                result.file_size_bytes += file_size;
            }
        }
        
        return result;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Entity export failed with exception: " + String(e.what());
        return result;
    }
    catch (...) {
        result.success = false;
        result.error_message = "Entity export failed with unknown exception";
        return result;
    }
}

DependencyGraph AnalyzeDependencies(const AnimationProject& project) {
    DependencyGraph graph;
    
    // First, create all resource nodes
    Vector<ResourceDependency> dependencies;
    
    // Analyze sprite dependencies
    for (const auto& sprite : project.sprites) {
        ResourceDependency dep;
        dep.resource_id = sprite.id;
        dep.resource_type = "sprite";
        
        // For sprites, we might depend on texture files
        if (!sprite.texture_path.IsEmpty()) {
            dep.references.Add(sprite.texture_path);
        }
        
        dependencies.Add(pick(dep));  // Use pick to move instead of copy
    }
    
    // Analyze frame dependencies
    for (const auto& frame : project.frames) {
        ResourceDependency dep;
        dep.resource_id = frame.id;
        dep.resource_type = "frame";
        
        // Frames depend on sprites
        for (const auto& sprite_instance : frame.sprites) {
            dep.references.Add(sprite_instance.sprite_id);
        }
        
        dependencies.Add(pick(dep));
    }
    
    // Analyze animation dependencies
    for (const auto& animation : project.animations) {
        ResourceDependency dep;
        dep.resource_id = animation.id;
        dep.resource_type = "animation";
        
        // Animations depend on frames
        for (const auto& frame_ref : animation.frames) {
            dep.references.Add(frame_ref.frame_id);
        }
        
        dependencies.Add(pick(dep));
    }
    
    // Analyze entity dependencies
    for (const auto& entity : project.entities) {
        ResourceDependency dep;
        dep.resource_id = entity.id;
        dep.resource_type = "entity";
        
        // Entities depend on animations
        for (const auto& anim_slot : entity.animation_slots) {
            dep.references.Add(anim_slot.animation_id);
        }
        
        dependencies.Add(pick(dep));
    }
    
    // Now calculate dependents (reverse dependencies)
    for (int i = 0; i < dependencies.GetCount(); i++) {
        for (const auto& ref_id : dependencies[i].references) {
            // Find the resource that is referenced and add this resource as a dependent
            for (int j = 0; j < dependencies.GetCount(); j++) {
                if (dependencies[j].resource_id == ref_id) {
                    // Add this resource as a dependent of the referenced resource
                    bool alreadyAdded = false;
                    for(int k = 0; k < dependencies[j].dependents.GetCount(); k++) {
                        if(dependencies[j].dependents[k] == dependencies[i].resource_id) {
                            alreadyAdded = true;
                            break;
                        }
                    }
                    if(!alreadyAdded) {
                        dependencies[j].dependents.Add(dependencies[i].resource_id);
                    }
                    break; // Found the target, move to next reference
                }
            }
        }
    }
    
    // Use a manual approach to avoid copy issues
    for(int i = 0; i < dependencies.GetCount(); i++) {
        graph.dependencies.Add(pick(dependencies[i]));  // Use pick when adding
    }
    
    // Identify root and leaf nodes
    for (const auto& dep : graph.dependencies) {
        if (dep.references.GetCount() == 0) {
            graph.leaf_nodes.Add(dep.resource_id);  // Leaf nodes depend on nothing
        }
        if (dep.dependents.GetCount() == 0) {
            graph.root_nodes.Add(dep.resource_id);  // Root nodes are not depended on
        }
    }
    
    return graph;
}

Vector<String> FindUnusedResources(const AnimationProject& project) {
    Vector<String> unused;
    DependencyGraph graph = AnalyzeDependencies(project);
    
    // Resources that are not referenced by anything else are unused
    for (const auto& dep : graph.dependencies) {
        if (dep.dependents.GetCount() == 0) {
            // However, we need to be careful - sprites, animations, etc. might be used directly
            // by game code even if not referenced by other assets in the project
            // For now, we'll consider only those that are never referenced as unused
            unused.Add(dep.resource_id);
        }
    }
    
    return unused;
}

Vector<String> FindMissingDependencies(const AnimationProject& project, const String& base_path) {
    Vector<String> missing;
    DependencyGraph graph = AnalyzeDependencies(project);
    
    for (const auto& dep : graph.dependencies) {
        for (const auto& ref : dep.references) {
            bool found = false;
            
            // Check if the reference is to an internal resource (sprite, frame, animation, entity)
            if (project.FindSprite(ref) ||
                project.FindFrame(ref) ||
                project.FindAnimation(ref) ||
                project.FindEntity(ref)) {
                found = true;
            }
            // If not internal, check if it refers to a file
            else if (!base_path.IsEmpty()) {
                String full_path = AppendFileName(base_path, ref);
                if (FileExists(full_path)) {
                    found = true;
                }
            }
            
            if (!found) {
                // This dependency is missing
                String missing_ref = "Resource '" + dep.resource_id + "' (" + dep.resource_type + ") references missing resource: " + ref;
                bool alreadyAdded = false;
                for(int i = 0; i < missing.GetCount(); i++) {
                    if(missing[i] == missing_ref) {
                        alreadyAdded = true;
                        break;
                    }
                }
                if(!alreadyAdded) {
                    missing.Add(missing_ref);
                }
            }
        }
    }
    
    return missing;
}

Vector<String> GetBuildOrder(const DependencyGraph& graph) {
    Vector<String> build_order;
    Vector<String> visited;
    
    // Use a topological sort algorithm to determine build order
    for (const auto& node_id : graph.leaf_nodes) {  // Start with leaf nodes (no dependencies)
        bool nodeAlreadyVisited = false;
        for(int i = 0; i < visited.GetCount(); i++) {
            if(visited[i] == node_id) {
                nodeAlreadyVisited = true;
                break;
            }
        }
        if (!nodeAlreadyVisited) {
            Vector<String> stack;
            stack.Add(node_id);
            
            while (stack.GetCount() > 0) {
                String current = stack.Top();
                
                bool currentAlreadyVisited = false;
                for(int i = 0; i < visited.GetCount(); i++) {
                    if(visited[i] == current) {
                        currentAlreadyVisited = true;
                        break;
                    }
                }
                
                if (!currentAlreadyVisited) {
                    visited.Add(current);
                    
                    // Add dependent resources to stack
                    for (const auto& dep : graph.dependencies) {
                        if (dep.resource_id == current) {
                            // Add all resources that depend on this one
                            for (const auto& dependent : dep.dependents) {
                                bool dependentAlreadyVisited = false;
                                for(int j = 0; j < visited.GetCount(); j++) {
                                    if(visited[j] == dependent) {
                                        dependentAlreadyVisited = true;
                                        break;
                                    }
                                }
                                if (!dependentAlreadyVisited) {
                                    stack.Add(dependent);
                                }
                            }
                            break;
                        }
                    }
                } else {
                    stack.Drop();
                    bool currentAlreadyInBuildOrder = false;
                    for(int i = 0; i < build_order.GetCount(); i++) {
                        if(build_order[i] == current) {
                            currentAlreadyInBuildOrder = true;
                            break;
                        }
                    }
                    if (!currentAlreadyInBuildOrder) {
                        build_order.Add(current);
                    }
                }
            }
        }
    }
    
    return build_order;
}

void OptimizeAnimation(Animation& anim) {
    // For now, do nothing - in a full implementation, we'd properly handle the optimization
    // This function would modify the animation in place
}

void CompressAnimation(Animation& anim) {
    // For now, do nothing - in a full implementation, we'd properly handle the compression
    // This function would modify the animation in place
}

void CompressAnimations(Vector<Animation>& animations) {
    // In-place compression - modify each animation
    for (auto& anim : animations) {
        CompressAnimation(anim);  // This modifies the animation in place
    }
}

void CompressSprites(Vector<Sprite>& sprites, const ExportOptions& options) {
    // For now, do nothing - in a full implementation, we'd properly handle the compression
    // This function would modify the sprites in place
}

void OptimizeProject(AnimationProject& project) {
    // For now, do nothing - in a full implementation, we'd properly handle the optimization
    // This function would modify the project in place
}

VersionInfo ParseVersion(const String& version_string) {
    VersionInfo version;
    
    // Simple parsing of version strings like "1.2.3" or "1.2.3-alpha"
    Vector<String> parts = Split(version_string, '.');
    if (parts.GetCount() > 0) {
        version.major = StrInt(parts[0]);
        if (parts.GetCount() > 1) {
            // Check if there's a prerelease part in the minor version
            Vector<String> minor_parts = Split(parts[1], '-');
            version.minor = StrInt(minor_parts[0]);
            if (minor_parts.GetCount() > 1) {
                version.prerelease = minor_parts[1];
            }
            
            if (parts.GetCount() > 2) {
                // Check if there's a prerelease part in the patch version
                Vector<String> patch_parts = Split(parts[2], '-');
                version.patch = StrInt(patch_parts[0]);
                if (patch_parts.GetCount() > 1 && version.prerelease.IsEmpty()) {
                    version.prerelease = patch_parts[1];
                }
            }
        }
    }
    
    return version;
}

bool IsVersionCompatible(const VersionInfo& project_version, const VersionInfo& target_version) {
    // Major version must match for compatibility
    if (project_version.major != target_version.major) {
        return false;
    }
    
    // For now, we just check major version compatibility
    // More sophisticated version compatibility checking could be implemented here
    // For example, checking if the target version supports all features used by the project version
    
    return true;
}

bool IsVersionCompatible(const String& project_version, const String& target_version) {
    VersionInfo proj_ver = ParseVersion(project_version);
    VersionInfo target_ver = ParseVersion(target_version);
    
    return IsVersionCompatible(proj_ver, target_ver);
}

bool ValidateForExport(const AnimationProject& project, String& error_out) {
    // Validate that the project has at least one animation
    if (project.animations.GetCount() == 0) {
        error_out = "Project must contain at least one animation for export";
        return false;
    }
    
    // Validate that all referenced resources exist
    Vector<String> dangling_refs = FindDanglingSpriteReferences(project);
    if (dangling_refs.GetCount() > 0) {
        error_out = "Project has dangling sprite references: ";
        for (int i = 0; i < dangling_refs.GetCount(); i++) {
            error_out += dangling_refs[i];
            if (i < dangling_refs.GetCount() - 1) {
                error_out += ", ";
            }
        }
        return false;
    }
    
    // Validate all animations
    for (const auto& anim : project.animations) {
        if (anim.frames.GetCount() == 0) {
            error_out = "Animation '" + anim.id + "' has no frames";
            return false;
        }
    }
    
    // Validate all entities
    for (const auto& entity : project.entities) {
        String entity_error;
        if (!ValidateEntity(project, entity, entity_error)) {
            error_out = "Entity '" + entity.id + "' validation failed: " + entity_error;
            return false;
        }
    }
    
    return true;
}

} // namespace Upp