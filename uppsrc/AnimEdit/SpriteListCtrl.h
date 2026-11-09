#ifndef _AnimEdit_SpriteListCtrl_h_
#define _AnimEdit_SpriteListCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class SpriteListCtrl : public Ctrl {
public:
    typedef SpriteListCtrl CLASSNAME;
    
    SpriteListCtrl();
    virtual ~SpriteListCtrl();

    void SetProject(const AnimationProject* project);
    void SetFrame(const AnimationFrame* frame); // Needed to check which sprites are in the current frame
    void RefreshList();
    
    // Selection
    int GetSelectedIndex() const { return selected_index; }
    const Sprite* GetSelectedSprite() const;
    
    // Events
    void SetSelectCallback(std::function<void(const Sprite*)> callback) { select_callback = callback; }

protected:
    virtual void Paint(Draw& w);
    virtual void MouseDown(Point pos, dword button);
    virtual void MouseMove(Point pos, dword keyflags);
    virtual void RightDown(Point pos, dword flags);

private:
    const AnimationProject* project;
    const AnimationFrame* frame; // For reference to see which sprites are in current frame
    Vector<int> display_indices; // Indices of sprites to display after filtering
    int selected_index;
    
    // For dragging
    bool is_dragging;
    Point drag_start;
    
    // For filtering and searching
    String filter_text;
    String category_filter;
    
    // For image loading and caching
    VectorMap<String, Image> texture_cache;
    int cache_size_limit = 50; // Limit for the texture cache
    
    void SetFilterText(const String& text);
    void SetCategoryFilter(const String& category);
    
    // Image caching methods
    bool LoadAndCacheTexture(const String& texture_path, Image& img);
    void ClearTextureCache();
    
    // Callbacks
    std::function<void(const Sprite*)> select_callback;

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
    String GenerateUniqueIdForSprite(const String& baseId);
    
    // Sorting
    void SetSortType(SortType sortType);
    SortType GetSortType() const { return sort_type; }
    void SortSprites();
    
private:
    SortType sort_type;
};

#endif