#ifndef _AnimEditLib_AnimSerialize_h_
#define _AnimEditLib_AnimSerialize_h_

#include <Core/Core.h>
#include "AnimCore.h"

namespace Upp {

// Jsonize functions for serialization
void Jsonize(JsonIO& jio, Vec2& v);
void Jsonize(JsonIO& jio, RectF& r);
void Jsonize(JsonIO& jio, Sprite& s);
void Jsonize(JsonIO& jio, SpriteInstance& si);
void Jsonize(JsonIO& jio, CollisionRect& cr);
void Jsonize(JsonIO& jio, AnimationFrame& f);
void Jsonize(JsonIO& jio, AnimationFrameRef& fr);
void Jsonize(JsonIO& jio, Animation& a);
void Jsonize(JsonIO& jio, AnimationEvent& event);
void Jsonize(JsonIO& jio, AnimationTransition& transition);
void Jsonize(JsonIO& jio, AnimationBlendParams& params);
void Jsonize(JsonIO& jio, EntityAnimationParams& params);
void Jsonize(JsonIO& jio, NamedAnimationSlot& slot);
void Jsonize(JsonIO& jio, Entity& e);
void Jsonize(JsonIO& jio, AnimationProject& p);

// Convenience functions
String SaveProjectJson(const AnimationProject& p);
bool   LoadProjectJson(AnimationProject& p, const String& json);

} // namespace Upp

#endif