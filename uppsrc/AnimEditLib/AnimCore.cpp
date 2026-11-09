#include "AnimCore.h"

NAMESPACE_UPP

const Sprite* AnimationProject::FindSprite(const String& id) const {
    for (const auto& sprite : sprites) {
        if (sprite.id == id) {
            return &sprite;
        }
    }
    return nullptr;
}

Sprite* AnimationProject::FindSprite(const String& id) {
    for (auto& sprite : sprites) {
        if (sprite.id == id) {
            return &sprite;
        }
    }
    return nullptr;
}

const AnimationFrame* AnimationProject::FindFrame(const String& id) const {
    for (const auto& frame : frames) {
        if (frame.id == id) {
            return &frame;
        }
    }
    return nullptr;
}

AnimationFrame* AnimationProject::FindFrame(const String& id) {
    for (auto& frame : frames) {
        if (frame.id == id) {
            return &frame;
        }
    }
    return nullptr;
}

const Animation* AnimationProject::FindAnimation(const String& id) const {
    for (const auto& anim : animations) {
        if (anim.id == id) {
            return &anim;
        }
    }
    return nullptr;
}

Animation* AnimationProject::FindAnimation(const String& id) {
    for (auto& anim : animations) {
        if (anim.id == id) {
            return &anim;
        }
    }
    return nullptr;
}

const Entity* AnimationProject::FindEntity(const String& id) const {
    for (const auto& entity : entities) {
        if (entity.id == id) {
            return &entity;
        }
    }
    return nullptr;
}

Entity* AnimationProject::FindEntity(const String& id) {
    for (auto& entity : entities) {
        if (entity.id == id) {
            return &entity;
        }
    }
    return nullptr;
}

const BehaviorTree* AnimationProject::FindBehaviorTree(const String& id) const {
    for (const auto& bt : behavior_trees) {
        if (bt.id == id) {
            return &bt;
        }
    }
    return nullptr;
}

BehaviorTree* AnimationProject::FindBehaviorTree(const String& id) {
    for (auto& bt : behavior_trees) {
        if (bt.id == id) {
            return &bt;
        }
    }
    return nullptr;
}

const StateMachine* AnimationProject::FindStateMachine(const String& id) const {
    for (const auto& sm : state_machines) {
        if (sm.id == id) {
            return &sm;
        }
    }
    return nullptr;
}

StateMachine* AnimationProject::FindStateMachine(const String& id) {
    for (auto& sm : state_machines) {
        if (sm.id == id) {
            return &sm;
        }
    }
    return nullptr;
}

const AnimationCurve* AnimationProject::FindCurve(const String& id) const {
    for (const auto& curve : curves) {
        if (curve.id == id) {
            return &curve;
        }
    }
    return nullptr;
}

AnimationCurve* AnimationProject::FindCurve(const String& id) {
    for (auto& curve : curves) {
        if (curve.id == id) {
            return &curve;
        }
    }
    return nullptr;
}

const TriggerSystem* AnimationProject::FindTriggerSystem(const String& id) const {
    for (const auto& system : trigger_systems) {
        if (system.id == id) {
            return &system;
        }
    }
    return nullptr;
}

TriggerSystem* AnimationProject::FindTriggerSystem(const String& id) {
    for (auto& system : trigger_systems) {
        if (system.id == id) {
            return &system;
        }
    }
    return nullptr;
}

END_UPP_NAMESPACE