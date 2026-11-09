#include "BehaviorTreeEditorCtrl.h"

// BehaviorTreeNodeCtrl implementation
BehaviorTreeNodeCtrl::BehaviorTreeNodeCtrl() {
    bt_node = nullptr;
    position = Point(0, 0);
    // Set minimum size for the node
    SetRect(0, 0, 120, 60);
}

BehaviorTreeNodeCtrl::~BehaviorTreeNodeCtrl() {
}

void BehaviorTreeNodeCtrl::SetNode(const BTNode* node) {
    bt_node = node;
    Refresh();
}

void BehaviorTreeNodeCtrl::Paint(Draw& w) {
    if (!bt_node) {
        return;
    }
    
    // Draw node background based on type
    Color bgColor;
    switch (bt_node->node_type) {
        case BTNodeType::ACTION:     bgColor = Color(180, 220, 255); break;  // Light blue
        case BTNodeType::CONDITION:  bgColor = Color(180, 255, 180); break;  // Light green
        case BTNodeType::SEQUENCE:   bgColor = Color(255, 220, 180); break;  // Light orange
        case BTNodeType::SELECTOR:   bgColor = Color(255, 180, 180); break;  // Light red
        case BTNodeType::DECORATOR:  bgColor = Color(220, 180, 255); break;  // Light purple
        case BTNodeType::ROOT:       bgColor = Color(255, 255, 180); break;  // Light yellow
        default:                     bgColor = GrayColor(); break;
    }
    
    // Draw the node rectangle
    w.DrawRect(GetSize(), bgColor);
    
    // Draw border
    w.DrawRect(GetSize(), Black());
    
    // Draw node name
    w.DrawText(5, 5, bt_node->name, StdFont(10), Black());
    
    // Draw node type
    w.DrawText(5, 20, bt_node->type_name, StdFont(8), DarkGray());
}

void BehaviorTreeNodeCtrl::LeftDown(Point p, dword keyflags) {
    if (on_select) {
        on_select(this);
    }
}

void BehaviorTreeNodeCtrl::Layout() {
    // Nothing needed for this simple control
}

// BehaviorTreeEditorCtrl implementation
BehaviorTreeEditorCtrl::BehaviorTreeEditorCtrl() {
    project = nullptr;
    current_bt = nullptr;
    current_bt_copy = nullptr;
    
    canvas_offset = Point(0, 0);
    canvas_zoom = 1.0;
    
    // Set up the layout
    CtrlLayout(*this);
    
    // Initialize the splitter
    Add(splitter.SizePos());
    splitter.Vert();
    
    // Add the three panels to the splitter
    Ctrl& left_panel = node_palette;
    Ctrl& center_panel = tree_view;
    Ctrl& right_panel = properties_panel;
    
    splitter << left_panel << center_panel << right_panel;
    splitter.SetPos(1, 3000, 1);  // Left and right panels get 30% each, center gets 40%
    
    // Initialize UI components
    BuildToolbar();
    BuildNodePalette();
    BuildPropertiesPanel();
    
    // Set up the tree view as a canvas area
    tree_view.WhenLeftDown = [this](Point p, dword keyflags) {
        // Handle canvas interaction
    };
}

BehaviorTreeEditorCtrl::~BehaviorTreeEditorCtrl() {
    delete current_bt_copy;
}

void BehaviorTreeEditorCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
}

void BehaviorTreeEditorCtrl::SetBehaviorTree(const BehaviorTree* bt) {
    current_bt = bt;
    
    // Create a copy for editing
    delete current_bt_copy;
    if (bt) {
        current_bt_copy = new BehaviorTree(*bt);
    } else {
        current_bt_copy = new BehaviorTree();
    }
    
    RefreshTreeView();
}

void BehaviorTreeEditorCtrl::BuildToolbar() {
    // Create toolbar with common actions
    toolbar.Add("New", CtrlImg::newg(), [this] { 
        // Create new behavior tree
        delete current_bt_copy;
        current_bt_copy = new BehaviorTree("new_bt_" + Uuid().ToString().Left(8), "New Behavior Tree");
        RefreshTreeView();
    }).Tip("Create new behavior tree");
    
    toolbar.Add("Save", CtrlImg::save(), [this] { 
        // Save the current tree
        OnSaveTree();
    }).Tip("Save behavior tree");
    
    toolbar.Add("Load", CtrlImg::open(), [this] { 
        // Load a behavior tree
        OnLoadTree();
    }).Tip("Load behavior tree");
    
    toolbar.Separator();
    
    // Add node creation buttons
    toolbar.Add("Action", CtrlImg::file(), [this] { 
        OnAddNode(BTNodeType::ACTION);
    }).Tip("Add Action Node");
    
    toolbar.Add("Condition", CtrlImg::file(), [this] { 
        OnAddNode(BTNodeType::CONDITION);
    }).Tip("Add Condition Node");
    
    toolbar.Add("Sequence", CtrlImg::file(), [this] { 
        OnAddNode(BTNodeType::SEQUENCE);
    }).Tip("Add Sequence Node");
    
    toolbar.Add("Selector", CtrlImg::file(), [this] { 
        OnAddNode(BTNodeType::SELECTOR);
    }).Tip("Add Selector Node");
}

void BehaviorTreeEditorCtrl::BuildNodePalette() {
    // Create a simple list of available node types
    ArrayCtrl* list = new ArrayCtrl();
    node_palette.Add(*list);
    list->NoHeader();
    list->SetFrame(ThinInsetFrame());
    list->AddColumn("Node Type", 120);
    
    // Add different node types
    list->Add("Action");
    list->Add("Condition");
    list->Add("Sequence");
    list->Add("Selector");
    list->Add("Decorator");
    list->Add("Root");
    
    list->WhenLeftDouble = [this, list]() {
        String selected = list->Get(0, list->GetCursor());
        BTNodeType node_type;
        
        if (selected == "Action") node_type = BTNodeType::ACTION;
        else if (selected == "Condition") node_type = BTNodeType::CONDITION;
        else if (selected == "Sequence") node_type = BTNodeType::SEQUENCE;
        else if (selected == "Selector") node_type = BTNodeType::SELECTOR;
        else if (selected == "Decorator") node_type = BTNodeType::DECORATOR;
        else if (selected == "Root") node_type = BTNodeType::ROOT;
        else return;
        
        OnAddNode(node_type);
    };
}

void BehaviorTreeEditorCtrl::BuildPropertiesPanel() {
    // Create a simple property grid for node properties
    CtrlLayout(properties_panel);
    
    // This would contain controls to edit node properties when a node is selected
    // For now, we'll just add a placeholder
    StaticRect* placeholder = new StaticRect();
    placeholder->SetLabel("Node Properties");
    placeholder->SetFrame(ThinInsetFrame());
    properties_panel.Add(*placeholder);
}

void BehaviorTreeEditorCtrl::RefreshTreeView() {
    // Clear existing nodes
    for (BehaviorTreeNodeCtrl* node_ctrl : node_controls) {
        node_ctrl->Remove();
        delete node_ctrl;
    }
    node_controls.Clear();
    
    if (!current_bt_copy) return;
    
    // Create controls for each node in the tree
    for (int i = 0; i < current_bt_copy->nodes.GetCount(); i++) {
        const BTNode& node = current_bt_copy->nodes[i];
        
        BehaviorTreeNodeCtrl* node_ctrl = new BehaviorTreeNodeCtrl();
        node_ctrl->SetNode(&node);
        node_ctrl->SetSelectionCallback([this](BehaviorTreeNodeCtrl* ctrl) {
            OnNodeSelected(ctrl);
        });
        
        // Add to the tree view (center panel)
        tree_view.Add(*node_ctrl);
        node_controls.Add(node_ctrl);
        
        // Position the node (this is a simplified positioning)
        node_ctrl->SetPosition(Point(100 + (i % 5) * 140, 50 + (i / 5) * 80));
        node_ctrl->SetRect(node_ctrl->GetPosition().x, 
                          node_ctrl->GetPosition().y, 
                          120, 60);
    }
    
    tree_view.Refresh();
}

void BehaviorTreeEditorCtrl::OnNodeSelected(BehaviorTreeNodeCtrl* node) {
    // Update the properties panel with the selected node's properties
    // This is where we would populate the properties panel with controls
    // for editing the selected node's parameters
}

void BehaviorTreeEditorCtrl::OnNodePropertyChanged() {
    // When a node property changes, update the internal representation
    // and potentially trigger a refresh
}

void BehaviorTreeEditorCtrl::OnAddNode(BTNodeType node_type) {
    if (!current_bt_copy) return;
    
    // Create a new node with a default name based on type
    BTNode new_node;
    new_node.id = "node_" + Uuid().ToString().Left(8);
    
    switch (node_type) {
        case BTNodeType::ACTION:
            new_node.name = "Action";
            new_node.type_name = "DoAction";
            break;
        case BTNodeType::CONDITION:
            new_node.name = "Condition";
            new_node.type_name = "CheckCondition";
            break;
        case BTNodeType::SEQUENCE:
            new_node.name = "Sequence";
            new_node.type_name = "SequenceNode";
            break;
        case BTNodeType::SELECTOR:
            new_node.name = "Selector";
            new_node.type_name = "SelectorNode";
            break;
        case BTNodeType::DECORATOR:
            new_node.name = "Decorator";
            new_node.type_name = "DecoratorNode";
            break;
        case BTNodeType::ROOT:
            new_node.name = "Root";
            new_node.type_name = "RootNode";
            break;
    }
    
    new_node.node_type = node_type;
    
    // Add to the tree
    current_bt_copy->nodes.Add(new_node);
    
    // Refresh the view
    RefreshTreeView();
}

void BehaviorTreeEditorCtrl::OnDeleteNode() {
    // Implementation for deleting selected node
    // This would require keeping track of the selected node
}

void BehaviorTreeEditorCtrl::OnSaveTree() {
    // Implementation for saving the behavior tree back to the project
    // This would prompt the user for a name and update the project
}

void BehaviorTreeEditorCtrl::OnLoadTree() {
    // Implementation for loading a behavior tree
    // This would show a dialog to select a tree to load
}

void BehaviorTreeEditorCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), White());
}

void BehaviorTreeEditorCtrl::MouseMove(Point p, dword keyflags) {
    // Handle pan/drag functionality if needed
}

void BehaviorTreeEditorCtrl::LeftDown(Point p, dword keyflags) {
    // Handle selection or panning
}

void BehaviorTreeEditorCtrl::LeftUp(Point p, dword keyflags) {
    // Handle end of drag operations
}

void BehaviorTreeEditorCtrl::MouseWheel(Point p, int zdelta, dword keyflags) {
    // Handle zoom if needed
    if (keyflags & K_CTRL) {  // Zoom with Ctrl+Wheel
        canvas_zoom += (zdelta > 0) ? 0.1 : -0.1;
        canvas_zoom = max(0.1, min(3.0, canvas_zoom)); // Clamp zoom
        RefreshTreeView(); // Redraw with new zoom
    }
}

void BehaviorTreeEditorCtrl::Layout() {
    // Layout the toolbar and main content
    Size sz = GetSize();
    
    // Toolbar at the top
    toolbar.SetRect(0, 0, sz.cx, 30);
    
    // Main content below the toolbar
    splitter.SetRect(0, 30, sz.cx, sz.cy - 30);
}