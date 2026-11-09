#include "EntityPropertiesCtrl.h"

EntityPropertiesCtrl::EntityPropertiesCtrl() {
    project = nullptr;
    current_entity = nullptr;
    
    // Create the layout
    CtrlLayout(*this);
    
    // Set up type options
    type_option.Add("character");
    type_option.Add("environment");
    type_option.Add("item");
    type_option.Add("other");
    type_option <<= 0;  // Default to character
    
    // Set up animation slots control
    animation_slots_ctrl.AddColumn("Slot Name", 100);
    animation_slots_ctrl.AddColumn("Animation ID", 150);
    animation_slots_ctrl.SetFrame(ThinInsetFrame());
    animation_slots_ctrl.NoHeader();
    
    // Set up properties control
    properties_ctrl.AddColumn("Property Name", 120);
    properties_ctrl.AddColumn("Value", 120);
    properties_ctrl.SetFrame(ThinInsetFrame());
    properties_ctrl.NoHeader();
    
    // Set up slot editing fields
    slot_name_field.SetLabel("Slot Name");
    slot_name_field.ToolTip("Name for the animation slot (e.g. Idle, Run, Attack)");
    
    // Set up animation dropdown
    animation_dropdown.SetLabel("(No Animation)");
    animation_dropdown.ToolTip("Select animation for this slot");
    
    // Set up property editing fields
    add_property_btn.SetLabel("+");
    remove_property_btn.SetLabel("-");
    add_slot_btn.SetLabel("+");
    remove_slot_btn.SetLabel("-");
    
    // Set up validation status display
    validation_status.SetFrame(ThinInsetFrame());
    validation_status.Set("Validation: OK");
    
    // Set up animation playback controls
    play_btn.SetLabel("▶ Play");
    play_btn.ToolTip("Play the selected animation");
    pause_btn.SetLabel("⏸ Pause");
    pause_btn.ToolTip("Pause the animation");
    stop_btn.SetLabel("⏹ Stop");
    stop_btn.ToolTip("Stop and reset the animation");
    loop_check.SetLabel("Loop");
    loop_check.ToolTip("Loop the animation continuously");
    loop_check.Set(true);  // Loop by default
    speed_label.SetLabel("Speed: 1.0x");
    speed_slider.MinMax(1, 500).SetValue(100);  // Range 0.01x to 5.0x (value/100)
    speed_slider.ToolTip("Adjust animation playback speed");
    
    // Set up blending controls
    blend_weight_label.SetLabel("Blend Weight: 1.00");
    blend_weight_slider.MinMax(0, 100).SetValue(100);  // 0.00 to 1.00 as 0-100
    blend_weight_slider.ToolTip("Adjust the blend weight for the selected animation (0.0 to 1.0)");
    transition_label.SetLabel("Transition Time (s):");
    transition_time_edit.MinMax(0.0, 5.0).Set(0.0);  // 0 to 5 seconds
    transition_time_edit.ToolTip("Time to transition to this animation (in seconds)");
    apply_blend_btn.SetLabel("Apply Blend");
    apply_blend_btn.SetTip("Apply the current blend settings to the selected animation slot");
    
    // Set up event management controls
    event_list_ctrl.AddColumn("Name", 80);
    event_list_ctrl.AddColumn("Type", 60);
    event_list_ctrl.AddColumn("Frame", 40);
    event_list_ctrl.SetFrame(ThinInsetFrame());
    event_list_ctrl.NoHeader();
    
    add_event_btn.SetLabel("+");
    add_event_btn.SetTip("Add new animation event");
    remove_event_btn.SetLabel("-");
    remove_event_btn.SetTip("Remove selected animation event");
    edit_event_btn.SetLabel("Edit");
    edit_event_btn.SetTip("Edit selected animation event");
    
    event_name_field.SetPrompt("Event Name");
    event_type_option.Add("sound");
    event_type_option.Add("particle");
    event_type_option.Add("callback");
    event_type_option.Add("trigger");
    event_type_option.SetIndex(0);  // Default to "sound"
    event_frame_field.SetRange(0, 9999).Set(0);  // Frame index from 0 to 9999
    event_frame_field.SetTip("Frame number at which event should trigger");
    
    // Set up transition management controls
    transition_list_ctrl.AddColumn("From", 80);
    transition_list_ctrl.AddColumn("To", 80);
    transition_list_ctrl.AddColumn("Time(s)", 60);
    transition_list_ctrl.SetFrame(ThinInsetFrame());
    transition_list_ctrl.NoHeader();
    
    add_transition_btn.SetLabel("+");
    add_transition_btn.SetTip("Add new animation transition");
    remove_transition_btn.SetLabel("-");
    remove_transition_btn.SetTip("Remove selected animation transition");
    edit_transition_btn.SetLabel("Edit");
    edit_transition_btn.SetTip("Edit selected animation transition");
    
    transition_time_field.SetRange(0.0, 5.0).Set(0.2);  // Transition time from 0 to 5 seconds, default 0.2
    condition_field.SetPrompt("Condition (e.g. speed > 0)");
    condition_field.SetTip("Condition for the transition to occur");
    
    // Set up entity animation parameters controls
    speed_multiplier_field.SetRange(0.01, 10.0).Set(1.0);  // Range from 0.01x to 10x speed
    speed_multiplier_field.SetTip("Multiplier for animation playback speed (1.0 = normal speed)");
    time_offset_field.SetRange(0.0, 10.0).Set(0.0);  // Time offset from 0 to 10 seconds
    time_offset_field.SetTip("Time offset to start animation from a different point (in seconds)");
    looping_check.SetLabel("Loop animations");
    looping_check.SetTip("Whether animations should loop by default for this entity");
    looping_check.Set(true);  // Default to true (looping)
    apply_anim_params_btn.SetLabel("Apply Params");
    apply_anim_params_btn.SetTip("Apply the current animation parameters to the entity");

    // Set up script management controls
    script_list_ctrl.AddColumn("ID", 80);
    script_list_ctrl.AddColumn("Name", 100);
    script_list_ctrl.AddColumn("Type", 60);
    script_list_ctrl.SetFrame(ThinInsetFrame());
    script_list_ctrl.NoHeader();

    add_script_btn.SetLabel("+");
    add_script_btn.SetTip("Add new script to this entity");
    remove_script_btn.SetLabel("-");
    remove_script_btn.SetTip("Remove selected script from this entity");
    edit_script_btn.SetLabel("Edit");
    edit_script_btn.SetTip("Edit the content of selected script");

    script_type_option.Add("lua");
    script_type_option.Add("javascript");
    script_type_option.Add("expression");
    script_type_option.Add("python");
    script_type_option.SetIndex(0);  // Default to "lua"
    script_type_option.ToolTip("Script type/language");

    script_name_field.SetPrompt("Script Name");
    script_id_field.SetPrompt("Script ID");
    script_content_edit.SetLineCount(5);  // Show 5 lines of script content initially
    script_content_edit.SetTip("Script content");
    script_active_check.SetLabel("Active");
    script_active_check.SetTip("Whether this script should be executed");
    script_active_check.Set(true);  // Default to active
    apply_script_btn.SetLabel("Apply Script");
    apply_script_btn.SetTip("Apply changes to the selected script");
    
    // Connect events
    id_field.WhenAction = [this]() { OnEntityChanged(); };
    name_field.WhenAction = [this]() { OnEntityChanged(); };
    type_option.WhenAction = [this]() { OnEntityChanged(); };
    animation_slots_ctrl.WhenLeftClick = [this]() { OnSlotChanged(); };
    properties_ctrl.WhenLeftClick = [this]() { OnPropertyChanged(); };
    add_property_btn <<= [this]() { OnAddProperty(); };
    remove_property_btn <<= [this]() { OnRemoveProperty(); };
    add_slot_btn <<= [this]() { OnAddSlot(); };
    remove_slot_btn <<= [this]() { OnRemoveSlot(); };
    animation_dropdown.WhenAction = [this]() { OnSlotChanged(); };
    slot_name_field.WhenAction = [this]() { OnSlotChanged(); };
    play_btn <<= [this]() { OnPlayClicked(); };
    pause_btn <<= [this]() { OnPauseClicked(); };
    stop_btn <<= [this]() { OnStopClicked(); };
    loop_check.WhenAction = [this]() { OnLoopToggled(); };
    speed_slider.WhenAction = [this]() { OnSpeedChanged(); };
    blend_weight_slider.WhenAction = [this]() { OnBlendWeightChanged(); };
    transition_time_edit.WhenAction = [this]() { OnTransitionTimeChanged(); };
    apply_blend_btn <<= [this]() { OnApplyBlendClicked(); };
    event_list_ctrl.WhenLeftClick = [this]() { OnEventSelectionChanged(); };
    add_event_btn <<= [this]() { OnAddEventClicked(); };
    remove_event_btn <<= [this]() { OnRemoveEventClicked(); };
    edit_event_btn <<= [this]() { OnEditEventClicked(); };
    event_name_field.WhenAction = [this]() { OnEventFieldChanged(); };
    event_type_option.WhenAction = [this]() { OnEventFieldChanged(); };
    event_frame_field.WhenAction = [this]() { OnEventFieldChanged(); };
    
    transition_list_ctrl.WhenLeftClick = [this]() { OnTransitionSelectionChanged(); };
    add_transition_btn <<= [this]() { OnAddTransitionClicked(); };
    remove_transition_btn <<= [this]() { OnRemoveTransitionClicked(); };
    edit_transition_btn <<= [this]() { OnEditTransitionClicked(); };
    from_animation_option.WhenAction = [this]() { OnTransitionFieldChanged(); };
    to_animation_option.WhenAction = [this]() { OnTransitionFieldChanged(); };
    transition_time_field.WhenAction = [this]() { OnTransitionFieldChanged(); };
    condition_field.WhenAction = [this]() { OnTransitionFieldChanged(); };
    
    speed_multiplier_field.WhenAction = [this]() { OnAnimParamsChanged(); };
    time_offset_field.WhenAction = [this]() { OnAnimParamsChanged(); };
    looping_check.WhenAction = [this]() { OnAnimParamsChanged(); };
    apply_anim_params_btn <<= [this]() { OnApplyAnimParamsClicked(); };

    // Connect script management events
    script_list_ctrl.WhenLeftClick = [this]() { OnScriptSelectionChanged(); };
    add_script_btn <<= [this]() { OnAddScriptClicked(); };
    remove_script_btn <<= [this]() { OnRemoveScriptClicked(); };
    edit_script_btn <<= [this]() { OnEditScriptClicked(); };
    script_type_option.WhenAction = [this]() { OnScriptFieldChanged(); };
    script_name_field.WhenAction = [this]() { OnScriptFieldChanged(); };
    script_id_field.WhenAction = [this]() { OnScriptFieldChanged(); };
    script_content_edit.WhenAction = [this]() { OnScriptContentChanged(); };
    script_active_check.WhenAction = [this]() { OnScriptFieldChanged(); };
    apply_script_btn <<= [this]() { OnApplyScriptClicked(); };
}

EntityPropertiesCtrl::~EntityPropertiesCtrl() {
}

void EntityPropertiesCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
    // Update controls if we have an entity selected
    if (current_entity) {
        UpdateControls();
    }
}

void EntityPropertiesCtrl::SetEntity(const Entity* entity) {
    current_entity = const_cast<Entity*>(entity);
    UpdateControls();
}

void EntityPropertiesCtrl::SetChangeCallback(std::function<void()> callback) {
    change_callback = callback;
}

Entity EntityPropertiesCtrl::GetEntity() const {
    if (!current_entity) {
        return Entity();
    }

    // We need to update the entity with current values from controls
    Entity updated_entity = *current_entity;
    updated_entity.id = ~id_field;
    updated_entity.name = ~name_field;
    updated_entity.type = AsString(type_option.Get());

    // For animation slots and properties, we would need to populate them from the controls
    // This is a simplified implementation
    return updated_entity;
}

void EntityPropertiesCtrl::UpdateControls() {
    if (!current_entity) {
        // Clear all controls
        id_field.Clear();
        name_field.Clear();
        type_option <<= 0;
        animation_slots_ctrl.Clear();
        properties_ctrl.Clear();

        // Clear the preview
        preview_ctrl.SetProject(project);
        preview_ctrl.SetEntity(nullptr);
        preview_ctrl.SetAnimation(nullptr);

        // Clear and populate animation dropdown with available animations
        if (project) {
            animation_dropdown.Clear();
            animation_dropdown.Add("(No Animation)");
            for (const auto& anim : project->animations) {
                animation_dropdown.Add(anim.id);
            }
            
            // Also update the transition animation dropdowns
            from_animation_option.Clear();
            to_animation_option.Clear();
            for (const auto& anim : project->animations) {
                from_animation_option.Add(anim.id);
                to_animation_option.Add(anim.id);
            }
        }
        return;
    }

    // Update basic entity fields
    id_field = current_entity->id;
    name_field = current_entity->name;

    // Update type selection
    int type_idx = type_option.Find(current_entity->type);
    if (type_idx >= 0) {
        type_option <<= type_idx;
    } else {
        type_option <<= 0;  // Default to first option
    }

    // Clear and populate animation dropdown with available animations
    animation_dropdown.Clear();
    animation_dropdown.Add("(No Animation)");
    if (project) {
        for (const auto& anim : project->animations) {
            animation_dropdown.Add(anim.id);
        }
        
        // Also update the transition animation dropdowns
        from_animation_option.Clear();
        to_animation_option.Clear();
        for (const auto& anim : project->animations) {
            from_animation_option.Add(anim.id);
            to_animation_option.Add(anim.id);
        }
    }

    // Update animation slots
    animation_slots_ctrl.Clear();
    for (const auto& slot : current_entity->animation_slots) {
        animation_slots_ctrl.Add(slot.name, slot.animation_id);

        // If this is the currently selected slot in the array control,
        // update the dropdown to show the selected animation
        if (animation_slots_ctrl.GetCount() - 1 == animation_slots_ctrl.GetCursor()) {
            int anim_idx = animation_dropdown.Find(slot.animation_id);
            if (anim_idx >= 0) {
                animation_dropdown <<= anim_idx;
            } else {
                animation_dropdown <<= 0; // "(No Animation)"
            }
        }
    }

    // Update properties
    properties_ctrl.Clear();
    for (const auto& kv : current_entity->properties) {
        properties_ctrl.Add(kv.GetKey(), SConvert<ConvertTxt>::Single<ConvertTxt>(kv.GetValue()));
    }

    // Update transitions list
    transition_list_ctrl.Clear();
    for (const auto& trans : current_entity->animation_transitions) {
        transition_list_ctrl.Add(trans.from_animation_id, trans.to_animation_id, 
                                Format("%.2f", trans.transition_time));
    }

    // Update animation parameters fields
    speed_multiplier_field <<= current_entity->anim_params.speed_multiplier;
    time_offset_field <<= current_entity->anim_params.time_offset;
    looping_check.Set(current_entity->anim_params.is_looping);

    // Update the preview with the current entity and its first animation
    preview_ctrl.SetProject(project);
    preview_ctrl.SetEntity(current_entity);
    if (current_entity->animation_slots.GetCount() > 0) {
        const NamedAnimationSlot& slot = current_entity->animation_slots[0];
        if (project) {
            const Animation* anim = project->FindAnimation(slot.animation_id);
            preview_ctrl.SetAnimation(anim);
        }
    }

    // Update script list
    script_list_ctrl.Clear();
    for (const auto& script : current_entity->scripts) {
        script_list_ctrl.Add(script.id, script.name, script.type);
    }

    // Validate the entity after updating controls
    ValidateEntity();
}

void EntityPropertiesCtrl::OnEntityChanged() {
    if (current_entity && change_callback) {
        // Update the current entity with values from UI
        current_entity->id = ~id_field;
        current_entity->name = ~name_field;
        current_entity->type = AsString(type_option.Get());
        change_callback();
        ValidateEntity();
    }
}

void EntityPropertiesCtrl::OnSlotChanged() {
    // Animation slot selection changed - update the selected slot's animation
    if (!current_entity) return;

    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx >= 0 && selected_slot_idx < current_entity->animation_slots.GetCount()) {
        // Update the currently selected slot
        String selected_anim_id = AsString(animation_dropdown.Get());

        // Only update if it's different
        if (current_entity->animation_slots[selected_slot_idx].animation_id != selected_anim_id) {
            current_entity->animation_slots[selected_slot_idx].animation_id = selected_anim_id;
            if (change_callback) {
                change_callback();
            }
            ValidateEntity();
        }
        
        // Update blending controls to reflect current slot values
        const auto& slot = current_entity->animation_slots[selected_slot_idx];
        blend_weight_slider.SetPos((int)(slot.blend_params.weight * 100)); // Convert 0.0-1.0 to 0-100
        String weightStr = Format("%.2f", slot.blend_params.weight);
        blend_weight_label.SetLabel("Blend Weight: " + weightStr);
        transition_time_edit <<= slot.blend_params.transition_time;
        
        // Update events list to show events for this slot
        UpdateEventsList(selected_slot_idx);
    }
}

void EntityPropertiesCtrl::OnPropertyChanged() {
    // Property selection changed
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnAddSlot() {
    if (!current_entity) return;

    // Get the slot name from the field, default to "NewSlot" if empty
    String slot_name = ~slot_name_field;
    if (slot_name.IsEmpty()) {
        slot_name = "NewSlot";
    }

    // Add a new animation slot
    NamedAnimationSlot new_slot;
    new_slot.name = slot_name;
    new_slot.animation_id = "(No Animation)"; // Default to no animation

    current_entity->animation_slots.Add(new_slot);

    UpdateControls();
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnRemoveSlot() {
    if (!current_entity) return;

    int sel = animation_slots_ctrl.GetCursor();
    if (sel >= 0 && sel < current_entity->animation_slots.GetCount()) {
        current_entity->animation_slots.Remove(sel);
        UpdateControls();
        if (change_callback) {
            change_callback();
        }
    }
}

void EntityPropertiesCtrl::OnAddProperty() {
    if (!current_entity) return;

    // Add a new property with default values
    String name = ~property_name_field;
    if (name.IsEmpty()) {
        name = "NewProp";
    }

    // Check if a property with this name already exists to avoid duplicates
    if (current_entity->properties.Find(name)) {
        Exclamation("A property with name '" + name + "' already exists!");
        return;
    }

    // For now, we'll add a string value - in a full implementation, we'd have
    // type selection options
    current_entity->properties.Set(name, ~property_value_field);

    // Clear the input fields after adding the property
    property_name_field.Clear();
    property_value_field.Clear();

    UpdateControls();
    if (change_callback) {
        change_callback();
    }
    ValidateEntity();
}

void EntityPropertiesCtrl::OnRemoveProperty() {
    if (!current_entity) return;

    int sel = properties_ctrl.GetCursor();
    if (sel >= 0) {
        String key = properties_ctrl.Get(0, sel);
        current_entity->properties.Remove(key);
        UpdateControls();
        if (change_callback) {
            change_callback();
        }
    }
}

void EntityPropertiesCtrl::ValidateEntity() {
    if (!current_entity) {
        validation_status.SetLabel("Validation: No entity selected");
        validation_status.SetInk(Black());
        validation_status.SetPaper(LightGray());
        return;
    }

    if (!project) {
        validation_status.SetLabel("Validation: No project loaded");
        validation_status.SetInk(Black());
        validation_status.SetPaper(LightGray());
        return;
    }

    String errorMsg;
    if (ValidateEntity(*project, *current_entity, errorMsg)) {
        validation_status.SetLabel("Validation: OK");
        validation_status.SetInk(Green());
        validation_status.SetPaper(LightGreen());
    } else {
        validation_status.SetLabel("Validation: Error - " + errorMsg);
        validation_status.SetInk(Red());
        validation_status.SetPaper(LightYellow());
    }

    // Refresh the validation control
    validation_status.Refresh();
}

void EntityPropertiesCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), White());
}

void EntityPropertiesCtrl::Layout() {
    // Layout the properties control
    Size sz = GetSize();

    // Calculate heights for each section
    int validation_height = 30;
    int preview_height = 150;  // Height for the preview area
    int top_height = sz.cy - preview_height - validation_height;

    // Position the property controls in the top area
    id_field.SetRect(10, 10, sz.cx - 20, 24);
    name_field.SetRect(10, 40, sz.cx - 20, 24);

    // Position the type option
    type_option.SetRect(10, 70, sz.cx - 20, 24);

    // Position the animation slots controls
    int slot_y = 100;
    slot_name_field.SetRect(10, slot_y, 120, 24);
    animation_dropdown.SetRect(140, slot_y, sz.cx - 150, 24);
    add_slot_btn.SetRect(sz.cx - 60, slot_y, 24, 24);
    remove_slot_btn.SetRect(sz.cx - 30, slot_y, 24, 24);

    // Position the animation slots list
    animation_slots_ctrl.SetRect(10, slot_y + 30, sz.cx - 20, 80);

    // Position the properties controls
    int prop_y = slot_y + 115;
    property_name_field.SetRect(10, prop_y, 120, 24);
    property_value_field.SetRect(140, prop_y, sz.cx - 180, 24);
    add_property_btn.SetRect(sz.cx - 40, prop_y, 24, 24);
    remove_property_btn.SetRect(sz.cx - 10, prop_y, 24, 24);

    // Position the properties list
    properties_ctrl.SetRect(10, prop_y + 30, sz.cx - 20, 80);

    // Position animation playback controls (in the properties area)
    int controls_y = prop_y + 115;
    play_btn.SetRect(10, controls_y, 60, 24);
    pause_btn.SetRect(80, controls_y, 60, 24);
    stop_btn.SetRect(150, controls_y, 60, 24);
    loop_check.SetRect(220, controls_y, 60, 24);
    speed_label.SetRect(290, controls_y, 80, 24);
    speed_slider.SetRect(370, controls_y + 4, sz.cx - 380, 16);

    // Position blending controls (below the playback controls)
    int blend_y = controls_y + 30;
    blend_weight_label.SetRect(10, blend_y, 120, 24);
    blend_weight_slider.SetRect(140, blend_y + 4, 150, 16);
    transition_label.SetRect(300, blend_y, 120, 24);
    transition_time_edit.SetRect(420, blend_y, 60, 24);
    apply_blend_btn.SetRect(490, blend_y, 100, 24);

    // Position event management controls (below blending controls)
    int event_y = blend_y + 30;
    event_name_field.SetRect(10, event_y, 100, 24);
    event_type_option.SetRect(120, event_y, 80, 24);
    event_frame_field.SetRect(210, event_y, 60, 24);
    add_event_btn.SetRect(280, event_y, 24, 24);
    remove_event_btn.SetRect(310, event_y, 24, 24);
    edit_event_btn.SetRect(340, event_y, 50, 24);

    // Position the event list (below the event controls)
    int list_y = event_y + 30;
    event_list_ctrl.SetRect(10, list_y, sz.cx - 20, 80);
    
    // Position transition management controls (below the event list)
    int trans_y = list_y + 90; // 80 for event list + 10px spacing
    from_animation_option.SetRect(10, trans_y, 120, 24);
    to_animation_option.SetRect(140, trans_y, 120, 24);
    transition_time_field.SetRect(270, trans_y, 60, 24);
    add_transition_btn.SetRect(340, trans_y, 24, 24);
    remove_transition_btn.SetRect(370, trans_y, 24, 24);
    edit_transition_btn.SetRect(400, trans_y, 50, 24);
    
    // Position the transition list (below the transition controls)
    int trans_list_y = trans_y + 30;
    transition_list_ctrl.SetRect(10, trans_list_y, sz.cx - 20, 80);
    
    // Position entity animation parameters controls (below the transition list)
    int params_y = trans_list_y + 90; // 80 for transition list + 10px spacing
    Label speed_label, offset_label;
    speed_label.SetLabel("Speed:");
    offset_label.SetLabel("Offset:");
    speed_label.SetRect(10, params_y, 50, 24);
    speed_multiplier_field.SetRect(60, params_y, 80, 24);
    offset_label.SetRect(150, params_y, 50, 24);
    time_offset_field.SetRect(200, params_y, 80, 24);
    looping_check.SetRect(300, params_y, 100, 24);
    apply_anim_params_btn.SetRect(410, params_y, 100, 24);

    // Position script management controls (below animation parameters)
    int script_y = params_y + 30;
    script_id_field.SetRect(10, script_y, 100, 24);
    script_name_field.SetRect(120, script_y, 120, 24);
    script_type_option.SetRect(250, script_y, 80, 24);
    script_active_check.SetRect(340, script_y, 70, 24);
    add_script_btn.SetRect(420, script_y, 24, 24);
    remove_script_btn.SetRect(450, script_y, 24, 24);
    edit_script_btn.SetRect(480, script_y, 50, 24);

    // Position the preview control at the bottom (above validation)
    int preview_y = sz.cy - preview_height - validation_height;

    // Position the script list (below the script controls)
    int script_list_y = script_y + 30;
    script_list_ctrl.SetRect(10, script_list_y, sz.cx - 20, 80);

    // Position script content editor (below the script list)
    int script_content_y = script_list_y + 90;
    int script_content_height = (preview_y - script_content_y > 80) ? 80 : (preview_y - script_content_y - 10);
    if (script_content_height > 0) {
        script_content_edit.SetRect(10, script_content_y, sz.cx - 20, script_content_height);
        apply_script_btn.SetRect(sz.cx - 100, script_content_y - 25, 90, 24); // Position above the editor
    }

    // Position the preview control at the bottom (above validation)
    preview_ctrl.SetRect(0, preview_y, sz.cx, preview_height);

    // Position the validation status at the very bottom
    validation_status.SetRect(0, sz.cy - validation_height, sz.cx, validation_height);
}

void EntityPropertiesCtrl::OnPlayClicked() {
    if (preview_ctrl.GetAnimation()) {
        preview_ctrl.StartAnimation();
    } else {
        Exclamation("No animation selected to play!");
    }
}

void EntityPropertiesCtrl::OnPauseClicked() {
    preview_ctrl.PauseAnimation();
}

void EntityPropertiesCtrl::OnStopClicked() {
    preview_ctrl.StopAnimation();
}

void EntityPropertiesCtrl::OnLoopToggled() {
    preview_ctrl.SetLoopEnabled(loop_check);
}

void EntityPropertiesCtrl::OnSpeedChanged() {
    double speed = speed_slider.GetValue() / 100.0;  // Convert 1-500 range to 0.01-5.0
    String speedText = Format("%.2f", speed) + "x";
    speed_label.SetLabel("Speed: " + speedText);
    preview_ctrl.SetPlaybackSpeed(speed);
    Refresh(); // Refresh the control to update the label
}

void EntityPropertiesCtrl::OnBlendWeightChanged() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx >= 0 && selected_slot_idx < current_entity->animation_slots.GetCount()) {
        double weight = blend_weight_slider.GetPos() / 100.0; // Convert 0-100 to 0.0-1.0
        current_entity->animation_slots[selected_slot_idx].blend_params.weight = weight;
        
        String weightStr = Format("%.2f", weight);
        blend_weight_label.SetLabel("Blend Weight: " + weightStr);
        
        if (change_callback) {
            change_callback();
        }
    }
}

void EntityPropertiesCtrl::OnTransitionTimeChanged() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx >= 0 && selected_slot_idx < current_entity->animation_slots.GetCount()) {
        double transition_time = ~transition_time_edit;
        current_entity->animation_slots[selected_slot_idx].blend_params.transition_time = transition_time;
        
        if (change_callback) {
            change_callback();
        }
    }
}

void EntityPropertiesCtrl::OnApplyBlendClicked() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx >= 0 && selected_slot_idx < current_entity->animation_slots.GetCount()) {
        // Update the slot with current UI values
        double weight = blend_weight_slider.GetPos() / 100.0; // Convert 0-100 to 0.0-1.0
        double transition_time = ~transition_time_edit;
        
        current_entity->animation_slots[selected_slot_idx].blend_params.weight = weight;
        current_entity->animation_slots[selected_slot_idx].blend_params.transition_time = transition_time;
        
        // Update the UI elements to reflect the changes
        String weightStr = Format("%.2f", weight);
        blend_weight_label.SetLabel("Blend Weight: " + weightStr);
        
        if (change_callback) {
            change_callback();
        }
        
        // Show confirmation
        PromptOK("Blend parameters applied to animation slot '" + 
                 current_entity->animation_slots[selected_slot_idx].name + "'");
    } else {
        Exclamation("Please select an animation slot first!");
    }
}

void EntityPropertiesCtrl::UpdateEventsList(int slotIndex) {
    if (!current_entity || slotIndex < 0 || slotIndex >= current_entity->animation_slots.GetCount()) {
        event_list_ctrl.Clear();
        return;
    }
    
    const auto& slot = current_entity->animation_slots[slotIndex];
    event_list_ctrl.Clear();
    
    for (const auto& event : slot.blend_params.events) {
        event_list_ctrl.Add(event.name, event.type, IntStr(event.frame_index));
    }
}

void EntityPropertiesCtrl::OnEventSelectionChanged() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx < 0 || selected_slot_idx >= current_entity->animation_slots.GetCount()) {
        return;
    }
    
    int selected_event_idx = event_list_ctrl.GetCursor();
    if (selected_event_idx < 0 || selected_event_idx >= current_entity->animation_slots[selected_slot_idx].blend_params.events.GetCount()) {
        return;
    }
    
    const auto& event = current_entity->animation_slots[selected_slot_idx].blend_params.events[selected_event_idx];
    
    // Update the event editing fields to show the selected event's properties
    event_name_field = event.name;
    int type_idx = event_type_option.Find(event.type);
    if (type_idx >= 0) {
        event_type_option <<= type_idx;
    }
    event_frame_field <<= event.frame_index;
}

void EntityPropertiesCtrl::OnEventFieldChanged() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx < 0 || selected_slot_idx >= current_entity->animation_slots.GetCount()) {
        return;
    }
    
    int selected_event_idx = event_list_ctrl.GetCursor();
    if (selected_event_idx < 0 || selected_event_idx >= current_entity->animation_slots[selected_slot_idx].blend_params.events.GetCount()) {
        return;
    }
    
    // Update the selected event with values from the UI fields
    auto& event = current_entity->animation_slots[selected_slot_idx].blend_params.events[selected_event_idx];
    event.name = ~event_name_field;
    event.type = AsString(event_type_option.Get());
    event.frame_index = ~event_frame_field;
    
    // Refresh the events list to show updated values
    UpdateEventsList(selected_slot_idx);
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnAddEventClicked() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx < 0 || selected_slot_idx >= current_entity->animation_slots.GetCount()) {
        Exclamation("Please select an animation slot first!");
        return;
    }
    
    // Create a new event with default values
    AnimationEvent new_event;
    new_event.id = "event_" + Uuid().ToString();
    new_event.name = "New Event";
    new_event.type = AsString(event_type_option.Get());
    new_event.frame_index = ~event_frame_field;
    
    // Add to the selected slot's events
    current_entity->animation_slots[selected_slot_idx].blend_params.events.Add(new_event);
    
    // Refresh the events list
    UpdateEventsList(selected_slot_idx);
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnRemoveEventClicked() {
    if (!current_entity) return;
    
    int selected_slot_idx = animation_slots_ctrl.GetCursor();
    if (selected_slot_idx < 0 || selected_slot_idx >= current_entity->animation_slots.GetCount()) {
        Exclamation("Please select an animation slot first!");
        return;
    }
    
    int selected_event_idx = event_list_ctrl.GetCursor();
    if (selected_event_idx < 0 || selected_event_idx >= current_entity->animation_slots[selected_slot_idx].blend_params.events.GetCount()) {
        Exclamation("Please select an event to remove!");
        return;
    }
    
    // Remove the selected event
    current_entity->animation_slots[selected_slot_idx].blend_params.events.Remove(selected_event_idx);
    
    // Refresh the events list
    UpdateEventsList(selected_slot_idx);
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnEditEventClicked() {
    // This would open a dialog to edit event parameters in more detail
    // For now, we'll just show a simple message
    PromptOK("Event editing functionality would open a detailed editor dialog here.");
}

void EntityPropertiesCtrl::OnTransitionSelectionChanged() {
    if (!current_entity) return;
    
    int selected_transition_idx = transition_list_ctrl.GetCursor();
    if (selected_transition_idx < 0 || selected_transition_idx >= current_entity->animation_transitions.GetCount()) {
        return;
    }
    
    const auto& trans = current_entity->animation_transitions[selected_transition_idx];
    
    // Update the transition editing fields to show the selected transition's properties
    int from_idx = from_animation_option.Find(trans.from_animation_id);
    if (from_idx >= 0) {
        from_animation_option <<= from_idx;
    }
    int to_idx = to_animation_option.Find(trans.to_animation_id);
    if (to_idx >= 0) {
        to_animation_option <<= to_idx;
    }
    transition_time_field <<= trans.transition_time;
    condition_field = trans.condition;
}

void EntityPropertiesCtrl::OnTransitionFieldChanged() {
    if (!current_entity) return;
    
    int selected_transition_idx = transition_list_ctrl.GetCursor();
    if (selected_transition_idx < 0 || selected_transition_idx >= current_entity->animation_transitions.GetCount()) {
        return;
    }
    
    // Update the selected transition with values from the UI fields
    auto& trans = current_entity->animation_transitions[selected_transition_idx];
    trans.from_animation_id = AsString(from_animation_option.Get());
    trans.to_animation_id = AsString(to_animation_option.Get());
    trans.transition_time = ~transition_time_field;
    trans.condition = ~condition_field;
    
    // Refresh the transitions list to show updated values
    UpdateControls(); // Full update since it affects multiple UI elements
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnAddTransitionClicked() {
    if (!current_entity) return;
    
    // Create a new transition with default values
    AnimationTransition new_trans;
    new_trans.from_animation_id = AsString(from_animation_option.Get());
    new_trans.to_animation_id = AsString(to_animation_option.Get());
    new_trans.transition_time = ~transition_time_field;
    new_trans.condition = ~condition_field;
    
    // Add to the entity's transitions
    current_entity->animation_transitions.Add(new_trans);
    
    // Refresh the transitions list
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnRemoveTransitionClicked() {
    if (!current_entity) return;
    
    int selected_transition_idx = transition_list_ctrl.GetCursor();
    if (selected_transition_idx < 0 || selected_transition_idx >= current_entity->animation_transitions.GetCount()) {
        Exclamation("Please select a transition to remove!");
        return;
    }
    
    // Remove the selected transition
    current_entity->animation_transitions.Remove(selected_transition_idx);
    
    // Refresh the transitions list
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnEditTransitionClicked() {
    // This would open a dialog to edit transition parameters in more detail
    // For now, we'll just show a simple message
    PromptOK("Transition editing functionality would open a detailed editor dialog here.");
}

void EntityPropertiesCtrl::OnAnimParamsChanged() {
    if (!current_entity) return;
    
    // Update the current entity's animation parameters with values from the UI fields
    current_entity->anim_params.speed_multiplier = ~speed_multiplier_field;
    current_entity->anim_params.time_offset = ~time_offset_field;
    current_entity->anim_params.is_looping = looping_check;
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnApplyAnimParamsClicked() {
    if (!current_entity) return;
    
    // Update the current entity's animation parameters with values from the UI fields
    current_entity->anim_params.speed_multiplier = ~speed_multiplier_field;
    current_entity->anim_params.time_offset = ~time_offset_field;
    current_entity->anim_params.is_looping = looping_check;
    
    if (change_callback) {
        change_callback();
    }
    
    // Show confirmation
    PromptOK("Animation parameters applied to entity '" + current_entity->name + "'");
}

void EntityPropertiesCtrl::OnScriptSelectionChanged() {
    if (!current_entity) return;
    
    int selected_script_idx = script_list_ctrl.GetCursor();
    if (selected_script_idx < 0 || selected_script_idx >= current_entity->scripts.GetCount()) {
        return;
    }
    
    const auto& script = current_entity->scripts[selected_script_idx];
    
    // Update the script editing fields to show the selected script's properties
    script_id_field = script.id;
    script_name_field = script.name;
    int type_idx = script_type_option.Find(script.type);
    if (type_idx >= 0) {
        script_type_option <<= type_idx;
    }
    script_content_edit = script.content;
    script_active_check.Set(script.is_active);
}

void EntityPropertiesCtrl::OnScriptFieldChanged() {
    if (!current_entity) return;
    
    int selected_script_idx = script_list_ctrl.GetCursor();
    if (selected_script_idx < 0 || selected_script_idx >= current_entity->scripts.GetCount()) {
        return;
    }
    
    // Update the selected script with values from the UI fields (except content)
    auto& script = current_entity->scripts[selected_script_idx];
    script.id = ~script_id_field;
    script.name = ~script_name_field;
    script.type = AsString(script_type_option.Get());
    script.is_active = script_active_check;
    
    // Refresh the scripts list to show updated values
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnScriptContentChanged() {
    if (!current_entity) return;
    
    int selected_script_idx = script_list_ctrl.GetCursor();
    if (selected_script_idx < 0 || selected_script_idx >= current_entity->scripts.GetCount()) {
        return;
    }
    
    // Update the selected script's content
    current_entity->scripts[selected_script_idx].content = script_content_edit.GetData();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnAddScriptClicked() {
    if (!current_entity) return;
    
    // Create a new script with default values
    EntityScript new_script;
    new_script.id = "script_" + Uuid().ToString().Left(8);
    new_script.name = "New Script";
    new_script.type = AsString(script_type_option.Get());
    new_script.content = "-- New script content";
    new_script.is_active = script_active_check;
    
    // Add to the entity's scripts
    current_entity->scripts.Add(new_script);
    
    // Refresh the scripts list
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnRemoveScriptClicked() {
    if (!current_entity) return;
    
    int selected_script_idx = script_list_ctrl.GetCursor();
    if (selected_script_idx < 0 || selected_script_idx >= current_entity->scripts.GetCount()) {
        Exclamation("Please select a script to remove!");
        return;
    }
    
    // Remove the selected script
    current_entity->scripts.Remove(selected_script_idx);
    
    // Refresh the scripts list
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
}

void EntityPropertiesCtrl::OnEditScriptClicked() {
    // This would open a more advanced script editor dialog
    // For now, we'll just show a simple message
    PromptOK("Script editing functionality would open a dedicated script editor dialog here.");
}

void EntityPropertiesCtrl::OnApplyScriptClicked() {
    if (!current_entity) return;
    
    int selected_script_idx = script_list_ctrl.GetCursor();
    if (selected_script_idx < 0 || selected_script_idx >= current_entity->scripts.GetCount()) {
        Exclamation("Please select a script to apply changes to!");
        return;
    }
    
    // Update the selected script with values from the UI fields
    auto& script = current_entity->scripts[selected_script_idx];
    script.id = ~script_id_field;
    script.name = ~script_name_field;
    script.type = AsString(script_type_option.Get());
    script.content = script_content_edit.GetData();
    script.is_active = script_active_check;
    
    // Refresh the scripts list to reflect changes
    UpdateControls();
    
    if (change_callback) {
        change_callback();
    }
    
    // Show confirmation
    PromptOK("Script applied to entity '" + current_entity->name + "'");
}