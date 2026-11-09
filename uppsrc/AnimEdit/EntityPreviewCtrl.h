#ifndef _AnimEdit_EntityPreviewCtrl_h_
#define _AnimEdit_EntityPreviewCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class EntityPreviewCtrl : public Ctrl {
public:
    typedef EntityPreviewCtrl CLASSNAME;

    EntityPreviewCtrl();
    virtual ~EntityPreviewCtrl();

    void SetProject(const AnimationProject* project);
    void SetEntity(const Entity* entity);
    void SetAnimation(const Animation* animation);
    
    // Animation control methods
    void StartAnimation();
    void PauseAnimation();
    void StopAnimation();
    void SetLoopEnabled(bool enabled);
    void SetPlaybackSpeed(double speed);
    
    // Getters
    const Animation* GetAnimation() const { return animation; }
    bool IsPlaying() const { return is_playing; }

private:
    const AnimationProject* project;
    const Entity* entity;
    const Animation* animation;
    int current_frame_index;
    bool is_playing;
    bool is_paused;
    bool loop_enabled;
    double playback_speed;
    TimeCallback animation_timer;

    void UpdateAnimation();
    void OnTimer();

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
};

#endif