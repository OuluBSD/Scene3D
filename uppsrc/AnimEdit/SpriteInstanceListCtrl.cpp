#include "SpriteInstanceListCtrl.h"
#include <Core/Core.h>

SpriteInstanceListCtrl::SpriteInstanceListCtrl()
    : project(nullptr)
    , frame(nullptr)
    , selected_index(-1)
{
    AddFrame(WhiteFrame());
}

SpriteInstanceListCtrl::~SpriteInstanceListCtrl() {
}

void SpriteInstanceListCtrl::SetProject(const AnimationProject* project) {
    this->project = project;
    RefreshList();
}

void SpriteInstanceListCtrl::SetFrame(const AnimationFrame* frame) {
    this->frame = frame;
    RefreshList();
}

void SpriteInstanceListCtrl::RefreshList() {
    ApplyFilters();
    Refresh();
}

void SpriteInstanceListCtrl::ApplyFilters() {
    display_indices.Clear();

    if (!frame) return;

    for (int i = 0; i < frame->sprites.GetCount(); i++) {
        display_indices.Add(i);
    }
}

int SpriteInstanceListCtrl::HitTest(Point pos) const {
    if (!frame) return -1;

    int y = 0;
    int item_height = 40; // Fixed height for each item

    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        if (item_rect.Contains(pos)) {
            return i; // Return the display index
        }
        y += item_height;
    }

    return -1;
}

void SpriteInstanceListCtrl::DrawItem(Draw& w, int display_index, const Rect& rc) const {
    if (!frame || display_index < 0 || display_index >= display_indices.GetCount()) {
        return;
    }

    int frame_index = display_indices[display_index];
    const SpriteInstance& si = frame->sprites[frame_index];

    // Find the corresponding sprite in the project
    const Sprite* sprite = nullptr;
    if (project) {
        sprite = project->FindSprite(si.sprite_id);
    }

    // Draw background
    Color bg_color = (display_index == selected_index) ? LtBlue() : (display_index % 2 == 0 ? White() : SclLightGray());
    w.DrawRect(rc, bg_color);

    // Draw sprite name/ID
    String displayName = sprite ? sprite->name : si.sprite_id;
    if (displayName.IsEmpty()) displayName = si.sprite_id;
    w.DrawText(rc.left + 8, rc.top + 3, displayName, Arial(12), Black());

    // Draw transform properties
    String transform = Format("Pos: (%.1f, %.1f), Scl: (%.2f, %.2f), Rot: %.1f°", 
                              si.position.x, si.position.y,
                              si.scale.x, si.scale.y,
                              si.rotation);
    w.DrawText(rc.left + 8, rc.top + 18, transform, Arial(10), Gray());

    // Draw z-index
    String z_index = "Z: " + IntStr(si.zindex);
    w.DrawText(rc.left + 8, rc.top + 30, z_index, Arial(10), SclGray());
}

Size SpriteInstanceListCtrl::GetItemSize(int index) const {
    return Size(GetSize().cx, 40); // Fixed height
}

void SpriteInstanceListCtrl::Paint(Draw& w) {
    if (!frame) {
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

void SpriteInstanceListCtrl::MouseDown(Point pos, dword button) {
    int item_index = HitTest(pos);

    if (item_index >= 0) {
        selected_index = item_index;
        Refresh();

        // Call the selection callback
        if (select_callback) {
            const SpriteInstance* selected_si = GetSelectedSpriteInstance();
            select_callback(selected_si);
        }
    } else {
        selected_index = -1;
        Refresh();

        if (select_callback) {
            select_callback(nullptr);
        }
    }

    Ctrl::LeftDown(pos, button);
}

void SpriteInstanceListCtrl::RightDown(Point pos, dword flags) {
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

void SpriteInstanceListCtrl::PopupContextMenu(Point pos) {
    if (!frame || !project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return;
    }

    int frame_index = display_indices[selected_index];
    SpriteInstance& si = frame->sprites[frame_index];

    // Create context menu
    PopupWindow popup;
    popup.AddFrame(BlackFrame());

    // Create a menu layout
    CtrlLayout<ParentCtrl> content;
    content.Ctrl::SizeHint([this]() { return Size(150, 150); });

    // Create menu items
    Button edit_btn, delete_btn, move_up_btn, move_down_btn, cancel_btn;

    edit_btn.SetLabel("Edit Transform");
    move_up_btn.SetLabel("Move Up (Z)");
    move_down_btn.SetLabel("Move Down (Z)");
    delete_btn.SetLabel("Delete Instance");
    cancel_btn.SetLabel("Cancel");

    content.Add(edit_btn.TopPos(0, 24).HSizePos());
    content.Add(move_up_btn.TopPos(28, 24).HSizePos());
    content.Add(move_down_btn.TopPos(56, 24).HSizePos());
    content.Add(delete_btn.TopPos(84, 24).HSizePos());
    content.Add(cancel_btn.TopPos(112, 24).HSizePos());

    // Define actions
    edit_btn <<= [this, &si, &popup]() {
        // Create dialog for editing transform
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(300, 200); });

        SpinEdit pos_x, pos_y, rot;
        EditDouble scale_x, scale_y;
        SpinEdit z_index;
        Button ok_btn, cancel_btn;

        // Set ranges and initial values
        pos_x.SetRange(-1000, 1000); pos_x.Set(si.position.x);
        pos_y.SetRange(-1000, 1000); pos_y.Set(si.position.y);
        rot.SetRange(-360, 360); rot.Set(si.rotation);
        scale_x <<= si.scale.x;
        scale_y <<= si.scale.y;
        z_index.SetRange(-1000, 1000); z_index.Set(si.zindex);

        // Add controls
        dlg.Add(pos_x.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(pos_y.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(rot.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(scale_x.HSizePos(80, 50).TopPos(80, 20));
        dlg.Add(scale_y.HSizePos(80, 50).TopPos(104, 20));
        dlg.Add(z_index.HSizePos(80, 50).TopPos(128, 20));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Labels
        Label pos_x_label, pos_y_label, rot_label, scale_x_label, scale_y_label, z_label;
        pos_x_label.SetLabel("Pos X:");
        pos_y_label.SetLabel("Pos Y:");
        rot_label.SetLabel("Rotation:");
        scale_x_label.SetLabel("Scale X:");
        scale_y_label.SetLabel("Scale Y:");
        z_label.SetLabel("Z-Index:");

        dlg.Add(pos_x_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(pos_y_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(rot_label.LeftPos(8, 60).TopPos(56, 20));
        dlg.Add(scale_x_label.LeftPos(8, 60).TopPos(80, 20));
        dlg.Add(scale_y_label.LeftPos(8, 60).TopPos(104, 20));
        dlg.Add(z_label.LeftPos(8, 60).TopPos(128, 20));

        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Edit Sprite Instance Transform");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK) {
            // Update transform values
            si.position.x = ~pos_x;
            si.position.y = ~pos_y;
            si.rotation = ~rot;
            si.scale.x = ~scale_x;
            si.scale.y = ~scale_y;
            si.zindex = ~z_index;

            // Notify of change
            if (change_callback) {
                change_callback();
            }
            
            Refresh();
        }
        
        popup.Break();
    };

    move_up_btn <<= [this, frame_index, &popup]() {
        if (frame_index > 0) {
            Upp::Swap(frame->sprites[frame_index], frame->sprites[frame_index-1]);
            selected_index = frame_index - 1; // Update selection to moved item
            if (change_callback) {
                change_callback();
            }
            RefreshList();
        }
        popup.Break();
    };

    move_down_btn <<= [this, frame_index, &popup]() {
        if (frame_index < frame->sprites.GetCount() - 1) {
            Upp::Swap(frame->sprites[frame_index], frame->sprites[frame_index+1]);
            selected_index = frame_index + 1; // Update selection to moved item
            if (change_callback) {
                change_callback();
            }
            RefreshList();
        }
        popup.Break();
    };

    delete_btn <<= [this, frame_index, &popup, this_frame = frame]() {
        if (PromptYesNo("Delete this sprite instance?")) {
            this_frame->sprites.Remove(frame_index);
            selected_index = -1; // Deselect
            if (change_callback) {
                change_callback();
            }
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

const SpriteInstance* SpriteInstanceListCtrl::GetSelectedSpriteInstance() const {
    if (!frame || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return nullptr;
    }

    int frame_index = display_indices[selected_index];
    return &frame->sprites[frame_index];
}