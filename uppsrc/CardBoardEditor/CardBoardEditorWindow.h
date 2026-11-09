#ifndef _CardBoardEditor_CardBoardEditorWindow_h_
#define _CardBoardEditor_CardBoardEditorWindow_h_

class CardBoardEditorWindow : public DockWindow {
public:
	typedef CardBoardEditorWindow CLASSNAME;

	CardBoardEditorWindow();
	void DockInit() override;
	void Close() override;

private:
	MenuBar menu_;
	ToolBar toolbar_;
	CardBoardCanvas canvas_;
	CardBoardTreePanel tree_;
	CardBoardPropertiesPanel properties_;
	CardBoardDiagnosticsPanel diagnostics_;
	String default_layout_data_;
	String selected_path_;
	String current_file_;
	bool loaded_ = false;

	void MainMenu(Bar& bar);
	void MenuFile(Bar& bar);
	void MenuView(Bar& bar);
	void MenuHelp(Bar& bar);
	void ToolBar(Bar& bar);
	void ResetLayout();
	void RefreshPanels();
	void SelectElement(const String& path);
	void ApplyPropertyChanges();
	void OpenJsonFile();
	void SaveJsonFile();
	void SaveJsonFileAs();
	bool SaveJsonFileTo(const String& path);
	void LoadJsonFileFrom(const String& path);
	void ExportPngFile();
	void CacheDefaultLayout();
	void PrintDiagnosticsToStdout();
};

int RunCardBoardEditorCli(const Vector<String>& args);

#endif
