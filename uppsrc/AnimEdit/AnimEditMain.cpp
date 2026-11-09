#define CLASSNAME AnimEditMain
#include "AnimEditMain.h"
#include <AnimEditLib/AnimSerialize.h>
#include "AnimEditorState.h"

void AnimEditMain::Menu(Bar& bar) {
    bar.Add("File", THISBACK(MenuFile));
    bar.Add("Editors", THISBACK(MenuEditors));
}

void AnimEditMain::MenuFile(Bar& bar) {
    bar.Add("New Project", [this]() { animwin.Open(); animwin.NewProject(); });
    bar.Add("Open Project...", [this]() { animwin.Open(); animwin.OpenProject(); });
    bar.Add("Save Project", [this]() { animwin.SaveProject(); });
    bar.Add("Save Project As...", [this]() { animwin.SaveProjectAs(); });
    bar.Separator();
    bar.Add("Exit", THISBACK(Exit));
}

void AnimEditMain::MenuEditors(Bar& bar) {
    bar.Add("Animation Editor", THISBACK(OpenAnimEditor));
    bar.Add("Entity Editor", THISBACK(OpenEntityEditor));
    bar.Add("Texture Editor", THISBACK(OpenTextureEditor));
}

void AnimEditMain::Exit() { Break(); }

void AnimEditMain::OpenAnimEditor() { animwin.Open(); }
void AnimEditMain::OpenEntityEditor() { 
    // Make sure the entity editor has the current project when it opens
    if (animwin.GetState().project.id.IsVoid() || animwin.GetState().project.entities.GetCount() == 0) {
        // Project is empty, the entity editor will still open but with empty list
    }
    entitywin.Open(); 
}
void AnimEditMain::OpenTextureEditor() { texwin.Open(); }

void AnimEditMain::SetupSharedState() {
    entitywin.SetState(&animwin.GetState());
}

AnimEditMain::AnimEditMain() {
    Title("AnimEdit");
    Sizeable().Zoomable();
    MaximizeBox();
    MinimizeBox();

    MenuBar menu;
    menu.Set(THISBACK(Menu));
    AddFrame(menu);
    
    // Set up shared state between editors
    SetupSharedState();
}

// AnimEditorWindow implementations

void AnimEditorWindow::UpdateTitle() {
    String t = "Animation Editor";
    if(!IsEmpty(state.project.name))
        t << " - " << state.project.name;
    if(!IsEmpty(state.current_path))
        t << " [" << GetFileName(state.current_path) << "]";
    if(state.dirty)
        t << " *";
    Title(t);
}

void AnimEditorWindow::NewProject() {
    state.Clear();
    state.project.id = Uuid().ToString();
    state.project.name = "Untitled";
    state.dirty = false;
    UpdateTitle();
    UpdateSpriteList();  // Refresh the sprite list
    SetSelectedAnimation(nullptr); // No animation selected initially
}

void AnimEditorWindow::OpenProject() {
    FileSel fs;
    fs.Type("Animation Project (*.json)", "*.json");
    fs.AllFilesType();
    if(!fs.ExecuteOpen("Open Animation Project"))
        return;

    String path = ~fs;
    String json = LoadFile(path);
    if(IsNull(json)) {
        Exclamation(Format("Failed to read file:\n%s", path));
        return;
    }

    AnimationProject proj;
    if(!LoadProjectJson(proj, json)) {
        Exclamation("Failed to parse animation project JSON.");
        return;
    }

    state.project = pick(proj);
    state.current_path = path;
    state.dirty = false;
    UpdateTitle();
    UpdateSpriteList();  // Refresh the sprite list
    SetSelectedAnimation(nullptr); // Reset animation selection after load
}

void AnimEditorWindow::SaveProject() {
    if(IsEmpty(state.current_path)) {
        SaveProjectAs();
        return;
    }

    String json = SaveProjectJson(state.project);
    if(!SaveFile(state.current_path, json)) {
        Exclamation(Format("Failed to save file:\n%s", state.current_path));
        return;
    }

    state.dirty = false;
    UpdateTitle();
}

void AnimEditorWindow::SaveProjectAs() {
    FileSel fs;
    fs.Type("Animation Project (*.json)", "*.json");
    fs.AllFilesType();
    if(!IsEmpty(state.current_path))
        fs <<= state.current_path;
    if(!fs.ExecuteSaveAs("Save Animation Project As"))
        return;

    String path = ~fs;
    String json = SaveProjectJson(state.project);
    if(!SaveFile(path, json)) {
        Exclamation(Format("Failed to save file:\n%s", path));
        return;
    }

    state.current_path = path;
    state.dirty = false;
    UpdateTitle();
}

void AnimEditorWindow::UpdateZoomLabel() {
    int zoomPercent = (int)(canvas_ctrl.GetZoom() * 100);
    zoom_label.SetLabel(IntStr(zoomPercent) + "%");
}

void AnimEditorWindow::UpdateUndoRedoButtons() {
    undo_btn.Enable(canvas_ctrl.CanUndo());
    redo_btn.Enable(canvas_ctrl.CanRedo());
}

void AnimEditorWindow::UpdateSpriteList() {
    sprite_list_ctrl.SetProject(&state.project);
    // If no animation is selected and there are animations in the project,
    // select the first one
    if (!selected_animation && state.project.animations.GetCount() > 0) {
        SetSelectedAnimation(&state.project.animations[0]);
    }
}

void AnimEditorWindow::SetActiveFrame(const AnimationFrame* frame) {
    sprite_list_ctrl.SetFrame(frame);
    sprite_instance_list_ctrl.SetFrame(frame);
    collision_list_ctrl.SetFrame(frame);
}

void AnimEditorWindow::SetSelectedAnimation(const Animation* anim) {
    selected_animation = anim;
    timeline_ctrl.SetAnimation(anim);
}

void AnimEditorWindow::CreateNewSprite() {
    // Create dialog content using CtrlLayout function to avoid template ambiguity
    ParentCtrl dlg;
    CtrlLayout(dlg);  // Setup layout context with function form
    dlg.SetRect(0, 0, 400, 360); // Set size directly since ParentCtrl doesn't have SizeHint

    // Create input fields first
    EditField id_field, name_field, texture_path_field, tags_field, description_field;
    EditInt region_x, region_y, region_cx, region_cy;
    EditInt pivot_x, pivot_y;
    DropList category_option;
    Button ok_btn, cancel_btn, browse_btn;

    // Set up category options
    category_option.Add("All Categories");
    category_option.Add("character");
    category_option.Add("environment");
    category_option.Add("effect");
    category_option.Add("other");
    category_option <<= 0; // Default to "All Categories"

    // Set up numeric fields
    region_x.MinMax(0, 10000) <<= 0;
    region_y.MinMax(0, 10000) <<= 0;
    region_cx.MinMax(1, 10000) <<= 32;
    region_cy.MinMax(1, 10000) <<= 32;
    pivot_x.MinMax(-1000, 1000) <<= 0;
    pivot_y.MinMax(-1000, 1000) <<= 0;

    // Add controls with positioning
    dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
    dlg.Add(name_field.HSizePos(80, 50).TopPos(32, 20));
    dlg.Add(category_option.HSizePos(80, 50).TopPos(56, 20));
    dlg.Add(texture_path_field.HSizePos(80, 25).TopPos(80, 20));
    dlg.Add(browse_btn.RightPos(8, 20).TopPos(80, 20));
    dlg.Add(region_x.RightPos(120, 25).TopPos(104, 20));
    dlg.Add(region_y.RightPos(85, 25).TopPos(104, 20));
    dlg.Add(region_cx.RightPos(45, 25).TopPos(104, 20));
    dlg.Add(region_cy.RightPos(5, 25).TopPos(104, 20));
    dlg.Add(pivot_x.RightPos(45, 25).TopPos(128, 20));
    dlg.Add(pivot_y.RightPos(5, 25).TopPos(128, 20));
    dlg.Add(tags_field.HSizePos(80, 50).TopPos(152, 20));
    dlg.Add(description_field.HSizePos(80, 50).TopPos(176, 60));
    dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
    dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

    // Labels
    Label id_label, name_label, category_label, texture_path_label, region_label, pivot_label, tags_label, description_label;
    id_label.SetLabel("ID:");
    name_label.SetLabel("Name:");
    category_label.SetLabel("Category:");
    texture_path_label.SetLabel("Texture Path:");
    region_label.SetLabel("Region (x,y,cx,cy):");
    pivot_label.SetLabel("Pivot (x,y):");
    tags_label.SetLabel("Tags:");
    description_label.SetLabel("Description:");

    dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
    dlg.Add(name_label.LeftPos(8, 60).TopPos(32, 20));
    dlg.Add(category_label.LeftPos(8, 60).TopPos(56, 20));
    dlg.Add(texture_path_label.LeftPos(8, 60).TopPos(80, 20));
    dlg.Add(region_label.LeftPos(8, 80).TopPos(104, 20));
    dlg.Add(pivot_label.LeftPos(8, 50).TopPos(128, 20));
    dlg.Add(tags_label.LeftPos(8, 60).TopPos(152, 20));
    dlg.Add(description_label.LeftPos(8, 60).TopPos(176, 20));

    browse_btn.SetLabel("...");
    browse_btn.SetTip("Browse for texture file");

    ok_btn.SetLabel("OK");
    cancel_btn.SetLabel("Cancel");

    // Browse button functionality
    browse_btn <<= [&]() {
        FileSel fs;
        fs.Type("Image Files (*.png, *.jpg, *.jpeg, *.bmp, *.tga)", "*.png;*.jpg;*.jpeg;*.bmp;*.tga");
        fs.Type("PNG Files", "*.png");
        fs.Type("JPG Files", "*.jpg;*.jpeg");
        fs.Type("BMP Files", "*.bmp");
        fs.Type("TGA Files", "*.tga");
        fs.AllFilesType();

        if (fs.ExecuteOpen("Select Texture File")) {
            String path = ~fs;
            texture_path_field = path;

            // Try to get image dimensions and set region accordingly
            FileIn in(path);
            if (in && in.IsOpen()) {
                Image img = StreamRaster::LoadImage(in);
                if (img) {
                    region_cx <<= img.GetWidth();
                    region_cy <<= img.GetHeight();
                }
            }
        }
    };
    
    // Create dialog window
    PromptOKCancelFrame prompt_dlg;
    prompt_dlg.Title("Create New Sprite");
    prompt_dlg.Add(dlg.SizePos());
    prompt_dlg.OK(ok_btn);
    prompt_dlg.Cancel(cancel_btn);

    if(prompt_dlg.Execute() == IDOK) {
        // Validate inputs
        String id = ~id_field;
        String name = ~name_field;
        String category = AsString(category_option.Get());
        String texture_path = ~texture_path_field;
        String tags_text = ~tags_field;
        String description_text = ~description_field;

        if (id.IsEmpty()) {
            Exclamation("Sprite ID cannot be empty!");
            return;
        }

        // Check if a sprite with this ID already exists
        if (state.project.FindSprite(id)) {
            Exclamation("A sprite with ID '" + id + "' already exists!");
            return;
        }

        // Create the new sprite
        Sprite new_sprite;
        new_sprite.id = id;
        new_sprite.name = name.IsEmpty() ? id : name;
        new_sprite.category = category;
        new_sprite.texture_path = texture_path;
        new_sprite.region = RectF(~region_x, ~region_y, ~region_cx, ~region_cy);
        new_sprite.pivot = Vec2(~pivot_x, ~pivot_y);

        // Parse tags - split by commas
        Vector<String> tokens = Split(tags_text, ',', true);
        for (int i = 0; i < tokens.GetCount(); i++) {
            String tag = tokens[i];  // Use Trim if available, otherwise just trim manually
            if (!tag.IsEmpty()) {
                new_sprite.tags.Add(tag);
            }
        }

        new_sprite.description = description_text;

        // Add the new sprite to the project
        state.project.sprites.Add(new_sprite);

        // Refresh the sprite list to show the new sprite
        UpdateSpriteList();

        // Mark project as dirty
        state.dirty = true;
        UpdateTitle();
    }
}

AnimEditorWindow::AnimEditorWindow() {
    Title("Animation Editor");
    Sizeable().Zoomable().MinimizeBox().MaximizeBox();

    InitLayout();
    canvas_ctrl.SetProject(&state.project);
    canvas_ctrl.SetZoomCallback([this]() { UpdateZoomLabel(); });
    canvas_ctrl.SetGeneralCallback([this]() { UpdateUndoRedoButtons(); });
    canvas_ctrl.AcceptFiles(); // Enable drag & drop
    sprite_list_ctrl.SetProject(&state.project);
    sprite_list_ctrl.SetSelectCallback([this](const Sprite* sprite) {
        // Callback when a sprite is selected in the list
        // Currently just for handling selection events
    });
    timeline_ctrl.SetProject(&state.project);
timeline_ctrl.SetFrameCallback([this](const Frame* frame) {
        // Callback when a frame is selected in the timeline
        SetActiveFrame(frame);
        canvas_ctrl.SetFrame(frame);
        canvas_ctrl.Refresh();
    });
    
    // Connect timeline toolbar buttons
    insert_frame_btn <<= [this] {
        InsertFrame();
    };
    add_existing_frame_btn <<= [this] {
        AddExistingFrame();
    };
    duplicate_frame_btn <<= [this] {
        DuplicateFrame();
    };
    delete_frame_btn <<= [this] {
        DeleteFrame();
    };
    
    // For now, play/pause/stop buttons are placeholder functionality
    play_btn <<= [this] {
        PlayAnimation();
    };
    pause_btn <<= [this] {
        PauseAnimation();
    };
    stop_btn <<= [this] {
        StopAnimation();
    };
    
    // Loop preview checkbox
    loop_preview_check <<= [this] {
        // Handle loop checkbox toggle
    };
    });
    
    grid_snap_check <<= [this] { 
        canvas_ctrl.SetGridSnapping(grid_snap_check);
    };
    origin_snap_check <<= [this] { 
        canvas_ctrl.SetOriginSnapping(origin_snap_check);
    };
    undo_btn <<= [this] { 
        canvas_ctrl.Undo();
    };
    redo_btn <<= [this] { 
        canvas_ctrl.Redo();
    };
    
    // Connect search field
    search_field <<= [this] { 
        sprite_list_ctrl.SetFilterText(search_field);
    };
    
    // Connect category filter
    category_option <<= [this] { 
        String selected_category = category_option.Get();
        sprite_list_ctrl.SetCategoryFilter(selected_category);
    };
    
    // Connect create sprite button
    create_sprite_btn <<= [this] {
        CreateNewSprite();
    };
    
    // Connect sort option
    sort_option <<= [this] {
        int sort_idx = sort_option.Get();
        SpriteListCtrl::SortType sortType;
        
        switch (sort_idx) {
            case 0: // "Sort: Name"
                sortType = SpriteListCtrl::SORT_BY_NAME;
                break;
            case 1: // "Sort: ID"
                sortType = SpriteListCtrl::SORT_BY_ID;
                break;
            case 2: // "Sort: Category"
                sortType = SpriteListCtrl::SORT_BY_CATEGORY;
                break;
            case 3: // "Sort: Recent"
                sortType = SpriteListCtrl::SORT_BY_RECENT_USE;
                break;
            default:
                sortType = SpriteListCtrl::SORT_BY_NAME;
                break;
        }
        
                sprite_list_ctrl.SetSortType(sortType);
    };

    // Connect frame list control
    frame_list_ctrl.SetProject(&state.project);
    frame_list_ctrl.SetSelectCallback([this](const Frame* frame) {
        // Callback when a frame is selected in the frames list
        SetActiveFrame(frame);
        canvas_ctrl.SetFrame(frame);
        canvas_ctrl.Refresh();
    });

    // Connect animation list control
    anim_list_ctrl.SetProject(&state.project);
    anim_list_ctrl.SetSelectCallback([this](const Animation* anim) {
        // Callback when an animation is selected in the animations list
        selected_animation = anim;
        timeline_ctrl.SetAnimation(anim);
        timeline_ctrl.Refresh();
    });

    // Connect new frame button
    new_frame_btn <<= [this] {
        // Create a new frame and add it to the project
        Frame new_frame;
        new_frame.id = "frame_" + Uuid().ToString();
        new_frame.name = "New Frame";
        state.project.frames.Add(new_frame);

        // If there's a selected animation, add the new frame to it
        if (selected_animation) {
            AnimationFrameRef frame_ref;
            frame_ref.frame_id = new_frame.id;
            frame_ref.has_duration = false;
            frame_ref.duration = 0.1; // default duration
            selected_animation->frames.Add(frame_ref);
        }

        state.dirty = true;
        UpdateTitle();
        frame_list_ctrl.RefreshList();
    };

    // Connect new animation button
    new_anim_btn <<= [this] {
        // Create a new animation and add it to the project
        Animation new_anim;
        new_anim.id = "anim_" + Uuid().ToString();
        new_anim.name = "New Animation";
        new_anim.category = "default";
        new_anim.loop_type = "No Loop";
        state.project.animations.Add(new_anim);

        state.dirty = true;
        UpdateTitle();
        anim_list_ctrl.RefreshList();
    };

    // Connect sprite instance list control
    sprite_instance_list_ctrl.SetProject(&state.project);
    sprite_instance_list_ctrl.SetFrame(nullptr); // Will be set when frame changes
    sprite_instance_list_ctrl.SetSelectCallback([this](const SpriteInstance* si) {
        // Callback when a sprite instance is selected in the sprites list
        // For now, just refresh the canvas to potentially highlight the selected instance
        canvas_ctrl.Refresh();
    });
    sprite_instance_list_ctrl.SetChangeCallback([this]() {
        // Callback when sprite instances change
        state.dirty = true;
        UpdateTitle();
        canvas_ctrl.Refresh(); // Refresh the canvas to show updates
    });

    // Connect collision list control
    collision_list_ctrl.SetFrame(nullptr); // Will be set when frame changes
    collision_list_ctrl.SetSelectCallback([this](const CollisionRect* cr) {
        // Callback when a collision rectangle is selected in the collisions list
        canvas_ctrl.Refresh(); // Refresh the canvas to potentially highlight the selected collision
    });
    collision_list_ctrl.SetChangeCallback([this]() {
        // Callback when collisions change
        state.dirty = true;
        UpdateTitle();
        canvas_ctrl.Refresh(); // Refresh the canvas to show updates
    });

    // Connect new sprite instance button
    new_sprite_instance_btn <<= [this] {
        if (!canvas_ctrl.GetFrame()) {
            Exclamation("No active frame to add sprite instance to!");
            return;
        }

        // Create a dialog to select a sprite to add
        Vector<String> spriteIDs;
        Vector<String> spriteNames;
        for (int i = 0; i < state.project.sprites.GetCount(); i++) {
            spriteIDs.Add(state.project.sprites[i].id);
            spriteNames.Add(!state.project.sprites[i].name.IsEmpty() ? 
                           state.project.sprites[i].name : state.project.sprites[i].id);
        }

        if (spriteIDs.GetCount() == 0) {
            Exclamation("No sprites available to add. Please create a sprite first.");
            return;
        }

        // Create a combo box to select a sprite
        CtrlLayout dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(300, 80); });

        ArrayCtrl array_ctrl;
        array_ctrl.AddColumn("Name", 200);
        array_ctrl.SetFrame(ThinInsetFrame());
        array_ctrl.NoHeader();

        for (int i = 0; i < spriteNames.GetCount(); i++) {
            array_ctrl.Add(i, spriteNames[i]);
        }

        Button ok_btn, cancel_btn;
        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        dlg.Add(array_ctrl.HSizePos(8, 8).VSizePos(8, 32));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Select Sprite to Add");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK && array_ctrl.GetCount() > 0) {
            int selected = array_ctrl.GetCursor();
            if (selected >= 0 && selected < spriteIDs.GetCount()) {
                // Create a new sprite instance with the selected sprite
                SpriteInstance new_si;
                new_si.sprite_id = spriteIDs[selected];
                new_si.transform.position = Vec2(0, 0);  // Default position
                new_si.transform.scale = Vec2(1, 1);     // Default scale
                new_si.transform.rotation = 0;           // Default rotation
                new_si.z_index = 0;                      // Default z-index

                // Add to the current frame
                const Frame* current_frame = canvas_ctrl.GetFrame();
                if (current_frame) {
                    // Find the frame in the project and modify it
                    for (int i = 0; i < state.project.frames.GetCount(); i++) {
                        if (state.project.frames[i].id == current_frame->id) {
                            state.project.frames[i].sprites.Add(new_si);
                            break;
                        }
                    }
                }

                state.dirty = true;
                UpdateTitle();
                sprite_instance_list_ctrl.SetFrame(current_frame); // Refresh the list
                sprite_instance_list_ctrl.RefreshList();
                canvas_ctrl.Refresh(); // Refresh the canvas to show the new instance
            }
        }
    };

    // Connect new collision button
    new_collision_btn <<= [this] {
        if (!canvas_ctrl.GetFrame()) {
            Exclamation("No active frame to add collision to!");
            return;
        }

        // Create a new collision rectangle
        CollisionRect new_cr;
        new_cr.id = "collision_" + Uuid().ToString();
        new_cr.rect = RectF(0, 0, 32, 32); // Default size at origin

        // Add to the current frame
        const Frame* current_frame = canvas_ctrl.GetFrame();
        if (current_frame) {
            // Find the frame in the project and modify it
            for (int i = 0; i < state.project.frames.GetCount(); i++) {
                if (state.project.frames[i].id == current_frame->id) {
                    state.project.frames[i].collisions.Add(new_cr);
                    break;
                }
            }
        }

        state.dirty = true;
        UpdateTitle();
        collision_list_ctrl.SetFrame(current_frame); // Refresh the list
        collision_list_ctrl.RefreshList();
        canvas_ctrl.Refresh(); // Refresh the canvas to show the new collision
    };
    
    UpdateZoomLabel();  // Initialize the zoom label
    UpdateUndoRedoButtons();  // Initialize undo/redo buttons
    selected_animation = nullptr; // Initialize selected animation
    NewProject();
}

void AnimEditorWindow::InitLayout() {
    // Setup labels
    timeline_label.SetLabel("Timeline");
    frames_label.SetLabel("Frames");
    sprites_label.SetLabel("Sprites");
    collisions_label.SetLabel("Collisions");
    animations_label.SetLabel("Animations");

    // Setup parts panel with toolbar and sprite list
    parts_toolbar.SetFrame(ThinInsetFrame());
    parts_toolbar.Add(search_field.VSizePos(4, 4).HSizePos(4, 40));  // Search field on left
    search_field.SetPrompt("Search sprites...");
    parts_toolbar.Add(category_option.HSizePos(44, 40).VSizePos(4, 4));  // Category filter
    category_option.Add("All Categories");
    category_option.Add("character");
    category_option.Add("environment");
    category_option.Add("effect");
    category_option.Add("other");
    category_option <<= 0;  // Default to "All Categories"
    parts_toolbar.Add(sort_option.HSizePos(88, 40).VSizePos(4, 4));  // Sort option
    sort_option.Add("Sort: Name");
    sort_option.Add("Sort: ID");
    sort_option.Add("Sort: Category");
    sort_option.Add("Sort: Recent");
    sort_option <<= 0;  // Default to "Sort: Name"
    parts_toolbar.Add(create_sprite_btn.HSizePos(132, 24).VSizePos(4, 4));  // Create button
    create_sprite_btn.SetLabel("+");
    create_sprite_btn.SetTip("Create new sprite");
    
    parts_panel.SetFrame(ThinInsetFrame());
    parts_panel.Add(parts_toolbar.TopPos(0, 28).HSizePos());  // Toolbar at top
    parts_panel.Add(sprite_list_ctrl.VSizePos(28).HSizePos());  // List below toolbar
    
    // Setup canvas toolbar and controls
    canvas_toolbar.SetFrame(ThinInsetFrame());
    canvas_toolbar.Add(undo_btn.LeftPos(4, 30).VSizePos(4, 4));
    undo_btn.SetLabel("Undo");
    canvas_toolbar.Add(redo_btn.HSizePos(38, 30).VSizePos(4, 4));
    redo_btn.SetLabel("Redo");
    canvas_toolbar.Add(grid_snap_check.HSizePos(72, 80).VSizePos(4, 4));
    grid_snap_check.SetLabel("Grid Snap");
    canvas_toolbar.Add(origin_snap_check.HSizePos(156, 80).VSizePos(4, 4));
    origin_snap_check.SetLabel("Origin Snap");
    canvas_toolbar.Add(zoom_label.RightPos(80).VSizePos(4, 4));
canvas_toolbar.Add(zoom_label.RightPos(80).VSizePos(4, 4));
zoom_label.SetLabel("100%");

canvas_panel.SetFrame(ThinInsetFrame());
canvas_panel.Add(canvas_toolbar.TopPos(0, 24).HSizePos());
canvas_panel.Add(canvas_ctrl.VSizePos(24).HSizePos());  // Leave space at top for toolbar

// Setup timeline toolbar
timeline_toolbar.SetFrame(ThinInsetFrame());
timeline_toolbar.Add(insert_frame_btn.LeftPos(4, 30).VSizePos(4, 4));
insert_frame_btn.SetLabel("Insert");
insert_frame_btn.SetTip("Insert new frame");
timeline_toolbar.Add(add_existing_frame_btn.HSizePos(38, 30).VSizePos(4, 4));
add_existing_frame_btn.SetLabel("Add");
add_existing_frame_btn.SetTip("Add existing frame");
timeline_toolbar.Add(duplicate_frame_btn.HSizePos(72, 30).VSizePos(4, 4));
duplicate_frame_btn.SetLabel("Dup");
duplicate_frame_btn.SetTip("Duplicate selected frame");
timeline_toolbar.Add(delete_frame_btn.HSizePos(106, 30).VSizePos(4, 4));
delete_frame_btn.SetLabel("Del");
delete_frame_btn.SetTip("Delete selected frame");
timeline_toolbar.Add(play_btn.HSizePos(140, 30).VSizePos(4, 4));
play_btn.SetLabel("Play");
play_btn.SetTip("Play animation");
timeline_toolbar.Add(pause_btn.HSizePos(174, 30).VSizePos(4, 4));
pause_btn.SetLabel("Pause");
pause_btn.SetTip("Pause animation");
timeline_toolbar.Add(stop_btn.HSizePos(208, 30).VSizePos(4, 4));
stop_btn.SetLabel("Stop");
stop_btn.SetTip("Stop animation");
timeline_toolbar.Add(loop_preview_check.HSizePos(242, 80).VSizePos(4, 4));
loop_preview_check.SetLabel("Loop Preview");

timeline_panel.SetFrame(ThinInsetFrame());
timeline_panel.Add(timeline_toolbar.TopPos(0, 24).HSizePos());  // Toolbar at top
timeline_panel.Add(timeline_ctrl.VSizePos(24).HSizePos());  // Leave space at top for toolbar

    // Frames panel - with controls and list
    frame_controls_layout.Ctrl::SizeHint([this]() { return Size(100, 24); });
    frame_controls_layout.Add(new_frame_btn.HSizePos(4, 24).VSizePos(4, 4));
    new_frame_btn.SetLabel("+");
    new_frame_btn.SetTip("Create new frame");
    
    frames_panel.SetFrame(ThinInsetFrame());
    frames_panel.Add(frame_controls_layout.TopPos(0, 28).HSizePos());  // Controls at top
    frames_panel.Add(frame_list_ctrl.VSizePos(28).HSizePos());    // List below controls

    // Sprites panel - for sprite instances list
    sprite_instance_controls_layout.Ctrl::SizeHint([this]() { return Size(100, 24); });
    sprite_instance_controls_layout.Add(new_sprite_instance_btn.HSizePos(4, 24).VSizePos(4, 4));
    new_sprite_instance_btn.SetLabel("+");
    new_sprite_instance_btn.SetTip("Create new sprite instance");
    
    sprites_panel.SetFrame(ThinInsetFrame());
    sprites_panel.Add(sprite_instance_controls_layout.TopPos(0, 28).HSizePos());  // Controls at top
    sprites_panel.Add(sprite_instance_list_ctrl.VSizePos(28).HSizePos());  // List below controls

    // Collisions panel - for collision rectangles list
    collision_controls_layout.Ctrl::SizeHint([this]() { return Size(100, 24); });
    collision_controls_layout.Add(new_collision_btn.HSizePos(4, 24).VSizePos(4, 4));
    new_collision_btn.SetLabel("+");
    new_collision_btn.SetTip("Create new collision");
    
    collisions_panel.SetFrame(ThinInsetFrame());
    collisions_panel.Add(collision_controls_layout.TopPos(0, 28).HSizePos());  // Controls at top
    collisions_panel.Add(collision_list_ctrl.VSizePos(28).HSizePos());  // List below controls

    // Animations panel - with controls and list
    anim_controls_layout.Ctrl::SizeHint([this]() { return Size(100, 24); });
    anim_controls_layout.Add(new_anim_btn.HSizePos(4, 24).VSizePos(4, 4));
    new_anim_btn.SetLabel("+");
    new_anim_btn.SetTip("Create new animation");
    
    animations_panel.SetFrame(ThinInsetFrame());
    animations_panel.Add(anim_controls_layout.TopPos(0, 28).HSizePos());  // Controls at top
    animations_panel.Add(anim_list_ctrl.VSizePos(28).HSizePos());    // List below controls
    vsplit_mid.Horz()
        .SetPos(70) // Percentage
        << sprites_panel
        << collisions_panel;

    // Right side (frames | mid | animations)
    hsplit_right.Vert()
        .SetPos(33, 66) // Two split points
        << frames_panel
        << vsplit_mid
        << animations_panel;

    // Center (canvas | timeline)
    vsplit_center.Horz()
        .SetPos(80) // Percentage
        << canvas_panel
        << timeline_panel;

    // Main horizontal layout (left | center | right)
    hsplit_main.Vert()
        .SetPos(20, 75) // Left 20%, center 55%, right 25% (20+55=75)
        << parts_panel
        << vsplit_center
        << hsplit_right;

    Add(hsplit_main.SizePos());
}void AnimEditorWindow::InsertFrame() {
    if (!selected_animation) {
        Exclamation("No animation selected!");
        return;
    }
    
    // Create a new frame and add it to the project
    Frame new_frame;
    new_frame.id = "frame_" + Uuid().ToString();
    new_frame.name = "New Frame";
    state.project.frames.Add(new_frame);
    
    // Add the new frame to the selected animation at the current selection index or at the end
    int insert_index = (timeline_ctrl.GetSelectedFrameIndex() >= 0) ? 
                       timeline_ctrl.GetSelectedFrameIndex() + 1 : 
                       selected_animation->frames.GetCount();
    
    AnimationFrameRef frame_ref;
    frame_ref.frame_id = new_frame.id;
    frame_ref.has_duration = false;
    frame_ref.duration = 0.1; // default duration
    selected_animation->frames.Insert(insert_index, frame_ref);
    
    state.dirty = true;
    UpdateTitle();
    
    // Update the timeline to show the new frame
    SetSelectedAnimation(selected_animation);
}

void AnimEditorWindow::AddExistingFrame() {
    if (!selected_animation) {
        Exclamation("No animation selected!");
        return;
    }
    
    // Create a simple dialog to select an existing frame
    CtrlLayout dlg;
    dlg.Ctrl::SizeHint([this]() { return Size(300, 400); });
    
    ArrayCtrl array_ctrl;
    array_ctrl.AddColumn("ID", 100);
    array_ctrl.AddColumn("Name", 150);
    
    // Add all existing frames to the array
    for (const Frame& frame : state.project.frames) {
        array_ctrl.Add(frame.id, frame.name);
    }
    
    Button ok_btn, cancel_btn;
    ok_btn.SetLabel("OK");
    cancel_btn.SetLabel("Cancel");
    
    dlg.Add(array_ctrl.HSizePos().VSizePos(0, 40));
    dlg.Add(ok_btn.LeftPos(50, 60).BottomPos(5, 25));
    dlg.Add(cancel_btn.RightPos(50, 60).BottomPos(5, 25));
    
    // Create dialog window
    PromptOKCancelFrame prompt_dlg;
    prompt_dlg.Title("Select Frame to Add");
    prompt_dlg.Add(dlg.SizePos());
    prompt_dlg.OK(ok_btn);
    prompt_dlg.Cancel(cancel_btn);
    
    if (prompt_dlg.Execute() == IDOK && array_ctrl.GetCount() > 0 && array_ctrl.Get() >= 0) {
        int selected_row = array_ctrl.Get();
        String selected_frame_id = array_ctrl.Get(0, selected_row); // Get ID
        
        // Add the selected frame to the animation
        AnimationFrameRef frame_ref;
        frame_ref.frame_id = selected_frame_id;
        frame_ref.has_duration = false;
        frame_ref.duration = 0.1; // default duration
        int insert_index = (timeline_ctrl.GetSelectedFrameIndex() >= 0) ? 
                           timeline_ctrl.GetSelectedFrameIndex() + 1 : 
                           selected_animation->frames.GetCount();
        selected_animation->frames.Insert(insert_index, frame_ref);
        
        state.dirty = true;
        UpdateTitle();
        
        // Update the timeline to show the new frame
        SetSelectedAnimation(selected_animation);
    }
}

void AnimEditorWindow::DuplicateFrame() {
    if (!selected_animation) {
        Exclamation("No animation selected!");
        return;
    }
    
    int selected_frame_index = timeline_ctrl.GetSelectedFrameIndex();
    if (selected_frame_index < 0 || selected_frame_index >= selected_animation->frames.GetCount()) {
        Exclamation("No frame selected to duplicate!");
        return;
    }
    
    // Get the frame reference to duplicate
    AnimationFrameRef original_ref = selected_animation->frames[selected_frame_index];
    const Frame* original_frame = state.project.FindFrame(original_ref.frame_id);
    
    if (!original_frame) {
        Exclamation("Original frame not found!");
        return;
    }
    
    // Create a copy of the frame
    Frame new_frame = *original_frame;
    new_frame.id = "dup_" + original_frame->id + "_" + Uuid().ToString();
    new_frame.name = original_frame->name + " (copy)";
    state.project.frames.Add(new_frame);
    
    // Add the new frame reference to the animation after the original
    AnimationFrameRef new_ref;
    new_ref.frame_id = new_frame.id;
    new_ref.has_duration = original_ref.has_duration;
    new_ref.duration = original_ref.duration;
    selected_animation->frames.Insert(selected_frame_index + 1, new_ref);
    
    state.dirty = true;
    UpdateTitle();
    
    // Update the timeline to show the new frame
    SetSelectedAnimation(selected_animation);
}

void AnimEditorWindow::DeleteFrame() {
    if (!selected_animation) {
        Exclamation("No animation selected!");
        return;
    }
    
    int selected_frame_index = timeline_ctrl.GetSelectedFrameIndex();
    if (selected_frame_index < 0 || selected_frame_index >= selected_animation->frames.GetCount()) {
        Exclamation("No frame selected to delete!");
        return;
    }
    
    // Confirm deletion
    if (!PromptYesNo("Are you sure you want to delete this frame?")) {
        return;
    }
    
    // Remove the frame reference from the animation
    selected_animation->frames.Remove(selected_frame_index);
    
    state.dirty = true;
    UpdateTitle();
    
    // Update the timeline
    SetSelectedAnimation(selected_animation);
}

void AnimEditorWindow::PlayAnimation() {
    // Placeholder for animation playback functionality
    PromptOK("Animation playback would start here.");
}

void AnimEditorWindow::PauseAnimation() {
    // Placeholder for animation pause functionality
    PromptOK("Animation pause would happen here.");
}

void AnimEditorWindow::StopAnimation() {
    // Placeholder for animation stop functionality
    PromptOK("Animation stop would happen here.");
}

// Entity Editor Window Implementation

EntityEditorWindow::EntityEditorWindow() {
    Title("Entity Editor");
    Sizeable().Zoomable().MinimizeBox().MaximizeBox();
    state = nullptr;
    
    InitLayout();
    
    // Set up the entity list and properties controls
    entity_list_ctrl.SetSelectCallback([this](const Entity* entity) {
        OnEntitySelectionChanged(entity);
    });
    
    entity_properties_ctrl.SetChangeCallback([this]() {
        OnEntityChanged();
    });
}

void EntityEditorWindow::Open() {
    OpenMain();
}

void EntityEditorWindow::InitLayout() {
    // Setup labels
    search_field.SetPrompt("Search entities...");
    
    // Setup category filter
    category_option.Add("All Categories");
    category_option.Add("character");
    category_option.Add("environment");
    category_option.Add("item");
    category_option.Add("other");
    category_option <<= 0;  // Default to "All Categories"
    
    // Setup sort options
    sort_option.Add("Sort: Name");
    sort_option.Add("Sort: ID");
    sort_option.Add("Sort: Type");
    sort_option <<= 0;  // Default to "Sort: Name"
    
    // Setup toolbar
    entity_toolbar.SetFrame(ThinInsetFrame());
    entity_toolbar.Add(search_field.HSizePos(4, 40).VSizePos(4, 4));
    entity_toolbar.Add(category_option.HSizePos(44, 40).VSizePos(4, 4));
    entity_toolbar.Add(sort_option.HSizePos(88, 40).VSizePos(4, 4));
    entity_toolbar.Add(create_entity_btn.HSizePos(132, 24).VSizePos(4, 4));
    entity_toolbar.Add(duplicate_entity_btn.HSizePos(160, 24).VSizePos(4, 4));
    entity_toolbar.Add(delete_entity_btn.HSizePos(188, 24).VSizePos(4, 4));
    entity_toolbar.Add(import_entity_btn.HSizePos(216, 24).VSizePos(4, 4));
    entity_toolbar.Add(export_entity_btn.HSizePos(244, 24).VSizePos(4, 4));
    
    create_entity_btn.SetLabel("+");
    create_entity_btn.SetTip("Create new entity");
    duplicate_entity_btn.SetLabel("Dup");
    duplicate_entity_btn.SetTip("Duplicate selected entity");
    delete_entity_btn.SetLabel("Del");
    delete_entity_btn.SetTip("Delete selected entity");
    import_entity_btn.SetLabel("Import");
    import_entity_btn.SetTip("Import entity from JSON file");
    export_entity_btn.SetLabel("Export");
    export_entity_btn.SetTip("Export selected entity to JSON file");
    
    // Set up the splitter
    hsplit_main.Vert().SetPos(30);  // 30% for list, 70% for properties
    
    // Left panel - entity list
    ParentCtrl list_panel;
    list_panel.SetFrame(ThinInsetFrame());
    list_panel.Add(entity_toolbar.TopPos(0, 28).HSizePos());
    list_panel.Add(entity_list_ctrl.VSizePos(28).HSizePos());
    
    // Right panel - entity properties
    ParentCtrl props_panel;
    props_panel.SetFrame(ThinInsetFrame());
    props_panel.Add(entity_properties_ctrl.SizePos());
    
    hsplit_main << list_panel << props_panel;
    Add(hsplit_main.SizePos());
    
    // Connect events
    search_field <<= [this] {
        entity_list_ctrl.SetFilterText(search_field);
    };
    
    category_option <<= [this] {
        String selected_category = AsString(category_option.Get());
        entity_list_ctrl.SetCategoryFilter(selected_category);
    };
    
    sort_option <<= [this] {
        int sort_idx = sort_option.Get();
        EntityListCtrl::SortType sortType;
        
        switch (sort_idx) {
            case 0: // "Sort: Name"
                sortType = EntityListCtrl::SORT_BY_NAME;
                break;
            case 1: // "Sort: ID"
                sortType = EntityListCtrl::SORT_BY_ID;
                break;
            case 2: // "Sort: Type"
                sortType = EntityListCtrl::SORT_BY_TYPE;
                break;
            case 3: // "Sort: Recent"
                sortType = EntityListCtrl::SORT_BY_RECENT_USE;
                break;
            default:
                sortType = EntityListCtrl::SORT_BY_NAME;
                break;
        }
        
        entity_list_ctrl.SetSortType(sortType);
    };
    
    create_entity_btn <<= [this] {
        CreateNewEntity();
    };
    
    duplicate_entity_btn <<= [this] {
        DuplicateEntity();
    };
    
    delete_entity_btn <<= [this] {
        DeleteEntity();
    };
    
    import_entity_btn <<= [this] {
        ImportEntity();
    };
    
    export_entity_btn <<= [this] {
        ExportEntity();
    };
}

void EntityEditorWindow::UpdateTitle() {
    String t = "Entity Editor";
    if (state && !IsEmpty(state->project.name)) {
        t << " - " << state->project.name;
    }
    if (state && !IsEmpty(state->current_path)) {
        t << " [" << GetFileName(state->current_path) << "]";
    }
    if (state && state->dirty) {
        t << " *";
    }
    Title(t);
}

void EntityEditorWindow::Open() {
    // Initialize with reference to the main state
    if (IsOpen())
        return;
    
    // Update the controls with the current project data
    if (state) {
        entity_list_ctrl.SetProject(&state->project);
    }
    
    OpenMain();
}

void EntityEditorWindow::CreateNewEntity() {
    if (!state) return;
    
    // Create a new entity and add it to the project
    Entity new_entity;
    new_entity.id = "entity_" + Uuid().ToString();
    new_entity.name = "New Entity";
    new_entity.type = "character";
    
    state->project.entities.Add(new_entity);
    state->dirty = true;
    UpdateTitle();
    entity_list_ctrl.RefreshList();
}

void EntityEditorWindow::DuplicateEntity() {
    if (!state) return;

    // Get the currently selected entity from the list
    int selected_row = entity_list_ctrl.GetCursor();
    if (selected_row < 0 || selected_row >= state->project.entities.GetCount()) {
        Exclamation("No entity selected for duplication!");
        return;
    }

    const Entity& original_entity = state->project.entities[selected_row];

    // Create a copy of the entity with a new ID
    Entity new_entity = original_entity;
    new_entity.id = "entity_" + Uuid().ToString();
    new_entity.name = original_entity.name + " (copy)";

    // Check if the entity has any name conflicts in the project and adjust if needed
    int counter = 1;
    String original_name = new_entity.name;
    while (state->project.FindEntity(new_entity.id)) {
        new_entity.id = "entity_" + Uuid().ToString();
    }
    
    // Also ensure the name is unique
    while (true) {
        bool is_unique = true;
        for (int i = 0; i < state->project.entities.GetCount(); i++) {
            if (state->project.entities[i].name == new_entity.name && 
                state->project.entities[i].id != original_entity.id) {
                new_entity.name = original_name + " (" + IntStr(++counter) + ")";
                is_unique = false;
                break;
            }
        }
        if (is_unique) break;
    }

    // Add the new entity to the project
    state->project.entities.Add(new_entity);
    state->dirty = true;
    UpdateTitle();
    entity_list_ctrl.RefreshList();
}

void EntityEditorWindow::DeleteEntity() {
    if (!state) return;

    // Get the currently selected entity from the list
    int selected_row = entity_list_ctrl.GetCursor();
    if (selected_row < 0 || selected_row >= state->project.entities.GetCount()) {
        Exclamation("No entity selected for deletion!");
        return;
    }

    const Entity& entity_to_delete = state->project.entities[selected_row];

    // Confirm deletion with the user
    if (!PromptYesNo("Are you sure you want to delete the entity '" + entity_to_delete.name + "'?")) {
        return;
    }

    // Check if any other entities reference this entity (currently not applicable for our basic implementation)
    // But we might want to check if other parts of the project reference this entity

    // Remove the entity from the project
    state->project.entities.Remove(selected_row);
    
    // Update the UI to show the changes
    entity_list_ctrl.RefreshList();
    entity_properties_ctrl.SetEntity(nullptr); // Clear properties panel
    
    state->dirty = true;
    UpdateTitle();
}
void EntityEditorWindow::OnEntitySelectionChanged(const Entity* entity) {
    entity_properties_ctrl.SetProject(&state->project);
    entity_properties_ctrl.SetEntity(entity);
}

void EntityEditorWindow::OnEntityChanged() {
    if (state) {
        state->dirty = true;
        UpdateTitle();
    }
}

void EntityEditorWindow::ImportEntity() {
    if (!state) {
        Exclamation("No project loaded!");
        return;
    }
    
    FileSel fs;
    fs.Type("Entity JSON (*.json)", "*.json");
    fs.AllFilesType();
    if (!fs.ExecuteOpen("Import Entity")) {
        return;
    }
    
    String path = ~fs;
    String json = LoadFile(path);
    if (IsNull(json)) {
        Exclamation(Format("Failed to read file:\n%s", path));
        return;
    }
    
    // Parse the entity from JSON
    Entity entity;
    if (!LoadFromJson(entity, json)) {
        Exclamation("Failed to parse entity JSON.");
        return;
    }
    
    // Check if an entity with this ID already exists
    if (state->project.FindEntity(entity.id)) {
        Exclamation("An entity with ID '" + entity.id + "' already exists in the project!");
        return;
    }
    
    // Add the entity to the project
    state->project.entities.Add(entity);
    state->dirty = true;
    UpdateTitle();
    entity_list_ctrl.RefreshList();
    
    PromptOK("Entity imported successfully!");
}

void EntityEditorWindow::ExportEntity() {
    if (!state) {
        Exclamation("No project loaded!");
        return;
    }
    
    // For now, export all entities - in a real implementation, we'd export just the selected one
    // Get the selected entity from the list
    int selected_row = entity_list_ctrl.GetCursor(); // This assumes the EntityListCtrl has a GetCursor method
    if (selected_row < 0 || selected_row >= state->project.entities.GetCount()) {
        Exclamation("No entity selected for export!");
        return;
    }
    
    const Entity& entity = state->project.entities[selected_row];
    
    FileSel fs;
    fs.Type("Entity JSON (*.json)", "*.json");
    fs.AllFilesType();
    if (!fs.ExecuteSaveAs("Export Entity As")) {
        return;
    }
    
    String path = ~fs;
    String json = StoreAsJson(entity, true); // true for pretty formatting
    
    if (!SaveFile(path, json)) {
        Exclamation(Format("Failed to save file:\n%s", path));
        return;
    }
    
    PromptOK("Entity exported successfully!");
}