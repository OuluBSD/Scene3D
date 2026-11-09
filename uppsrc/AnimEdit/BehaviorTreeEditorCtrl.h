#ifndef _AnimEdit_BehaviorTreeEditorCtrl_h_
#define _AnimEdit_BehaviorTreeEditorCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

// Forward declaration
class BehaviorTreeNodeCtrl;

class BehaviorTreeEditorCtrl : public Ctrl {
public:
    typedef BehaviorTreeEditorCtrl CLASSNAME;

    BehaviorTreeEditorCtrl();
    virtual ~BehaviorTreeEditorCtrl();

    void SetProject(const AnimationProject* project);
    void SetBehaviorTree(const BehaviorTree* bt);
    const BehaviorTree* GetBehaviorTree() const { return current_bt; }

    // For JSON serialization
    void Jsonize(JsonIO& json) {
        json("behavior_tree", *current_bt_copy);
    }

private:
    const AnimationProject* project;
    const BehaviorTree* current_bt;      // Points to the BT in the project
    BehaviorTree* current_bt_copy;       // Editable copy
    
    // UI elements
    ToolBar toolbar;
    Splitter splitter;
    Ctrl node_palette;  // Left panel with available node types
    Ctrl tree_view;     // Main canvas area for the behavior tree
    Ctrl properties_panel; // Right panel for node properties

    // Canvas-related
    Point canvas_offset;
    double canvas_zoom;
    Vector<BehaviorTreeNodeCtrl*> node_controls;
    
    // Methods
    void BuildToolbar();
    void BuildNodePalette();
    void BuildPropertiesPanel();
    void RefreshTreeView();
    void OnNodeSelected(BehaviorTreeNodeCtrl* node);
    void OnNodePropertyChanged();
    void OnAddNode(BTNodeType node_type);
    void OnDeleteNode();
    void OnSaveTree();
    void OnLoadTree();

    // Override Ctrl methods
    virtual void Paint(Draw& w) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual void Layout() override;
};

// Individual node control for the behavior tree view
class BehaviorTreeNodeCtrl : public Ctrl {
public:
    typedef BehaviorTreeNodeCtrl CLASSNAME;

    BehaviorTreeNodeCtrl();
    virtual ~BehaviorTreeNodeCtrl();

    void SetNode(const BTNode* node);
    const BTNode* GetNode() const { return bt_node; }
    void SetPosition(Point pos) { position = pos; }
    Point GetPosition() const { return position; }

    // Event callback
    void SetSelectionCallback(std::function<void(BehaviorTreeNodeCtrl*)> cb) { 
        on_select = cb; 
    }

private:
    const BTNode* bt_node;
    Point position;  // Position in the canvas
    std::function<void(BehaviorTreeNodeCtrl*)> on_select;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void Layout() override;
};

#endif