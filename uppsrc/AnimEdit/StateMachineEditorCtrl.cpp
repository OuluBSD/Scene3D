#include "StateMachineEditorCtrl.h"

// StateNodeCtrl implementation
StateNodeCtrl::StateNodeCtrl() {
    state_node = nullptr;
    position = Point(0, 0);
    // Set minimum size for the state
    SetRect(0, 0, 120, 80);
}

StateNodeCtrl::~StateNodeCtrl() {
}

void StateNodeCtrl::SetState(const StateNode* state) {
    state_node = state;
    Refresh();
}

void StateNodeCtrl::Paint(Draw& w) {
    if (!state_node) {
        return;
    }
    
    // Draw state background
    Color bgColor = Color(220, 220, 255);  // Light blue-gray
    
    // Draw the state rectangle with rounded corners
    w.DrawRect(GetSize(), bgColor);
    w.DrawRect(GetSize(), Black());
    
    // Draw state name
    w.DrawText(5, 5, state_node->name, StdFont(10), Black());
    
    // Draw state type
    w.DrawText(5, 20, state_node->type, StdFont(8), DarkGray());
    
    // Draw an inner rectangle to indicate it's a state
    w.DrawRect(5, 35, GetSize().cx - 10, 40, White());
    w.DrawRect(5, 35, GetSize().cx - 10, 40, Black());
}

void StateNodeCtrl::LeftDown(Point p, dword keyflags) {
    if (on_select) {
        on_select(this);
    }
}

void StateNodeCtrl::Layout() {
    // Nothing needed for this simple control
}

// StateMachineEditorCtrl implementation
StateMachineEditorCtrl::StateMachineEditorCtrl() {
    project = nullptr;
    current_sm = nullptr;
    current_sm_copy = nullptr;
    
    canvas_offset = Point(0, 0);
    canvas_zoom = 1.0;
    
    // Set up the layout
    CtrlLayout(*this);
    
    // Initialize the splitter
    Add(splitter.SizePos());
    splitter.Vert();
    
    // Add the three panels to the splitter
    Ctrl& left_panel = node_palette;
    Ctrl& center_panel = state_view;
    Ctrl& right_panel = properties_panel;
    
    splitter << left_panel << center_panel << right_panel;
    splitter.SetPos(1, 3000, 1);  // Left and right panels get 30% each, center gets 40%
    
    // Initialize UI components
    BuildToolbar();
    BuildNodePalette();
    BuildPropertiesPanel();
    
    // Set up the state view as a canvas area
    state_view.WhenLeftDown = [this](Point p, dword keyflags) {
        // Handle canvas interaction
    };
}

StateMachineEditorCtrl::~StateMachineEditorCtrl() {
    delete current_sm_copy;
}

void StateMachineEditorCtrl::SetProject(const AnimationProject* proj) {
    project = proj;
}

void StateMachineEditorCtrl::SetStateMachine(const StateMachine* sm) {
    current_sm = sm;
    
    // Create a copy for editing
    delete current_sm_copy;
    if (sm) {
        current_sm_copy = new StateMachine(*sm);
    } else {
        current_sm_copy = new StateMachine();
    }
    
    RefreshStateView();
}

void StateMachineEditorCtrl::BuildToolbar() {
    // Create toolbar with common actions
    toolbar.Add("New", CtrlImg::newg(), [this] { 
        // Create new state machine
        delete current_sm_copy;
        current_sm_copy = new StateMachine("new_sm_" + Uuid().ToString().Left(8), "New State Machine");
        RefreshStateView();
    }).Tip("Create new state machine");
    
    toolbar.Add("Save", CtrlImg::save(), [this] { 
        // Save the current state machine
        OnSaveStateMachine();
    }).Tip("Save state machine");
    
    toolbar.Add("Load", CtrlImg::open(), [this] { 
        // Load a state machine
        OnLoadStateMachine();
    }).Tip("Load state machine");
    
    toolbar.Separator();
    
    // Add state and transition creation buttons
    toolbar.Add("State", CtrlImg::file(), [this] { 
        OnAddState();
    }).Tip("Add State");
    
    toolbar.Add("Transition", CtrlImg::file(), [this] { 
        OnAddTransition();
    }).Tip("Add Transition");
}

void StateMachineEditorCtrl::BuildNodePalette() {
    // Create a simple list of available state types
    ArrayCtrl* list = new ArrayCtrl();
    node_palette.Add(*list);
    list->NoHeader();
    list->SetFrame(ThinInsetFrame());
    list->AddColumn("State Type", 120);
    
    // Add different state types
    list->Add("Idle");
    list->Add("Walking");
    list->Add("Running");
    list->Add("Attacking");
    list->Add("Jumping");
    list->Add("Damaged");
    list->Add("Dying");
    
    list->WhenLeftDouble = [this, list]() {
        String selected = list->Get(0, list->GetCursor());
        // Add a new state of the selected type
        StateNode new_state;
        new_state.id = "state_" + Uuid().ToString().Left(8);
        new_state.name = selected + " State";
        new_state.type = selected;
        
        if (current_sm_copy) {
            current_sm_copy->states.Add(new_state);
            RefreshStateView();
        }
    };
}

void StateMachineEditorCtrl::BuildPropertiesPanel() {
    // Create a simple property grid for state properties
    CtrlLayout(properties_panel);
    
    // This would contain controls to edit state/transition properties when selected
    // For now, we'll just add a placeholder
    StaticRect* placeholder = new StaticRect();
    placeholder->SetLabel("State/Transition Properties");
    placeholder->SetFrame(ThinInsetFrame());
    properties_panel.Add(*placeholder);
}

void StateMachineEditorCtrl::RefreshStateView() {
    // Clear existing states
    for (StateNodeCtrl* state_ctrl : state_controls) {
        state_ctrl->Remove();
        delete state_ctrl;
    }
    state_controls.Clear();
    
    if (!current_sm_copy) return;
    
    // Create controls for each state in the machine
    for (int i = 0; i < current_sm_copy->states.GetCount(); i++) {
        const StateNode& state = current_sm_copy->states[i];
        
        StateNodeCtrl* state_ctrl = new StateNodeCtrl();
        state_ctrl->SetState(&state);
        state_ctrl->SetSelectionCallback([this](StateNodeCtrl* ctrl) {
            OnStateSelected(ctrl);
        });
        
        // Add to the state view (center panel)
        state_view.Add(*state_ctrl);
        state_controls.Add(state_ctrl);
        
        // Position the state (this is a simplified positioning)
        state_ctrl->SetPosition(Point(100 + (i % 4) * 150, 50 + (i / 4) * 120));
        state_ctrl->SetRect(state_ctrl->GetPosition().x, 
                           state_ctrl->GetPosition().y, 
                           120, 80);
    }
    
    state_view.Refresh();
}

void StateMachineEditorCtrl::OnStateSelected(StateNodeCtrl* state) {
    // Update the properties panel with the selected state's properties
    // This is where we would populate the properties panel with controls
    // for editing the selected state's parameters
}

void StateMachineEditorCtrl::OnStatePropertyChanged() {
    // When a state property changes, update the internal representation
    // and potentially trigger a refresh
}

void StateMachineEditorCtrl::OnAddState() {
    if (!current_sm_copy) return;
    
    // Create a new default state
    StateNode new_state;
    new_state.id = "state_" + Uuid().ToString().Left(8);
    new_state.name = "New State";
    new_state.type = "Default";
    
    // Add to the state machine
    current_sm_copy->states.Add(new_state);
    
    // Refresh the view
    RefreshStateView();
}

void StateMachineEditorCtrl::OnAddTransition() {
    if (!current_sm_copy) return;
    
    // This would typically involve selecting two states and creating a transition between them
    // For simplicity, we'll create a transition with placeholder values
    StateTransition new_transition;
    new_transition.id = "trans_" + Uuid().ToString().Left(8);
    new_transition.from_state = "state1";  // Placeholder
    new_transition.to_state = "state2";    // Placeholder
    new_transition.condition = "always";   // Placeholder
    new_transition.action = "none";        // Placeholder
    
    // Add to the state machine
    current_sm_copy->transitions.Add(new_transition);
    
    // In a real implementation, we would connect this to the visual representation
}

void StateMachineEditorCtrl::OnDeleteElement() {
    // Implementation for deleting selected state or transition
    // This would require keeping track of the selected element
}

void StateMachineEditorCtrl::OnSaveStateMachine() {
    // Implementation for saving the state machine back to the project
    // This would prompt the user for a name and update the project
}

void StateMachineEditorCtrl::OnLoadStateMachine() {
    // Implementation for loading a state machine
    // This would show a dialog to select a machine to load
}

void StateMachineEditorCtrl::Paint(Draw& w) {
    w.DrawRect(GetSize(), White());
}

void StateMachineEditorCtrl::MouseMove(Point p, dword keyflags) {
    // Handle pan/drag functionality if needed
}

void StateMachineEditorCtrl::LeftDown(Point p, dword keyflags) {
    // Handle selection or panning
}

void StateMachineEditorCtrl::LeftUp(Point p, dword keyflags) {
    // Handle end of drag operations
}

void StateMachineEditorCtrl::MouseWheel(Point p, int zdelta, dword keyflags) {
    // Handle zoom if needed
    if (keyflags & K_CTRL) {  // Zoom with Ctrl+Wheel
        canvas_zoom += (zdelta > 0) ? 0.1 : -0.1;
        canvas_zoom = max(0.1, min(3.0, canvas_zoom)); // Clamp zoom
        RefreshStateView(); // Redraw with new zoom
    }
}

void StateMachineEditorCtrl::Layout() {
    // Layout the toolbar and main content
    Size sz = GetSize();
    
    // Toolbar at the top
    toolbar.SetRect(0, 0, sz.cx, 30);
    
    // Main content below the toolbar
    splitter.SetRect(0, 30, sz.cx, sz.cy - 30);
}