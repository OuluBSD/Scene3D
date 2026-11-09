#ifndef _AnimEdit_FrameListCtrl_h_
#define _AnimEdit_FrameListCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class FrameListCtrl : public Ctrl {
public:
    typedef FrameListCtrl CLASSNAME;

    FrameListCtrl();
    virtual ~FrameListCtrl();

    void SetProject(const AnimationProject* project);
    void SetAnimation(const Animation* animation); // To identify which frames are used by this animation
    void RefreshList();

    // Selection
    int GetSelectedIndex() const { return selected_index; }
    const AnimationFrame* GetSelectedFrame() const;

    // Events
    void SetSelectCallback(std::function<void(const AnimationFrame*)> callback) { select_callback = callback; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseDown(Point pos, dword button);
    virtual void MouseMove(Point pos, dword keyflags);
    virtual void RightDown(Point pos, dword flags);

private:
    const AnimationProject* project;
    const Animation* animation; // For reference to see which frames are in current animation
    Vector<int> display_indices; // Indices of frames to display after filtering
    int selected_index;

    // For dragging
    bool is_dragging;
    Point drag_start;

    // For filtering and searching
    String filter_text;

    void SetFilterText(const String& text);

    // Callbacks
    std::function<void(const AnimationFrame*)> select_callback;

    enum SortType {
        SORT_BY_NAME = 0,
        SORT_BY_ID,
        SORT_BY_RECENT_USE
    };

    void ApplyFilters();
    int HitTest(Point pos) const; // Return index of item at position, or -1
    void DrawItem(Draw& w, int index, const Rect& rc) const;
    Size GetItemSize(int index) const;
    void PopupContextMenu(Point pos);
    String GenerateUniqueIdForFrame(const String& baseId);

    // Sorting
    void SetSortType(SortType sortType);
    SortType GetSortType() const { return sort_type; }
    void SortFrames();

private:
    SortType sort_type;
};

#endif