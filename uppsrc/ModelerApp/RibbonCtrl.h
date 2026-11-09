#ifndef _ModelerApp_RibbonCtrl_h_
#define _ModelerApp_RibbonCtrl_h_

class XmlNode;
struct Edit3D;

struct ModelerAppRibbon : RibbonBar {
	Edit3D* owner = nullptr;
	bool loaded = false;
	String spec_path;
	int quick_access_pos = RibbonBar::QAT_TOP;

	struct SpecItem : Moveable<SpecItem> {
		String id;
		String text;
		String type;
		String group_id;
		String tab_id;
	};
	VectorMap<String, Ctrl*> control_by_id;
	Vector<One<Ctrl>> owned_ctrls;
	VectorMap<String, Event<>> action_handlers;

	Event<String> WhenAction;

	ModelerAppRibbon();
	void Init(Edit3D* o);
	void BuildDefaultTabs();
	void Clear();
	void OnAction(const String& id);
	Ctrl* FindControl(const String& id) const;
	void BindAction(const String& id, Event<> cb);
	void SetupQuickAccess();
	void AddContextTabs();
	int GetDisplayMode() const { return RibbonBar::GetDisplayMode(); }
	void SetDisplayMode(int mode) { RibbonBar::SetDisplayMode((RibbonBar::DisplayMode)mode); }
	int GetQuickAccessPos() const { return quick_access_pos; }
	void SetQuickAccessPos(int pos) { quick_access_pos = pos; RibbonBar::SetQuickAccessPos((RibbonBar::QuickAccessPos)pos); }
};

#endif
