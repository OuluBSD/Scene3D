#ifndef _AnimEdit_EntityListCtrl_h_
#define _AnimEdit_EntityListCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

class EntityListCtrl : public Ctrl {
public:
    typedef EntityListCtrl CLASSNAME;

    enum SortType {
        SORT_BY_NAME,
        SORT_BY_ID,
        SORT_BY_TYPE,
        SORT_BY_RECENT_USE
    };

    EntityListCtrl();
    virtual ~EntityListCtrl();

    void SetProject(const AnimationProject* project);
    void SetSelectCallback(std::function<void(const Entity*)> callback);
    void SetFilterText(const String& text);
    void SetCategoryFilter(const String& category);
    void SetSortType(SortType sort_type);
    void RefreshList();
    int GetCursor() const { return array_ctrl.GetCursor(); }

private:
    const AnimationProject* project;
    std::function<void(const Entity*)> select_callback;
    String filter_text;
    String category_filter;
    SortType sort_type;
    ArrayCtrl array_ctrl;

    void RebuildList();
    Vector<int> GetFilteredEntityIndices() const;

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
};

#endif