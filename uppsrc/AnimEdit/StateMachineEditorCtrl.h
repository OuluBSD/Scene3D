#ifndef _AnimEdit_StateMachineEditorCtrl_h_
#define _AnimEdit_StateMachineEditorCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

// Forward declaration
class StateNodeCtrl;

class StateMachineEditorCtrl : public Ctrl {
public:
    typedef StateMachineEditorCtrl CLASSNAME;

    StateMachineEditorCtrl();
    virtual ~StateMachineEditorCtrl();

    void SetProject(const AnimationProject* project);
    void SetStateMachine(const StateMachine* sm);
    const StateMachine* GetStateMachine() const { return current_sm; }

    // For JSON serialization
    void Jsonize(JsonIO& json) {
        json("state_machine", *current_sm_copy);
    }

private:
    const AnimationProject* project;
    const StateMachine* current_sm;      // Points to the SM in the project
    StateMachine* current_sm_copy;       // Editable copy
    
    // UI elements
    ToolBar toolbar;
    Splitter splitter;
    Ctrl node_palette;      // Left panel with available state types
    Ctrl state_view;        // Main canvas area for the state machine
    Ctrl properties_panel;  // Right panel for state/transition properties

    // Canvas-related
    Point canvas_offset;
    double canvas_zoom;
    Vector<StateNodeCtrl*> state_controls;
    
    // Methods
    void BuildToolbar();
    void BuildNodePalette();
    void BuildPropertiesPanel();
    void RefreshStateView();
    void OnStateSelected(StateNodeCtrl* state);
    void OnStatePropertyChanged();
    void OnAddState();
    void OnAddTransition();
    void OnDeleteElement();
    void OnSaveStateMachine();
    void OnLoadStateMachine();

    // Override Ctrl methods
    virtual void Paint(Draw& w) override;
    virtual void MouseMove(Point p, dword keyflags) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void LeftUp(Point p, dword keyflags) override;
    virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
    virtual void Layout() override;
};

// Individual state control for the state machine view
class StateNodeCtrl : public Ctrl {
public:
    typedef StateNodeCtrl CLASSNAME;

    StateNodeCtrl();
    virtual ~StateNodeCtrl();

    void SetState(const StateNode* state);
    const StateNode* GetState() const { return state_node; }
    void SetPosition(Point pos) { position = pos; }
    Point GetPosition() const { return position; }

    // Event callback
    void SetSelectionCallback(std::function<void(StateNodeCtrl*)> cb) { 
        on_select = cb; 
    }

private:
    const StateNode* state_node;
    Point position;  // Position in the canvas
    std::function<void(StateNodeCtrl*)> on_select;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;
    virtual void Layout() override;
};

#endif