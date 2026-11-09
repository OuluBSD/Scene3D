#ifndef _CardBoardEditor_CardBoardRenderer_h_
#define _CardBoardEditor_CardBoardRenderer_h_

struct CardBoardRenderDiagnostics : Moveable<CardBoardRenderDiagnostics> {
	Vector<String> used_assets;
	Vector<String> missing_assets;

	void Clear();
};

class CardBoardRenderer {
public:
	void Render(Draw& draw, const Rect& area, const CardBoardDocument& document,
	            const CardBoardState& state = CardBoardState()) const;
	void Render(Draw& draw, const Rect& area, const CardBoardDocument& document,
	            const CardBoardState& state, CardBoardRenderDiagnostics& diagnostics) const;
	void DumpRects(String& out, const Rect& area, const CardBoardDocument& document) const;
	void RenderReport(String& out, const Rect& area, const CardBoardDocument& document,
	                  const CardBoardState& state = CardBoardState()) const;

private:
	struct RenderContext;

	void RenderElement(Draw& draw, const Rect& parent, const CardBoardElement& element,
	                   RenderContext& context, int depth) const;
	void RenderPrimitive(Draw& draw, const Rect& rect, const CardBoardElement& element,
	                     const String& label, RenderContext& context) const;
	void DumpElement(String& out, const Rect& parent, const CardBoardElement& element,
	                 int depth) const;
	void ReportElement(String& out, const Rect& parent, const CardBoardElement& element,
	                   const CardBoardState& state, const CardBoardDocument& document, int depth) const;
	String ResolveLabel(const CardBoardElement& element, const CardBoardState& state) const;
	String ResolveAssetPath(const CardBoardDocument& document, const CardBoardElement& element) const;
	String PrimitiveName(const CardBoardElement& element) const;
	void ForEachChildByZ(const Vector<CardBoardElement>& children,
	                     Function<void (const CardBoardElement&)> callback) const;
};

#endif
