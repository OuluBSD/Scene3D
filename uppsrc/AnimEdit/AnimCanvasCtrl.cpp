#include "AnimCanvasCtrl.h"

void AnimCanvasCtrl::PushUndoState() {
    if (!frame) return;
    
    // Create a copy of the current frame
    AnimationFrame frame_copy = *frame;
    
    // Clear any redo history beyond current position
    while (undo_stack.GetCount() > undo_index + 1) {
        undo_stack.Remove(undo_stack.GetCount() - 1);
    }
    
    // Add the copy to the stack
    undo_stack.Add(frame_copy);
    undo_index++;
    
    // Limit the size of the undo stack to prevent excessive memory usage
    const int MAX_UNDO_LEVELS = 50;
    if (undo_stack.GetCount() > MAX_UNDO_LEVELS) {
        undo_stack.Remove(0);
        undo_index--;
    }
    
    // Notify that state has changed
    if (general_callback) {
        general_callback();
    }
}

void AnimCanvasCtrl::Undo() {
    if (!frame || undo_index < 0) return;
    
    // Apply the state from the previous position in the stack
    *frame = undo_stack[undo_index];
    undo_index--;
    
    Refresh();
    
    // Notify that state has changed
    if (general_callback) {
        general_callback();
    }
}

void AnimCanvasCtrl::Redo() {
    if (!frame || undo_index >= undo_stack.GetCount() - 1) return;
    
    undo_index++;
    // Apply the state from the next position in the stack
    *frame = undo_stack[undo_index];
    
    Refresh();
    
    // Notify that state has changed
    if (general_callback) {
        general_callback();
    }
}

AnimCanvasCtrl::AnimCanvasCtrl() 
    : project(nullptr)
    , frame(nullptr)
    , grid_spacing(32.0)
    , show_grid(true)
    , show_crosshair(true)
    , grid_snapping(false)
    , origin_snapping(false)
    , zoom(1.0)
    , pan(0, 0)
    , is_panning(false)
    , is_dragging_instance(false)
    , selected_instance(-1)
    , undo_index(-1)
    , zoom_callback(nullptr)
    , general_callback(nullptr)
{
    AddFrame(BlackFrame());
}

AnimCanvasCtrl::~AnimCanvasCtrl() {
}

void AnimCanvasCtrl::SetGridSpacing(double spacing) {
    if (spacing > 0) {
        grid_spacing = spacing;
    }
}

void AnimCanvasCtrl::SetZoom(double zoom) {
    if (zoom > 0) {
        this->zoom = zoom;
        if (zoom_callback) {
            zoom_callback();
        }
    }
}

void AnimCanvasCtrl::SetPan(const Vec2& pan) {
    this->pan = pan;
}

Point AnimCanvasCtrl::WorldToScreen(const Vec2& world_pos) const {
    int x = (int)((world_pos.x + pan.x) * zoom);
    int y = (int)((world_pos.y + pan.y) * zoom);
    return Point(x, y);
}

Vec2 AnimCanvasCtrl::ScreenToWorld(const Point& screen_pos) const {
    double x = (double)screen_pos.x / zoom - pan.x;
    double y = (double)screen_pos.y / zoom - pan.y;
    return Vec2(x, y);
}

void AnimCanvasCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), White());
    
    // Save current transform
    Size sz = GetSize();
    
    if (show_grid) {
        DrawGrid(w);
    }
    
    if (show_crosshair) {
        DrawCrosshair(w);
    }
    
    // Draw sprite instances if frame is set
    if (frame) {
        DrawSpriteInstances(w);
        DrawSelectionHighlight(w);
    }
}

void AnimCanvasCtrl::DrawGrid(Draw& w) {
    if (grid_spacing <= 0) return;
    
    Size sz = GetSize();
    Vec2 world_tl = ScreenToWorld(Point(0, 0));
    Vec2 world_br = ScreenToWorld(Point(sz.cx, sz.cy));
    
    // Draw vertical lines
    int x_start = (int)(world_tl.x / grid_spacing);
    int x_end = (int)(world_br.x / grid_spacing);
    
    for (int x = x_start; x <= x_end; x++) {
        double world_x = x * grid_spacing;
        Point screen_start = WorldToScreen(Vec2(world_x, world_tl.y));
        Point screen_end = WorldToScreen(Vec2(world_x, world_br.y));
        
        // Only draw if it's within screen bounds
        if (screen_start.x >= 0 && screen_start.x <= sz.cx) {
            w.DrawRect(screen_start.x, 0, 1, sz.cy, Gray());
        }
    }
    
    // Draw horizontal lines
    int y_start = (int)(world_tl.y / grid_spacing);
    int y_end = (int)(world_br.y / grid_spacing);
    
    for (int y = y_start; y <= y_end; y++) {
        double world_y = y * grid_spacing;
        Point screen_start = WorldToScreen(Vec2(world_tl.x, world_y));
        Point screen_end = WorldToScreen(Vec2(world_br.x, world_y));
        
        // Only draw if it's within screen bounds
        if (screen_start.y >= 0 && screen_start.y <= sz.cy) {
            w.DrawRect(0, screen_start.y, sz.cx, 1, Gray());
        }
    }
}

void AnimCanvasCtrl::DrawCrosshair(Draw& w) {
    Size sz = GetSize();
    
    // Draw center crosshair
    Point center = WorldToScreen(Vec2(0, 0));
    
    if (center.x >= 0 && center.x <= sz.cx) {
        w.DrawRect(center.x, 0, 1, sz.cy, Red());
    }
    
    if (center.y >= 0 && center.y <= sz.cy) {
        w.DrawRect(0, center.y, sz.cx, 1, Red());
    }
}

void AnimCanvasCtrl::DrawSpriteInstances(Draw& w) {
    if (!frame || !project) return;
    
    for (int i = 0; i < frame->sprites.GetCount(); i++) {
        const SpriteInstance& si = frame->sprites[i];
        const Sprite* sprite = project->FindSprite(si.sprite_id);
        
        // If we can't find the sprite, just draw a placeholder
        Rect r;
        if (sprite) {
            // Calculate position based on sprite region and instance transform
            Vec2 pos = si.position;
            Point p = WorldToScreen(pos);
            
            // For now, draw a simple rectangle representing the sprite
            int width = sprite->region.cx > 0 ? (int)(sprite->region.cx * zoom) : 32;
            int height = sprite->region.cy > 0 ? (int)(sprite->region.cy * zoom) : 32;
            
            r = RectC(p.x - width/2, p.y - height/2, width, height);
        } else {
            // Draw a red rectangle as placeholder for missing sprite
            Point p = WorldToScreen(si.position);
            int width = (int)(32 * zoom);
            int height = (int)(32 * zoom);
            r = RectC(p.x - width/2, p.y - height/2, width, height);
            w.DrawRect(r, Red());
            continue; // Skip drawing the normal color for placeholders
        }
        
        // Use different colors based on selection state
        Color color = (i == selected_instance) ? Green() : Blue();
        w.DrawRect(r, color);
    }
}

void AnimCanvasCtrl::DrawSelectionHighlight(Draw& w) {
    if (selected_instance < 0 || !frame || selected_instance >= frame->sprites.GetCount()) {
        return;
    }
    
    const SpriteInstance& si = frame->sprites[selected_instance];
    const Sprite* sprite = project->FindSprite(si.sprite_id);
    
    Rect r;
    if (sprite) {
        Point p = WorldToScreen(si.position);
        int width = sprite->region.cx > 0 ? (int)(sprite->region.cx * zoom) : 32;
        int height = sprite->region.cy > 0 ? (int)(sprite->region.cy * zoom) : 32;
        r = RectC(p.x - width/2, p.y - height/2, width, height);
    } else {
        Point p = WorldToScreen(si.position);
        int width = (int)(32 * zoom);
        int height = (int)(32 * zoom);
        r = RectC(p.x - width/2, p.y - height/2, width, height);
    }
    
    w.DrawRect(r.left - 2, r.top - 2, r.Width() + 4, 2, Green());
    w.DrawRect(r.right - 2, r.top, 2, r.Height() + 4, Green());
    w.DrawRect(r.left, r.bottom - 2, r.Width() + 4, 2, Green());
    w.DrawRect(r.left - 2, r.top - 2, 2, r.Height() + 4, Green());
}

Vec2 AnimCanvasCtrl::SnapToPosition(const Vec2& pos) const {
    Vec2 result = pos;
    
    // Apply grid snapping if enabled
    if (grid_snapping && grid_spacing > 0) {
        double snapped_x = floor(pos.x / grid_spacing + 0.5) * grid_spacing;
        double snapped_y = floor(pos.y / grid_spacing + 0.5) * grid_spacing;
        result = Vec2(snapped_x, snapped_y);
    }
    
    // Apply origin snapping if enabled
    if (origin_snapping) {
        // Check if the position is close to origin (within grid spacing / 2)
        if (fabs(pos.x) < grid_spacing / 2.0) result.x = 0;
        if (fabs(pos.y) < grid_spacing / 2.0) result.y = 0;
    }
    
    return result;
}

void AnimCanvasCtrl::MouseMove(Point pos, dword flags) {
    if (is_dragging_instance && frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
        // Only push the initial state when the dragging starts (not continuously)
        if (frame->sprites[selected_instance].position == original_instance_pos) {
            PushUndoState();
        }
        
        // Calculate the world offset based on mouse movement
        Vec2 drag_offset_world = ScreenToWorld(pos) - ScreenToWorld(drag_start);
        
        // Update the selected sprite's position
        Vec2 new_pos = original_instance_pos + drag_offset_world;
        
        // Apply position snapping if enabled
        new_pos = SnapToPosition(new_pos);
        if (grid_snapping || origin_snapping) {
            new_pos = SnapToPosition(new_pos);
        }
        
        frame->sprites[selected_instance].position = new_pos;
        
        Refresh();
    } else if (flags & MMBUTTON || (flags & LBUTTON && (flags & K_ALT))) {  // Pan with middle mouse or Alt+left
        if (is_panning) {
            Vec2 current_world = ScreenToWorld(pos);
            Vec2 start_world = ScreenToWorld(pan_start);
            Vec2 delta = Vec2(current_world.x - start_world.x, current_world.y - start_world.y);
            pan = pan_offset_start - delta;
            Refresh();
        }
    }
}

void AnimCanvasCtrl::MouseWheel(Point pos, dword flags, int zdelta) {
    // Calculate world position before zooming
    Vec2 world_pos_before = ScreenToWorld(pos);
    
    // Update zoom level
    double zoom_factor = zdelta > 0 ? 1.25 : 0.8;
    SetZoom(zoom * zoom_factor);
    
    // Calculate where the mouse position should be in world coordinates after zooming
    Vec2 world_pos_after = ScreenToWorld(pos);
    
    // Adjust pan to keep the mouse position fixed in world coordinates
    pan.x += (world_pos_after.x - world_pos_before.x);
    pan.y += (world_pos_after.y - world_pos_before.y);
    
    Refresh();
}

void AnimCanvasCtrl::LeftDown(Point pos, dword flags) {
    if (flags & K_CTRL) {  // Start panning
        is_panning = true;
        pan_start = pos;
        pan_offset_start = pan;
        SetCapture();
        return;
    }
    
    // Check if we're clicking on a selected sprite instance to start dragging
    if (frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
        const SpriteInstance& si = frame->sprites[selected_instance];
        Point sprite_screen_pos = WorldToScreen(si.position);
        
        // Check if click is close to the selected sprite (within 20 pixels)
        int dist_sq = (pos.x - sprite_screen_pos.x) * (pos.x - sprite_screen_pos.x) + 
                      (pos.y - sprite_screen_pos.y) * (pos.y - sprite_screen_pos.y);
        
        if (sqrt(dist_sq) < 20) {
            is_dragging_instance = true;
            drag_start = pos;
            original_instance_pos = si.position;
            SetCapture();
            return;
        }
    }
    
    // For now, just handle selection
    if (frame) {
        // Convert click position to world coordinates
        Vec2 world_pos = ScreenToWorld(pos);
        
        // Simple selection: find the closest sprite instance
        int closest_idx = -1;
        double min_dist = 1000000.0;  // A large number
        
        for (int i = 0; i < frame->sprites.GetCount(); i++) {
            const SpriteInstance& si = frame->sprites[i];
            double dist = sqrt(pow(si.position.x - world_pos.x, 2) + pow(si.position.y - world_pos.y, 2));
            
            if (dist < min_dist) {
                min_dist = dist;
                closest_idx = i;
            }
        }
        
        // For now, if we're close enough to a sprite instance, select it
        if (closest_idx != -1 && min_dist < 50.0) {  // 50 world units threshold
            selected_instance = closest_idx;
            Refresh();
        } else {
            selected_instance = -1;
            Refresh();
        }
    }
    
    Ctrl::LeftDown(pos, flags);
}

void AnimCanvasCtrl::LeftUp(Point pos, dword flags) {
    if (is_panning) {
        is_panning = false;
        ReleaseCapture();
    }
    
    if (is_dragging_instance) {
        is_dragging_instance = false;
        ReleaseCapture();
    }
    
    Ctrl::LeftUp(pos, flags);
}

void AnimCanvasCtrl::RightDown(Point pos, dword flags) {
    // For now, just select the right-clicked sprite instance, similar to left-click
    if (frame) {
        // Convert click position to world coordinates
        Vec2 world_pos = ScreenToWorld(pos);
        
        // Find the closest sprite instance
        int closest_idx = -1;
        double min_dist = 1000000.0;  // A large number
        
        for (int i = 0; i < frame->sprites.GetCount(); i++) {
            const SpriteInstance& si = frame->sprites[i];
            double dist = sqrt(pow(si.position.x - world_pos.x, 2) + pow(si.position.y - world_pos.y, 2));
            
            if (dist < min_dist) {
                min_dist = dist;
                closest_idx = i;
            }
        }
        
        // If we're close enough to a sprite instance, select it
        if (closest_idx != -1 && min_dist < 50.0) {  // 50 world units threshold
            selected_instance = closest_idx;
            Refresh();
        } else {
            selected_instance = -1;
            Refresh();
        }
    }
    
    return Ctrl::RightDown(pos, flags);
}

bool AnimCanvasCtrl::Drop(Point pos, PasteClip& d) {
    if (!frame || !project) {
        return false;
    }
    
    // Accept text drops (which contain sprite IDs from the sprite list)
    String text_data;
    if (AcceptText(d) && !(text_data = GetString(d)).IsEmpty()) {
        // Find the sprite by ID in the project
        const Sprite* sprite = project->FindSprite(text_data);
        if (sprite) {
            // Convert the drop position to world coordinates
            Vec2 world_pos = ScreenToWorld(pos);
            
            // Create a new SpriteInstance with the dropped sprite
            SpriteInstance new_instance;
            new_instance.sprite_id = sprite->id;
            new_instance.position = world_pos;
            new_instance.scale = Vec2(1, 1);
            new_instance.rotation = 0;
            new_instance.alpha = 1.0;
            new_instance.zindex = 0;
            
            // Add the new instance to the current frame
            PushUndoState();  // Save state before modifying
            frame->sprites.Add(new_instance);
            
            // Select the newly created instance
            selected_instance = frame->sprites.GetCount() - 1;
            
            Refresh();
            return true;
        }
    }
    
    return false;
}

bool AnimCanvasCtrl::Key(dword key, int count) {
    switch(key) {
        case '-':
        case K_SHIFT | '-':
            SetZoom(zoom * 0.8);
            Refresh();
            return true;
        case '+':
        case '=':  // Some keyboards have + on =
            SetZoom(zoom * 1.25);
            Refresh();
            return true;
        case K_CTRL | '+':
            if (frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
                PushUndoState();
                frame->sprites[selected_instance].scale.x *= 1.1;
                frame->sprites[selected_instance].scale.y *= 1.1;
                Refresh();
                return true;
            }
            break;
        case K_CTRL | '-':
            if (frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
                PushUndoState();
                frame->sprites[selected_instance].scale.x /= 1.1;
                frame->sprites[selected_instance].scale.y /= 1.1;
                Refresh();
                return true;
            }
            break;
        case K_CTRL | 'Z':
        case K_CTRL | 'z':
            Undo();
            return true;
        case K_CTRL | 'Y':
        case K_CTRL | 'y':
            Redo();
            return true;
        case K_HOME:
            zoom = 1.0;
            pan = Vec2(0, 0);
            Refresh();
            return true;
        case 'Q':
        case 'q':
            if (frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
                PushUndoState();
                frame->sprites[selected_instance].rotation -= M_PI / 12; // Rotate -15 degrees
                Refresh();
                return true;
            }
            break;
        case 'E':
        case 'e':
            if (frame && selected_instance >= 0 && selected_instance < frame->sprites.GetCount()) {
                PushUndoState();
                frame->sprites[selected_instance].rotation += M_PI / 12; // Rotate +15 degrees
                Refresh();
                return true;
            }
            break;
    }
    return Ctrl::Key(key, count);
}