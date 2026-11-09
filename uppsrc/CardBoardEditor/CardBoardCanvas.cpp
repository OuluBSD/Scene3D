#include "CardBoardEditor.h"

NAMESPACE_UPP

CardBoardCanvas::CardBoardCanvas()
{
	document_.MakePokerSample();
	last_click_ = Null;
}

void CardBoardCanvas::SetDocument(const CardBoardDocument& document)
{
	document_.CopyFrom(document);
	Refresh();
}

CardBoardDocument& CardBoardCanvas::GetDocument()
{
	return document_;
}

const CardBoardDocument& CardBoardCanvas::GetDocument() const
{
	return document_;
}

void CardBoardCanvas::Paint(Draw& draw)
{
	Size size = GetSize();
	renderer_.Render(draw, RectC(0, 0, size.cx, size.cy), document_, state_);
	if(!IsNull(last_click_.x)) {
		draw.DrawEllipse(RectC(last_click_.x - 4, last_click_.y - 4, 8, 8), Yellow());
	}
}

void CardBoardCanvas::LeftDown(Point p, dword)
{
	last_click_ = p;
	LOG(Format("CardBoardCanvas click: %d,%d", p.x, p.y));
	Refresh();
}

String CardBoardCanvas::GetDiagnostics() const
{
	String out;
	document_.DumpRects(out, GetSize());
	return out;
}

String CardBoardCanvas::GetRenderReport() const
{
	String out;
	document_.RenderReport(out, GetSize(), document_.MakePokerSampleState());
	return out;
}

END_UPP_NAMESPACE
