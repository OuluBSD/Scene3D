#include "CardBoardEditor.h"

NAMESPACE_UPP

static CardBoardRect Rel(double x, double y, double w, double h)
{
	CardBoardRect r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	r.relative = true;
	return r;
}

static String ColorToJsonString(Color color)
{
	if(IsNull(color))
		return "null";
	return Format("#%02x%02x%02x", color.GetR(), color.GetG(), color.GetB());
}

static Color JsonStringToColor(const String& text)
{
	if(text.IsEmpty() || text == "null")
		return Null;
	if(text.GetCount() == 7 && text[0] == '#') {
		int r = ScanInt("0x" + text.Mid(1, 2));
		int g = ScanInt("0x" + text.Mid(3, 2));
		int b = ScanInt("0x" + text.Mid(5, 2));
		return Color(r, g, b);
	}
	return Null;
}

static void JsonizeColor(JsonIO& json, const char *key, Color& color)
{
	String value = ColorToJsonString(color);
	json(key, value);
	if(json.IsLoading())
		color = JsonStringToColor(value);
}

Rect CardBoardRect::Realize(const Rect& parent) const
{
	if(!relative)
		return RectC(parent.left + fround(x), parent.top + fround(y), fround(w), fround(h));
	return RectC(parent.left + fround(parent.GetWidth() * x),
	             parent.top + fround(parent.GetHeight() * y),
	             fround(parent.GetWidth() * w),
	             fround(parent.GetHeight() * h));
}

String CardBoardRect::ToString() const
{
	return relative ? Format("%.4f %.4f %.4f %.4f rel", x, y, w, h)
	                : Format("%.1f %.1f %.1f %.1f px", x, y, w, h);
}

void CardBoardRect::Jsonize(JsonIO& json)
{
	json
		("x", x)
		("y", y)
		("w", w)
		("h", h)
		("relative", relative);
}

void CardBoardStyle::Jsonize(JsonIO& json)
{
	JsonizeColor(json, "fill", fill);
	JsonizeColor(json, "border", border);
	JsonizeColor(json, "text", text);
	json
		("asset", asset)
		("font_face", font_face)
		("align", align)
		("outline", outline)
		("shadow", shadow)
		("font_height", font_height)
		("pen", pen)
		("opacity", opacity)
		("rotation", rotation)
		("fan_start", fan_start)
		("fan_end", fan_end)
		("ellipse", ellipse)
		("rounded", rounded);
}

String CardBoardElementTypeName(CardBoardElementType type)
{
	switch(type) {
	case CARD_BOARD_WINDOW: return "Window";
	case CARD_BOARD_BOARD: return "Board";
	case CARD_BOARD_SEAT: return "Seat";
	case CARD_BOARD_PLAYER_PANEL: return "PlayerPanel";
	case CARD_BOARD_AVATAR: return "Avatar";
	case CARD_BOARD_AVATAR_RING: return "AvatarRing";
	case CARD_BOARD_HOLE_CARDS: return "HoleCards";
	case CARD_BOARD_NAMEPLATE: return "Nameplate";
	case CARD_BOARD_ACTION_BADGE: return "ActionBadge";
	case CARD_BOARD_BALANCE_TEXT: return "BalanceText";
	case CARD_BOARD_FLAG: return "Flag";
	case CARD_BOARD_PLAYER_NUMBER: return "PlayerNumber";
	case CARD_BOARD_DEALER_BUTTON: return "DealerButton";
	case CARD_BOARD_POT_LABEL: return "PotLabel";
	case CARD_BOARD_BOARD_CARDS: return "BoardCards";
	case CARD_BOARD_CHIP_STACK: return "ChipStack";
	case CARD_BOARD_ACTION_BUTTONS: return "ActionButtons";
	case CARD_BOARD_CHECKBOXES: return "Checkboxes";
	case CARD_BOARD_TOP_TOOLBAR: return "TopToolbar";
	case CARD_BOARD_TEXT: return "Text";
	case CARD_BOARD_BUTTON: return "Button";
	case CARD_BOARD_CARD: return "Card";
	default: return "Unknown";
	}
}

CardBoardElementType CardBoardElementTypeFromName(const String& name)
{
	for(int i = CARD_BOARD_WINDOW; i <= CARD_BOARD_CARD; i++) {
		CardBoardElementType type = (CardBoardElementType)i;
		if(CardBoardElementTypeName(type) == name)
			return type;
	}
	return CARD_BOARD_TEXT;
}

CardBoardElement MakeCardBoardElement(CardBoardElementType type, const String& id,
                                      const String& label, const CardBoardRect& rect)
{
	CardBoardElement element;
	element.type = type;
	element.id = id;
	element.label = label;
	element.rect = rect;
	return element;
}

CardBoardElement& CardBoardElement::Add(CardBoardElementType type, const String& id,
                                        const String& label, const CardBoardRect& rect)
{
	return children.Add(MakeCardBoardElement(type, id, label, rect));
}

void CardBoardElement::Jsonize(JsonIO& json)
{
	String type_name = CardBoardElementTypeName(type);
	json
		("id", id)
		("type", type_name)
		("label", label)
		("binding", binding)
		("template_id", template_id)
		("rect", rect)
		("style", style)
		("children", children)
		("z", z)
		("visible", visible);
	if(json.IsLoading())
		type = CardBoardElementTypeFromName(type_name);
}

void CardBoardDocument::Clear()
{
	provider_id.Clear();
	game_family.Clear();
	format_version = "1";
	asset_root.Clear();
	elements.Clear();
}

static void CopyStyle(CardBoardStyle& target, const CardBoardStyle& source)
{
	target.fill = source.fill;
	target.border = source.border;
	target.text = source.text;
	target.asset = source.asset;
	target.font_face = source.font_face;
	target.align = source.align;
	target.outline = source.outline;
	target.shadow = source.shadow;
	target.font_height = source.font_height;
	target.pen = source.pen;
	target.opacity = source.opacity;
	target.rotation = source.rotation;
	target.fan_start = source.fan_start;
	target.fan_end = source.fan_end;
	target.ellipse = source.ellipse;
	target.rounded = source.rounded;
}

static void CopyRect(CardBoardRect& target, const CardBoardRect& source)
{
	target.x = source.x;
	target.y = source.y;
	target.w = source.w;
	target.h = source.h;
	target.relative = source.relative;
}

static void CopyElement(CardBoardElement& target, const CardBoardElement& source)
{
	target.id = source.id;
	target.label = source.label;
	target.binding = source.binding;
	target.template_id = source.template_id;
	target.type = source.type;
	CopyRect(target.rect, source.rect);
	CopyStyle(target.style, source.style);
	target.z = source.z;
	target.visible = source.visible;
	target.children.Clear();
	for(const CardBoardElement& child : source.children)
		CopyElement(target.children.Add(), child);
}

void CardBoardDocument::CopyFrom(const CardBoardDocument& source)
{
	provider_id = source.provider_id;
	game_family = source.game_family;
	format_version = source.format_version;
	asset_root = source.asset_root;
	design_size = source.design_size;
	elements.Clear();
	for(const CardBoardElement& element : source.elements)
		CopyElement(elements.Add(), element);
}

static CardBoardElement *FindElementPath(Vector<CardBoardElement>& elements, const Vector<String>& parts, int depth)
{
	if(depth >= parts.GetCount())
		return nullptr;
	int index = ScanInt(parts[depth]);
	if(index < 0 || index >= elements.GetCount())
		return nullptr;
	CardBoardElement& element = elements[index];
	if(depth == parts.GetCount() - 1)
		return &element;
	return FindElementPath(element.children, parts, depth + 1);
}

static const CardBoardElement *FindElementPath(const Vector<CardBoardElement>& elements,
                                               const Vector<String>& parts, int depth)
{
	if(depth >= parts.GetCount())
		return nullptr;
	int index = ScanInt(parts[depth]);
	if(index < 0 || index >= elements.GetCount())
		return nullptr;
	const CardBoardElement& element = elements[index];
	if(depth == parts.GetCount() - 1)
		return &element;
	return FindElementPath(element.children, parts, depth + 1);
}

CardBoardElement *CardBoardDocument::FindElementPath(const String& path)
{
	if(path.IsEmpty())
		return nullptr;
	return Upp::FindElementPath(elements, Split(path, '/'), 0);
}

const CardBoardElement *CardBoardDocument::FindElementPath(const String& path) const
{
	if(path.IsEmpty())
		return nullptr;
	return Upp::FindElementPath(elements, Split(path, '/'), 0);
}

static void StylePanel(CardBoardElement& element, Color fill, Color border, Color text = White())
{
	element.style.fill = fill;
	element.style.border = border;
	element.style.text = text;
}

static void AddSeat(CardBoardElement& window, int seat, const String& name, const String& stack,
                    const CardBoardRect& rect, Color panel_border, bool hero = false,
                    const String& action = String())
{
	CardBoardElement& s = window.Add(CARD_BOARD_SEAT, Format("seat%d", seat), name, rect);
	s.template_id = hero ? "hero-seat" : "standard-seat";

	CardBoardElement& avatar = s.Add(CARD_BOARD_AVATAR, Format("seat%d.avatar", seat), "",
	                                 Rel(0.28, 0.00, 0.44, 0.44));
	StylePanel(avatar, Color(75, 75, 75), Color(215, 215, 215));
	avatar.style.ellipse = true;

	CardBoardElement& ring = s.Add(CARD_BOARD_AVATAR_RING, Format("seat%d.ring", seat), "",
	                               Rel(0.25, -0.03, 0.50, 0.50));
	StylePanel(ring, Null, panel_border);
	ring.style.ellipse = true;
	ring.style.pen = 3;

	CardBoardElement& cards = s.Add(CARD_BOARD_HOLE_CARDS, Format("seat%d.cards", seat), "",
	                                Rel(0.27, 0.02, 0.46, 0.28));
	StylePanel(cards, Color(150, 45, 38), White());
	cards.style.rotation = hero ? 4.0 : 0.0;
	cards.style.fan_start = -4.0;
	cards.style.fan_end = 4.0;
	cards.z = 20;

	CardBoardElement& panel = s.Add(CARD_BOARD_PLAYER_PANEL, Format("seat%d.panel", seat), "",
	                                Rel(0.12, 0.38, 0.76, 0.46));
	StylePanel(panel, Color(18, 22, 28), panel_border);
	panel.style.rounded = true;
	panel.z = 10;

	if(!action.IsEmpty()) {
		CardBoardElement& badge = s.Add(CARD_BOARD_ACTION_BADGE, Format("seat%d.action", seat), action,
		                                Rel(0.25, 0.32, 0.50, 0.16));
		StylePanel(badge, Color(45, 32, 8), Color(230, 185, 35), Color(255, 225, 70));
		badge.style.rounded = true;
		badge.style.font_height = 9;
		badge.binding = Format("seat%d_action", seat);
		badge.z = 30;
	}

	CardBoardElement& number = s.Add(CARD_BOARD_PLAYER_NUMBER, Format("seat%d.number", seat),
	                                 AsString(20 + seat), Rel(0.07, 0.43, 0.18, 0.18));
	StylePanel(number, Color(75, 30, 30), Color(130, 120, 110));
	number.style.font_height = 9;

	CardBoardElement& flag = s.Add(CARD_BOARD_FLAG, Format("seat%d.flag", seat), "FI",
	                               Rel(0.73, 0.43, 0.20, 0.14));
	StylePanel(flag, Color(230, 230, 245), Color(30, 60, 160), Blue());
	flag.style.font_height = 8;

	CardBoardElement& label = s.Add(CARD_BOARD_NAMEPLATE, Format("seat%d.name", seat), name,
	                                Rel(0.18, 0.46, 0.64, 0.17));
	label.style.text = White();
	label.style.font_height = hero ? 13 : 10;

	CardBoardElement& balance = s.Add(CARD_BOARD_BALANCE_TEXT, Format("seat%d.balance", seat), stack,
	                                  Rel(0.18, 0.64, 0.64, 0.18));
	balance.style.text = Color(90, 200, 255);
	balance.style.font_height = hero ? 14 : 11;
}

void CardBoardDocument::MakePokerSample()
{
	Clear();
	provider_id = "Original";
	game_family = "Poker";
	format_version = "1";
	asset_root = "share/cardboard/original";
	design_size = Size(610, 438);

	CardBoardElement& window = elements.Add(MakeCardBoardElement(CARD_BOARD_WINDOW, "window",
	                                                             "Original poker table",
	                                                             Rel(0, 0, 1, 1)));
	StylePanel(window, Color(18, 18, 20), Null);

	CardBoardElement& toolbar = window.Add(CARD_BOARD_TOP_TOOLBAR, "top.toolbar",
	                                       "NLH Original 02 - $5 / $10", Rel(0, 0, 1, 0.075));
	StylePanel(toolbar, Color(23, 24, 27), Color(45, 45, 48), Color(180, 180, 180));
	toolbar.style.font_height = 13;

	CardBoardElement& leaderboard = window.Add(CARD_BOARD_BUTTON, "top.leaderboard",
	                                           "DAILY $50,000 LEADERBOARD", Rel(0.01, 0.075, 0.27, 0.055));
	StylePanel(leaderboard, Color(28, 70, 38), Color(110, 135, 115), Color(230, 245, 160));
	leaderboard.style.rounded = true;
	leaderboard.style.font_height = 8;

	CardBoardElement& top_buttons = window.Add(CARD_BOARD_TOP_TOOLBAR, "top.buttons", "",
	                                           Rel(0.87, 0.075, 0.12, 0.055));
	StylePanel(top_buttons, Null, Null);
	for(int i = 0; i < 3; i++) {
		CardBoardElement& button = top_buttons.Add(CARD_BOARD_BUTTON, Format("top.button%d", i + 1),
		                                           i == 0 ? "⌨" : i == 1 ? "+" : "♠",
		                                           Rel(i * 0.34, 0.02, 0.28, 0.84));
		StylePanel(button, Color(85, 150, 60), Color(150, 220, 100), White());
		button.style.rounded = true;
		button.style.font_height = 12;
	}

	CardBoardElement& board = window.Add(CARD_BOARD_BOARD, "board", "", Rel(0.07, 0.20, 0.86, 0.57));
	StylePanel(board, Color(19, 112, 43), Color(28, 30, 28));
	board.style.rounded = true;

	CardBoardElement& pot = board.Add(CARD_BOARD_POT_LABEL, "board.pot", "Total Pot: $155",
	                                  Rel(0.39, 0.11, 0.22, 0.075));
	StylePanel(pot, Color(42, 78, 28), Null, Color(255, 214, 71));
	pot.style.rounded = true;
	pot.style.font_height = 12;
	pot.binding = "pot_label";
	pot.z = 20;

	CardBoardElement& cards = board.Add(CARD_BOARD_BOARD_CARDS, "board.cards", "",
	                                    Rel(0.31, 0.28, 0.36, 0.30));
	StylePanel(cards, Null, Null);
	const char *flop[] = { "K", "5", "3" };
	for(int i = 0; i < 5; i++) {
		CardBoardElement& card = cards.Add(CARD_BOARD_CARD, Format("board.card%d", i + 1),
		                                   i < 3 ? flop[i] : "",
		                                   Rel(i * 0.205, 0, 0.18, 1));
		StylePanel(card, i == 1 ? Color(35, 38, 43) : i == 2 ? Color(150, 28, 28) : Color(24, 145, 53),
		           Color(210, 210, 210));
		card.style.font_height = 32;
		card.binding = Format("board_card%d", i + 1);
	}

	CardBoardElement& chips = board.Add(CARD_BOARD_CHIP_STACK, "board.chips", "$155",
	                                    Rel(0.46, 0.56, 0.08, 0.12));
	StylePanel(chips, Color(210, 210, 210), Color(80, 80, 80), White());
	chips.style.font_height = 10;
	chips.binding = "pot_amount";
	chips.z = 30;

	AddSeat(window, 1, "Misha Inner", "$930", Rel(0.40, 0.70, 0.20, 0.25), Color(245, 120, 70), true, "LIVE");
	AddSeat(window, 2, "LlenoXX", "$2,934", Rel(0.42, 0.09, 0.16, 0.20), Color(130, 255, 70));
	AddSeat(window, 3, "cascconss", "$1,048", Rel(0.76, 0.20, 0.17, 0.22), Color(130, 70, 210));
	AddSeat(window, 4, "H Pesonen", "$1,190", Rel(0.78, 0.61, 0.17, 0.23), Color(130, 70, 210));
	AddSeat(window, 5, "Denis Chernous", "$1,013", Rel(0.05, 0.61, 0.18, 0.23), Color(130, 70, 210));
	AddSeat(window, 6, "Majiaz Vogrinec", "$1,974", Rel(0.08, 0.20, 0.17, 0.22), Color(130, 70, 210));

	CardBoardElement& dealer = window.Add(CARD_BOARD_DEALER_BUTTON, "dealer.button", "D",
	                                      Rel(0.44, 0.31, 0.025, 0.035));
	StylePanel(dealer, Color(235, 205, 38), Color(80, 75, 10), Color(70, 55, 5));
	dealer.style.ellipse = true;
	dealer.style.font_height = 9;

	CardBoardElement& checks = window.Add(CARD_BOARD_CHECKBOXES, "bottom.checks",
	                                      "Move Table\nStraddle\nGlobal Cash Game Sit-out",
	                                      Rel(0.025, 0.83, 0.26, 0.12));
	checks.style.text = Color(185, 185, 185);
	checks.style.font_height = 10;

	CardBoardElement& rank = window.Add(CARD_BOARD_TEXT, "bottom.rank", "43",
	                                    Rel(0.22, 0.925, 0.09, 0.075));
	StylePanel(rank, Color(50, 53, 58), Null, Color(130, 255, 190));
	rank.style.font_height = 20;

	CardBoardElement& chat = window.Add(CARD_BOARD_BUTTON, "bottom.chat", "●",
	                                    Rel(0.32, 0.93, 0.04, 0.05));
	StylePanel(chat, Color(205, 210, 218), Null, Color(80, 85, 95));
	chat.style.ellipse = true;
	chat.style.font_height = 12;

	CardBoardElement& emoji = window.Add(CARD_BOARD_BUTTON, "bottom.emoji", "☺",
	                                     Rel(0.37, 0.925, 0.05, 0.06));
	StylePanel(emoji, Color(230, 170, 50), Null, Color(95, 55, 10));
	emoji.style.ellipse = true;
	emoji.style.font_height = 14;

	CardBoardElement& buttons = window.Add(CARD_BOARD_ACTION_BUTTONS, "bottom.actions",
	                                       "", Rel(0.61, 0.84, 0.27, 0.11));
	StylePanel(buttons, Color(20, 20, 22), Color(45, 45, 50), Color(225, 225, 225));
	buttons.style.font_height = 11;
	for(int i = 0; i < 3; i++) {
		CardBoardElement& button = buttons.Add(CARD_BOARD_BUTTON, Format("bottom.action%d", i + 1),
		                                       i == 0 ? "Fold" : i == 1 ? "Call $10" : "Bet",
		                                       Rel(i * 0.34, 0.10, 0.30, 0.70));
		StylePanel(button, Color(32, 18, 20), Color(70, 35, 38), Color(230, 210, 210));
		button.style.rounded = true;
		button.style.font_height = 10;
	}
}

static void AddPokerGgSeat(CardBoardElement& window, int seat, const String& name, const String& stack,
                           const String& tag, const String& flag, const CardBoardRect& rect,
                           Color border, bool hero = false, bool sitting_out = false)
{
	CardBoardElement& s = window.Add(CARD_BOARD_SEAT, Format("pgg.seat%d", seat), name, rect);
	s.template_id = hero ? "pokergg-hero-seat" : "pokergg-seat";
	s.z = hero ? 35 : 25;

	CardBoardElement& bounty = s.Add(CARD_BOARD_POT_LABEL, Format("pgg.seat%d.bounty", seat), tag,
	                                 Rel(0.28, -0.02, 0.44, 0.16));
	StylePanel(bounty, Color(232, 212, 160), Color(120, 85, 30), Color(80, 45, 15));
	bounty.style.font_height = hero ? 13 : 10;
	bounty.z = 30;

	CardBoardElement& avatar = s.Add(CARD_BOARD_AVATAR, Format("pgg.seat%d.avatar", seat), "",
	                                 Rel(0.26, 0.08, 0.48, 0.42));
	StylePanel(avatar, Color(85 + seat * 12 % 90, 60 + seat * 19 % 120, 45 + seat * 23 % 120),
	           Color(215, 185, 110));
	avatar.style.ellipse = true;
	avatar.z = 5;

	CardBoardElement& ring = s.Add(CARD_BOARD_AVATAR_RING, Format("pgg.seat%d.ring", seat), "",
	                               Rel(0.22, 0.04, 0.56, 0.50));
	StylePanel(ring, Null, Color(205, 170, 90));
	ring.style.ellipse = true;
	ring.style.pen = 3;
	ring.z = 6;

	if(hero || seat == 3) {
		CardBoardElement& cards = s.Add(CARD_BOARD_HOLE_CARDS, Format("pgg.seat%d.cards", seat), "",
		                                hero ? Rel(0.35, 0.14, 0.42, 0.36) : Rel(0.34, 0.16, 0.44, 0.24));
		StylePanel(cards, hero ? Color(150, 35, 30) : Color(120, 210, 145), White());
		cards.z = 25;
	}

	CardBoardElement& panel = s.Add(CARD_BOARD_PLAYER_PANEL, Format("pgg.seat%d.panel", seat), "",
	                                Rel(0.10, 0.48, 0.80, 0.42));
	StylePanel(panel, Color(24, 28, 32), border);
	panel.style.pen = hero ? 4 : 3;
	panel.z = 10;

	CardBoardElement& number = s.Add(CARD_BOARD_PLAYER_NUMBER, Format("pgg.seat%d.number", seat),
	                                 AsString(20 + seat), Rel(0.05, 0.51, 0.16, 0.15));
	StylePanel(number, Color(78, 32, 34), Color(200, 185, 160), White());
	number.style.font_height = 10;
	number.z = 20;

	CardBoardElement& flag_el = s.Add(CARD_BOARD_FLAG, Format("pgg.seat%d.flag", seat), flag,
	                                  Rel(0.73, 0.51, 0.21, 0.13));
	StylePanel(flag_el, Color(235, 235, 240), Color(20, 40, 140), Blue());
	flag_el.style.font_height = 8;
	flag_el.z = 20;

	CardBoardElement& name_el = s.Add(CARD_BOARD_NAMEPLATE, Format("pgg.seat%d.name", seat),
	                                  name, Rel(0.18, 0.55, 0.64, 0.16));
	name_el.style.text = sitting_out ? Color(205, 205, 205) : White();
	name_el.style.font_height = hero ? 13 : 10;
	name_el.z = 20;

	CardBoardElement& stack_el = s.Add(CARD_BOARD_BALANCE_TEXT, Format("pgg.seat%d.balance", seat), stack,
	                                   Rel(0.18, 0.72, 0.64, 0.17));
	stack_el.style.text = Color(110, 190, 245);
	stack_el.style.font_height = hero ? 18 : 14;
	stack_el.z = 20;

	if(sitting_out) {
		CardBoardElement& badge = s.Add(CARD_BOARD_ACTION_BADGE, Format("pgg.seat%d.action", seat), "Sitting Out",
		                                Rel(0.26, 0.36, 0.48, 0.16));
		StylePanel(badge, Color(20, 22, 24), Color(220, 220, 220), White());
		badge.style.font_height = 10;
		badge.z = 35;
	}
}

void CardBoardDocument::MakePokerGg8pSample()
{
	Clear();
	provider_id = "PokerGG8p";
	game_family = "Poker";
	format_version = "1";
	asset_root = "share/cardboard/pokergg_8p";
	design_size = Size(1371, 1019);

	CardBoardElement& window = elements.Add(MakeCardBoardElement(CARD_BOARD_WINDOW, "window",
	                                                             "PokerGG 8p table",
	                                                             Rel(0, 0, 1, 1)));
	StylePanel(window, Color(34, 20, 12), Null, White());

	CardBoardElement& toolbar = window.Add(CARD_BOARD_TOP_TOOLBAR, "pgg.top.toolbar",
	                                       "Bounty Hunters Forty Stack $44 --  Game Rules",
	                                       Rel(0, 0, 1, 0.066));
	StylePanel(toolbar, Color(20, 20, 22), Color(52, 52, 55), Color(190, 190, 190));
	toolbar.style.font_height = 28;

	CardBoardElement& hud = window.Add(CARD_BOARD_TOP_TOOLBAR, "pgg.tournament.hud",
	                                   "", Rel(0.005, 0.068, 0.36, 0.065));
	StylePanel(hud, Color(10, 34, 70), Color(25, 65, 120), Color(220, 235, 255));
	hud.style.font_height = 16;
	hud.z = 40;
	const char *hud_text[] = {
		"Level 17     07:23",
		"Places Paid     216",
		"Avg Stack     39.2 BB",
		"My Rank     145 / 399"
	};
	for(int i = 0; i < 4; i++) {
		CardBoardElement& cell = hud.Add(CARD_BOARD_TEXT, Format("pgg.hud.cell%d", i + 1), hud_text[i],
		                                 Rel((i % 2) * 0.50, (i / 2) * 0.50, 0.50, 0.50));
		StylePanel(cell, Color(10, 34, 70), Color(25, 65, 120), Color(220, 235, 255));
		cell.style.font_height = 15;
		cell.z = 5;
	}

	CardBoardElement& room = window.Add(CARD_BOARD_TEXT, "pgg.room.backdrop", "",
	                                    Rel(0, 0.066, 1, 0.934));
	StylePanel(room, Color(63, 35, 16), Null);
	room.z = 0;
	CardBoardElement& back_wall = window.Add(CARD_BOARD_TEXT, "pgg.room.backwall", "",
	                                         Rel(0, 0.066, 1, 0.22));
	StylePanel(back_wall, Color(78, 45, 22), Null);
	back_wall.z = 1;
	CardBoardElement& left_shadow = window.Add(CARD_BOARD_TEXT, "pgg.room.leftshadow", "",
	                                           Rel(0, 0.066, 0.16, 0.934));
	StylePanel(left_shadow, Color(35, 20, 12), Null);
	left_shadow.z = 2;
	CardBoardElement& right_shadow = window.Add(CARD_BOARD_TEXT, "pgg.room.rightshadow", "",
	                                            Rel(0.84, 0.066, 0.16, 0.934));
	StylePanel(right_shadow, Color(35, 20, 12), Null);
	right_shadow.z = 2;

	CardBoardElement& board = window.Add(CARD_BOARD_BOARD, "pgg.table", "", Rel(0.09, 0.24, 0.82, 0.56));
	StylePanel(board, Color(118, 74, 35), Color(55, 32, 18));
	board.style.pen = 5;
	board.z = 5;

	CardBoardElement& pot = board.Add(CARD_BOARD_POT_LABEL, "pgg.pot", "Total Pot : 16.7 BB",
	                                  Rel(0.38, 0.22, 0.24, 0.08));
	StylePanel(pot, Color(55, 35, 12), Null, Color(255, 215, 55));
	pot.style.font_height = 22;
	pot.z = 30;

	CardBoardElement& cards = board.Add(CARD_BOARD_BOARD_CARDS, "pgg.board.cards", "",
	                                    Rel(0.31, 0.32, 0.27, 0.27));
	const char *flop[] = { "A♠", "6♠", "8♣" };
	for(int i = 0; i < 3; i++) {
		CardBoardElement& card = cards.Add(CARD_BOARD_CARD, Format("pgg.board.card%d", i + 1),
		                                   flop[i], Rel(i * 0.34, 0, 0.30, 1));
		StylePanel(card, i == 2 ? Color(20, 140, 55) : Color(28, 30, 32), Color(160, 160, 150));
		card.style.font_height = 54;
		card.binding = Format("board_card%d", i + 1);
	}

	CardBoardElement& chips = board.Add(CARD_BOARD_CHIP_STACK, "pgg.center.chips", "6.6 BB",
	                                    Rel(0.42, 0.58, 0.19, 0.14));
	StylePanel(chips, Color(210, 210, 210), Color(80, 80, 80), White());
	chips.style.font_height = 16;
	chips.binding = "pot_amount";
	chips.z = 40;

	AddPokerGgSeat(window, 1, "Daisy Madik", "35.7 BB", "$10", "BE",
	               Rel(0.42, 0.72, 0.18, 0.25), Color(235, 245, 250), true);
	AddPokerGgSeat(window, 2, "TBN1", "14.7 BB", "$20", "CN",
	               Rel(0.43, 0.10, 0.16, 0.22), Color(105, 255, 60));
	AddPokerGgSeat(window, 3, "mclazz3", "17.1 BB", "$15", "UK",
	               Rel(0.70, 0.12, 0.17, 0.24), Color(255, 25, 25));
	AddPokerGgSeat(window, 4, "alcarezD7", "34.4 BB", "$10", "SG",
	               Rel(0.84, 0.39, 0.15, 0.21), Color(255, 25, 25));
	AddPokerGgSeat(window, 5, "Pietro Corsi", "34.8 BB", "$10", "IT",
	               Rel(0.70, 0.62, 0.17, 0.23), Color(35, 205, 110), false, true);
	AddPokerGgSeat(window, 6, "shlpin2026", "14.4 BB", "$41.25", "PL",
	               Rel(0.14, 0.62, 0.17, 0.23), Color(40, 205, 105));
	AddPokerGgSeat(window, 7, "RamiPlayer", "22.4 BB", "$10", "AT",
	               Rel(0.01, 0.39, 0.15, 0.21), Color(35, 35, 40));
	AddPokerGgSeat(window, 8, "Mancunderground", "44.3 BB", "$42.50", "PL",
	               Rel(0.17, 0.13, 0.17, 0.23), Color(35, 35, 40));

	CardBoardElement& dealer = window.Add(CARD_BOARD_DEALER_BUTTON, "pgg.dealer", "D",
	                                      Rel(0.285, 0.655, 0.025, 0.035));
	StylePanel(dealer, Color(240, 210, 35), Color(80, 75, 10), Color(70, 55, 5));
	dealer.style.ellipse = true;
	dealer.style.font_height = 18;
	dealer.z = 60;

	CardBoardElement& sitout = window.Add(CARD_BOARD_CHECKBOXES, "pgg.bottom.sitout",
	                                      "Global Sit-Out", Rel(0.03, 0.86, 0.18, 0.08));
	sitout.style.text = Color(230, 230, 230);
	sitout.style.font_height = 18;

	CardBoardElement& icons = window.Add(CARD_BOARD_ACTION_BUTTONS, "pgg.bottom.icons", "",
	                                     Rel(0.28, 0.885, 0.14, 0.06));
	for(int i = 0; i < 3; i++) {
		CardBoardElement& icon = icons.Add(CARD_BOARD_BUTTON, Format("pgg.icon%d", i + 1),
		                                   i == 0 ? "▶" : i == 1 ? "●" : "☺",
		                                   Rel(i * 0.34, 0, 0.28, 1));
		StylePanel(icon, i == 0 ? Color(40, 170, 90) : i == 1 ? Color(210, 220, 230) : Color(230, 180, 45),
		           Null, i == 1 ? Color(70, 80, 90) : White());
		icon.style.ellipse = true;
		icon.style.font_height = 20;
	}

	CardBoardElement& presets = window.Add(CARD_BOARD_ACTION_BUTTONS, "pgg.bet.presets", "",
	                                       Rel(0.61, 0.82, 0.22, 0.055));
	const char *preset[] = { "33%", "50%", "75%", "100%" };
	for(int i = 0; i < 4; i++) {
		CardBoardElement& p = presets.Add(CARD_BOARD_BUTTON, Format("pgg.preset%d", i + 1),
		                                  preset[i], Rel(i * 0.25, 0, 0.23, 1));
		StylePanel(p, Color(60, 62, 66), Color(35, 35, 38), Color(235, 235, 235));
		p.style.font_height = 17;
	}

	CardBoardElement& actions = window.Add(CARD_BOARD_ACTION_BUTTONS, "pgg.bottom.actions", "",
	                                       Rel(0.61, 0.88, 0.38, 0.11));
	const char *labels[] = { "Fold", "Call\n4.9 BB", "Raise to\n12.4 BB" };
	for(int i = 0; i < 3; i++) {
		CardBoardElement& action = actions.Add(CARD_BOARD_BUTTON, Format("pgg.action%d", i + 1),
		                                       labels[i], Rel(i * 0.335, 0, 0.31, 0.95));
		StylePanel(action, Color(185, 54, 58), Color(120, 35, 38), White());
		action.style.font_height = 21;
		action.style.rounded = true;
	}
}

CardBoardState CardBoardDocument::MakePokerSampleState() const
{
	CardBoardState state;
	if(provider_id == "PokerGG8p") {
		state.values.Add("pot_label", "Total Pot : 16.7 BB");
		state.values.Add("pot_amount", "6.6 BB");
		state.values.Add("board_card1", "A♠");
		state.values.Add("board_card2", "6♠");
		state.values.Add("board_card3", "8♣");
	}
	else {
		state.values.Add("pot_label", "Total Pot: $155");
		state.values.Add("pot_amount", "$155");
		state.values.Add("board_card1", "K");
		state.values.Add("board_card2", "5");
		state.values.Add("board_card3", "3");
	}
	state.values.Add("board_card4", "");
	state.values.Add("board_card5", "");
	state.values.Add("seat1_action", "LIVE");
	return state;
}

String CardBoardDocument::Validate() const
{
	if(provider_id.IsEmpty())
		return "provider_id is empty";
	if(game_family.IsEmpty())
		return "game_family is empty";
	if(design_size.cx <= 0 || design_size.cy <= 0)
		return "design_size is invalid";
	if(elements.IsEmpty())
		return "document has no elements";
	return String();
}

static void DumpElementTree(String& out, const CardBoardElement& element, int depth)
{
	out.Cat(String(' ', depth * 2));
	out.Cat(Format("%s id=%s label=%s rect=%s\n",
	               CardBoardElementTypeName(element.type), element.id, element.label,
	               element.rect.ToString()));
	for(const CardBoardElement& child : element.children)
		DumpElementTree(out, child, depth + 1);
}

void CardBoardDocument::DumpTree(String& out) const
{
	out.Cat(Format("CardBoardDocument provider=%s game=%s design=%d`x%d\n",
	               provider_id, game_family, design_size.cx, design_size.cy));
	for(const CardBoardElement& element : elements)
		DumpElementTree(out, element, 0);
}

void CardBoardDocument::DumpRects(String& out, Size canvas_size) const
{
	CardBoardRenderer renderer;
	renderer.DumpRects(out, RectC(0, 0, canvas_size.cx, canvas_size.cy), *this);
}

void CardBoardDocument::RenderReport(String& out, Size canvas_size, const CardBoardState& state) const
{
	CardBoardRenderer renderer;
	renderer.RenderReport(out, RectC(0, 0, canvas_size.cx, canvas_size.cy), *this, state);
}

String CardBoardDocument::StoreJson() const
{
	CardBoardDocument copy;
	copy.CopyFrom(*this);
	return StoreAsJson(copy, true);
}

bool CardBoardDocument::LoadJson(const String& json, String& error)
{
	CardBoardDocument loaded;
	if(!LoadFromJson(loaded, json)) {
		error = "failed to parse CardBoard JSON";
		return false;
	}
	error = loaded.Validate();
	if(!error.IsEmpty())
		return false;
	CopyFrom(loaded);
	return true;
}

void Jsonize(JsonIO& json, Size& size)
{
	json
		("cx", size.cx)
		("cy", size.cy);
}

void Jsonize(JsonIO& json, CardBoardDocument& document)
{
	json
		("format_version", document.format_version)
		("provider_id", document.provider_id)
		("game_family", document.game_family)
		("asset_root", document.asset_root)
		("design_size", document.design_size)
		("elements", document.elements);
}

END_UPP_NAMESPACE
