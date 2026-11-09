#ifndef _AnimEdit_CollisionListCtrl_h_
#define _AnimEdit_CollisionListCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class CollisionListCtrl : public Ctrl {
public:
    typedef CollisionListCtrl CLASSNAME;

    CollisionListCtrl();
    virtual ~CollisionListCtrl();

    void SetFrame(const AnimationFrame* frame);
    void RefreshList();

    // Selection
    int GetSelectedIndex() const { return selected_index; }
    const CollisionRect* GetSelectedCollision() const;

    // Events
    void SetSelectCallback(std::function<void(const CollisionRect*)> callback) { select_callback = callback; }
    void SetChangeCallback(std::function<void()> callback) { change_callback = callback; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseDown(Point pos, dword button);
    virtual void RightDown(Point pos, dword flags);

private:
    const AnimationFrame* frame;
    Vector<int> display_indices; // Indices of collision rectangles to display
    int selected_index;

    // Callbacks
    std::function<void(const CollisionRect*)> select_callback;
    std::function<void()> change_callback;

    int HitTest(Point pos) const; // Return index of item at position, or -1
    void DrawItem(Draw& w, int index, const Rect& rc) const;
    Size GetItemSize(int index) const;
    void PopupContextMenu(Point pos);

    void ApplyFilters();
};

#endif