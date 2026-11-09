#ifndef _AnimEdit_EntityPropertiesCtrl_h_
#define _AnimEdit_EntityPropertiesCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>
#include "EntityPreviewCtrl.h"

using namespace Upp;

class EntityPropertiesCtrl : public Ctrl {
public:
    typedef EntityPropertiesCtrl CLASSNAME;

    EntityPropertiesCtrl();
    virtual ~EntityPropertiesCtrl();

    void SetProject(const AnimationProject* project);
    void SetEntity(const Entity* entity);
    void SetChangeCallback(std::function<void()> callback);

    // Get the current entity values (for saving back to project)
    Entity GetEntity() const;

private:
    const AnimationProject* project;
    Entity* current_entity;  // Not owned, just a reference to the one in project
    std::function<void()> change_callback;
    
    // Controls for entity properties
    EditField id_field;
    EditField name_field;
    Option type_option;
    ArrayCtrl animation_slots_ctrl;
    Option animation_dropdown;  // For selecting available animations
    EditField slot_name_field;  // For naming the slot
    EditField property_name_field;
    EditField property_value_field;
    Button add_property_btn;
    Button remove_property_btn;
    ArrayCtrl properties_ctrl;
    Button add_slot_btn;
    Button remove_slot_btn;
    
    // Blending controls
    ScrollBar blend_weight_slider;  // U++ uses ScrollBar for slider functionality
    Label blend_weight_label;
    EditDouble transition_time_edit;  // For numeric input
    Label transition_label;
    Button apply_blend_btn;
    
    // Event management controls
    ArrayCtrl event_list_ctrl;
    Button add_event_btn;
    Button remove_event_btn;
    Button edit_event_btn;
    EditField event_name_field;
    Option event_type_option;
    EditInt event_frame_field;  // For integer input
    
    // Transition management controls
    ArrayCtrl transition_list_ctrl;
    Option from_animation_option;
    Option to_animation_option;
    EditDouble transition_time_field;  // For numeric input
    EditField condition_field;
    Button add_transition_btn;
    Button remove_transition_btn;
    Button edit_transition_btn;
    
    // Entity animation parameters controls
    EditDouble speed_multiplier_field;  // For numeric input
    EditDouble time_offset_field;       // For numeric input
    Option looping_check;
    Button apply_anim_params_btn;
    
    // Script management controls
    ArrayCtrl script_list_ctrl;
    Button add_script_btn;
    Button remove_script_btn;
    Button edit_script_btn;
    Option script_type_option;
    EditField script_name_field;
    EditField script_id_field;
    EditField script_content_edit;
    Option script_active_check;
    Button apply_script_btn;
    
    // Validation feedback
    StaticRect validation_status;  // Shows validation status
    
    // Preview control
    EntityPreviewCtrl preview_ctrl;
    
    // Animation playback controls
    Button play_btn;
    Button pause_btn;
    Button stop_btn;
    Option loop_check;
    ScrollBar speed_slider;  // U++ uses ScrollBar for slider functionality
    Label speed_label;

    void UpdateControls();
    void OnEntityChanged();
    void OnSlotChanged();
    void OnPropertyChanged();
    void OnAddSlot();
    void OnRemoveSlot();
    void OnAddProperty();
    void OnRemoveProperty();
    void ValidateEntity();
    void OnPlayClicked();
    void OnPauseClicked();
    void OnStopClicked();
    void OnLoopToggled();
    void OnSpeedChanged();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
};

#endif