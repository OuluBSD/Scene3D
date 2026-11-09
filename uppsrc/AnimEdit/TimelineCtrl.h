#ifndef _AnimEdit_AnimEditTimelineCtrl_h_
#define _AnimEdit_AnimEditTimelineCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>
#include <functional>

using namespace Upp;

// Define a type alias to avoid Frame name collision
using AnimFrame = AnimationFrame;  // The Frame from AnimCore.h (animation frame)

class AnimEditTimelineCtrl : public Ctrl {
public:
    typedef AnimEditTimelineCtrl CLASSNAME;

    AnimEditTimelineCtrl();
    virtual ~AnimEditTimelineCtrl();

    void SetProject(const AnimationProject* project);
    void SetAnimation(const Animation* animation);
    void SetFrameCallback(std::function<void(const AnimationFrame*)> callback);
    void SetOnFrameModified(std::function<void()> callback);
    
    int GetSelectedFrameIndex() const { return selected_frame_index; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseDown(Point pos, dword button);
    virtual void MouseMove(Point pos, dword keyflags);
    virtual void LeftDown(Point pos, dword flags);
    virtual void LeftUp(Point pos, dword flags);
    virtual bool Key(dword key, int count);

private:
    const AnimationProject* project;
    const Animation* animation;
    int selected_frame_index;
    int drag_start_index;
    int drag_current_index;
    bool is_dragging;
    bool is_editing_duration = false;
    int editing_frame_index = -1;
    int duration_editor_x = 0;
    int duration_editor_y = 0;

    // Callbacks
    std::function<void(const AnimationFrame*)> frame_callback;
    std::function<void()> on_frame_modified_callback;

    // Layout
    int frame_width;
    int frame_height;
    int frame_spacing;

    // Methods
    int HitTest(Point pos) const; // Returns frame index or -1
    int HitTestDurationControl(Point pos) const; // Returns frame index if duration control is clicked
    void DrawFrame(Draw& w, int index, const Rect& rc);
    void RefreshLayout();
    void UpdateScroll();
    void SelectFrame(int index);
    void ReorderFrame(int from_index, int to_index);
    void StartDrag(int index);
    void EndDrag();
    bool IsDragThresholdExceeded(Point pos);
    
    // Duration editing
    void ShowDurationEditor(int frame_index, int x, int y);
    void OnDurationChanged(int frame_index, double new_duration);
};

#endif