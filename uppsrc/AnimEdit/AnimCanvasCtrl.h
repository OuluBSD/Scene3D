#ifndef _AnimEdit_AnimCanvasCtrl_h_
#define _AnimEdit_AnimCanvasCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>
#include <functional>

using namespace Upp;

class AnimCanvasCtrl : public Ctrl {
public:
    typedef AnimCanvasCtrl CLASSNAME;
    
    AnimCanvasCtrl();
    virtual ~AnimCanvasCtrl();

    // Setters
    void SetProject(const AnimationProject* project) { this->project = project; }
    void SetFrame(AnimationFrame* frame) { this->frame = frame; }
    void SetGridSpacing(double spacing);
    void SetShowGrid(bool show) { show_grid = show; }
    void SetShowCrosshair(bool show) { show_crosshair = show; }
    void SetGridSnapping(bool snapping) { grid_snapping = snapping; }
    void SetOriginSnapping(bool snapping) { origin_snapping = snapping; }
    
    // Getters
    double GetGridSpacing() const { return grid_spacing; }
    bool GetShowGrid() const { return show_grid; }
    bool GetShowCrosshair() const { return show_crosshair; }
    bool GetGridSnapping() const { return grid_snapping; }
    bool GetOriginSnapping() const { return origin_snapping; }
    
    // Coordinate transformation
    Point WorldToScreen(const Vec2& world_pos) const;
    Vec2 ScreenToWorld(const Point& screen_pos) const;
    
    // Zoom and pan
    void SetZoom(double zoom);
    double GetZoom() const { return zoom; }
    void SetPan(const Vec2& pan);
    Vec2 GetPan() const { return pan; }
    
    // Callbacks
    void SetZoomCallback(std::function<void()> callback) { zoom_callback = callback; }
    void SetGeneralCallback(std::function<void()> callback) { general_callback = callback; }
    
    // Undo/Redo functionality
    void Undo();
    void Redo();
    void PushUndoState();
    bool CanUndo() const { return undo_index >= 0; }
    bool CanRedo() const { return undo_index < undo_stack.GetCount() - 1; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseMove(Point pos, dword flags);
    virtual void LeftDown(Point pos, dword flags);
    virtual void LeftUp(Point pos, dword flags);
    virtual void MouseWheel(Point pos, dword flags, int zdelta);
    virtual bool Key(dword key, int count);
    virtual bool Drop(Point pos, PasteClip& d);
    virtual void RightDown(Point pos, dword flags);

private:
    // Project data
    const AnimationProject* project;
    AnimationFrame* frame;  // This is the Frame from AnimCore.h (anim frame, not Ctrl Frame)
    
    // Grid settings
    double grid_spacing;
    bool show_grid;
    bool show_crosshair;
    bool grid_snapping;
    bool origin_snapping;
    
    // View transformation
    double zoom;
    Vec2 pan;
    
    // Mouse interaction
    bool is_panning;
    bool is_dragging_instance;
    Point drag_start;
    Vec2 original_instance_pos;
    Point pan_start;
    Vec2 pan_offset_start;
    
    // Selection
    int selected_instance;
    
    // Callbacks
    std::function<void()> zoom_callback;
    std::function<void()> general_callback;
    
    // Undo/Redo
    Vector<AnimationFrame> undo_stack;  // Store frames for undo
    int undo_index;            // Current position in undo stack
    
    Vec2 SnapToPosition(const Vec2& pos) const;
    void DrawGrid(Draw& w);
    void DrawCrosshair(Draw& w);
    void DrawSpriteInstances(Draw& w);
    void DrawSelectionHighlight(Draw& w);
};

#endif