#include "CardBoardEditor.h"

NAMESPACE_UPP

static String ColorToPropertyString(Color color)
{
	if(IsNull(color))
		return String();
	return Format("#%02x%02x%02x", color.GetR(), color.GetG(), color.GetB());
}

static Color PropertyStringToColor(const String& text)
{
	String value = TrimBoth(text);
	if(value.IsEmpty())
		return Null;
	if(value.GetCount() == 7 && value[0] == '#') {
		int r = ScanInt("0x" + value.Mid(1, 2));
		int g = ScanInt("0x" + value.Mid(3, 2));
		int b = ScanInt("0x" + value.Mid(5, 2));
		return Color(r, g, b);
	}
	return Null;
}

CardBoardTreePanel::CardBoardTreePanel()
{
	Title("Board Tree").SizeHint(Size(280, 360));
	Add(tree_.SizePos());
	tree_.NoRoot();
	tree_.WhenSel = THISBACK(SelectionChanged);
}

void CardBoardTreePanel::SetDocument(const CardBoardDocument& document, const String& selected_path)
{
	tree_.Clear();
	for(int i = 0; i < document.elements.GetCount(); i++)
		AddElement(0, document.elements[i], AsString(i));
	if(!selected_path.IsEmpty())
		tree_.FindSetCursor(selected_path);
}

void CardBoardTreePanel::AddElement(int parent, const CardBoardElement& element, const String& path)
{
	String text = Format("%s  %s", CardBoardElementTypeName(element.type), element.id);
	int id = parent ? tree_.Add(parent, CtrlImg::Dir(), path, text)
	                : tree_.Add(0, CtrlImg::Dir(), path, text);
	for(int i = 0; i < element.children.GetCount(); i++)
		AddElement(id, element.children[i], path + "/" + AsString(i));
	tree_.Open(id);
}

void CardBoardTreePanel::SelectionChanged()
{
	Value value = tree_.Get();
	if(!IsNull(value))
		WhenElementSelected(AsString(value));
}

CardBoardPropertiesPanel::CardBoardPropertiesPanel()
{
	Title("Properties").SizeHint(Size(320, 360));
	Add(title_);
	Add(id_label_);
	Add(label_label_);
	Add(binding_label_);
	Add(asset_label_);
	Add(font_face_label_);
	Add(font_height_label_);
	Add(fill_label_);
	Add(border_label_);
	Add(text_label_);
	Add(id_);
	Add(label_);
	Add(binding_);
	Add(asset_);
	Add(font_face_);
	Add(font_height_);
	Add(fill_);
	Add(border_);
	Add(text_);
	id_label_.SetLabel("Id");
	label_label_.SetLabel("Label");
	binding_label_.SetLabel("Binding");
	asset_label_.SetLabel("Asset");
	font_face_label_.SetLabel("Font");
	font_height_label_.SetLabel("Font px");
	fill_label_.SetLabel("Fill");
	border_label_.SetLabel("Border");
	text_label_.SetLabel("Text");
	for(EditString *edit : { &id_, &label_, &binding_, &asset_, &font_face_, &font_height_,
	                         &fill_, &border_, &text_ })
		edit->WhenAction = THISBACK(ApplyFields);
	ClearFields();
}

void CardBoardPropertiesPanel::SetDocument(const CardBoardDocument& document)
{
	document_ = nullptr;
	path_.Clear();
	ClearFields();
	String title = Format("Provider %s / %s / %d`x%d",
	                      document.provider_id, document.game_family,
	                      document.design_size.cx, document.design_size.cy);
	String error = document.Validate();
	if(!error.IsEmpty())
		title << " / " << error;
	title_.SetLabel(title);
}

void CardBoardPropertiesPanel::SetElement(CardBoardDocument& document, const String& path)
{
	document_ = &document;
	path_ = path;
	CardBoardElement *element = document.FindElementPath(path);
	if(!element) {
		ClearFields();
		return;
	}
	LoadFields(*element);
}

void CardBoardPropertiesPanel::Layout()
{
	int y = 4;
	title_.SetRect(4, y, GetSize().cx - 8, 22);
	y += 28;
	Label *labels[] = { &id_label_, &label_label_, &binding_label_, &asset_label_, &font_face_label_,
	                    &font_height_label_, &fill_label_, &border_label_, &text_label_ };
	EditString *edits[] = { &id_, &label_, &binding_, &asset_, &font_face_, &font_height_,
	                        &fill_, &border_, &text_ };
	for(int i = 0; i < __countof(labels); i++) {
		labels[i]->SetRect(4, y, 72, 22);
		edits[i]->SetRect(80, y, GetSize().cx - 84, 22);
		y += 26;
	}
}

void CardBoardPropertiesPanel::ClearFields()
{
	loading_ = true;
	id_.Clear();
	label_.Clear();
	binding_.Clear();
	asset_.Clear();
	font_face_.Clear();
	font_height_.Clear();
	fill_.Clear();
	border_.Clear();
	text_.Clear();
	loading_ = false;
}

void CardBoardPropertiesPanel::LoadFields(CardBoardElement& element)
{
	loading_ = true;
	title_.SetLabel(Format("%s %s", CardBoardElementTypeName(element.type), element.id));
	id_.SetData(element.id);
	label_.SetData(element.label);
	binding_.SetData(element.binding);
	asset_.SetData(element.style.asset);
	font_face_.SetData(element.style.font_face);
	font_height_.SetData(AsString(element.style.font_height));
	fill_.SetData(ColorToPropertyString(element.style.fill));
	border_.SetData(ColorToPropertyString(element.style.border));
	text_.SetData(ColorToPropertyString(element.style.text));
	loading_ = false;
}

void CardBoardPropertiesPanel::ApplyFields()
{
	if(loading_ || !document_)
		return;
	CardBoardElement *element = document_->FindElementPath(path_);
	if(!element)
		return;
	element->id = AsString(id_.GetData());
	element->label = AsString(label_.GetData());
	element->binding = AsString(binding_.GetData());
	element->style.asset = AsString(asset_.GetData());
	element->style.font_face = AsString(font_face_.GetData());
	int height = ScanInt(AsString(font_height_.GetData()));
	if(height > 0)
		element->style.font_height = height;
	element->style.fill = PropertyStringToColor(AsString(fill_.GetData()));
	element->style.border = PropertyStringToColor(AsString(border_.GetData()));
	element->style.text = PropertyStringToColor(AsString(text_.GetData()));
	WhenChanged();
}

CardBoardDiagnosticsPanel::CardBoardDiagnosticsPanel()
{
	Title("Diagnostics").SizeHint(Size(640, 180));
	text_.SetReadOnly();
	Add(text_.SizePos());
}

void CardBoardDiagnosticsPanel::SetText(const String& text)
{
	text_.Set(text);
}

END_UPP_NAMESPACE
