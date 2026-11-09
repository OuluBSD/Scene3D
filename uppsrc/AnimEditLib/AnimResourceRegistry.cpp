#include "AnimResourceRegistry.h"
#include "AnimCore.h"
#include "AnimUtils.h"

namespace Upp {

AnimResourceRegistry::AnimResourceRegistry() {
    // Constructor - nothing specific needed
}

AnimResourceRegistry::~AnimResourceRegistry() {
    // Destructor - cleanup handled by containers automatically
}

// Sprite management
void AnimResourceRegistry::AddSprite(Sprite sprite) {
    if (sprite.id.IsEmpty()) {
        // Don't add sprites without IDs
        return;
    }
    
    sprites.Add(sprite.id, pick(sprite));  // Use pick to move it to VectorMap
    NotifyResourceAdded(sprite.id, 'S');  // S for Sprite
}

void AnimResourceRegistry::RemoveSprite(const String& id) {
    if (sprites.Find(id) >= 0) {
        sprites.RemoveKey(id);
        NotifyResourceRemoved(id, 'S');  // S for Sprite
    }
}

const Sprite* AnimResourceRegistry::GetSprite(const String& id) const {
    int idx = sprites.Find(id);
    if (idx >= 0) {
        return &sprites[idx];
    }
    return nullptr;
}

Vector<String> AnimResourceRegistry::GetAllSpriteIds() const {
    Vector<String> ids;
    for (int i = 0; i < sprites.GetCount(); i++) {
        ids.Add(sprites.GetKey(i));
    }
    return ids;
}

void AnimResourceRegistry::ClearSprites() {
    Vector<String> allIds = GetAllSpriteIds();
    for (int i = 0; i < allIds.GetCount(); i++) {
        NotifyResourceRemoved(allIds[i], 'S');
    }
    sprites.Clear();
}

// AnimationFrame management
void AnimResourceRegistry::AddFrame(AnimationFrame frame) {
    if (frame.id.IsEmpty()) {
        // Don't add frames without IDs
        return;
    }
    
    frames.Add(frame.id, pick(frame));  // Use pick to move it to VectorMap
    NotifyResourceAdded(frame.id, 'F');  // F for AnimationFrame
}

void AnimResourceRegistry::RemoveFrame(const String& id) {
    if (frames.Find(id) >= 0) {
        frames.RemoveKey(id);
        NotifyResourceRemoved(id, 'F');  // F for AnimationFrame
    }
}

const AnimationFrame* AnimResourceRegistry::GetFrame(const String& id) const {
    int idx = frames.Find(id);
    if (idx >= 0) {
        return &frames[idx];
    }
    return nullptr;
}

Vector<String> AnimResourceRegistry::GetAllFrameIds() const {
    Vector<String> ids;
    for (int i = 0; i < frames.GetCount(); i++) {
        ids.Add(frames.GetKey(i));
    }
    return ids;
}

void AnimResourceRegistry::ClearFrames() {
    Vector<String> allIds = GetAllFrameIds();
    for (int i = 0; i < allIds.GetCount(); i++) {
        NotifyResourceRemoved(allIds[i], 'F');
    }
    frames.Clear();
}

// Animation management
void AnimResourceRegistry::AddAnimation(Animation anim) {
    if (anim.id.IsEmpty()) {
        // Don't add animations without IDs
        return;
    }
    
    animations.Add(anim.id, pick(anim));  // Use pick to move it to VectorMap
    NotifyResourceAdded(anim.id, 'A');  // A for Animation
}

void AnimResourceRegistry::RemoveAnimation(const String& id) {
    if (animations.Find(id) >= 0) {
        animations.RemoveKey(id);
        NotifyResourceRemoved(id, 'A');  // A for Animation
    }
}

const Animation* AnimResourceRegistry::GetAnimation(const String& id) const {
    int idx = animations.Find(id);
    if (idx >= 0) {
        return &animations[idx];
    }
    return nullptr;
}

Vector<String> AnimResourceRegistry::GetAllAnimationIds() const {
    Vector<String> ids;
    for (int i = 0; i < animations.GetCount(); i++) {
        ids.Add(animations.GetKey(i));
    }
    return ids;
}

void AnimResourceRegistry::ClearAnimations() {
    Vector<String> allIds = GetAllAnimationIds();
    for (int i = 0; i < allIds.GetCount(); i++) {
        NotifyResourceRemoved(allIds[i], 'A');
    }
    animations.Clear();
}

// Project management
void AnimResourceRegistry::LoadProject(AnimationProject project) {
    // Clear existing resources first
    ClearProject();
    
    // Add all sprites from the project
    for (int i = 0; i < project.sprites.GetCount(); i++) {
        AddSprite(pick(project.sprites[i]));  // Use pick to move
    }
    
    // Add all frames from the project
    for (int i = 0; i < project.frames.GetCount(); i++) {
        AddFrame(pick(project.frames[i]));  // Use pick to move
    }
    
    // Add all animations from the project
    for (int i = 0; i < project.animations.GetCount(); i++) {
        AddAnimation(pick(project.animations[i]));  // Use pick to move
    }
}

void AnimResourceRegistry::ClearProject() {
    ClearSprites();
    ClearFrames();
    ClearAnimations();
}

// Helper methods for notifications
void AnimResourceRegistry::NotifyResourceAdded(const String& id, char resourceType) {
    WhenResourceAdded();
    
    switch (resourceType) {
        case 'S': {
            auto& event = WhenSpriteAdded;
            event();
            break;
        }
        case 'F': {
            auto& event = WhenFrameAdded;
            event();
            break;
        }
        case 'A': {
            auto& event = WhenAnimationAdded;
            event();
            break;
        }
    }
}

void AnimResourceRegistry::NotifyResourceRemoved(const String& id, char resourceType) {
    WhenResourceRemoved();
    
    switch (resourceType) {
        case 'S': {
            auto& event = WhenSpriteRemoved;
            event();
            break;
        }
        case 'F': {
            auto& event = WhenFrameRemoved;
            event();
            break;
        }
        case 'A': {
            auto& event = WhenAnimationRemoved;
            event();
            break;
        }
    }
}

void AnimResourceRegistry::NotifyResourceModified(const String& id, char resourceType) {
    WhenResourceModified();
    
    switch (resourceType) {
        case 'S': {
            auto& event = WhenSpriteModified;
            event();
            break;
        }
        case 'F': {
            auto& event = WhenFrameModified;
            event();
            break;
        }
        case 'A': {
            auto& event = WhenAnimationModified;
            event();
            break;
        }
    }
}

} // namespace Upp