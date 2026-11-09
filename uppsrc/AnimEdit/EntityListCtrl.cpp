#include "EntityListCtrl.h"

EntityListCtrl::EntityListCtrl() {
    project = nullptr;
    sort_type = SORT_BY_NAME;
    
    Add(array_ctrl.HSizePos().VSizePos());
    array_ctrl.AddColumn("Name", 120);
    array_ctrl.AddColumn("ID", 100);
    array_ctrl.AddColumn("Type", 80);
    array_ctrl.NoHeader();
    
    array_ctrl.WhenSel = [this]() {
        if (select_callback && project && array_ctrl.GetCursor() >= 0 && array_ctrl.GetCursor() < project->entities.GetCount()) {
            select_callback(&project->entities[array_ctrl.GetCursor()]);
        }
    };
}

EntityListCtrl::~EntityListCtrl() {
}

void EntityListCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
    RebuildList();
}

void EntityListCtrl::SetSelectCallback(std::function<void(const Entity*)> callback) {
    select_callback = callback;
}

void EntityListCtrl::SetFilterText(const String& text) {
    filter_text = text;
    RebuildList();
}

void EntityListCtrl::SetCategoryFilter(const String& category) {
    category_filter = category;
    RebuildList();
}

void EntityListCtrl::SetSortType(SortType sort_type) {
    this->sort_type = sort_type;
    RebuildList();
}

void EntityListCtrl::RefreshList() {
    RebuildList();
}

Vector<int> EntityListCtrl::GetFilteredEntityIndices() const {
    Vector<int> indices;
    
    if (!project) return indices;
    
    for (int i = 0; i < project->entities.GetCount(); i++) {
        const Entity& entity = project->entities[i];
        
        // Apply category filter if set
        if (!category_filter.IsEmpty() && category_filter != "All Categories" && entity.type != category_filter) {
            continue;
        }
        
        // Apply text filter
        if (!filter_text.IsEmpty()) {
            String lower_filter = ToLower(filter_text);
            if (ToLower(entity.id).Find(lower_filter) == -1 && 
                ToLower(entity.name).Find(lower_filter) == -1) {
                continue;
            }
        }
        
        indices.Add(i);
    }
    
    // Sort the indices based on sort type
    switch (sort_type) {
        case SORT_BY_NAME:
            Sort(indices, [=](int a, int b) {
                return ToLower(project->entities[a].name) < ToLower(project->entities[b].name);
            });
            break;
        case SORT_BY_ID:
            Sort(indices, [=](int a, int b) {
                return ToLower(project->entities[a].id) < ToLower(project->entities[b].id);
            });
            break;
        case SORT_BY_TYPE:
            Sort(indices, [=](int a, int b) {
                return ToLower(project->entities[a].type) < ToLower(project->entities[b].type);
            });
            break;
        case SORT_BY_RECENT_USE:
            // For now, keep the original order (recent would need additional tracking)
            break;
    }
    
    return indices;
}

void EntityListCtrl::RebuildList() {
    array_ctrl.Clear();
    
    if (!project) return;
    
    Vector<int> filtered_indices = GetFilteredEntityIndices();
    
    for (int idx : filtered_indices) {
        const Entity& entity = project->entities[idx];
        array_ctrl.Add(entity.name, entity.id, entity.type);
    }
}

void EntityListCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), White());
}

void EntityListCtrl::Layout() {
    // Handle layout if needed
}