#include "TimelineCtrl.h"
#include <Draw/Draw.h>
#include <CtrlLib/CtrlLib.h>

AnimEditTimelineCtrl::AnimEditTimelineCtrl()
    : project(nullptr)
    , animation(nullptr)
    , selected_frame_index(-1)
    , drag_start_index(-1)
    , drag_current_index(-1)
    , is_dragging(false)
    , frame_width(80)
    , frame_height(60)
    , frame_spacing(10)
{
    SetFrameProfile(1);
    selected_frame_index = -1;
}

AnimEditTimelineCtrl::~AnimEditTimelineCtrl()
{
}

void AnimEditTimelineCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
}

void AnimEditTimelineCtrl::SetAnimation(const Animation* anim) {
    animation = anim;
    selected_frame_index = -1; // Reset selection
    RefreshLayout();
    Refresh();
}

void AnimEditTimelineCtrl::SetFrameCallback(std::function<void(const AnimationFrame*)> callback) {
    frame_callback = callback;
}

void AnimEditTimelineCtrl::SetOnFrameModified(std::function<void()> callback) {
    on_frame_modified_callback = callback;
}

int AnimEditTimelineCtrl::HitTest(Point pos) const {
    if (!animation) return -1;
    
    int current_x = 4; // Padding
    for (int i = 0; i < animation->frames.GetCount(); i++) {
        Rect frame_rc(current_x, 4, current_x + frame_width, 4 + frame_height);
        if (frame_rc.Contains(pos)) {
            return i;
        }
        current_x += frame_width + frame_spacing;
    }
    return -1;
}

int AnimEditTimelineCtrl::HitTestDurationControl(Point pos) const {
    if (!animation) return -1;

    int current_x = 4; // Padding
    for (int i = 0; i < animation->frames.GetCount(); i++) {
        Rect frame_rc(current_x, 4, current_x + frame_width, 4 + frame_height);
        
        // Check if click is in the duration control area
        int duration_control_width = 24;
        int duration_control_height = 12;
        int duration_control_x = frame_rc.right - duration_control_width - 2;
        int duration_control_y = frame_rc.top + 2;
        Rect duration_rc = RectC(duration_control_x, duration_control_y, duration_control_width, duration_control_height);
        
        if (duration_rc.Contains(pos)) {
            return i; // Return frame index if duration control was clicked
        }
        
        current_x += frame_width + frame_spacing;
    }
    return -1;
}

void AnimEditTimelineCtrl::DrawFrame(Draw& w, int index, const Rect& rc) {
    if (!animation || index < 0 || index >= animation->frames.GetCount()) return;
    
    const AnimationFrameRef& ref = animation->frames[index];
    const AnimationFrame* frame = project ? project->FindFrame(ref.frame_id) : nullptr;
    
    // Draw frame background
    Color bg_color = (index == selected_frame_index) ? RGB(50, 150, 200) : RGB(200, 200, 200);
    w.DrawRect(rc, bg_color);
    
    // Draw frame border
    w.DrawRect(rc, 1, Black());
    
    // Draw frame index
    String frame_text = IntStr(index);
    w.DrawText(rc.left + 4, rc.top + 2, frame_text, StdFont(), Black());
    
    // Draw frame duration if different from default
    if (ref.has_duration) {
        String dur_text = Format("%.2f", ref.duration) + "s";
        w.DrawText(rc.left + 4, rc.top + 20, dur_text, StdFont(), Black());
    } else if (frame) {
        String dur_text = Format("%.2f", frame->default_duration) + "s";
        w.DrawText(rc.left + 4, rc.top + 20, dur_text, StdFont(), Black());
    }
    
    // Draw frame ID or name if available
    if (frame) {
        String name = frame->name;
        if (name.IsEmpty()) name = frame->id;
        if (name.GetLength() > 8) name = name.Mid(0, 8) + "...";
        w.DrawText(rc.left + 4, rc.top + 38, name, StdFont(), Black());
        
        // Draw a tiny thumbnail of the frame's sprite instances
        if (frame->sprites.GetCount() > 0 && project) {
            int thumb_width = rc.Width() - 8;
            int thumb_height = 16;
            int thumb_top = rc.top + frame_height - thumb_height - 2;
            Rect thumb_rect = RectC(rc.left + 4, thumb_top, thumb_width, thumb_height);
            
            // Draw a light background for the thumbnail
            w.DrawRect(thumb_rect, SColorFace());
            
            // Draw small representations of each sprite in the frame
            for (int i = 0; i < frame->sprites.GetCount(); i++) {
                const SpriteInstance& si = frame->sprites[i];
                const Sprite* sprite = project->FindSprite(si.sprite_id);
                
                // Calculate a relative position within the thumbnail
                // For simplicity, let's just draw a colored rectangle for each sprite
                int sprite_width = max(2, thumb_width / max(1, frame->sprites.GetCount()));
                int sprite_left = thumb_rect.left + (i * sprite_width);
                int sprite_rect_width = min(sprite_width - 1, thumb_width - (i * sprite_width));
                
                Color sprite_color = sprite ? RGB(0, 100, 200) : RGB(200, 0, 0); // Blue for valid sprites, red for missing
                w.DrawRect(sprite_left, thumb_top, sprite_rect_width, thumb_height, sprite_color);
            }
        }
    }
    
    // Draw duration control/slider area
    int duration_control_width = 24;
    int duration_control_height = 12;
    int duration_control_x = rc.right - duration_control_width - 2;
    int duration_control_y = rc.top + 2;
    Rect duration_rc = RectC(duration_control_x, duration_control_y, duration_control_width, duration_control_height);
    
    // Draw a small rectangle to represent the duration control
    Color control_color = (editing_frame_index == index) ? RGB(255, 150, 0) : RGB(180, 180, 180); // Orange when editing, gray otherwise
    w.DrawRect(duration_rc, control_color);
    w.DrawRect(duration_rc, 1, Black());
    
    // Draw a small icon or indicator inside the control
    w.DrawText(duration_control_x + 2, duration_control_y + 1, "⏱", StdFont(), Black());
}
}

void AnimEditTimelineCtrl::RefreshLayout() {
    if (!animation) return;
    
    // Calculate how many frames we can show
    int available_width = GetSize().cx - 8; // Account for padding
    int total_frame_space = (frame_width + frame_spacing) * animation->frames.GetCount() - frame_spacing;
    
    // If all frames fit, we're fine; otherwise we might need to implement scrolling
    // For now, just ensure we don't overflow the available space
    Refresh();
}

void AnimEditTimelineCtrl::UpdateScroll() {
    // For now, a simple scroll mechanism - would need to be more sophisticated for production
}

void AnimEditTimelineCtrl::SelectFrame(int index) {
    if (index == selected_frame_index) return;
    
    selected_frame_index = index;
    
    if (frame_callback && animation && index >= 0 && index < animation->frames.GetCount()) {
        const AnimationFrameRef& ref = animation->frames[index];
        const AnimationFrame* frame = project ? project->FindFrame(ref.frame_id) : nullptr;
        if (frame) {
            frame_callback(frame);
        }
    }
    
    Refresh();
}

void AnimEditTimelineCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), SColorFace());
    
    if (!animation) return;

    int current_x = 4; // Left padding
    int y_pos = 4;     // Top padding
    
    for (int i = 0; i < animation->frames.GetCount(); i++) {
        Rect frame_rc(current_x, y_pos, current_x + frame_width, y_pos + frame_height);
        DrawFrame(w, i, frame_rc);
        current_x += frame_width + frame_spacing;
    }
}

void AnimEditTimelineCtrl::MouseDown(Point pos, dword button) {
    Ctrl::MouseDown(pos, button);
    
    int frame_idx = HitTest(pos);
    if (frame_idx >= 0) {
        SelectFrame(frame_idx);
    }
}

void AnimEditTimelineCtrl::LeftDown(Point pos, dword flags) {
    // Check if the click is on a duration control first
    int duration_frame_idx = HitTestDurationControl(pos);
    if (duration_frame_idx >= 0) {
        // Clicked on duration control, show duration editor
        editing_frame_index = duration_frame_idx;
        ShowDurationEditor(duration_frame_idx, pos.x, pos.y);
        return;
    }
    
    // Otherwise, handle as normal frame click
    int frame_idx = HitTest(pos);
    if (frame_idx >= 0) {
        // Start drag operation if we're clicking on a frame
        drag_start_index = frame_idx;
        is_dragging = false; // We'll set this to true after a threshold movement
        SetCapture();
    } else {
        // Clicked outside a frame, deselect
        if (selected_frame_index != -1) {
            selected_frame_index = -1;
            Refresh();
        }
    }
    Ctrl::LeftDown(pos, flags);
}

void AnimEditTimelineCtrl::LeftUp(Point pos, dword flags) {
    if (IsCapture()) {
        ReleaseCapture();
    }
    
    if (is_dragging && drag_start_index >= 0 && drag_current_index >= 0 && drag_start_index != drag_current_index) {
        // Perform the reordering
        ReorderFrame(drag_start_index, drag_current_index);
        if (on_frame_modified_callback) {
            on_frame_modified_callback();
        }
    }
    
    drag_start_index = -1;
    drag_current_index = -1;
    is_dragging = false;
    Ctrl::LeftUp(pos, flags);
}

void AnimEditTimelineCtrl::MouseMove(Point pos, dword keyflags) {
    Ctrl::MouseMove(pos, keyflags);

    if (IsCapture() && drag_start_index >= 0) {
        if (!is_dragging && IsDragThresholdExceeded(pos)) {
            is_dragging = true;
        }

        if (is_dragging) {
            // Find the position where we are now to potentially swap frames
            int new_index = HitTest(pos);
            if (new_index >= 0 && new_index != drag_current_index) {
                drag_current_index = new_index;
                // Visual feedback would go here
                Refresh();
            }
        }
    }
}

bool AnimEditTimelineCtrl::Key(dword key, int count) {
    if (!animation || selected_frame_index < 0) return false;
    
    switch(key) {
        case K_LEFT:
            if (selected_frame_index > 0) {
                SelectFrame(selected_frame_index - 1);
                return true;
            }
            break;
        case K_RIGHT:
            if (selected_frame_index < animation->frames.GetCount() - 1) {
                SelectFrame(selected_frame_index + 1);
                return true;
            }
            break;
        case K_DELETE:
            // Delete the selected frame from the animation
            if (animation && selected_frame_index >= 0) {
                // This would require modification of the animation, which should go through a proper API
                // For now, just show an alert
                PromptOK("Delete frame functionality would go here");
            }
            break;
    }
    return Ctrl::Key(key, count);
}bool AnimEditTimelineCtrl::IsDragThresholdExceeded(Point pos) {
    if (drag_start_index < 0) return false;
    
    // Get the rect of the starting frame
    int current_x = 4; // Padding
    for (int i = 0; i < animation->frames.GetCount(); i++) {
        Rect frame_rc(current_x, 4, current_x + frame_width, 4 + frame_height);
        if (i == drag_start_index) {
            // Calculate the center of the frame
            Point frame_center = frame_rc.CenterPos();
            int distance = abs(pos.x - frame_center.x);
            static const int DRAG_THRESHOLD = 5;
            return distance > DRAG_THRESHOLD;
        }
        current_x += frame_width + frame_spacing;
    }
    return false; // Should not happen if drag_start_index is valid
}

void AnimEditTimelineCtrl::ReorderFrame(int from_index, int to_index) {
    if (!animation || from_index < 0 || to_index < 0 || 
        from_index >= animation->frames.GetCount() || to_index >= animation->frames.GetCount()) {
        return;
    }
    
    // Move the frame from from_index to to_index
    AnimationFrameRef temp = animation->frames[from_index];
    animation->frames.Remove(from_index);
    animation->frames.Insert(to_index, temp);
    
    // Update the selected index if necessary
    if (selected_frame_index == from_index) {
        selected_frame_index = to_index;
    } else if (selected_frame_index == to_index && from_index < to_index) {
        selected_frame_index = to_index - 1;  // Adjust for the removed element
    } else if (selected_frame_index == to_index && from_index > to_index) {
        selected_frame_index = to_index + 1;  // Adjust for the shifted element
    }
    
    Refresh();
}

void AnimEditTimelineCtrl::StartDrag(int index) {
    drag_start_index = index;
    is_dragging = false; // We'll set this to true after the threshold is exceeded
    drag_current_index = index;
    SetCapture();
}

void AnimEditTimelineCtrl::EndDrag() {
    if (IsCapture()) {
        ReleaseCapture();
    }
    drag_start_index = -1;
    drag_current_index = -1;
    is_dragging = false;
}

void AnimEditTimelineCtrl::ShowDurationEditor(int frame_index, int x, int y) {
    if (!animation || frame_index < 0 || frame_index >= animation->frames.GetCount()) {
        return;
    }
    
    // Get the current duration value
    const AnimationFrameRef& ref = animation->frames[frame_index];
    double current_duration = ref.has_duration ? ref.duration : 0.1; // Default to 0.1 if not set
    
    // Create a simple dialog for duration editing
    WithTextCtrlLayout<ParentCtrl> dlg;
    dlg.Ctrl::SizeHint([this]() { return Size(200, 100); });
    
    EditDouble duration_edit;
    duration_edit <<= current_duration;
    duration_edit.MinMax(0.01, 10.0); // Min 0.01s, Max 10s
    
    Button ok_btn, cancel_btn;
    ok_btn.SetLabel("OK");
    cancel_btn.SetLabel("Cancel");
    
    // Layout
    dlg.Add(duration_edit.VSizePos(8, 32).HSizePos(8, 8));
    dlg.Add(ok_btn.LeftPos(10, 80).VSizePos(36, 24));
    dlg.Add(cancel_btn.RightPos(10, 80).VSizePos(36, 24));
    
    Label label;
    label.SetLabel("Duration (s):");
    dlg.Add(label.LeftPos(8, 80).TopPos(8, 20));
    
    // Set up the dialog
    PromptOKCancelFrame prompt_dlg;
    prompt_dlg.Title("Edit Frame Duration");
    prompt_dlg.Add(dlg.SizePos());
    prompt_dlg.OK(ok_btn);
    prompt_dlg.Cancel(cancel_btn);
    
    if (prompt_dlg.Execute() == IDOK) {
        double new_duration = ~duration_edit;
        OnDurationChanged(frame_index, new_duration);
    }
    
    // Reset editing state
    editing_frame_index = -1;
    Refresh();
}

void AnimEditTimelineCtrl::OnDurationChanged(int frame_index, double new_duration) {
    if (!animation || frame_index < 0 || frame_index >= animation->frames.GetCount()) {
        return;
    }
    
    // Update the frame duration
    AnimationFrameRef& ref = animation->frames[frame_index];
    ref.has_duration = true;
    ref.duration = new_duration;
    
    // Mark as dirty and notify
    if (on_frame_modified_callback) {
        on_frame_modified_callback();
    }
    
    Refresh();
}