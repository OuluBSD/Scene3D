#ifndef _CardBoardEditor_CardBoardPanels_h_
#define _CardBoardEditor_CardBoardPanels_h_

class CardBoardTreePanel : public DockableCtrl {
public:
	typedef CardBoardTreePanel CLASSNAME;
	CardBoardTreePanel();
	void SetDocument(const CardBoardDocument& document, const String& selected_path = String());
	Event<String> WhenElementSelected;

private:
	TreeCtrl tree_;
	void AddElement(int parent, const CardBoardElement& element, const String& path);
	void SelectionChanged();
};

class CardBoardPropertiesPanel : public DockableCtrl {
public:
	typedef CardBoardPropertiesPanel CLASSNAME;
	CardBoardPropertiesPanel();
	void SetDocument(const CardBoardDocument& document);
	void SetElement(CardBoardDocument& document, const String& path);
	Event<> WhenChanged;

private:
	Label title_;
	Label id_label_;
	Label label_label_;
	Label binding_label_;
	Label asset_label_;
	Label font_face_label_;
	Label font_height_label_;
	Label fill_label_;
	Label border_label_;
	Label text_label_;
	EditString id_;
	EditString label_;
	EditString binding_;
	EditString asset_;
	EditString font_face_;
	EditString font_height_;
	EditString fill_;
	EditString border_;
	EditString text_;
	CardBoardDocument *document_ = nullptr;
	String path_;
	bool loading_ = false;

	void Layout() override;
	void ClearFields();
	void LoadFields(CardBoardElement& element);
	void ApplyFields();
};

class CardBoardDiagnosticsPanel : public DockableCtrl {
public:
	typedef CardBoardDiagnosticsPanel CLASSNAME;
	CardBoardDiagnosticsPanel();
	void SetText(const String& text);

private:
	LineEdit text_;
};

#endif
