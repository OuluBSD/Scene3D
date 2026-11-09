#ifndef _AnimEdit_AnimEditMain_h_
#define _AnimEdit_AnimEditMain_h_

#include <CtrlLib/CtrlLib.h>
#include "AnimEditorState.h"
#include "AnimCanvasCtrl.h"
#include "SpriteListCtrl.h"
#include "TimelineCtrl.h"
#include "FrameListCtrl.h"
#include "AnimListCtrl.h"
#include "SpriteInstanceListCtrl.h"
#include "CollisionListCtrl.h"
#include "EntityListCtrl.h"
#include "EntityPropertiesCtrl.h"

using namespace Upp;

class AnimEditorWindow : public TopWindow {
public:
    AnimEditorWindow();

    // Public API for main window to call
    void NewProject();
    void OpenProject();
    void SaveProject();
    void SaveProjectAs();
    
    AnimEditorState& GetState() { return state; }

private:
    AnimEditorState state;
    const Animation* selected_animation; // Currently selected animation

    void InitLayout();
    void UpdateTitle();

    // Main splitters
    Splitter  hsplit_main;      // left | center+bottom | right
    Splitter  vsplit_center;    // canvas (top) | timeline (bottom)

    // Left panel
    ParentCtrl parts_panel;
    SpriteListCtrl sprite_list_ctrl;
    StaticRect parts_toolbar;   // Toolbar for parts panel controls
    EditField search_field;     // For text search
    DropList category_option;   // For category filtering
    DropList sort_option;       // For sorting options
    Button create_sprite_btn;   // For creating new sprites

    // Center-top (canvas)
    ParentCtrl canvas_panel;
    AnimCanvasCtrl canvas_ctrl;
    StaticRect canvas_toolbar;  // Toolbar for canvas controls
    Label zoom_label;           // Label to show zoom level
    // Layout for canvas controls - using ParentCtrl as container
    ParentCtrl canvas_controls_layout;  
    Option grid_snap_check;   // Checkbox for grid snapping
    Option origin_snap_check; // Checkbox for origin snapping
    Button undo_btn;            // Button for undo
    Button redo_btn;            // Button for redo

    // Center-bottom (timeline)
    ParentCtrl timeline_panel;
    StaticRect timeline_toolbar;  // Toolbar for timeline controls
    AnimEditTimelineCtrl timeline_ctrl;  // Timeline control
    Label      timeline_label;
    
    // Timeline toolbar buttons
    Button insert_frame_btn;      // Insert new frame
    Button add_existing_frame_btn; // Add existing frame
    Button duplicate_frame_btn;   // Duplicate selected frame
    Button delete_frame_btn;      // Delete selected frame
    Option loop_preview_check;  // Loop mode for preview (was CheckBox)
    Button play_btn;              // Play button
    Button pause_btn;             // Pause button
    Button stop_btn;              // Stop button

    // Right side split components
    Splitter  hsplit_right;     // frames | mid | animations
    ParentCtrl frames_panel;
    Splitter  vsplit_mid;       // sprites (top) | collisions (bottom)
    ParentCtrl sprites_panel;
    ParentCtrl collisions_panel;
    ParentCtrl animations_panel;

    // Labels for panels
    Label      frames_label;
    Label      sprites_label;
    Label      collisions_label;
    Label      animations_label;

    // Right side controls
    FrameListCtrl frame_list_ctrl;     // For frames list
    ParentCtrl frame_controls_layout;  // For frames controls
    Button new_frame_btn;              // For creating new frames

    // Animation controls
    AnimListCtrl anim_list_ctrl;       // For animations list
    ParentCtrl anim_controls_layout;   // For animations controls
    Button new_anim_btn;               // For creating new animations

    // Sprite instances list (for the sprites panel in the middle section)
    SpriteInstanceListCtrl sprite_instance_list_ctrl;  // For sprite instances list
    ParentCtrl sprite_instance_controls_layout;        // For sprite instances controls
    Button new_sprite_instance_btn;                    // For creating new sprite instances

    // Collision rectangles list (for the collisions panel in the middle section)
    CollisionListCtrl collision_list_ctrl;             // For collision rectangles list
    ParentCtrl collision_controls_layout;              // For collision controls
    Button new_collision_btn;                          // For creating new collisions

    // Helper methods
    void UpdateZoomLabel();
    void UpdateUndoRedoButtons();
    void UpdateSpriteList();
    void SetActiveFrame(const AnimationFrame* frame);
    void CreateNewSprite();
    void SetSelectedAnimation(const Animation* anim);
    
    // Timeline methods
    void InsertFrame();
    void AddExistingFrame();
    void DuplicateFrame();
    void DeleteFrame();
    void PlayAnimation();
    void PauseAnimation();
    void StopAnimation();
};

class EntityEditorWindow : public TopWindow {
public:
    EntityEditorWindow();
    void Open();
    void SetState(AnimEditorState* state) { this->state = state; }

private:
    AnimEditorState* state;  // Reference to shared state
    EntityListCtrl entity_list_ctrl;
    EntityPropertiesCtrl entity_properties_ctrl;
    Splitter hsplit_main;  // Left list | Right properties
    StaticRect entity_toolbar;  // Toolbar for entity controls
    EditField search_field;  // For text search
    DropList category_option;  // For category filtering
    DropList sort_option;  // For sorting options
    Button create_entity_btn;  // For creating new entities
    Button duplicate_entity_btn;  // For duplicating entities
    Button delete_entity_btn;  // For deleting entities
    Button import_entity_btn;  // For importing entities
    Button export_entity_btn;  // For exporting entities

    void InitLayout();
    void UpdateTitle();
    void CreateNewEntity();
    void DuplicateEntity();
    void DeleteEntity();
    void OnEntitySelectionChanged(const Entity* entity);
    void OnEntityChanged();
    
    // Import/export functionality
    void ImportEntity();
    void ExportEntity();
};

class TextureEditorWindow : public TopWindow {
public:
    TextureEditorWindow() {
        Title("Texture Editor");
        Sizeable().Zoomable();
    }
};

class AnimEditMain : public TopWindow {
public:
    AnimEditMain();

private:
    void Menu(Bar& bar);
    void MenuFile(Bar& bar);
    void MenuEditors(Bar& bar);
    void NewProject();
    void OpenProject();
    void SaveProject();
    void Exit();
    void OpenAnimEditor();
    void OpenEntityEditor();
    void OpenTextureEditor();

    AnimEditorWindow animwin;
    EntityEditorWindow entitywin;
    TextureEditorWindow texwin;
    
    // Set up shared state between editors
    void SetupSharedState();
};

#endif