#include "CollisionListCtrl.h"
#include <Core/Core.h>

CollisionListCtrl::CollisionListCtrl()
    : frame(nullptr)
    , selected_index(-1)
{
    AddFrame(WhiteFrame());
}

CollisionListCtrl::~CollisionListCtrl() {
}

void CollisionListCtrl::SetFrame(const AnimationFrame* frame) {
    this->frame = frame;
    RefreshList();
}

void CollisionListCtrl::RefreshList() {
    ApplyFilters();
    Refresh();
}

void CollisionListCtrl::ApplyFilters() {
    display_indices.Clear();

    if (!frame) return;

    for (int i = 0; i < frame->collisions.GetCount(); i++) {
        display_indices.Add(i);
    }
}

int CollisionListCtrl::HitTest(Point pos) const {
    if (!frame) return -1;

    int y = 0;
    int item_height = 30; // Fixed height for each item

    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        if (item_rect.Contains(pos)) {
            return i; // Return the display index
        }
        y += item_height;
    }

    return -1;
}

void CollisionListCtrl::DrawItem(Draw& w, int display_index, const Rect& rc) const {
    if (!frame || display_index < 0 || display_index >= display_indices.GetCount()) {
        return;
    }

    int frame_index = display_indices[display_index];
    const CollisionRect& cr = frame->collisions[frame_index];

    // Draw background
    Color bg_color = (display_index == selected_index) ? LtBlue() : (display_index % 2 == 0 ? White() : SclLightGray());
    w.DrawRect(rc, bg_color);

    // Draw collision properties
    String info = Format("ID: %s, Pos: (%.1f, %.1f), Size: (%.1f, %.1f)", 
                         cr.id, cr.rect.x, cr.rect.y, cr.rect.cx, cr.rect.cy);
    w.DrawText(rc.left + 8, rc.top + 8, info, Arial(10), Black());
}

Size CollisionListCtrl::GetItemSize(int index) const {
    return Size(GetSize().cx, 30); // Fixed height
}

void CollisionListCtrl::Paint(Draw& w) {
    if (!frame) {
        w.DrawRect(GetSize(), White());
        return;
    }

    w.DrawRect(GetSize(), White());

    int y = 0;
    int item_height = 30;

    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        DrawItem(w, i, item_rect);
        y += item_height;
    }
}

void CollisionListCtrl::MouseDown(Point pos, dword button) {
    int item_index = HitTest(pos);

    if (item_index >= 0) {
        selected_index = item_index;
        Refresh();

        // Call the selection callback
        if (select_callback) {
            const CollisionRect* selected_cr = GetSelectedCollision();
            select_callback(selected_cr);
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

void CollisionListCtrl::RightDown(Point pos, dword flags) {
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

void CollisionListCtrl::PopupContextMenu(Point pos) {
    if (!frame || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return;
    }

    int frame_index = display_indices[selected_index];
    CollisionRect& cr = frame->collisions[frame_index];

    // Create context menu
    PopupWindow popup;
    popup.AddFrame(BlackFrame());

    // Create a menu layout
    CtrlLayout<ParentCtrl> content;
    content.Ctrl::SizeHint([this]() { return Size(150, 150); });

    // Create menu items
    Button edit_btn, delete_btn, new_btn, cancel_btn;

    edit_btn.SetLabel("Edit Collision");
    new_btn.SetLabel("New Collision");
    delete_btn.SetLabel("Delete Collision");
    cancel_btn.SetLabel("Cancel");

    content.Add(edit_btn.TopPos(0, 24).HSizePos());
    content.Add(new_btn.TopPos(28, 24).HSizePos());
    content.Add(delete_btn.TopPos(56, 24).HSizePos());
    content.Add(cancel_btn.TopPos(84, 24).HSizePos());

    // Define actions
    edit_btn <<= [this, &cr, &popup]() {
        // Create dialog for editing collision
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(300, 160); });

        EditField id_field;
        SpinEdit pos_x, pos_y, size_cx, size_cy;
        Button ok_btn, cancel_btn;

        // Set ranges and initial values
        id_field = cr.id;
        pos_x.SetRange(-1000, 1000); pos_x.Set(cr.rect.x);
        pos_y.SetRange(-1000, 1000); pos_y.Set(cr.rect.y);
        size_cx.SetRange(1, 10000); size_cx.Set(cr.rect.cx);
        size_cy.SetRange(1, 10000); size_cy.Set(cr.rect.cy);

        // Add controls
        dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(pos_x.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(pos_y.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(size_cx.HSizePos(80, 50).TopPos(80, 20));
        dlg.Add(size_cy.HSizePos(80, 50).TopPos(104, 20));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Labels
        Label id_label, pos_x_label, pos_y_label, size_cx_label, size_cy_label;
        id_label.SetLabel("ID:");
        pos_x_label.SetLabel("Pos X:");
        pos_y_label.SetLabel("Pos Y:");
        size_cx_label.SetLabel("Size X:");
        size_cy_label.SetLabel("Size Y:");

        dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(pos_x_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(pos_y_label.LeftPos(8, 60).TopPos(56, 20));
        dlg.Add(size_cx_label.LeftPos(8, 60).TopPos(80, 20));
        dlg.Add(size_cy_label.LeftPos(8, 60).TopPos(104, 20));

        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Edit Collision Rectangle");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK) {
            // Update collision values
            cr.id = ~id_field;
            cr.rect.x = ~pos_x;
            cr.rect.y = ~pos_y;
            cr.rect.cx = ~size_cx;
            cr.rect.cy = ~size_cy;

            // Notify of change
            if (change_callback) {
                change_callback();
            }
            
            Refresh();
        }
        
        popup.Break();
    };

    new_btn <<= [this, &popup, this_frame = frame]() {
        // Create a dialog for new collision
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(300, 160); });

        EditField id_field;
        SpinEdit pos_x, pos_y, size_cx, size_cy;
        Button ok_btn, cancel_btn;

        // Set ranges and default values
        pos_x.SetRange(-1000, 1000); pos_x.Set(0);
        pos_y.SetRange(-1000, 1000); pos_y.Set(0);
        size_cx.SetRange(1, 10000); size_cx.Set(32);
        size_cy.SetRange(1, 10000); size_cy.Set(32);

        // Add controls
        dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(pos_x.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(pos_y.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(size_cx.HSizePos(80, 50).TopPos(80, 20));
        dlg.Add(size_cy.HSizePos(80, 50).TopPos(104, 20));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));

        // Labels
        Label id_label, pos_x_label, pos_y_label, size_cx_label, size_cy_label;
        id_label.SetLabel("ID:");
        pos_x_label.SetLabel("Pos X:");
        pos_y_label.SetLabel("Pos Y:");
        size_cx_label.SetLabel("Size X:");
        size_cy_label.SetLabel("Size Y:");

        dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(pos_x_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(pos_y_label.LeftPos(8, 60).TopPos(56, 20));
        dlg.Add(size_cx_label.LeftPos(8, 60).TopPos(80, 20));
        dlg.Add(size_cy_label.LeftPos(8, 60).TopPos(104, 20));

        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");

        // Set default ID
        id_field.Set("collision_" + Uuid().ToString());

        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Create New Collision Rectangle");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);

        if(prompt_dlg.Execute() == IDOK) {
            // Create new collision
            CollisionRect new_cr;
            new_cr.id = ~id_field;
            new_cr.rect.x = ~pos_x;
            new_cr.rect.y = ~pos_y;
            new_cr.rect.cx = ~size_cx;
            new_cr.rect.cy = ~size_cy;

            this_frame->collisions.Add(new_cr);

            // Notify of change
            if (change_callback) {
                change_callback();
            }
            
            RefreshList();
        }
        
        popup.Break();
    };

    delete_btn <<= [this, frame_index, &popup, this_frame = frame]() {
        if (PromptYesNo("Delete this collision rectangle?")) {
            this_frame->collisions.Remove(frame_index);
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
    popup.SetRect(pos.x, pos.y, 150, 140);
    popup.Run();
}

const CollisionRect* CollisionListCtrl::GetSelectedCollision() const {
    if (!frame || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return nullptr;
    }

    int frame_index = display_indices[selected_index];
    return &frame->collisions[frame_index];
}