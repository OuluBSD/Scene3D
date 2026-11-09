#ifndef _CardBoardEditor_CardBoardDocument_h_
#define _CardBoardEditor_CardBoardDocument_h_

struct CardBoardElement : Moveable<CardBoardElement> {
	String id;
	String label;
	String binding;
	String template_id;
	CardBoardElementType type = CARD_BOARD_TEXT;
	CardBoardRect rect;
	CardBoardStyle style;
	Vector<CardBoardElement> children;
	int z = 0;
	bool visible = true;

	CardBoardElement& Add(CardBoardElementType type, const String& id, const String& label,
	                      const CardBoardRect& rect);
	void Jsonize(JsonIO& json);
};

struct CardBoardState : Moveable<CardBoardState> {
	ValueMap values;
};

struct CardBoardDocument : Moveable<CardBoardDocument> {
	String provider_id;
	String game_family;
	String format_version = "1";
	String asset_root;
	Size design_size = Size(610, 438);
	Vector<CardBoardElement> elements;

	void Clear();
	void CopyFrom(const CardBoardDocument& source);
	CardBoardElement *FindElementPath(const String& path);
	const CardBoardElement *FindElementPath(const String& path) const;
	void MakePokerSample();
	void MakePokerGg8pSample();
	CardBoardState MakePokerSampleState() const;
	String Validate() const;
	void DumpTree(String& out) const;
	void DumpRects(String& out, Size canvas_size) const;
	void RenderReport(String& out, Size canvas_size, const CardBoardState& state) const;
	String StoreJson() const;
	bool LoadJson(const String& json, String& error);
};

CardBoardElement MakeCardBoardElement(CardBoardElementType type, const String& id,
                                      const String& label, const CardBoardRect& rect);

#endif
