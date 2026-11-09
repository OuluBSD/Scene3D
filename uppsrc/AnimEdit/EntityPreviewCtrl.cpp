#include "EntityPreviewCtrl.h"

EntityPreviewCtrl::EntityPreviewCtrl() {
    project = nullptr;
    entity = nullptr;
    animation = nullptr;
    current_frame_index = 0;
    is_playing = false;
    is_paused = false;
    loop_enabled = true;
    playback_speed = 1.0;

    // Set up the animation timer
    animation_timer.Set(100, [this] { OnTimer(); }); // Update at 10fps as a placeholder
}

EntityPreviewCtrl::~EntityPreviewCtrl() {
}

void EntityPreviewCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
}

void EntityPreviewCtrl::SetEntity(const Entity* ent) {
    entity = ent;
    // Optionally, set to the entity's default animation
    if (project && entity && entity->animation_slots.GetCount() > 0) {
        const NamedAnimationSlot& slot = entity->animation_slots[0];
        const Animation* anim = project->FindAnimation(slot.animation_id);
        SetAnimation(anim);
    }
}

void EntityPreviewCtrl::SetAnimation(const Animation* anim) {
    animation = anim;
    current_frame_index = 0; // Reset to first frame
}

void EntityPreviewCtrl::StartAnimation() {
    if (animation && animation->frames.GetCount() > 0) {
        is_playing = true;
        is_paused = false;
        // Calculate the base interval based on average frame duration
        int base_interval = 100; // default to 10fps if no duration info
        if (animation->frames.GetCount() > 0) {
            // Calculate average duration from the frames if they have custom durations
            double total_duration = 0;
            int frame_count = 0;
            for (const AnimationFrameRef& frame_ref : animation->frames) {
                double duration = frame_ref.has_duration ? frame_ref.duration : 0.1; // default 0.1s
                total_duration += duration;
                frame_count++;
            }
            if (frame_count > 0) {
                base_interval = (int)((total_duration / frame_count) * 1000); // Convert to milliseconds
            }
        }
        
        // Adjust interval based on playback speed (inverse relationship: higher speed = lower interval)
        int adjusted_interval = base_interval > 0 ? (int)(base_interval / playback_speed) : 100;
        // Ensure we don't go too fast (minimum 10ms interval) or too slow (maximum 1000ms interval)
        adjusted_interval = max(10, min(1000, adjusted_interval));
        
        animation_timer.Set(adjusted_interval, [this] { OnTimer(); });
        animation_timer.StartTimer();
    }
}

void EntityPreviewCtrl::PauseAnimation() {
    if (is_playing) {
        is_paused = true;
        animation_timer.StopTimer();
    }
}

void EntityPreviewCtrl::StopAnimation() {
    is_playing = false;
    is_paused = false;
    current_frame_index = 0; // Reset to first frame
    animation_timer.StopTimer();
    Refresh(); // Redraw to show first frame
}

void EntityPreviewCtrl::SetLoopEnabled(bool enabled) {
    loop_enabled = enabled;
}

void EntityPreviewCtrl::SetPlaybackSpeed(double speed) {
    playback_speed = max(0.01, min(5.0, speed)); // Clamp between 0.01x and 5.0x
    // If currently playing, restart with new speed
    if (is_playing && !is_paused && animation && animation->frames.GetCount() > 0) {
        StartAnimation();
    }
}

void EntityPreviewCtrl::UpdateAnimation() {
    if (!is_playing || is_paused || !animation) return;

    if (animation->frames.GetCount() == 0) return;

    current_frame_index++;
    if (current_frame_index >= animation->frames.GetCount()) {
        if (loop_enabled) {
            current_frame_index = 0;
        } else {
            is_playing = false;
            animation_timer.StopTimer();
        }
    }

    Refresh(); // Redraw the control
}

void EntityPreviewCtrl::OnTimer() {
    UpdateAnimation();
}

void EntityPreviewCtrl::Paint(Draw& w) {
    // Draw background
    w.DrawRect(GetSize(), White());

    if (!project || !entity || !animation) {
        // Draw a placeholder message
        w.DrawText(10, 10, "No entity/animation selected for preview", StdFont(12), Black());
        return;
    }

    // Draw the current frame of the animation
    if (current_frame_index < animation->frames.GetCount()) {
        const AnimationFrameRef& frame_ref = animation->frames[current_frame_index];
        const AnimationFrame* frame = project->FindFrame(frame_ref.frame_id);

        if (frame) {
            // Center the drawing in our control
            int ctrl_cx = GetSize().cx;
            int ctrl_cy = GetSize().cy;

            // Simple visualization: draw each sprite instance as a colored rectangle
            for (int i = 0; i < frame->sprites.GetCount(); i++) {
                const SpriteInstance& si = frame->sprites[i];
                const Sprite* sprite = project->FindSprite(si.sprite_id);

                if (sprite) {
                    // Map world coordinates to screen coordinates
                    // For simplicity, we'll scale and center the sprites
                    int x = (int)(si.position.x * 2) + ctrl_cx/2;
                    int y = (int)(si.position.y * 2) + ctrl_cy/2;

                    // Draw a rectangle representing the sprite
                    int width = (int)(sprite->region.cx * 2);
                    int height = (int)(sprite->region.cy * 2);

                    w.DrawRect(x - width/2, y - height/2, width, height,
                              RGBA(100 + (i * 50) % 155, 100 + (i * 75) % 155, 200, 180));

                    // Draw a small label with the sprite ID
                    w.DrawText(x, y, si.sprite_id.Left(8), StdFont(8), Black());
                }
            }

            // Draw collision rectangles if present
            for (int i = 0; i < frame->collisions.GetCount(); i++) {
                const CollisionRect& cr = frame->collisions[i];

                int x = (int)(cr.rect.x * 2) + ctrl_cx/2;
                int y = (int)(cr.rect.y * 2) + ctrl_cy/2;
                int width = (int)(cr.rect.cx * 2);
                int height = (int)(cr.rect.cy * 2);

                w.DrawRect(x, y, width, height,
                          RGBA(255, 0, 0, 100)); // Semi-transparent red
            }
        }
    }

    // Draw current frame info
    String frame_info = "Frame: " + IntStr(current_frame_index + 1) + "/" +
                       IntStr(animation->frames.GetCount());
    w.DrawText(5, GetSize().cy - 35, frame_info, StdFont(10), Black());
    
    // Draw playback status
    String status_info = is_playing ? (is_paused ? "PAUSED" : "PLAYING") : "STOPPED";
    w.DrawText(5, GetSize().cy - 20, "Status: " + status_info, StdFont(10), Black());
}

void EntityPreviewCtrl::Layout() {
    // Handle layout if needed
    Refresh(); // Redraw after layout
}