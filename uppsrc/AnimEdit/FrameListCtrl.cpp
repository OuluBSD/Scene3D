#include "FrameListCtrl.h"
#include <Core/Core.h>

FrameListCtrl::FrameListCtrl()
    : project(nullptr)
    , animation(nullptr)
    , selected_index(-1)
    , is_dragging(false)
    , sort_type(SORT_BY_NAME)  // Default sort by name
{
    AddFrame(WhiteFrame());
}

FrameListCtrl::~FrameListCtrl() {
}

void FrameListCtrl::SetProject(const AnimationProject* project) {
    this->project = project;
    RefreshList();
}

void FrameListCtrl::SetAnimation(const Animation* animation) {
    this->animation = animation;
}

void FrameListCtrl::RefreshList() {
    ApplyFilters();
    Refresh();
}

void FrameListCtrl::SetFilterText(const String& text) {
    filter_text = text;
    ApplyFilters();
    Refresh();
}

void FrameListCtrl::ApplyFilters() {
    display_indices.Clear();

    if (!project) return;

    for (int i = 0; i < project->frames.GetCount(); i++) {
        const AnimationFrame& frame = project->frames[i];

        // Apply text filter
        if (!filter_text.IsEmpty()) {
            String search_text = ToLower(filter_text);
            String frame_id_lower = ToLower(frame.id);
            String frame_name_lower = ToLower(frame.name);

            if (frame_id_lower.Find(search_text) < 0 &&
                frame_name_lower.Find(search_text) < 0) {
                continue;
            }
        }

        display_indices.Add(i);
    }

    // Sort the filtered results
    SortFrames();
}

int FrameListCtrl::HitTest(Point pos) const {
    if (!project) return -1;

    int y = 0;
    int item_height = 40; // Fixed height for each item

    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        if (item_rect.Contains(pos)) {
            return i; // Return the display index, not the project index
        }
        y += item_height;
    }

    return -1;
}

void FrameListCtrl::DrawItem(Draw& w, int display_index, const Rect& rc) const {
    if (!project || display_index < 0 || display_index >= display_indices.GetCount()) {
        return;
    }

    int project_index = display_indices[display_index];
    const AnimationFrame& frame = project->frames[project_index];

    // Draw background
    Color bg_color = (display_index == selected_index) ? LtBlue() : (display_index % 2 == 0 ? White() : SdkLightGray());
    w.DrawRect(rc, bg_color);

    // Check if this frame is used in the current animation
    bool in_current_animation = false;
    if (animation) {
        for (int i = 0; i < animation->frames.GetCount(); i++) {
            if (animation->frames[i].frame_id == frame.id) {
                in_current_animation = true;
                break;
            }
        }
    }

    // Draw indicator if in current animation
    if (in_current_animation) {
        w.DrawRect(rc.left + 2, rc.top + 2, 4, 4, Green());
    }

    // Draw frame name (as main identifier)
    String displayName = !frame.name.IsEmpty() ? frame.name : frame.id; // Use name if available, otherwise id
    w.DrawText(rc.left + 8, rc.top + 3, displayName, Arial(12), Black());

    // Draw frame ID (as secondary info)
    w.DrawText(rc.left + 8, rc.top + 18, "ID: " + frame.id, Arial(10), Gray());

    // Draw number of sprites in the frame
    String sprite_count = "Sprites: " + IntStr(frame.sprites.GetCount());
    w.DrawText(rc.left + 8, rc.top + 30, sprite_count, Arial(10), DarkGray());
}

Size FrameListCtrl::GetItemSize(int index) const {
    return Size(GetSize().cx, 40); // Fixed height
}

void FrameListCtrl::Paint(Draw& w) {
    if (!project) {
        w.DrawRect(GetSize(), White());
        return;
    }

    w.DrawRect(GetSize(), White());

    int y = 0;
    int item_height = 40;

    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        DrawItem(w, i, item_rect);
        y += item_height;
    }
}

void FrameListCtrl::MouseDown(Point pos, dword button) {
    int item_index = HitTest(pos);

    if (item_index >= 0) {
        selected_index = item_index;
        Refresh();

        // Call the selection callback
        if (select_callback) {
            const AnimationFrame* selected_frame = GetSelectedFrame();
            select_callback(selected_frame);
        }

        // Start dragging if left button
        if (button == LEFT && selected_frame) {
            is_dragging = true;
            drag_start = pos;

            // Start drag operation
            String drag_data = selected_frame->id;
            ClipbdAction ca = DragAndDrop(this, drag_data, Image::Arrow());
        }
    } else {
        selected_index = -1;
        Refresh();

        if (select_callback) {
            select_callback(nullptr);
        }
    }

    Ctrl::MouseDown(pos, button);
}

void FrameListCtrl::MouseMove(Point pos, dword keyflags) {
    if (is_dragging && (pos - drag_start).GetLength() > 5) { // Threshold to start drag
        if (selected_index >= 0 && project && selected_index < display_indices.GetCount()) {
            int project_index = display_indices[selected_index];
            const AnimationFrame& frame = project->frames[project_index];

            String drag_data = frame.id;
            ClipbdAction ca = DragAndDrop(this, drag_data, Image::Arrow());
            is_dragging = false; // Reset dragging state after starting drag
        }
    }

    Ctrl::MouseMove(pos, keyflags);
}

void FrameListCtrl::RightDown(Point pos, dword flags) {
    int item_index = HitTest(pos);

    if (item_index >= 0) {
        // Select the item if it's not already selected
        if (selected_index != item_index) {
            selected_index = item_index;
            Refresh();
        }

        // Show context menu
        PopupContextMenu(pos);
    } else {
        // Deselect if clicking on empty space
        selected_index = -1;
        Refresh();
    }

    Ctrl::RightDown(pos, flags);
}

void FrameListCtrl::PopupContextMenu(Point pos) {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return;
    }

    int project_index = display_indices[selected_index];
    const AnimationFrame& frame = project->frames[project_index];

    // Create context menu
    PopupWindow popup;
    popup.AddFrame(BlackFrame());

    // Create a menu layout
    WithTextCtrlLayout<ParentCtrl> content;
    content.Ctrl::SizeHint([this]() { return Size(150, 150); });

    // Create menu items
    Button edit_btn, duplicate_btn, delete_btn, rename_btn, cancel_btn;

    edit_btn.SetLabel("Edit Frame");
    rename_btn.SetLabel("Rename Frame");
    duplicate_btn.SetLabel("Duplicate Frame");
    delete_btn.SetLabel("Delete Frame");
    cancel_btn.SetLabel("Cancel");

    content.Add(edit_btn.TopPos(0, 24).HSizePos());
    content.Add(rename_btn.TopPos(28, 24).HSizePos());
    content.Add(duplicate_btn.TopPos(56, 24).HSizePos());
    content.Add(delete_btn.TopPos(84, 24).HSizePos());
    content.Add(cancel_btn.TopPos(112, 24).HSizePos());

    // Define actions
    edit_btn <<= [this, project_index, &popup]() {
        // Implement edit dialog for the selected frame
        if (!project || project_index >= project->frames.GetCount()) {
            popup.Break();
            return;
        }

        AnimationFrame& frame = project->frames[project_index];

        // Create dialog content using simple layout
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(400, 200); });

        // Create input fields
        EditField id_field, name_field, description_field;
        EditDouble default_duration_field;
        Button ok_btn, cancel_btn;

        // Initialize with current values
        id_field = frame.id;
        name_field = frame.name;
        default_duration_field <<= frame.default_duration;
        description_field = frame.description;

        // Add controls with positioning
        dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(name_field.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(default_duration_field.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(description_field.HSizePos(80, 50).TopPos(80, 60));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Labels
        Label id_label, name_label, duration_label, description_label;
        id_label.SetLabel("ID:");
        name_label.SetLabel("Name:");
        duration_label.SetLabel("Default Duration:");
        description_label.SetLabel("Description:");

        dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(name_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(duration_label.LeftPos(8, 80).TopPos(56, 20));
        dlg.Add(description_label.LeftPos(8, 60).TopPos(80, 20));

        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Edit Frame");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK) {
            // Update the frame with new values
            String id = ~id_field;
            String name = ~name_field;
            double duration = ~default_duration_field;
            String description_text = ~description_field;

            // Validate inputs
            if (id.IsEmpty()) {
                Exclamation("Frame ID cannot be empty!");
                popup.Break();
                return;
            }

            // Check if ID is being changed and if new ID already exists
            if (id != frame.id && project->FindFrame(id)) {
                Exclamation("A frame with ID '" + id + "' already exists!");
                popup.Break();
                return;
            }

            // Update frame properties
            frame.id = id;
            frame.name = name.IsEmpty() ? id : name;
            frame.default_duration = duration;
            frame.description = description_text;

            // Update the IDs of any AnimationFrameRef in animations that point to this frame
            for (int i = 0; i < project->animations.GetCount(); i++) {
                Animation& anim = project->animations[i];
                for (int j = 0; j < anim.frames.GetCount(); j++) {
                    if (anim.frames[j].frame_id == frame.id) {
                        anim.frames[j].frame_id = id; // Update frame ID in the animation
                    }
                }
            }

            // Refresh the list to show the updated frame
            RefreshList();
        }

        popup.Break();
    };

    rename_btn <<= [this, project_index, &popup, &frame]() {
        if (!project) return;

        String newName = Prompt("Rename Frame", "Enter new name:", frame.name);
        if (!newName.IsEmpty()) {
            // Update the frame name
            project->frames[project_index].name = newName;
            RefreshList();
        }
        
        popup.Break();
    };

    duplicate_btn <<= [this, project_index, &popup]() {
        if (!project) return;

        // Create a duplicate of the selected frame
        AnimationFrame duplicated_frame = project->frames[project_index];

        // Generate a unique ID for the duplicate
        duplicated_frame.id = GenerateUniqueIdForFrame(duplicated_frame.id);
        duplicated_frame.name = duplicated_frame.name + "_copy";

        // Add the duplicated frame to the project
        project->frames.Add(duplicated_frame);

        // Refresh the list to show the new frame
        RefreshList();

        popup.Break();
    };

    delete_btn <<= [this, project_index, &popup, &frame]() {
        if (!project) return;

        // Confirm deletion
        if (PromptYesNo("Are you sure you want to delete frame '" + frame.id + "'?\n\n" +
                        "WARNING: This frame might be used by one or more animations.")) {
            // Check if this frame is used in any animations
            Vector<String> animations_using_frame;
            for (int i = 0; i < project->animations.GetCount(); i++) {
                const Animation& anim = project->animations[i];
                for (int j = 0; j < anim.frames.GetCount(); j++) {
                    if (anim.frames[j].frame_id == frame.id) {
                        if (animations_using_frame.Find(anim.id) == -1) {
                            animations_using_frame.Add(anim.id);
                        }
                    }
                }
            }

            if (animations_using_frame.GetCount() > 0) {
                String warning = "This frame is used in " + IntStr(animations_using_frame.GetCount()) +
                                " animation(s). Deleting it will remove all references from those animations.\n\n" +
                                "Affected animations: ";
                for (int i = 0; i < animations_using_frame.GetCount(); i++) {
                    warning += animations_using_frame[i];
                    if (i < animations_using_frame.GetCount() - 1) warning += ", ";
                }
                if (!PromptYesNo(warning + "\n\nAre you sure you want to continue?")) {
                    popup.Break();
                    return;
                }

                // Remove frame references from all animations that use it
                for (int i = 0; i < project->animations.GetCount(); i++) {
                    Animation& anim = project->animations[i];
                    for (int j = anim.frames.GetCount() - 1; j >= 0; j--) {
                        if (anim.frames[j].frame_id == frame.id) {
                            anim.frames.Remove(j);
                        }
                    }
                }
            }

            // Remove the frame from the project
            project->frames.Remove(project_index);

            // Refresh the list
            RefreshList();
        }
        popup.Break();
    };

    cancel_btn <<= [&popup]() {
        popup.Break();
    };

    // Set up the popup window
    popup.Add(content.SizePos());
    popup.SetRect(pos.x, pos.y, 150, 180);
    popup.Run();
}

String FrameListCtrl::GenerateUniqueIdForFrame(const String& baseId) {
    if (!project) return baseId + "_1";

    // Extract the base name without suffix number
    String base = baseId;
    int suffix_num = 1;

    // If the ID already has a number suffix, separate them
    int underscore_pos = baseId.ReverseFind('_');
    if (underscore_pos != -1) {
        String suffix = baseId.Mid(underscore_pos + 1);
        if (IsDigit(suffix[0])) {
            base = baseId.Mid(0, underscore_pos);
            suffix_num = atoi(AsString(suffix));
        }
    }

    // Find an available number
    String newId;
    do {
        newId = base + "_" + IntStr(++suffix_num);
    } while (project->FindFrame(newId) != nullptr);

    return newId;
}

const AnimationFrame* FrameListCtrl::GetSelectedFrame() const {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return nullptr;
    }

    int project_index = display_indices[selected_index];
    return &project->frames[project_index];
}

void FrameListCtrl::SetSortType(SortType sortType) {
    sort_type = sortType;
    ApplyFilters(); // Reapply filters which will cause resorting
}

void FrameListCtrl::SortFrames() {
    switch (sort_type) {
        case SORT_BY_NAME:
            SortBy([](const AnimationFrame& a, const AnimationFrame& b) {
                return ToLower(a.name) < ToLower(b.name);
            });
            break;
        case SORT_BY_ID:
            SortBy([](const AnimationFrame& a, const AnimationFrame& b) {
                return ToLower(a.id) < ToLower(b.id);
            });
            break;
        case SORT_BY_RECENT_USE:
        default:
            // For now, just maintain original order
            // In a real implementation, we'd sort by last used timestamp
            break;
    }
}

template<typename CompareFunc>
void FrameListCtrl::SortBy(CompareFunc compareFunc) {
    // Sort the display_indices based on the comparison function
    for (int i = 0; i < display_indices.GetCount() - 1; i++) {
        for (int j = i + 1; j < display_indices.GetCount(); j++) {
            int idx1 = display_indices[i];
            int idx2 = display_indices[j];

            if (compareFunc(project->frames[idx1], project->frames[idx2])) {
                // Already in correct order
            } else {
                // Swap to sort in ascending order
                Upp::Swap(display_indices[i], display_indices[j]);
            }
        }
    }
}