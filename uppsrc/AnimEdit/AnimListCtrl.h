#ifndef _AnimEdit_AnimListCtrl_h_
#define _AnimEdit_AnimListCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class AnimListCtrl : public Ctrl {
public:
    typedef AnimListCtrl CLASSNAME;

    AnimListCtrl();
    virtual ~AnimListCtrl();

    void SetProject(const AnimationProject* project);
    void RefreshList();

    // Selection
    int GetSelectedIndex() const { return selected_index; }
    const Animation* GetSelectedAnimation() const;

    // Events
    void SetSelectCallback(std::function<void(const Animation*)> callback) { select_callback = callback; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseDown(Point pos, dword button);
    virtual void MouseMove(Point pos, dword keyflags);
    virtual void RightDown(Point pos, dword flags);

private:
    const AnimationProject* project;
    Vector<int> display_indices; // Indices of animations to display after filtering
    int selected_index;

    // For dragging
    bool is_dragging;
    Point drag_start;

    // For filtering and searching
    String filter_text;

    void SetFilterText(const String& text);

    // Callbacks
    std::function<void(const Animation*)> select_callback;

    enum SortType {
        SORT_BY_NAME = 0,
        SORT_BY_ID,
        SORT_BY_CATEGORY,
        SORT_BY_RECENT_USE
    };

    void ApplyFilters();
    int HitTest(Point pos) const; // Return index of item at position, or -1
    void DrawItem(Draw& w, int index, const Rect& rc) const;
    Size GetItemSize(int index) const;
    void PopupContextMenu(Point pos);
    String GenerateUniqueIdForAnimation(const String& baseId);

    // Sorting
    void SetSortType(SortType sortType);
    SortType GetSortType() const { return sort_type; }
    void SortAnimations();

private:
    SortType sort_type;
};

#endif