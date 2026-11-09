#include "AnimListCtrl.h"
#include <Core/Core.h>

AnimListCtrl::AnimListCtrl()
    : project(nullptr)
    , selected_index(-1)
    , is_dragging(false)
    , sort_type(SORT_BY_NAME)  // Default sort by name
{
    AddFrame(WhiteFrame());
}

AnimListCtrl::~AnimListCtrl() {
}

void AnimListCtrl::SetProject(const AnimationProject* project) {
    this->project = project;
    RefreshList();
}

void AnimListCtrl::RefreshList() {
    ApplyFilters();
    Refresh();
}

void AnimListCtrl::SetFilterText(const String& text) {
    filter_text = text;
    ApplyFilters();
    Refresh();
}

void AnimListCtrl::ApplyFilters() {
    display_indices.Clear();

    if (!project) return;

    for (int i = 0; i < project->animations.GetCount(); i++) {
        const Animation& anim = project->animations[i];

        // Apply text filter
        if (!filter_text.IsEmpty()) {
            String search_text = ToLower(filter_text);
            String anim_id_lower = ToLower(anim.id);
            String anim_name_lower = ToLower(anim.name);

            if (anim_id_lower.Find(search_text) < 0 &&
                anim_name_lower.Find(search_text) < 0) {
                continue;
            }
        }

        display_indices.Add(i);
    }

    // Sort the filtered results
    SortAnimations();
}

int AnimListCtrl::HitTest(Point pos) const {
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

void AnimListCtrl::DrawItem(Draw& w, int display_index, const Rect& rc) const {
    if (!project || display_index < 0 || display_index >= display_indices.GetCount()) {
        return;
    }

    int project_index = display_indices[display_index];
    const Animation& anim = project->animations[project_index];

    // Draw background
    Color bg_color = (display_index == selected_index) ? LtBlue() : (display_index % 2 == 0 ? White() : SdkLightGray());
    w.DrawRect(rc, bg_color);

    // Draw animation name (as main identifier)
    String displayName = !anim.name.IsEmpty() ? anim.name : anim.id; // Use name if available, otherwise id
    w.DrawText(rc.left + 8, rc.top + 3, displayName, Arial(12), Black());

    // Draw animation ID (as secondary info)
    w.DrawText(rc.left + 8, rc.top + 18, "ID: " + anim.id, Arial(10), Gray());

    // Draw number of frames in the animation
    String frame_count = "Frames: " + IntStr(anim.frames.GetCount());
    w.DrawText(rc.left + 8, rc.top + 30, frame_count, Arial(10), DarkGray());
}

Size AnimListCtrl::GetItemSize(int index) const {
    return Size(GetSize().cx, 40); // Fixed height
}

void AnimListCtrl::Paint(Draw& w) {
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

void AnimListCtrl::MouseDown(Point pos, dword button) {
    int item_index = HitTest(pos);

    if (item_index >= 0) {
        selected_index = item_index;
        Refresh();

        // Call the selection callback
        if (select_callback) {
            const Animation* selected_anim = GetSelectedAnimation();
            select_callback(selected_anim);
        }

        // Start dragging if left button
        if (button == LEFT && selected_anim) {
            is_dragging = true;
            drag_start = pos;

            // Start drag operation
            String drag_data = selected_anim->id;
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

void AnimListCtrl::MouseMove(Point pos, dword keyflags) {
    if (is_dragging && (pos - drag_start).GetLength() > 5) { // Threshold to start drag
        if (selected_index >= 0 && project && selected_index < display_indices.GetCount()) {
            int project_index = display_indices[selected_index];
            const Animation& anim = project->animations[project_index];

            String drag_data = anim.id;
            ClipbdAction ca = DragAndDrop(this, drag_data, Image::Arrow());
            is_dragging = false; // Reset dragging state after starting drag
        }
    }

    Ctrl::MouseMove(pos, keyflags);
}

void AnimListCtrl::RightDown(Point pos, dword flags) {
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

void AnimListCtrl::PopupContextMenu(Point pos) {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return;
    }

    int project_index = display_indices[selected_index];
    const Animation& anim = project->animations[project_index];

    // Create context menu
    PopupWindow popup;
    popup.AddFrame(BlackFrame());

    // Create a menu layout
    WithTextCtrlLayout<ParentCtrl> content;
    content.Ctrl::SizeHint([this]() { return Size(150, 150); });

    // Create menu items
    Button edit_btn, duplicate_btn, delete_btn, rename_btn, cancel_btn;

    edit_btn.SetLabel("Edit Animation");
    rename_btn.SetLabel("Rename Animation");
    duplicate_btn.SetLabel("Duplicate Animation");
    delete_btn.SetLabel("Delete Animation");
    cancel_btn.SetLabel("Cancel");

    content.Add(edit_btn.TopPos(0, 24).HSizePos());
    content.Add(rename_btn.TopPos(28, 24).HSizePos());
    content.Add(duplicate_btn.TopPos(56, 24).HSizePos());
    content.Add(delete_btn.TopPos(84, 24).HSizePos());
    content.Add(cancel_btn.TopPos(112, 24).HSizePos());

    // Define actions
    edit_btn <<= [this, project_index, &popup]() {
        // Implement edit dialog for the selected animation
        if (!project || project_index >= project->animations.GetCount()) {
            popup.Break();
            return;
        }

        Animation& animation = project->animations[project_index];

        // Create dialog content using simple layout
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(400, 200); });

        // Create input fields
        EditField id_field, name_field, category_field, description_field;
        Option loop_option;
        Button ok_btn, cancel_btn;

        // Set up loop options
        loop_option.Add("No Loop");
        loop_option.Add("Loop");
        loop_option.Add("Ping-Pong");

        // Initialize with current values
        id_field = animation.id;
        name_field = animation.name;
        category_field = animation.category;
        description_field = animation.description;

        // Set loop type
        int loop_index = loop_option.Find(animation.loop_type);
        if (loop_index >= 0) {
            loop_option.SetIndex(loop_index);
        } else {
            loop_option.SetIndex(0); // Default to "No Loop"
        }

        // Add controls with positioning
        dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(name_field.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(category_field.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(loop_option.HSizePos(80, 50).TopPos(80, 20));
        dlg.Add(description_field.HSizePos(80, 50).TopPos(104, 60));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Labels
        Label id_label, name_label, category_label, loop_label, description_label;
        id_label.SetLabel("ID:");
        name_label.SetLabel("Name:");
        category_label.SetLabel("Category:");
        loop_label.SetLabel("Loop Type:");
        description_label.SetLabel("Description:");

        dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(name_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(category_label.LeftPos(8, 60).TopPos(56, 20));
        dlg.Add(loop_label.LeftPos(8, 60).TopPos(80, 20));
        dlg.Add(description_label.LeftPos(8, 60).TopPos(104, 20));

        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Edit Animation");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK) {
            // Update the animation with new values
            String id = ~id_field;
            String name = ~name_field;
            String category = ~category_field;
            String loop_type = AsString(loop_option.Get());
            String description_text = ~description_field;

            // Validate inputs
            if (id.IsEmpty()) {
                Exclamation("Animation ID cannot be empty!");
                popup.Break();
                return;
            }

            // Check if ID is being changed and if new ID already exists
            if (id != animation.id && project->FindAnimation(id)) {
                Exclamation("An animation with ID '" + id + "' already exists!");
                popup.Break();
                return;
            }

            // Update animation properties
            animation.id = id;
            animation.name = name.IsEmpty() ? id : name;
            animation.category = category;
            animation.loop_type = loop_type;
            animation.description = description_text;

            // Refresh the list to show the updated animation
            RefreshList();
        }

        popup.Break();
    };

    rename_btn <<= [this, project_index, &popup, &anim]() {
        if (!project) return;

        String newName = Prompt("Rename Animation", "Enter new name:", anim.name);
        if (!newName.IsEmpty()) {
            // Update the animation name
            project->animations[project_index].name = newName;
            RefreshList();
        }
        
        popup.Break();
    };

    duplicate_btn <<= [this, project_index, &popup]() {
        if (!project) return;

        // Create a duplicate of the selected animation
        Animation duplicated_anim = project->animations[project_index];

        // Generate a unique ID for the duplicate
        duplicated_anim.id = GenerateUniqueIdForAnimation(duplicated_anim.id);
        duplicated_anim.name = duplicated_anim.name + "_copy";

        // Add the duplicated animation to the project
        project->animations.Add(duplicated_anim);

        // Refresh the list to show the new animation
        RefreshList();

        popup.Break();
    };

    delete_btn <<= [this, project_index, &popup, &anim]() {
        if (!project) return;

        // Check if animation has frames
        if (anim.frames.GetCount() == 0) {
            // Confirm deletion for empty animation
            if (PromptYesNo("Are you sure you want to delete animation '" + anim.id + "'?")) {
                // Remove the animation from the project
                project->animations.Remove(project_index);
                // Refresh the list
                RefreshList();
            }
        } else {
            // Confirm deletion for animation with frames
            if (PromptYesNo("Are you sure you want to delete animation '" + anim.id + "'?\n\nIt contains " + 
                            IntStr(anim.frames.GetCount()) + " frame(s).")) {
                // Remove the animation from the project
                project->animations.Remove(project_index);
                // Refresh the list
                RefreshList();
            }
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

String AnimListCtrl::GenerateUniqueIdForAnimation(const String& baseId) {
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
    } while (project->FindAnimation(newId) != nullptr);

    return newId;
}

const Animation* AnimListCtrl::GetSelectedAnimation() const {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return nullptr;
    }

    int project_index = display_indices[selected_index];
    return &project->animations[project_index];
}

void AnimListCtrl::SetSortType(SortType sortType) {
    sort_type = sortType;
    ApplyFilters(); // Reapply filters which will cause resorting
}

void AnimListCtrl::SortAnimations() {
    switch (sort_type) {
        case SORT_BY_NAME:
            SortBy([](const Animation& a, const Animation& b) {
                return ToLower(a.name) < ToLower(b.name);
            });
            break;
        case SORT_BY_ID:
            SortBy([](const Animation& a, const Animation& b) {
                return ToLower(a.id) < ToLower(b.id);
            });
            break;
        case SORT_BY_CATEGORY:
            SortBy([](const Animation& a, const Animation& b) {
                return ToLower(a.category) < ToLower(b.category);
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
void AnimListCtrl::SortBy(CompareFunc compareFunc) {
    // Sort the display_indices based on the comparison function
    for (int i = 0; i < display_indices.GetCount() - 1; i++) {
        for (int j = i + 1; j < display_indices.GetCount(); j++) {
            int idx1 = display_indices[i];
            int idx2 = display_indices[j];

            if (compareFunc(project->animations[idx1], project->animations[idx2])) {
                // Already in correct order
            } else {
                // Swap to sort in ascending order
                Upp::Swap(display_indices[i], display_indices[j]);
            }
        }
    }
}