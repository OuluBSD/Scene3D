#ifndef _AnimEditLib_AnimResourceRegistry_h_
#define _AnimEditLib_AnimResourceRegistry_h_

#include <Core/Core.h>
#include "AnimCore.h"

namespace Upp {

// Forward declarations
struct Sprite;
struct AnimationFrame;
struct Animation;
struct AnimationProject;

// Resource registry that maintains runtime containers for all animation resources
class AnimResourceRegistry {
public:
    AnimResourceRegistry();
    ~AnimResourceRegistry();

    // Sprite management
    void AddSprite(Sprite sprite);
    void RemoveSprite(const String& id);
    const Sprite* GetSprite(const String& id) const;
    Vector<String> GetAllSpriteIds() const;
    void ClearSprites();
    
    // AnimationFrame management
    void AddFrame(AnimationFrame frame);
    void RemoveFrame(const String& id);
    const AnimationFrame* GetFrame(const String& id) const;
    Vector<String> GetAllFrameIds() const;
    void ClearFrames();
    
    // Animation management
    void AddAnimation(Animation anim);
    void RemoveAnimation(const String& id);
    const Animation* GetAnimation(const String& id) const;
    Vector<String> GetAllAnimationIds() const;
    void ClearAnimations();
    
    // Project management
    void LoadProject(AnimationProject project);
    void ClearProject();
    
    // Resource change notifications
    // Event fired when any resource is added
    Event<> WhenResourceAdded;
    // Event fired when any resource is removed
    Event<> WhenResourceRemoved;
    // Event fired when any resource is modified
    Event<> WhenResourceModified;
    
    // Specific resource type events
    Event<> WhenSpriteAdded;
    Event<> WhenSpriteRemoved;
    Event<> WhenSpriteModified;
    
    Event<> WhenFrameAdded;
    Event<> WhenFrameRemoved;
    Event<> WhenFrameModified;
    
    Event<> WhenAnimationAdded;
    Event<> WhenAnimationRemoved;
    Event<> WhenAnimationModified;

private:
    // Internal storage
    VectorMap<String, Sprite> sprites;
    VectorMap<String, AnimationFrame> frames;
    VectorMap<String, Animation> animations;
    
    // Helper methods
    void NotifyResourceAdded(const String& id, char resourceType);
    void NotifyResourceRemoved(const String& id, char resourceType);
    void NotifyResourceModified(const String& id, char resourceType);
};

} // namespace Upp

#endif