#include "CardBoardEditor.h"

NAMESPACE_UPP

struct CardBoardRenderer::RenderContext {
	const CardBoardDocument& document;
	const CardBoardState& state;
	CardBoardRenderDiagnostics *diagnostics = nullptr;
	VectorMap<String, Image> assets;
};

void CardBoardRenderDiagnostics::Clear()
{
	used_assets.Clear();
	missing_assets.Clear();
}

static Color EffectiveFill(const CardBoardElement& element)
{
	if(!IsNull(element.style.fill))
		return element.style.fill;
	switch(element.type) {
	case CARD_BOARD_WINDOW: return SColorFace();
	case CARD_BOARD_BOARD: return Color(24, 96, 42);
	case CARD_BOARD_CARD: return White();
	default: return Null;
	}
}

static Font ElementFont(const CardBoardElement& element, int fallback)
{
	Font font = StdFont(element.style.font_height > 0 ? element.style.font_height : fallback);
	if(!element.style.font_face.IsEmpty())
		font.FaceName(element.style.font_face);
	return font;
}

static void DrawAlignedText(Draw& draw, const Rect& rect, const String& text, Font font, Color ink, const String& align = "center")
{
	Vector<String> lines = Split(text, '\n');
	int line_height = font.GetHeight();
	int total_height = line_height * lines.GetCount();
	int y = rect.top + (rect.GetHeight() - total_height) / 2;
	int margin = max(2, min(rect.GetWidth(), rect.GetHeight()) / 12);
	for(const String& line : lines) {
		Size text_size = GetTextSize(line, font);
		int x = rect.left + (rect.GetWidth() - text_size.cx) / 2;
		if(align == "left")
			x = rect.left + margin;
		else
		if(align == "right")
			x = rect.right - margin - text_size.cx;
		draw.DrawText(x, y, line, font, ink);
		y += line_height;
	}
}

static void DrawCenteredText(Draw& draw, const Rect& rect, const String& text, Font font, Color ink)
{
	DrawAlignedText(draw, rect, text, font, ink);
}

static void DrawRectBorder(Draw& draw, const Rect& rect, int pen, Color border)
{
	pen = max(1, pen);
	draw.DrawRect(rect.left, rect.top, rect.GetWidth(), pen, border);
	draw.DrawRect(rect.left, rect.bottom - pen, rect.GetWidth(), pen, border);
	draw.DrawRect(rect.left, rect.top, pen, rect.GetHeight(), border);
	draw.DrawRect(rect.right - pen, rect.top, pen, rect.GetHeight(), border);
}

static Rect DeflateRect(const Rect& rect, int amount)
{
	return Rect(rect.left + amount, rect.top + amount, rect.right - amount, rect.bottom - amount);
}

static Color CardColor(const String& label)
{
	if(label.Find("♥") >= 0 || label.Find("♦") >= 0)
		return Color(170, 35, 35);
	if(label.Find("♣") >= 0)
		return Color(30, 125, 55);
	if(label.Find("♠") >= 0)
		return Color(35, 38, 43);
	return Color(30, 120, 55);
}

static void DrawFallbackCard(Draw& draw, const Rect& rect, const String& label, Font font)
{
	Color face = label.IsEmpty() ? Color(110, 32, 32) : CardColor(label);
	draw.DrawRect(rect, face);
	DrawRectBorder(draw, rect, 1, Color(220, 220, 220));
	if(!label.IsEmpty())
		DrawCenteredText(draw, rect, label, font.Bold(), White());
}

static void DrawChipStack(Draw& draw, const Rect& rect, const String& label)
{
	int r = max(4, min(rect.GetWidth(), rect.GetHeight()) / 5);
	Point center(rect.left + rect.GetWidth() / 2, rect.top + rect.GetHeight() / 2);
	Color colors[] = { Color(210, 30, 35), Color(245, 245, 245), Color(40, 120, 70), Color(30, 45, 95) };
	for(int i = 0; i < 4; i++) {
		Rect chip = RectC(center.x - r + i * r / 2 - r, center.y - r + (i % 2) * r / 2, 2 * r, 2 * r);
		draw.DrawEllipse(chip, colors[i], 1, Color(50, 50, 50));
	}
	if(!label.IsEmpty())
		DrawCenteredText(draw, RectC(rect.left, rect.bottom - rect.GetHeight() / 3, rect.GetWidth(), rect.GetHeight() / 3),
		                 label, StdFont(max(8, rect.GetHeight() / 5)).Bold(), White());
}

static void DrawFlagPlaceholder(Draw& draw, const Rect& rect, const String& label)
{
	draw.DrawRect(rect, White());
	draw.DrawRect(rect.left, rect.top + rect.GetHeight() / 3, rect.GetWidth(), max(1, rect.GetHeight() / 4), Color(35, 75, 170));
	draw.DrawRect(rect.left + rect.GetWidth() / 3, rect.top, max(1, rect.GetWidth() / 5), rect.GetHeight(), Color(35, 75, 170));
	DrawRectBorder(draw, rect, 1, Color(25, 25, 25));
	if(!label.IsEmpty())
		DrawCenteredText(draw, rect, label, StdFont(max(6, rect.GetHeight() / 2)), Color(20, 20, 80));
}

static bool AddUnique(Vector<String>& values, const String& value)
{
	for(const String& existing : values)
		if(existing == value)
			return false;
	values.Add(value);
	return true;
}

void CardBoardRenderer::Render(Draw& draw, const Rect& area, const CardBoardDocument& document,
                               const CardBoardState& state) const
{
	CardBoardRenderDiagnostics diagnostics;
	Render(draw, area, document, state, diagnostics);
}

void CardBoardRenderer::Render(Draw& draw, const Rect& area, const CardBoardDocument& document,
                               const CardBoardState& state, CardBoardRenderDiagnostics& diagnostics) const
{
	diagnostics.Clear();
	RenderContext context { document, state, &diagnostics };
	draw.DrawRect(area, SColorPaper());
	for(const CardBoardElement& element : document.elements)
		RenderElement(draw, area, element, context, 0);
}

void CardBoardRenderer::RenderElement(Draw& draw, const Rect& parent,
                                      const CardBoardElement& element,
                                      RenderContext& context, int depth) const
{
	if(!element.visible)
		return;

	Rect rect = element.rect.Realize(parent);
	String label = ResolveLabel(element, context.state);
	RenderPrimitive(draw, rect, element, label, context);

	ForEachChildByZ(element.children, [&](const CardBoardElement& child) {
		RenderElement(draw, rect, child, context, depth + 1);
	});
}

void CardBoardRenderer::RenderPrimitive(Draw& draw, const Rect& rect,
                                        const CardBoardElement& element,
                                        const String& label, RenderContext& context) const
{
	String asset_path = ResolveAssetPath(context.document, element);
	if(!asset_path.IsEmpty()) {
		Image image;
		int cached = context.assets.Find(asset_path);
		if(cached >= 0)
			image = context.assets[cached];
		else
		if(FileExists(asset_path)) {
			image = StreamRaster::LoadFileAny(asset_path);
			if(!image.IsEmpty())
				context.assets.Add(asset_path, image);
		}

		if(!image.IsEmpty()) {
			draw.DrawImage(rect.left, rect.top, rect.GetWidth(), rect.GetHeight(), image);
			if(context.diagnostics)
				AddUnique(context.diagnostics->used_assets, asset_path);
			return;
		}
		if(context.diagnostics)
			AddUnique(context.diagnostics->missing_assets, asset_path);
	}

	if(element.type == CARD_BOARD_BOARD) {
		Color felt_color = IsNull(element.style.fill) ? Color(17, 104, 39) : element.style.fill;
		Color border_color = IsNull(element.style.border) ? Color(8, 8, 8) : element.style.border;
		Color rail_color = Color(max(0, felt_color.GetR() - 45),
		                         max(0, felt_color.GetG() - 35),
		                         max(0, felt_color.GetB() - 25));
		Color inner_line = Color(min(255, felt_color.GetR() + 18),
		                         min(255, felt_color.GetG() + 18),
		                         min(255, felt_color.GetB() + 18));
		draw.DrawEllipse(rect, border_color, max(2, element.style.pen), Color(8, 8, 8));
		Rect rail = DeflateRect(rect, max(4, rect.GetHeight() / 18));
		draw.DrawEllipse(rail, rail_color, 2, border_color);
		Rect felt = DeflateRect(rail, max(6, rect.GetHeight() / 14));
		draw.DrawEllipse(felt, felt_color, 1, inner_line);
		draw.DrawEllipse(DeflateRect(felt, max(6, felt.GetHeight() / 10)), Null, 1, inner_line);
		return;
	}

	if(element.type == CARD_BOARD_CARD) {
		DrawFallbackCard(draw, rect, label, ElementFont(element, 12));
		return;
	}

	if(element.type == CARD_BOARD_CHIP_STACK) {
		DrawChipStack(draw, rect, label);
		return;
	}

	if(element.type == CARD_BOARD_FLAG) {
		DrawFlagPlaceholder(draw, rect, label);
		return;
	}

	if(element.type == CARD_BOARD_ACTION_BADGE || element.type == CARD_BOARD_ACTION_BUTTONS ||
	   element.type == CARD_BOARD_BUTTON || element.type == CARD_BOARD_PLAYER_PANEL ||
	   element.type == CARD_BOARD_POT_LABEL || element.type == CARD_BOARD_NAMEPLATE ||
	   element.type == CARD_BOARD_BALANCE_TEXT || element.type == CARD_BOARD_PLAYER_NUMBER) {
		Color fill = EffectiveFill(element);
		if(!IsNull(fill))
			draw.DrawRect(rect, fill);
		if(!IsNull(element.style.border))
			DrawRectBorder(draw, rect, max(1, element.style.pen), element.style.border);
		if(!label.IsEmpty())
			DrawAlignedText(draw, rect, label, ElementFont(element, 12), element.style.text, element.style.align);
		return;
	}

	if(element.type == CARD_BOARD_CHECKBOXES) {
		Vector<String> lines = Split(label, '\n');
		Font font = ElementFont(element, 10);
		int y = rect.top + 2;
		for(String line : lines) {
			draw.DrawEllipse(RectC(rect.left + 2, y + 4, 6, 6), Color(45, 45, 45));
			draw.DrawText(rect.left + 14, y, line, font, element.style.text);
			y += font.GetHeight() + 2;
		}
		return;
	}

	if(element.type == CARD_BOARD_HOLE_CARDS && element.children.IsEmpty()) {
		Rect left = RectC(rect.left, rect.top + rect.GetHeight() / 8, rect.GetWidth() / 2, rect.GetHeight() * 3 / 4);
		Rect right = RectC(rect.left + rect.GetWidth() / 2, rect.top + rect.GetHeight() / 8,
		                   rect.GetWidth() / 2, rect.GetHeight() * 3 / 4);
		DrawFallbackCard(draw, left, "", StdFont(8));
		DrawFallbackCard(draw, right, "", StdFont(8));
		return;
	}

	Color fill = EffectiveFill(element);
	Color border = element.style.border;
	if(!IsNull(fill)) {
		if(element.style.ellipse)
			draw.DrawEllipse(rect, fill);
		else
			draw.DrawRect(rect, fill);
	}
	if(!IsNull(border)) {
		if(element.style.ellipse)
			draw.DrawEllipse(rect, Null, max(1, element.style.pen), border);
		else
			DrawRectBorder(draw, rect, max(1, element.style.pen), border);
	}
	if(!label.IsEmpty())
		DrawAlignedText(draw, rect, label, ElementFont(element, 12), element.style.text, element.style.align);
}

void CardBoardRenderer::DumpRects(String& out, const Rect& area, const CardBoardDocument& document) const
{
	for(const CardBoardElement& element : document.elements)
		DumpElement(out, area, element, 0);
}

void CardBoardRenderer::DumpElement(String& out, const Rect& parent,
                                    const CardBoardElement& element, int depth) const
{
	Rect rect = element.rect.Realize(parent);
	out.Cat(String(' ', depth * 2));
	out.Cat(Format("%s id=%s rect=(%d,%d %d`x%d) label=%s\n",
	               CardBoardElementTypeName(element.type), element.id,
	               rect.left, rect.top, rect.GetWidth(), rect.GetHeight(), element.label));
	for(const CardBoardElement& child : element.children)
		DumpElement(out, rect, child, depth + 1);
}

void CardBoardRenderer::RenderReport(String& out, const Rect& area,
                                     const CardBoardDocument& document,
                                     const CardBoardState& state) const
{
	out.Cat(Format("RenderReport provider=%s game=%s area=(%d,%d %d`x%d) design=%d`x%d\n",
	               document.provider_id, document.game_family, area.left, area.top,
	               area.GetWidth(), area.GetHeight(), document.design_size.cx,
	               document.design_size.cy));
	for(const CardBoardElement& element : document.elements)
		ReportElement(out, area, element, state, document, 0);
}

void CardBoardRenderer::ReportElement(String& out, const Rect& parent,
                                      const CardBoardElement& element,
                                      const CardBoardState& state, const CardBoardDocument& document, int depth) const
{
	if(!element.visible)
		return;
	Rect rect = element.rect.Realize(parent);
	String asset_path = ResolveAssetPath(document, element);
	String asset_state;
	if(!asset_path.IsEmpty())
		asset_state = FileExists(asset_path) ? " asset=used:" + asset_path
		                                     : " asset=missing:" + asset_path;
	out.Cat(String(' ', depth * 2));
	out.Cat(Format("draw type=%s id=%s primitive=%s z=%d rect=(%d,%d %d`x%d) label=%s binding=%s%s\n",
	               CardBoardElementTypeName(element.type), element.id, PrimitiveName(element), element.z,
	               rect.left, rect.top, rect.GetWidth(), rect.GetHeight(),
	               ResolveLabel(element, state), element.binding, asset_state));
	ForEachChildByZ(element.children, [&](const CardBoardElement& child) {
		ReportElement(out, rect, child, state, document, depth + 1);
	});
}

String CardBoardRenderer::ResolveLabel(const CardBoardElement& element,
                                       const CardBoardState& state) const
{
	if(element.binding.IsEmpty())
		return element.label;
	int index = state.values.Find(element.binding);
	if(index < 0)
		return element.label;
	Value value = state.values.GetValue(index);
	if(IsNull(value))
		return element.label;
	return AsString(value);
}

String CardBoardRenderer::ResolveAssetPath(const CardBoardDocument& document,
                                           const CardBoardElement& element) const
{
	String asset = element.style.asset;
	if(asset.IsEmpty())
		return String();
	if(IsFullPath(asset))
		return NormalizePath(asset);
	if(document.asset_root.IsEmpty())
		return NormalizePath(asset);
	return NormalizePath(AppendFileName(document.asset_root, asset));
}

String CardBoardRenderer::PrimitiveName(const CardBoardElement& element) const
{
	switch(element.type) {
	case CARD_BOARD_BOARD: return "table-rail-felt";
	case CARD_BOARD_CARD: return "playing-card";
	case CARD_BOARD_CHIP_STACK: return "chip-stack";
	case CARD_BOARD_FLAG: return "flag-bands";
	case CARD_BOARD_ACTION_BADGE: return "action-badge";
	case CARD_BOARD_ACTION_BUTTONS: return "action-button-group";
	case CARD_BOARD_BUTTON: return "button";
	case CARD_BOARD_HOLE_CARDS: return "hole-card-pair";
	case CARD_BOARD_CHECKBOXES: return "checkbox-list";
	case CARD_BOARD_PLAYER_PANEL: return "player-panel";
	case CARD_BOARD_POT_LABEL: return "pot-label";
	default: return "basic";
	}
}

void CardBoardRenderer::ForEachChildByZ(const Vector<CardBoardElement>& children,
                                        Function<void (const CardBoardElement&)> callback) const
{
	Vector<int> order;
	for(int i = 0; i < children.GetCount(); i++)
		order.Add(i);
	Sort(order, [&](int a, int b) {
		if(children[a].z != children[b].z)
			return children[a].z < children[b].z;
		return a < b;
	});
	for(int index : order)
		callback(children[index]);
}

END_UPP_NAMESPACE
