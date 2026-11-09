#ifndef _CardBoardEditor_CardBoardCanvas_h_
#define _CardBoardEditor_CardBoardCanvas_h_

class CardBoardCanvas : public Ctrl {
public:
	typedef CardBoardCanvas CLASSNAME;

	CardBoardCanvas();
	void SetDocument(const CardBoardDocument& document);
	CardBoardDocument& GetDocument();
	const CardBoardDocument& GetDocument() const;
	void Paint(Draw& draw) override;
	void LeftDown(Point p, dword keyflags) override;
	String GetDiagnostics() const;
	String GetRenderReport() const;

private:
	CardBoardDocument document_;
	CardBoardState state_;
	CardBoardRenderer renderer_;
	Point last_click_;
};

#endif
