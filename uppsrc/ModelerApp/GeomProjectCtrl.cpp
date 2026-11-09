#include "ModelerApp.h"
#include <cfloat>
#include <functional>
#include <Vfs/Ecs/Ecs.h>

NAMESPACE_UPP

namespace {

struct IconToggle : Ctrl {
	bool value = false;
	
	virtual void Paint(Draw& w) {
		Size sz = GetSize();
		Color bg = SColorPaper();
		w.DrawRect(sz, bg);
		DrawIcon(w, Rect(sz), value, IsEnabled() ? SColorText() : SColorDisabled());
	}
	
	virtual void LeftDown(Point, dword) {
		if (!IsEnabled())
			return;
		value = !value;
		Refresh();
		WhenAction();
	}
	
	virtual void SetData(const Value& v) {
		value = !IsNull(v) && (bool)v;
		Refresh();
	}
	
	virtual Value GetData() const {
		return value;
	}
	
	virtual Size GetMinSize() const {
		return Size(16, 16);
	}
	
protected:
	virtual void DrawIcon(Draw& w, const Rect& r, bool on, Color c) = 0;
};

struct EyeToggle : IconToggle {
	virtual void DrawIcon(Draw& w, const Rect& r, bool on, Color c) {
		Rect rr = r.Deflated(2, 4);
		if (rr.Width() <= 2 || rr.Height() <= 2)
			return;
		w.DrawEllipse(rr, Null, 1, c);
		if (on) {
			Rect pupil = rr.CenterRect(Size(4, 4));
			w.DrawEllipse(pupil, c);
		}
		else {
			w.DrawLine(rr.left, rr.bottom, rr.right, rr.top, 1, c);
		}
	}
};

struct LockToggle : IconToggle {
	virtual void DrawIcon(Draw& w, const Rect& r, bool on, Color c) {
		Rect body = r.Deflated(4, 5);
		if (body.Width() <= 2 || body.Height() <= 2)
			return;
		w.DrawRect(body, c);
		Rect shackle = RectC(body.left + body.Width() / 4, r.top + 2, body.Width() / 2, body.Height() / 2);
		w.DrawEllipse(shackle, Null, 1, c);
		if (!on)
			w.DrawLine(body.left, body.top, body.right, body.bottom, 1, SColorPaper());
	}
};

struct TextToggle : IconToggle {
	String label;
	TextToggle(const char* text = "") { label = text; }
	virtual void DrawIcon(Draw& w, const Rect& r, bool on, Color c) {
		Font fnt = StdFont().Bold();
		Color ink = on ? c : Blend(c, SColorPaper(), 120);
		Size tsz = GetTextSize(label, fnt);
		Point pt(r.left + (r.Width() - tsz.cx) / 2, r.top + (r.Height() - tsz.cy) / 2);
		w.DrawText(pt.x, pt.y, label, fnt, ink);
	}
};

struct DragValueEdit : EditDoubleSpin {
	double drag_start = 0;
	double drag_inc = 0.01;
	Point  drag_pt = Point(0, 0);
	bool   dragging = false;
	
	DragValueEdit() { ShowSpin(false); }
	
	void SetDragInc(double v) { drag_inc = v; }
	double GetDragInc() const { return drag_inc; }
	
	virtual void LeftDown(Point p, dword keyflags) {
		EditDoubleSpin::LeftDown(p, keyflags);
		if (!IsEditable())
			return;
		drag_pt = p;
		drag_start = IsNull(GetData()) ? 0.0 : (double)GetData();
		dragging = false;
		SetCapture();
	}
	
	virtual void LeftDrag(Point p, dword keyflags) {
		if (!IsEditable())
			return;
		if (!HasCapture())
			return;
		if (!dragging) {
			if (abs(p.x - drag_pt.x) + abs(p.y - drag_pt.y) < 3)
				return;
			dragging = true;
		}
		double delta = (double)(p.x - drag_pt.x);
		SetData(drag_start + delta * drag_inc);
		WhenAction();
	}
	
	virtual void LeftUp(Point p, dword keyflags) {
		if (HasCapture())
			ReleaseCapture();
		EditDoubleSpin::LeftUp(p, keyflags);
	}
};

struct Vec3EditCtrl : Ctrl {
	Label lx, ly, lz;
	DragValueEdit ex, ey, ez;
	DragValueEdit* drag_edit = 0;
	double drag_start = 0;
	double drag_inc = 0.01;
	Point drag_pt = Point(0, 0);
	bool dragging = false;
	
	Vec3EditCtrl() {
		lx.SetText("X");
		ly.SetText("Y");
		lz.SetText("Z");
		Font f = StdFont();
		f.Height(max(6, f.GetHeight() - 2));
		lx.SetFont(f);
		ly.SetFont(f);
		lz.SetFont(f);
		Add(lx);
		Add(ly);
		Add(lz);
	Add(ex);
	Add(ey);
	Add(ez);
	ex.SetFont(f);
	ey.SetFont(f);
	ez.SetFont(f);
	ex.SetFrame(NullFrame());
	ey.SetFrame(NullFrame());
	ez.SetFrame(NullFrame());
	ex.SetDragInc(0.01);
		ey.SetDragInc(0.01);
		ez.SetDragInc(0.01);
		ex.WhenAction = ey.WhenAction = ez.WhenAction = [=] { WhenAction(); };
	}
	
	void SetValue(const vec3& v) {
		ex.SetData(v[0]);
		ey.SetData(v[1]);
		ez.SetData(v[2]);
	}
	
	vec3 GetValue() const {
		vec3 v;
		v[0] = (double)ex.GetData();
		v[1] = (double)ey.GetData();
		v[2] = (double)ez.GetData();
		return v;
	}
	
	void SetEditable(bool b) {
		ex.SetEditable(b);
		ey.SetEditable(b);
		ez.SetEditable(b);
	}
	
	virtual void LeftDown(Point p, dword keyflags) {
		if (lx.GetRect().Contains(p)) drag_edit = &ex;
		else if (ly.GetRect().Contains(p)) drag_edit = &ey;
		else if (lz.GetRect().Contains(p)) drag_edit = &ez;
		else drag_edit = 0;
		if (drag_edit && drag_edit->IsEditable()) {
			drag_pt = p;
			drag_start = IsNull(drag_edit->GetData()) ? 0.0 : (double)drag_edit->GetData();
			drag_inc = drag_edit->GetDragInc();
			dragging = false;
			SetCapture();
		}
		Ctrl::LeftDown(p, keyflags);
	}
	
	virtual void LeftDrag(Point p, dword keyflags) {
		if (!HasCapture() || !drag_edit || !drag_edit->IsEditable())
			return Ctrl::LeftDrag(p, keyflags);
		if (!dragging) {
			if (abs(p.x - drag_pt.x) + abs(p.y - drag_pt.y) < 3)
				return;
			dragging = true;
		}
		double delta = (double)(p.x - drag_pt.x);
		drag_edit->SetData(drag_start + delta * drag_inc);
		WhenAction();
	}
	
	virtual void LeftUp(Point p, dword keyflags) {
		if (HasCapture())
			ReleaseCapture();
		Ctrl::LeftUp(p, keyflags);
	}
	
	virtual void Layout() {
		Rect r = GetSize();
		int labelw = 12;
		int gap = 4;
		int fields = 3;
		int total_gap = gap * (fields - 1);
		int cellw = (r.Width() - fields * labelw - total_gap) / fields;
		int x = r.left;
		lx.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ex.SetRect(x, r.top, cellw, r.Height()); x += cellw + gap;
		ly.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ey.SetRect(x, r.top, cellw, r.Height()); x += cellw + gap;
		lz.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ez.SetRect(x, r.top, cellw, r.Height());
	}
};

struct QuatEditCtrl : Ctrl {
	Label lx, ly, lz, lw;
	DragValueEdit ex, ey, ez, ew;
	DragValueEdit* drag_edit = 0;
	double drag_start = 0;
	double drag_inc = 0.001;
	Point drag_pt = Point(0, 0);
	bool dragging = false;
	
	QuatEditCtrl() {
		lx.SetText("X");
		ly.SetText("Y");
		lz.SetText("Z");
		lw.SetText("W");
		Font f = StdFont();
		f.Height(max(6, f.GetHeight() - 2));
		lx.SetFont(f);
		ly.SetFont(f);
		lz.SetFont(f);
		lw.SetFont(f);
		Add(lx);
		Add(ly);
		Add(lz);
		Add(lw);
	Add(ex);
	Add(ey);
	Add(ez);
	Add(ew);
	ex.SetFont(f);
	ey.SetFont(f);
	ez.SetFont(f);
	ew.SetFont(f);
	ex.SetFrame(NullFrame());
	ey.SetFrame(NullFrame());
	ez.SetFrame(NullFrame());
	ew.SetFrame(NullFrame());
	ex.SetDragInc(0.001);
		ey.SetDragInc(0.001);
		ez.SetDragInc(0.001);
		ew.SetDragInc(0.001);
		ex.WhenAction = ey.WhenAction = ez.WhenAction = ew.WhenAction = [=] { WhenAction(); };
	}
	
	void SetValue(const quat& q) {
		ex.SetData(q[0]);
		ey.SetData(q[1]);
		ez.SetData(q[2]);
		ew.SetData(q[3]);
	}
	
	quat GetValue() const {
		quat q;
		q[0] = (double)ex.GetData();
		q[1] = (double)ey.GetData();
		q[2] = (double)ez.GetData();
		q[3] = (double)ew.GetData();
		return q;
	}
	
	void SetEditable(bool b) {
		ex.SetEditable(b);
		ey.SetEditable(b);
		ez.SetEditable(b);
		ew.SetEditable(b);
	}
	
	virtual void LeftDown(Point p, dword keyflags) {
		if (lx.GetRect().Contains(p)) drag_edit = &ex;
		else if (ly.GetRect().Contains(p)) drag_edit = &ey;
		else if (lz.GetRect().Contains(p)) drag_edit = &ez;
		else if (lw.GetRect().Contains(p)) drag_edit = &ew;
		else drag_edit = 0;
		if (drag_edit && drag_edit->IsEditable()) {
			drag_pt = p;
			drag_start = IsNull(drag_edit->GetData()) ? 0.0 : (double)drag_edit->GetData();
			drag_inc = drag_edit->GetDragInc();
			dragging = false;
			SetCapture();
		}
		Ctrl::LeftDown(p, keyflags);
	}
	
	virtual void LeftDrag(Point p, dword keyflags) {
		if (!HasCapture() || !drag_edit || !drag_edit->IsEditable())
			return Ctrl::LeftDrag(p, keyflags);
		if (!dragging) {
			if (abs(p.x - drag_pt.x) + abs(p.y - drag_pt.y) < 3)
				return;
			dragging = true;
		}
		double delta = (double)(p.x - drag_pt.x);
		drag_edit->SetData(drag_start + delta * drag_inc);
		WhenAction();
	}
	
	virtual void LeftUp(Point p, dword keyflags) {
		if (HasCapture())
			ReleaseCapture();
		Ctrl::LeftUp(p, keyflags);
	}
	
	virtual void Layout() {
		Rect r = GetSize();
		int labelw = 12;
		int gap = 4;
		int fields = 4;
		int total_gap = gap * (fields - 1);
		int cellw = (r.Width() - fields * labelw - total_gap) / fields;
		int x = r.left;
		lx.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ex.SetRect(x, r.top, cellw, r.Height()); x += cellw + gap;
		ly.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ey.SetRect(x, r.top, cellw, r.Height()); x += cellw + gap;
		lz.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ez.SetRect(x, r.top, cellw, r.Height()); x += cellw + gap;
		lw.SetRect(x, r.top, labelw, r.Height()); x += labelw;
		ew.SetRect(x, r.top, cellw, r.Height());
	}
};

struct ToggleRowCtrl : Ctrl {
	EyeToggle eye;
	LockToggle lock;
	Event<bool> WhenVisible;
	Event<bool> WhenLocked;
	
	ToggleRowCtrl() {
		Add(eye);
		Add(lock);
		eye.WhenAction << [=] { WhenVisible((bool)eye.GetData()); };
		lock.WhenAction << [=] { WhenLocked((bool)lock.GetData()); };
	}
	
	void SetVisible(bool v) { eye.SetData(v); }
	void SetLocked(bool v)  { lock.SetData(v); }
	
	virtual void Layout() {
		Rect r = GetSize();
		int w = r.Height();
		eye.SetRect(r.left, r.top, w, r.Height());
		lock.SetRect(r.left + w + 4, r.top, w, r.Height());
	}
};

struct KeyframeCtrl : Ctrl {
	Button left;
	Button key;
	Button right;
	Event<> WhenPrev;
	Event<> WhenToggle;
	Event<> WhenNext;
	bool keyed = false;
	
	KeyframeCtrl() {
		left.SetLabel("<");
		key.SetLabel("o");
		right.SetLabel(">");
		Add(left);
		Add(key);
		Add(right);
		left.WhenAction = [=] { WhenPrev(); };
		key.WhenAction = [=] { WhenToggle(); };
		right.WhenAction = [=] { WhenNext(); };
	}
	
	void SetKeyed(bool v) {
		keyed = v;
		key.SetLabel(keyed ? "*" : "o");
	}
	
	virtual void Layout() {
		Rect r = GetSize();
		int w = max(1, r.Width() / 3);
		left.SetRect(r.left, r.top, w, r.Height());
		key.SetRect(r.left + w, r.top, w, r.Height());
		right.SetRect(r.left + w * 2, r.top, r.Width() - w * 2, r.Height());
	}
};

struct MaterialPreviewCtrl : Ctrl {
	vec3 base = vec3(1, 1, 1);
	vec3 emissive = vec3(0, 0, 0);
	Image preview_img;

	static Image ByteImageToImage(const ByteImage& src) {
		if (!src.data || src.sz.cx <= 0 || src.sz.cy <= 0)
			return Image();
		int ch = src.channels;
		if (ch <= 0)
			ch = 4;
		ImageBuffer ib(src.sz);
		int pitch = src.pitch ? src.pitch : src.sz.cx * ch;
		for (int y = 0; y < src.sz.cy; y++) {
			const byte* s = src.data + y * pitch;
			RGBA* d = ib[y];
			for (int x = 0; x < src.sz.cx; x++) {
				if (ch >= 4) {
					d[x].r = s[x * ch + 0];
					d[x].g = s[x * ch + 1];
					d[x].b = s[x * ch + 2];
					d[x].a = s[x * ch + 3];
				}
				else if (ch == 3) {
					d[x].r = s[x * ch + 0];
					d[x].g = s[x * ch + 1];
					d[x].b = s[x * ch + 2];
					d[x].a = 255;
				}
				else {
					byte v = s[x * ch + 0];
					d[x].r = v;
					d[x].g = v;
					d[x].b = v;
					d[x].a = 255;
				}
			}
		}
		return ib;
	}

	struct PreviewPopup : TopWindow {
		ImageCtrl img;

		PreviewPopup() {
			NoWantFocus();
			ToolWindow();
			FrameLess();
			Add(img.SizePos());
		}

		void SetImage(const Image& m) {
			img.SetImage(m);
			Refresh();
		}

		virtual void LeftDown(Point, dword) {
			Close();
		}

		virtual void Deactivate() {
			Close();
		}
	};

	One<PreviewPopup> popup;

	void SetMaterial(const Material& m) {
		base = m.params->base_clr_factor.Splice();
		emissive = m.params->emissive_factor;
		preview_img.Clear();
		if (m.owner) {
			int tex_id = m.tex_id[TEXTYPE_DIFFUSE];
			if (tex_id >= 0) {
				int idx = m.owner->textures.Find(tex_id);
				if (idx >= 0)
					preview_img = ByteImageToImage(m.owner->textures[idx].img);
			}
		}
		Refresh();
	}

	virtual void LeftDown(Point, dword) {
		if (preview_img.IsEmpty())
			return;
		if (popup && popup->IsOpen()) {
			popup->Close();
			return;
		}
		if (!popup)
			popup.Create();
		Size sz = preview_img.GetSize();
		const int max_dim = 256;
		if (sz.cx > max_dim || sz.cy > max_dim) {
			float scale = min((float)max_dim / (float)sz.cx, (float)max_dim / (float)sz.cy);
			sz = Size(max(1, (int)ceil(sz.cx * scale)), max(1, (int)ceil(sz.cy * scale)));
		}
		Image shown = (sz == preview_img.GetSize()) ? preview_img : CachedRescale(preview_img, sz, FILTER_BILINEAR);
		popup->SetImage(shown);
		Rect r = GetScreenRect();
		Rect wr = GetWorkArea();
		int x = r.left;
		int y = r.bottom + 2;
		if (y + sz.cy > wr.bottom)
			y = max(wr.top, r.top - sz.cy - 2);
		if (x + sz.cx > wr.right)
			x = max(wr.left, wr.right - sz.cx);
		popup->SetRect(x, y, sz.cx, sz.cy);
		popup->PopUp(this, true, false, GUI_DropShadows());
	}

	virtual void Paint(Draw& w) {
		Size sz = GetSize();
		w.DrawRect(sz, SColorPaper());
		int r = min(sz.cx, sz.cy) / 2 - 2;
		Point c(sz.cx / 2, sz.cy / 2);
		if (r <= 0)
			return;
		for (int y = -r; y <= r; y++) {
			for (int x = -r; x <= r; x++) {
				if (x * x + y * y > r * r)
					continue;
				float nx = (float)x / (float)r;
				float ny = (float)y / (float)r;
				float nz2 = max(0.0f, 1.0f - nx * nx - ny * ny);
				float nz = sqrt(nz2);
				float diff = max(0.0f, 0.4f * nx + 0.7f * ny + 0.5f * nz);
				float intensity = 0.2f + diff * 0.8f;
				vec3 col = base * intensity + emissive;
				col[0] = Clamp(col[0], 0.0f, 1.0f);
				col[1] = Clamp(col[1], 0.0f, 1.0f);
				col[2] = Clamp(col[2], 0.0f, 1.0f);
				w.DrawRect(c.x + x, c.y + y, 1, 1, Color((byte)(col[0] * 255), (byte)(col[1] * 255), (byte)(col[2] * 255)));
			}
		}
	}
};

struct TexturePreviewCtrl : Ctrl {
	Image preview_img;

	struct PreviewPopup : TopWindow {
		ImageCtrl img;

		PreviewPopup() {
			NoWantFocus();
			ToolWindow();
			FrameLess();
			Add(img.SizePos());
		}

		void SetImage(const Image& m) {
			img.SetImage(m);
			Refresh();
		}

		virtual void LeftDown(Point, dword) {
			Close();
		}

		virtual void Deactivate() {
			Close();
		}
	};

	One<PreviewPopup> popup;

	void SetImage(const Image& img) {
		preview_img = img;
		Refresh();
	}

	void SetPath(const String& path) {
		preview_img.Clear();
		if (!path.IsEmpty()) {
			String p = path;
			if (!FileExists(p)) {
				String base = GetCurrentDirectory();
				if (!base.IsEmpty()) {
					String alt = AppendFileName(base, path);
					if (FileExists(alt))
						p = alt;
				}
				if (!FileExists(p)) {
					String share = RealizeShareFile(path);
					if (FileExists(share))
						p = share;
				}
			}
			if (FileExists(p))
				preview_img = StreamRaster::LoadFileAny(p);
		}
		Refresh();
	}

	virtual void LeftDown(Point, dword) {
		if (preview_img.IsEmpty())
			return;
		if (popup && popup->IsOpen()) {
			popup->Close();
			return;
		}
		if (!popup)
			popup.Create();
		Size sz = preview_img.GetSize();
		const int max_dim = 256;
		if (sz.cx > max_dim || sz.cy > max_dim) {
			float scale = min((float)max_dim / (float)sz.cx, (float)max_dim / (float)sz.cy);
			sz = Size(max(1, (int)ceil(sz.cx * scale)), max(1, (int)ceil(sz.cy * scale)));
		}
		Image shown = (sz == preview_img.GetSize()) ? preview_img : CachedRescale(preview_img, sz, FILTER_BILINEAR);
		popup->SetImage(shown);
		Rect r = GetScreenRect();
		Rect wr = GetWorkArea();
		int x = r.left;
		int y = r.bottom + 2;
		if (y + sz.cy > wr.bottom)
			y = max(wr.top, r.top - sz.cy - 2);
		if (x + sz.cx > wr.right)
			x = max(wr.left, wr.right - sz.cx);
		popup->SetRect(x, y, sz.cx, sz.cy);
		popup->PopUp(this, true, false, GUI_DropShadows());
	}

	virtual void Paint(Draw& w) {
		Size sz = GetSize();
		w.DrawRect(sz, SColorPaper());
		if (preview_img.IsEmpty()) {
			w.DrawRect(0, 0, sz.cx, sz.cy, SColorFace());
			return;
		}
		Size isz = preview_img.GetSize();
		if (isz.cx <= 0 || isz.cy <= 0)
			return;
		float sx = (float)sz.cx / (float)isz.cx;
		float sy = (float)sz.cy / (float)isz.cy;
		float sc = min(sx, sy);
		Size tsz(max(1, (int)floor(isz.cx * sc)), max(1, (int)floor(isz.cy * sc)));
		Image res = (tsz == isz) ? preview_img : CachedRescale(preview_img, tsz, FILTER_BILINEAR);
		Point p((sz.cx - tsz.cx) / 2, (sz.cy - tsz.cy) / 2);
		w.DrawImage(p.x, p.y, res);
	}
};

}


struct BehaviorClipboardItem : Moveable<BehaviorClipboardItem> {
	String id;
	String file;
	bool enabled = true;
	bool run_on_load = false;
	bool run_every_frame = false;
};

static Vector<BehaviorClipboardItem> s_behavior_clipboard;

static bool IsScriptOwnerNode(const VfsValue* node) {
	if (!node)
		return false;
	return IsVfsType(*node, AsTypeHash<GeomObject>()) ||
	       IsVfsType(*node, AsTypeHash<GeomDirectory>()) ||
	       IsVfsType(*node, AsTypeHash<GeomScene>());
}

static String MakeUniqueScriptNodeId(const VfsValue& owner, const String& base_id) {
	const hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
	String safe = ToVarName(base_id, '_');
	if (safe.IsEmpty())
		safe = "script";
	String id = safe;
	int idx = 1;
	while (owner.Find(id, script_hash) >= 0)
		id = safe + "_" + IntStr(idx++);
	return id;
}

static String ResolveAssetPathForReload(const Edit3D& e, const String& ref) {
	if (ref.IsEmpty())
		return String();
	if (IsFullPath(ref))
		return NormalizePath(ref);
	String p = AppendFileName(e.GetProjectDir(), ref);
	if (FileExists(p))
		return NormalizePath(p);
	return NormalizePath(AppendFileName(e.GetAssetRootDir(), ref));
}

GeomProjectCtrl::GeomProjectCtrl(Edit3D* e) {
	this->e = e;
	
	time.WhenCursor << THISBACK(OnCursor);
	time.WhenRowMenu = THISBACK(TimelineRowMenu);
	time.WhenRowSelect = THISBACK(TimelineRowSelect);
	time.WhenRowToggle = THISBACK(TimelineRowToggle);
	time.WhenKeyframeToggle = THISBACK(TimelineToggleKeyframe);
	time.WhenKeyframeRemove = THISBACK(TimelineRemoveKeyframe);
	time.WhenKeyframeMove = THISBACK(TimelineMoveKeyframe);
	time.WhenToggleAutoKey = THISBACK(TimelineToggleAutoKey);
	time.WhenCopy << THISBACK(TimelineCopySelection);
	time.WhenPaste << [=](int frame) { TimelinePasteSelection(frame); };
	time.WhenRangeSelect << [=](int a, int b) {
		if (!e)
			return;
		if (e->timeline_scope == Edit3D::TS_SCENE && e->state && e->state->HasActiveScene()) {
			GeomSceneTimeline& tl = e->state->GetActiveScene().GetTimeline();
			tl.position = min(max(b, 0), max(tl.length - 1, 0));
		}
		else if (e->anim) {
			e->anim->position = b;
		}
	};
	tree.WhenCursor << THISBACK(TreeSelect);
	tree.WhenMenu = THISBACK(TreeMenu);
	props.WhenBar = THISBACK(PropsMenu);
	props.WhenCursor << [=] {
		if (props_refreshing)
			return;
		if (!current_tree_path.IsEmpty())
			StorePropsCursor(current_tree_path);
	};
	props.WhenScroll << [=] {
		if (props_refreshing)
			return;
		if (!current_tree_path.IsEmpty())
			StorePropsCursor(current_tree_path);
	};
	tree.LayoutId("Scene Tree");
	props.LayoutId("Properties");
	time.LayoutId("Timeline");
	
	
	tree.NoHeader();
	tree.LevelCxFromLineCy(0.5);
	tree_col_visible = tree.GetColumnCount();
	tree.AddColumn("", 18).NoEdit().FixedWidth(18).NoPadding().NoVertGrid().Ctrls([=](int line, One<Ctrl>& ctrl) {
		EyeToggle* t = new EyeToggle();
		t->NoWantFocus();
		int id = tree.GetItemAtLine(line);
		{
			Value v = tree.Get(id);
			VfsValue* node = v.Is<VfsValue*>() ? ValueTo<VfsValue*>(v) : 0;
			bool is_dir = node && IsVfsType(*node, AsTypeHash<GeomDirectory>());
			if (!GetNodeObject(tree.Get(id)) && !GetNodeRef(tree.Get(id)) && !is_dir)
				t->Disable();
		}
		int linei = line;
		t->WhenAction << [=] {
			int id = tree.GetItemAtLine(linei);
			TreeNodeRef* ref = GetNodeRef(tree.Get(id));
			GeomObject* obj = GetNodeObject(tree.Get(id));
			if (obj) {
				obj->is_visible = (bool)t->GetData();
				tree.SetRowValue(id, tree_col_visible, obj->is_visible);
			}
			else if (ref) {
				if (ref->kind == TreeNodeRef::K_PROGRAM) {
					e->state->program_visible = (bool)t->GetData();
					tree.SetRowValue(id, tree_col_visible, e->state->program_visible);
				}
				if (ref->kind == TreeNodeRef::K_FOCUS) {
					e->state->focus_visible = (bool)t->GetData();
					tree.SetRowValue(id, tree_col_visible, e->state->focus_visible);
				}
			}
			else {
				Value v = tree.Get(id);
				VfsValue* node = v.Is<VfsValue*>() ? ValueTo<VfsValue*>(v) : 0;
				if (node && IsVfsType(*node, AsTypeHash<GeomDirectory>())) {
					GeomDirectory& dir = node->GetExt<GeomDirectory>();
					bool any_visible = false;
					for (auto& s : dir.val.sub) {
						if (IsVfsType(s, AsTypeHash<GeomObject>())) {
							GeomObject& o = s.GetExt<GeomObject>();
							if (o.is_visible) {
								any_visible = true;
								break;
							}
						}
					}
					bool new_visible = !any_visible;
					for (auto& s : dir.val.sub) {
						if (IsVfsType(s, AsTypeHash<GeomObject>())) {
							GeomObject& o = s.GetExt<GeomObject>();
							o.is_visible = new_visible;
						}
					}
					Data();
				}
			}
			RefreshAll();
		};
		ctrl = t;
	});
	tree_col_locked = tree.GetColumnCount();
	tree.AddColumn("", 18).NoEdit().FixedWidth(18).NoPadding().NoVertGrid().Ctrls([=](int line, One<Ctrl>& ctrl) {
		LockToggle* t = new LockToggle();
		t->NoWantFocus();
		int id = tree.GetItemAtLine(line);
		if (!GetNodeObject(tree.Get(id)))
			t->Disable();
		int linei = line;
		t->WhenAction << [=] {
			int id = tree.GetItemAtLine(linei);
			GeomObject* obj = GetNodeObject(tree.Get(id));
			if (!obj)
				return;
			obj->is_locked = (bool)t->GetData();
			tree.SetRowValue(id, tree_col_locked, obj->is_locked);
		};
		ctrl = t;
	});
	tree_col_read = tree.GetColumnCount();
	tree.AddColumn("R", 18).NoEdit().FixedWidth(18).NoPadding().NoVertGrid().Ctrls([=](int line, One<Ctrl>& ctrl) {
		TextToggle* t = new TextToggle("R");
		t->NoWantFocus();
		int linei = line;
		t->WhenAction << [=] {
			int id = tree.GetItemAtLine(linei);
			TreeNodeRef* ref = GetNodeRef(tree.Get(id));
			GeomObject* obj = GetNodeObject(tree.Get(id));
			if (obj) {
				obj->read_enabled = (bool)t->GetData();
				if (!obj->read_enabled) {
					obj->write_enabled = false;
					tree.SetRowValue(id, tree_col_write, obj->write_enabled);
				}
				tree.SetRowValue(id, tree_col_read, obj->read_enabled);
			}
			else if (ref) {
				if (ref->kind == TreeNodeRef::K_PROGRAM) {
					program_read = (bool)t->GetData();
					if (!program_read) {
						program_write = false;
						tree.SetRowValue(id, tree_col_write, program_write);
					}
					tree.SetRowValue(id, tree_col_read, program_read);
				}
				if (ref->kind == TreeNodeRef::K_FOCUS) {
					focus_read = (bool)t->GetData();
					if (!focus_read) {
						focus_write = false;
						tree.SetRowValue(id, tree_col_write, focus_write);
					}
					tree.SetRowValue(id, tree_col_read, focus_read);
				}
			}
		};
		ctrl = t;
	});
	tree_col_write = tree.GetColumnCount();
	tree.AddColumn("W", 18).NoEdit().FixedWidth(18).NoPadding().NoVertGrid().Ctrls([=](int line, One<Ctrl>& ctrl) {
		TextToggle* t = new TextToggle("W");
		t->NoWantFocus();
		int linei = line;
		t->WhenAction << [=] {
			int id = tree.GetItemAtLine(linei);
			TreeNodeRef* ref = GetNodeRef(tree.Get(id));
			GeomObject* obj = GetNodeObject(tree.Get(id));
			if (obj) {
				bool w = (bool)t->GetData();
				if (w && !obj->read_enabled) {
					obj->read_enabled = true;
					tree.SetRowValue(id, tree_col_read, obj->read_enabled);
				}
				obj->write_enabled = w;
				tree.SetRowValue(id, tree_col_write, obj->write_enabled);
			}
			else if (ref) {
				if (ref->kind == TreeNodeRef::K_PROGRAM) {
					bool w = (bool)t->GetData();
					if (w && !program_read) {
						program_read = true;
						tree.SetRowValue(id, tree_col_read, program_read);
					}
					program_write = w;
					tree.SetRowValue(id, tree_col_write, program_write);
				}
				if (ref->kind == TreeNodeRef::K_FOCUS) {
					bool w = (bool)t->GetData();
					if (w && !focus_read) {
						focus_read = true;
						tree.SetRowValue(id, tree_col_read, focus_read);
					}
					focus_write = w;
					tree.SetRowValue(id, tree_col_write, focus_write);
				}
			}
		};
		ctrl = t;
	});
	grid.SetGridSize(2,2);
	for(int i = 0; i < 4; i++) {
		rends_v1[i].ctx = &e->render_ctx;
		rends_v2[i].ctx = &e->render_ctx;
		rends_v2_ogl[i].ctx = &e->render_ctx;
		rends_v1[i].WhenChanged = THISBACK1(RefreshRenderer, i);
		rends_v2[i].WhenChanged = THISBACK1(RefreshRenderer, i);
		rends_v2_ogl[i].WhenChanged = THISBACK1(RefreshRenderer, i);
		rends_v1[i].WhenMenu = THISBACK1(BuildViewMenu, i);
		rends_v2[i].WhenMenu = THISBACK1(BuildViewMenu, i);
		rends_v2_ogl[i].WhenMenu = THISBACK1(BuildViewMenu, i);
		rends_v1[i].WhenInput = [=](const String& type, const Point& p, dword flags, int key) {
			e->DispatchInputEvent(type, p, flags, key, i);
		};
		rends_v2[i].WhenInput = [=](const String& type, const Point& p, dword flags, int key) {
			e->DispatchInputEvent(type, p, flags, key, i);
		};
		rends_v2_ogl[i].WhenInput = [=](const String& type, const Point& p, dword flags, int key) {
			e->DispatchInputEvent(type, p, flags, key, i);
		};
	}
	rends_v1[0].SetViewMode(VIEWMODE_YZ);
	rends_v1[1].SetViewMode(VIEWMODE_XZ);
	rends_v1[2].SetViewMode(VIEWMODE_XY);
	rends_v1[3].SetViewMode(VIEWMODE_PERSPECTIVE);
	rends_v2[0].SetViewMode(VIEWMODE_YZ);
	rends_v2[1].SetViewMode(VIEWMODE_XZ);
	rends_v2[2].SetViewMode(VIEWMODE_XY);
	rends_v2[3].SetViewMode(VIEWMODE_PERSPECTIVE);
	rends_v2_ogl[0].SetViewMode(VIEWMODE_YZ);
	rends_v2_ogl[1].SetViewMode(VIEWMODE_XZ);
	rends_v2_ogl[2].SetViewMode(VIEWMODE_XY);
	rends_v2_ogl[3].SetViewMode(VIEWMODE_PERSPECTIVE);
	rends_v1[0].SetCameraSource(CAMSRC_FOCUS);
	rends_v1[1].SetCameraSource(CAMSRC_FOCUS);
	rends_v1[2].SetCameraSource(CAMSRC_PROGRAM);
	rends_v1[3].SetCameraSource(CAMSRC_FOCUS);
	rends_v2[0].SetCameraSource(CAMSRC_FOCUS);
	rends_v2[1].SetCameraSource(CAMSRC_FOCUS);
	rends_v2[2].SetCameraSource(CAMSRC_PROGRAM);
	rends_v2[3].SetCameraSource(CAMSRC_FOCUS);
	rends_v2_ogl[0].SetCameraSource(CAMSRC_FOCUS);
	rends_v2_ogl[1].SetCameraSource(CAMSRC_FOCUS);
	rends_v2_ogl[2].SetCameraSource(CAMSRC_PROGRAM);
	rends_v2_ogl[3].SetCameraSource(CAMSRC_FOCUS);
	for (int i = 0; i < 4; i++)
		rends_v2[i].SetWireframeOnly(true);
	for (int i = 0; i < 4; i++)
		rends_v2_ogl[i].SetWireframeOnly(true);
	for(int i = 0; i < 4; i++)
		rends[i] = &rends_v2_ogl[i];
	RebuildGrid();
	
	props.NoHeader();
	props.LevelCxFromLineCy(0.5);
	props_col_value = props.GetColumnCount();
	props.AddColumn("Value", 300).NoEdit();
	props_col_keyframe = props.GetColumnCount();
	props.AddColumn("", 72).NoEdit();
	props.ColumnAt(0).FixedWidth(180);
	props.ColumnAt(1).FixedWidth(300);
	props.ColumnAt(1).SetWidth(300);
	props.ColumnAt(2).FixedWidth(72);
	props.ColumnAt(2).SetWidth(72);
	for (int i = 3; i < props.GetColumnCount(); i++) {
		props.HeaderTab(i).Hide();
		props.ColumnAt(i).SetWidth(0);
	}
	props.WhenOpen << [=](int) {
		if (props_refreshing)
			return;
		PostCallback([=] { PropsData(); });
	};
	props.WhenClose << [=](int) {
		if (props_refreshing)
			return;
		PostCallback([=] { PropsData(); });
	};
	
	
}

void GeomProjectCtrl::RefreshRenderer(int i) {
	if (i >= 0 && i < 4 && rends[i])
		rends[i]->Refresh();
}

void GeomProjectCtrl::RefreshAll() {
	for (int i = 0; i < 4; i++)
		if (rends[i])
			rends[i]->Refresh();
}

void GeomProjectCtrl::Update(double dt) {
	GeomAnim& anim = *e->anim;
	GeomVideo& video = e->video;
	bool was_playing = anim.is_playing || video.is_importing;
	
	if (video.is_importing) {
		video.Update(dt);
		TimelineData();
	}
	else {
		anim.Update(dt);
	}

	int selected_col = anim.position;
	if (e && e->timeline_scope == Edit3D::TS_SCENE && e->state && e->state->HasActiveScene()) {
		GeomSceneTimeline& tl = e->state->GetActiveScene().GetTimeline();
		selected_col = tl.position;
	}
	time.SetSelectedColumn(selected_col);
	time.Refresh();
	
	if (anim.is_playing || was_playing) {
		for(int i = 0; i < 4; i++) {
			if (rends[i])
				rends[i]->Refresh();
		}
		if (anim.position != last_props_frame) {
			last_props_frame = anim.position;
			SyncPropsValues();
		}
	}
	// Scene timeline sync handled after GeomSceneTimeline::Update in Edit3D::Update.
}

VfsValue* GeomProjectCtrl::ResolveGeomValue(const Value& v) {
	if (!v.Is<VfsValue*>())
		return nullptr;
	VfsValue* node = ValueTo<VfsValue*>(v);
	if (!node)
		return nullptr;
	int idx = ecs_to_geom.Find(node);
	if (idx >= 0)
		return ecs_to_geom[idx];
	return node;
}

void GeomProjectCtrl::BuildEcsMirror() {
	ecs_root_val.sub.Clear();
	ecs_root_val.ext.Clear();
	ecs_root_val.type_hash = 0;
	ecs_root_val.id = "ecs";
	ecs_root_val.owner = nullptr;
	ecs_to_geom.Clear();
	geom_to_ecs.Clear();
	
	if (!e || !e->prj)
		return;
	
	GeomProject& prj = *e->prj;
	auto create_entity = [&](VfsValue& parent, const String& id) -> VfsValue& {
		VfsValue& node = parent.Add(id);
		auto* f = VfsValueExtFactory::FindFactoryEon("entity", VFSEXT_DEFAULT);
		if (!f)
			f = VfsValueExtFactory::FindFactory("Entity");
		if (f) {
			node.ext = f->new_fn(node);
			node.type_hash = node.ext ? node.ext->GetTypeHash() : 0;
		}
		return node;
	};
	auto add_mapping = [&](const VfsValue* ecs_node, VfsValue* geom_node, bool map_geom_to_ecs) {
		if (!ecs_node || !geom_node)
			return;
		int idx = ecs_to_geom.Find(ecs_node);
		if (idx >= 0)
			ecs_to_geom[idx] = geom_node;
		else
			ecs_to_geom.Add(ecs_node, geom_node);
		if (map_geom_to_ecs) {
			int gidx = geom_to_ecs.Find(geom_node);
			if (gidx >= 0)
				geom_to_ecs[gidx] = const_cast<VfsValue*>(ecs_node);
			else
				geom_to_ecs.Add(geom_node, const_cast<VfsValue*>(ecs_node));
		}
	};
	
	std::function<void(VfsValue&, VfsValue&)> build_node = [&](VfsValue& geom_node, VfsValue& ecs_parent) {
		auto add_bone = [&](auto&& add_bone, VfsValue& ecs_parent, VfsValue& bone_node) -> void {
			GeomBone& bone = bone_node.GetExt<GeomBone>();
			String name = bone.name.IsEmpty() ? bone_node.id : bone.name;
			VfsValue& ecs_bone = ecs_parent.Add(name);
			add_mapping(&ecs_bone, &bone_node, false);
			for (auto& sub : bone_node.sub) {
				if (IsVfsType(sub, AsTypeHash<GeomBone>()))
					add_bone(add_bone, ecs_bone, sub);
			}
		};
		
		for (auto& s : geom_node.sub) {
			if (IsVfsType(s, AsTypeHash<GeomDirectory>())) {
				GeomDirectory& dir = s.GetExt<GeomDirectory>();
				String name = dir.name.IsEmpty() ? s.id : dir.name;
				VfsValue& ecs_dir = ecs_parent.Add(name);
				add_mapping(&ecs_dir, &s, false);
				build_node(s, ecs_dir);
				continue;
			}
			if (IsVfsType(s, AsTypeHash<GeomObject>())) {
				GeomObject& o = s.GetExt<GeomObject>();
				String name = o.name.IsEmpty() ? s.id : o.name;
				VfsValue& ecs_ent = create_entity(ecs_parent, name);
				add_mapping(&ecs_ent, &s, true);
				Entity* ent = dynamic_cast<Entity*>(&*ecs_ent.ext);
				if (ent) {
					ComponentPtr tr = ent->CreateEon("transform3");
					if (tr) {
						tr->val.id = "Transform";
						add_mapping(&tr->val, &s, false);
					}
					if (o.type == GeomObject::O_MODEL) {
						ComponentPtr mdl = ent->CreateEon("model");
						if (mdl) {
							mdl->val.id = "Model";
							add_mapping(&mdl->val, &s, false);
						}
					}
					if (o.type == GeomObject::O_CAMERA) {
						ComponentPtr view = ent->CreateEon("viewable");
						if (view) {
							view->val.id = "Viewable";
							add_mapping(&view->val, &s, false);
						}
						ComponentPtr vp = ent->CreateEon("viewport");
						if (vp) {
							vp->val.id = "Viewport";
							add_mapping(&vp->val, &s, false);
						}
					}
				}
				for (auto& sub : s.sub) {
					if (!IsVfsType(sub, AsTypeHash<GeomSkeleton>()))
						continue;
					GeomSkeleton& sk = sub.GetExt<GeomSkeleton>();
					String sk_name = sk.name.IsEmpty() ? sub.id : sk.name;
					VfsValue& ecs_sk = ecs_ent.Add(sk_name);
					add_mapping(&ecs_sk, &sub, false);
					for (auto& b : sub.sub) {
						if (IsVfsType(b, AsTypeHash<GeomBone>()))
							add_bone(add_bone, ecs_sk, b);
					}
				}
				continue;
			}
			if (IsVfsType(s, AsTypeHash<GeomPointcloudDataset>())) {
				GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
				String name = ds.name.IsEmpty() ? s.id : ds.name;
				VfsValue& ecs_ent = create_entity(ecs_parent, name);
				add_mapping(&ecs_ent, &s, true);
				Entity* ent = dynamic_cast<Entity*>(&*ecs_ent.ext);
				if (ent) {
					ComponentPtr tr = ent->CreateEon("transform3");
					if (tr) {
						tr->val.id = "Transform";
						add_mapping(&tr->val, &s, false);
					}
					ComponentPtr mdl = ent->CreateEon("model");
					if (mdl) {
						mdl->val.id = "Pointcloud";
						add_mapping(&mdl->val, &s, false);
					}
				}
				continue;
			}
		}
	};
	
	int scene_idx = 0;
	for (auto& s : prj.val.sub) {
		if (!IsVfsType(s, AsTypeHash<GeomScene>()))
			continue;
		GeomScene& scene = s.GetExt<GeomScene>();
		String name = scene.name.IsEmpty() ? "Scene #" + IntStr(scene_idx) : scene.name;
		VfsValue& ecs_scene = ecs_root_val.Add(name);
		add_mapping(&ecs_scene, &scene.val, true);
		build_node(scene.val, ecs_scene);
		scene_idx++;
	}
}

void GeomProjectCtrl::TreeValueEcs(int id, VfsValue& node) {
	auto warn_unknown = [&](const VfsValue& n) {
		hash_t type_hash = n.ext ? n.ext->GetTypeHash() : n.type_hash;
		if (warned_tree_types.Find(type_hash) >= 0)
			return;
		warned_tree_types.Add(type_hash);
		LOG("GeomProjectCtrl: unexpected VfsValue type in ECS tree: " + n.GetTypeString());
	};
	auto get_geom_node = [&](const VfsValue& ecs_node) -> VfsValue* {
		int idx = ecs_to_geom.Find(&ecs_node);
		if (idx >= 0)
			return ecs_to_geom[idx];
		return nullptr;
	};
	auto add_entity = [&](int parent_id, VfsValue& ecs_node) -> void {
		VfsValue* geom_node = get_geom_node(ecs_node);
		GeomObject* obj = (geom_node && IsVfsType(*geom_node, AsTypeHash<GeomObject>())) ? &geom_node->GetExt<GeomObject>() : nullptr;
		GeomPointcloudDataset* ds = (geom_node && IsVfsType(*geom_node, AsTypeHash<GeomPointcloudDataset>())) ? &geom_node->GetExt<GeomPointcloudDataset>() : nullptr;
		Image img = ImagesImg::Object();
		if (obj) {
			switch (obj->type) {
				case GeomObject::O_CAMERA: img = ImagesImg::Camera(); break;
				case GeomObject::O_MODEL:  img = ImagesImg::Model(); break;
				case GeomObject::O_OCTREE: img = ImagesImg::Octree(); break;
				default: img = ImagesImg::Object();
			}
		}
		else if (ds) {
			img = ImagesImg::Octree();
		}
		String name = ecs_node.id.IsEmpty() ? ecs_node.GetTypeString() : ecs_node.id;
		int j = tree.Add(parent_id, img, RawToValue(&ecs_node), name);
		if (obj) {
			tree.SetRowValue(j, tree_col_visible, obj->is_visible);
			tree.SetRowValue(j, tree_col_locked, obj->is_locked);
			tree.SetRowValue(j, tree_col_read, obj->read_enabled);
			tree.SetRowValue(j, tree_col_write, obj->write_enabled);
		}
		else {
			tree.SetRowValue(j, tree_col_visible, Null);
			tree.SetRowValue(j, tree_col_locked, Null);
			tree.SetRowValue(j, tree_col_read, Null);
			tree.SetRowValue(j, tree_col_write, Null);
		}
		TreeValueEcs(j, ecs_node);
	};
	for (auto& sub : node.sub) {
		if (sub.ext && dynamic_cast<Entity*>(&*sub.ext)) {
			add_entity(id, sub);
			continue;
		}
		if (sub.ext && dynamic_cast<Component*>(&*sub.ext)) {
			String name = sub.id.IsEmpty() ? sub.GetTypeString() : sub.id;
			int j = tree.Add(id, ImagesImg::Object(), RawToValue(&sub), name);
			tree.SetRowValue(j, tree_col_visible, Null);
			tree.SetRowValue(j, tree_col_locked, Null);
			tree.SetRowValue(j, tree_col_read, Null);
			tree.SetRowValue(j, tree_col_write, Null);
			continue;
		}
		if (sub.ext) {
			warn_unknown(sub);
		}
		String name = sub.id.IsEmpty() ? sub.GetTypeString() : sub.id;
		int j = tree.Add(id, ImagesImg::Directory(), RawToValue(&sub), name);
		tree.SetRowValue(j, tree_col_visible, Null);
		tree.SetRowValue(j, tree_col_locked, Null);
		tree.SetRowValue(j, tree_col_read, Null);
		tree.SetRowValue(j, tree_col_write, Null);
		TreeValueEcs(j, sub);
	}
}

void GeomProjectCtrl::Data() {
	GeomProject& prj = *e->prj;
	StoreTreeOpenState();

	Index<const VfsValue*> open_nodes;
	Value cursor_value;
	int old_cursor = tree.GetCursor();
	if (old_cursor >= 0)
		cursor_value = tree.Get(old_cursor);
	for (int i = 0; i < tree.GetCount(); i++) {
		if (!tree.IsOpen(i))
			continue;
		Value v = tree.Get(i);
		if (v.Is<VfsValue*>())
			open_nodes.FindAdd(ValueTo<VfsValue*>(v));
	}
	
	tree.Clear();
	String root_name = "Project";
	if (e && e->state && e->prj && e->state->active_scene >= 0 && e->state->active_scene < e->prj->GetSceneCount()) {
		GeomScene& scene = e->prj->GetScene(e->state->active_scene);
		if (!scene.name.IsEmpty())
			root_name = scene.name;
		else
			root_name = "Scene #" + IntStr(e->state->active_scene);
	}
	tree.SetRoot(ImagesImg::Root(), root_name);
	
	tree_nodes.Clear();
	int builtin = tree.Add(0, ImagesImg::Cameras(), "Builtin");
	TreeNodeRef& program = tree_nodes.Add();
	program.kind = TreeNodeRef::K_PROGRAM;
	TreeNodeRef& focus = tree_nodes.Add();
	focus.kind = TreeNodeRef::K_FOCUS;
	int program_id = tree.Add(builtin, ImagesImg::Camera(), RawToValue(&program), "Program Camera");
	int focus_id = tree.Add(builtin, ImagesImg::Camera(), RawToValue(&focus), "Focus Camera");
	tree.SetRowValue(program_id, tree_col_visible, e->state->program_visible);
	tree.SetRowValue(program_id, tree_col_locked, false);
	tree.SetRowValue(program_id, tree_col_read, program_read);
	tree.SetRowValue(program_id, tree_col_write, program_write);
	tree.SetRowValue(focus_id, tree_col_visible, e->state->focus_visible);
	tree.SetRowValue(focus_id, tree_col_locked, false);
	tree.SetRowValue(focus_id, tree_col_read, focus_read);
	tree.SetRowValue(focus_id, tree_col_write, focus_write);

	
	tree_scenes = tree.Add(0, ImagesImg::Scenes(), "Scenes");
	if (use_ecs_tree)
		BuildEcsMirror();
	
	int scene_idx = 0;
	if (use_ecs_tree) {
		for (auto& scene_node : ecs_root_val.sub) {
			String name = scene_node.id.IsEmpty() ? "Scene #" + IntStr(scene_idx) : scene_node.id;
			int j = tree.Add(tree_scenes, ImagesImg::Scene(), RawToValue(&scene_node), name);
			tree.SetRowValue(j, tree_col_visible, Null);
			tree.SetRowValue(j, tree_col_locked, Null);
			tree.SetRowValue(j, tree_col_read, Null);
			tree.SetRowValue(j, tree_col_write, Null);
			
			TreeValueEcs(j, scene_node);
			
			if (scene_idx == 0 && !tree.HasFocus())
				tree.SetCursor(j);
			scene_idx++;
		}
	}
	else {
		for (auto& s : prj.val.sub) {
			if (!IsVfsType(s, AsTypeHash<GeomScene>()))
				continue;
			GeomScene& scene = s.GetExt<GeomScene>();
			String name = scene.name.IsEmpty() ? "Scene #" + IntStr(scene_idx) : scene.name;
			int j = tree.Add(tree_scenes, ImagesImg::Scene(), RawToValue(&scene.val), name);
			tree.SetRowValue(j, tree_col_visible, Null);
			tree.SetRowValue(j, tree_col_locked, Null);
			tree.SetRowValue(j, tree_col_read, Null);
			tree.SetRowValue(j, tree_col_write, Null);
			
			TreeValue(j, scene.val);
			
			if (scene_idx == 0 && !tree.HasFocus())
				tree.SetCursor(j);
			scene_idx++;
		}
	}
	
	/*for(int i = 0; i < prj.octrees.GetCount(); i++) {
		OctreePointModel& o = prj.octrees[i];
		String name = prj.dictionary[o.id];
		tree.Add(tree_octrees, ImagesImg::Octree(), o.id, name);
	}*/
	for (int i = 0; i < tree.GetCount(); i++) {
		Value v = tree.Get(i);
		if (v.Is<VfsValue*>()) {
			if (open_nodes.Find(ValueTo<VfsValue*>(v)) >= 0)
				tree.Open(i);
		}
	}
	tree.Open(0);
	RestoreTreeOpenState();
	
	if (cursor_value.Is<VfsValue*>()) {
		VfsValue* target = ValueTo<VfsValue*>(cursor_value);
		for (int i = 0; i < tree.GetCount(); i++) {
			Value v = tree.Get(i);
			if (v.Is<VfsValue*>() && ValueTo<VfsValue*>(v) == target) {
				tree.SetCursor(i);
				break;
			}
		}
	}
	else if (cursor_value.Is<TreeNodeRef*>()) {
		TreeNodeRef* ref = ValueTo<TreeNodeRef*>(cursor_value);
		for (int i = 0; i < tree.GetCount(); i++) {
			TreeNodeRef* cur = GetNodeRef(tree.Get(i));
			if (cur && ref && cur->kind == ref->kind) {
				tree.SetCursor(i);
				break;
			}
		}
	}
	
	TreeSelect();
}

void GeomProjectCtrl::TreeSelect() {
	int cursor = tree.GetCursor();
	if (cursor < 0)
		return;
	Value v = tree.Get(cursor);
	VfsValue* geom_node = ResolveGeomValue(v);
	if (!current_tree_path.IsEmpty())
		StorePropsCursor(current_tree_path);
	e->selected_bone = nullptr;
	TreeNodeRef* ref = GetNodeRef(v);
	GeomObject* obj = GetNodeObject(v);
	GeomPointcloudDataset* ds = GetNodeDataset(v);
	current_tree_path = GetTreePathForValue(v, cursor);
	selected_dataset = ds;
	if (!obj && geom_node) {
		if (IsVfsType(*geom_node, AsTypeHash<GeomBone>())) {
			e->selected_bone = geom_node;
			for (VfsValue* p = geom_node->owner; p; p = p->owner) {
				if (IsVfsType(*p, AsTypeHash<GeomObject>())) {
					obj = &p->GetExt<GeomObject>();
					break;
				}
			}
			if (obj) {
				e->state->focus_mode = 1;
				e->state->focus_object_key = obj->key;
			}
		}
	}
	if (ref) {
		if (ref->kind == TreeNodeRef::K_PROGRAM) {
			e->state->focus_mode = 2;
			e->state->focus_object_key = 0;
		}
		if (ref->kind == TreeNodeRef::K_FOCUS) {
			e->state->focus_mode = 3;
			e->state->focus_object_key = 0;
		}
		UpdateTreeFocus(cursor);
		PropsData();
		RestorePropsCursor(current_tree_path);
		e->state->UpdateObjects();
		RefreshAll();
		TimelineData();
		return;
	}
	if (obj) {
		e->timeline_scope = Edit3D::TS_OBJECT;
		e->timeline_component = Edit3D::TC_TRANSFORM;
		e->timeline_transform_field = Edit3D::TT_NONE;
		e->timeline_object_key = obj->key;
		e->state->focus_mode = 1;
		e->state->focus_object_key = obj->key;
		UpdateTreeFocus(cursor);
		PropsData();
		RestorePropsCursor(current_tree_path);
		e->UpdateTextureEditor(obj);
		e->state->UpdateObjects();
		RefreshAll();
		TimelineData();
		return;
	}
	if (ds) {
		e->state->focus_mode = 0;
		e->state->focus_object_key = 0;
		UpdateTreeFocus(cursor);
		PropsData();
		RestorePropsCursor(current_tree_path);
		e->state->UpdateObjects();
		RefreshAll();
		TimelineData();
		return;
	}
	if (!v.Is<VfsValue*>())
		return;
	VfsValue* node = ValueTo<VfsValue*>(v);
	if (!node)
		return;
	if (IsVfsType(*node, AsTypeHash<GeomScene>())) {
		int idx = 0;
		for (auto& s : e->prj->val.sub) {
			if (!IsVfsType(s, AsTypeHash<GeomScene>()))
				continue;
			if (&s == node) {
				e->timeline_scope = Edit3D::TS_SCENE;
				e->timeline_component = Edit3D::TC_NONE;
				e->timeline_transform_field = Edit3D::TT_NONE;
				e->timeline_object_key = 0;
				e->state->active_scene = idx;
				e->state->UpdateObjects();
				RefreshAll();
				break;
			}
			idx++;
		}
	}
	else if (IsVfsType(*node, AsTypeHash<GeomDirectory>())) {
		e->state->focus_mode = 0;
		e->state->focus_object_key = 0;
		e->state->UpdateObjects();
		RefreshAll();
	}
	UpdateTreeFocus(cursor);
	PropsData();
	RestorePropsCursor(current_tree_path);
	e->UpdateTextureEditor(obj);
	e->render_ctx.selected_bone = e->selected_bone;
	e->render_ctx.show_weights = e->weight_paint_mode;
	e->render_ctx.weight_bone.Clear();
	if (e->selected_bone && IsVfsType(*e->selected_bone, AsTypeHash<GeomBone>())) {
		GeomBone& bone = e->selected_bone->GetExt<GeomBone>();
		e->render_ctx.weight_bone = bone.name.IsEmpty() ? e->selected_bone->id : bone.name;
	}
	TimelineData();
}

GeomProjectCtrl::TreeNodeRef* GeomProjectCtrl::GetNodeRef(const Value& v) {
	if (v.Is<TreeNodeRef*>())
		return ValueTo<TreeNodeRef*>(v);
	return 0;
}

GeomObject* GeomProjectCtrl::GetNodeObject(const Value& v) {
	VfsValue* node = ResolveGeomValue(v);
	if (!node)
		return 0;
	if (!IsVfsType(*node, AsTypeHash<GeomObject>()))
		return 0;
	return &node->GetExt<GeomObject>();
}

GeomPointcloudDataset* GeomProjectCtrl::GetNodeDataset(const Value& v) {
	VfsValue* node = ResolveGeomValue(v);
	if (!node)
		return 0;
	hash_t dataset_hash = TypedStringHasher<GeomPointcloudDataset>("GeomPointcloudDataset");
	if (!IsVfsType(*node, dataset_hash))
		return 0;
	return &node->GetExt<GeomPointcloudDataset>();
}

String GeomProjectCtrl::GetTreePathFromId(int id) const {
	Vector<String> parts;
	for (int cur = id; cur >= 0; cur = tree.GetParent(cur)) {
		String label = AsString(tree.GetValue(cur));
		if (label.IsEmpty())
			label = AsString(tree.Get(cur));
		if (!label.IsEmpty())
			parts.Add(label);
	}
	Reverse(parts);
	return Join(parts, "/");
}

String GeomProjectCtrl::GetTreePathForValue(const Value& v, int id) const {
	TreeNodeRef* ref = v.Is<TreeNodeRef*>() ? ValueTo<TreeNodeRef*>(v) : 0;
	if (ref) {
		if (ref->kind == TreeNodeRef::K_PROGRAM)
			return "builtin/program";
		if (ref->kind == TreeNodeRef::K_FOCUS)
			return "builtin/focus";
	}
	if (v.Is<VfsValue*>()) {
		VfsValue* node = ValueTo<VfsValue*>(v);
		if (node) {
			VfsPath path = node->GetPath();
			String path_str = path.ToString();
			if (!path_str.IsEmpty())
				return path_str;
		}
	}
	if (id >= 0)
		return GetTreePathFromId(id);
	return String();
}

String GeomProjectCtrl::GetPropsPathForId(int id) const {
	Vector<String> parts;
	for (int cur = id; cur >= 0; cur = props.GetParent(cur)) {
		if (cur == 0)
			break;
		String label = AsString(props.GetValue(cur));
		if (label.IsEmpty())
			label = AsString(props.Get(cur));
		if (!label.IsEmpty())
			parts.Add(label);
	}
	Reverse(parts);
	return Join(parts, "/");
}

int GeomProjectCtrl::FindPropsIdByPath(const String& path, bool open) {
	if (path.IsEmpty())
		return -1;
	Vector<String> parts = Split(path, '/');
	if (!parts.IsEmpty() && parts[0] == "Properties")
		parts.Remove(0);
	int cur = 0;
	for (int i = 0; i < parts.GetCount(); i++) {
		const String& part = parts[i];
		int found = -1;
		int child_count = props.GetChildCount(cur);
		for (int j = 0; j < child_count; j++) {
			int child = props.GetChild(cur, j);
			String label = AsString(props.GetValue(child));
			if (label.IsEmpty())
				label = AsString(props.Get(child));
			if (label == part) {
				found = child;
				break;
			}
		}
		if (found < 0)
			return -1;
		if (open)
			props.Open(found, true);
		cur = found;
	}
	return cur;
}

Vector<int> GeomProjectCtrl::GetPropsIndexPathForId(int id) const {
	Vector<int> parts;
	for (int cur = id; cur >= 0; cur = props.GetParent(cur)) {
		int parent = props.GetParent(cur);
		if (parent < 0)
			break;
		int idx = props.GetChildIndex(parent, cur);
		parts.Add(idx);
	}
	Reverse(parts);
	return parts;
}

int GeomProjectCtrl::FindPropsIdByIndexPath(const Vector<int>& path, bool open) {
	if (path.IsEmpty())
		return -1;
	int cur = 0;
	for (int i = 0; i < path.GetCount(); i++) {
		int idx = path[i];
		int child_count = props.GetChildCount(cur);
		if (idx < 0 || idx >= child_count)
			return -1;
		int child = props.GetChild(cur, idx);
		if (open)
			props.Open(child, true);
		cur = child;
	}
	return cur;
}

Vector<GeomProjectCtrl::PropsCursorState::PropPathToken> GeomProjectCtrl::GetPropsTokensForId(int id) const {
	auto safe_label = [&](const Value& v) -> String {
		if (v.Is<String>())
			return v.To<String>();
		if (v.Is<WString>())
			return v.To<WString>().ToString();
		return String();
	};
	Vector<PropsCursorState::PropPathToken> out;
	for (int cur = id; cur >= 0; cur = props.GetParent(cur)) {
		if (cur == 0)
			break;
		PropsCursorState::PropPathToken t;
		String label = safe_label(props.GetValue(cur));
		t.label = label;
		Value v = props.Get(cur);
		if (v.Is<PropRef*>()) {
			PropRef* pr = ValueTo<PropRef*>(v);
			if (pr) {
				t.kind = (int)pr->kind;
				t.material_id = pr->material_id;
				t.mesh_index = pr->mesh_index;
			}
		}
		out.Add(t);
	}
	Reverse(out);
	return out;
}

int GeomProjectCtrl::FindPropsIdByTokens(const Vector<PropsCursorState::PropPathToken>& tokens, bool open) {
	if (tokens.IsEmpty())
		return -1;
	auto safe_label = [&](const Value& v) -> String {
		if (v.Is<String>())
			return v.To<String>();
		if (v.Is<WString>())
			return v.To<WString>().ToString();
		return String();
	};
	int cur = 0;
	for (int i = 0; i < tokens.GetCount(); i++) {
		const PropsCursorState::PropPathToken& want = tokens[i];
		int found = -1;
		int child_count = props.GetChildCount(cur);
		for (int j = 0; j < child_count; j++) {
			int child = props.GetChild(cur, j);
			PropsCursorState::PropPathToken have;
			have.label = safe_label(props.GetValue(child));
			Value v = props.Get(child);
			if (v.Is<PropRef*>()) {
				PropRef* pr = ValueTo<PropRef*>(v);
			if (pr) {
				have.kind = (int)pr->kind;
				have.material_id = pr->material_id;
				have.mesh_index = pr->mesh_index;
			}
			}
			bool match = have.kind == want.kind &&
			             have.material_id == want.material_id &&
			             have.mesh_index == want.mesh_index &&
			             have.label == want.label;
			if (match) {
				found = child;
				break;
			}
		}
		if (found < 0) {
			// fallback: label-only match
			for (int j = 0; j < child_count; j++) {
				int child = props.GetChild(cur, j);
				String label = safe_label(props.GetValue(child));
				if (label == want.label) {
					found = child;
					break;
				}
			}
		}
		if (found < 0)
			return -1;
		if (open)
			props.Open(found, true);
		cur = found;
	}
	return cur;
}

void GeomProjectCtrl::StorePropsCursor(const String& tree_path) {
	if (tree_path.IsEmpty())
		return;
	int id = props.GetCursor();
	if (id < 0) {
		props_cursor_by_tree.RemoveKey(tree_path);
		return;
	}
	int line = props.GetLineAtItem(id);
	PropsCursorState st;
	st.tokens = GetPropsTokensForId(id);
	st.index_path = GetPropsIndexPathForId(id);
	st.line = line;
	st.scroll = props.GetScroll();
	if (st.tokens.IsEmpty() && st.index_path.IsEmpty() && st.line < 0)
		props_cursor_by_tree.RemoveKey(tree_path);
	else
		props_cursor_by_tree.GetAdd(tree_path) = pick(st);
}

void GeomProjectCtrl::RestorePropsCursor(const String& tree_path) {
	if (tree_path.IsEmpty())
		return;
	int idx = props_cursor_by_tree.Find(tree_path);
	if (idx < 0)
		return;
	const PropsCursorState& st = props_cursor_by_tree[idx];
	bool set = false;
	if (!st.tokens.IsEmpty()) {
		int id = FindPropsIdByTokens(st.tokens, true);
		if (id >= 0) {
			props.SetCursor(id);
			set = true;
		}
	}
	if (!set && !st.index_path.IsEmpty()) {
		int id = FindPropsIdByIndexPath(st.index_path, true);
		if (id >= 0) {
			props.SetCursor(id);
			set = true;
		}
	}
	if (!set && st.line >= 0 && st.line < props.GetLineCount()) {
		int id = props.GetItemAtLine(st.line);
		if (id >= 0) {
			props.SetCursor(id);
			set = true;
		}
	}
	props.ScrollTo(max(st.scroll, 0));
	props.Refresh();
}

void GeomProjectCtrl::StoreTreeOpenState() {
	tree_open_paths.Clear();
	int count = tree.GetCount();
	for (int i = 0; i < count; i++) {
		if (!tree.IsOpen(i))
			continue;
		String path = GetTreePathForValue(tree.Get(i), i);
		if (!path.IsEmpty())
			tree_open_paths.Add(path);
	}
}

void GeomProjectCtrl::RestoreTreeOpenState() {
	if (tree_open_paths.IsEmpty())
		return;
	int count = tree.GetCount();
	for (const String& path : tree_open_paths) {
		for (int i = 0; i < count; i++) {
			String cur = GetTreePathForValue(tree.Get(i), i);
			if (cur == path) {
				tree.Open(i, true);
				break;
			}
		}
	}
}

void GeomProjectCtrl::UpdateTreeFocus(int new_id) {
	if (focus_tree_id >= 0) {
		int linei = tree.GetLineAtItem(focus_tree_id);
		if (linei >= 0)
			tree.SetLineColor(linei, Null);
	}
	focus_tree_id = new_id;
	if (focus_tree_id >= 0) {
		int linei = tree.GetLineAtItem(focus_tree_id);
		if (linei >= 0)
			tree.SetLineColor(linei, Blend(SColorHighlight(), SColorPaper(), 200));
	}
	if (e)
		e->UpdateRibbonContext();
}

void GeomProjectCtrl::TreeMenu(Bar& bar) {
	int cursor = tree.GetCursor();
	if (cursor < 0 && focus_tree_id >= 0)
		cursor = focus_tree_id;
	if (cursor < 0) {
		int line0 = tree.GetLineCount() > 0 ? tree.GetItemAtLine(0) : -1;
		if (tree.IsValid(line0))
			cursor = line0;
	}
	if (cursor < 0)
		return;
	Value v = tree.Get(cursor);
	TreeNodeRef* ref = GetNodeRef(v);
	GeomObject* obj = GetNodeObject(v);
	VfsValue* node = v.Is<VfsValue*>() ? ValueTo<VfsValue*>(v) : 0;
	bool is_root = (cursor == 0);
	GeomDirectory* dir = 0;
	if (node && IsVfsType(*node, AsTypeHash<GeomDirectory>()))
		dir = &node->GetExt<GeomDirectory>();
	if (!dir && node && IsVfsType(*node, AsTypeHash<GeomScene>()))
		dir = &node->GetExt<GeomScene>();
	if (!dir && node && IsVfsType(*node, AsTypeHash<GeomObject>()) && node->owner) {
		if (IsVfsType(*node->owner, AsTypeHash<GeomDirectory>()))
			dir = &node->owner->GetExt<GeomDirectory>();
		else if (IsVfsType(*node->owner, AsTypeHash<GeomScene>()))
			dir = &node->owner->GetExt<GeomScene>();
	}
	if (is_root && !dir && e && e->state)
		dir = &e->state->GetActiveScene();
	if (!ref && !obj && !dir && !is_root)
		return;
	bool can_create_path_node = false;
	if (obj) {
		if (obj->ui_type == "path" || ToLower(obj->name).Find("path") >= 0)
			can_create_path_node = true;
	}
	if (!is_root)
		bar.Add(t_("Clone"), [=] {
			if (!node || !node->owner)
				return;
			if (IsVfsType(*node, AsTypeHash<GeomScene>())) {
				LOG("Clone scene not implemented.");
				return;
		}
		if (e)
			e->PushUndo("Clone node");
		String base = node->id.IsEmpty() ? String("node") : node->id;
		String name = base + "_copy";
		int idx = 1;
		while (node->owner->Find(name) >= 0)
			name = base + "_copy" + IntStr(idx++);
		VfsValue& dst = node->owner->Add(name, node->type_hash);
		dst.CopyFrom(*node);
		dst.id = name;
		dst.owner = node->owner;
		dst.FixParent();
		if (IsVfsType(dst, AsTypeHash<GeomObject>()) && e && e->prj) {
			GeomObject& go = dst.GetExt<GeomObject>();
			go.key = e->prj->NewKey();
		}
		if (e && e->state) {
			e->state->UpdateObjects();
			e->RefreshData();
		}
		}).Key(K_CTRL|K_C);
	if (!is_root)
		bar.Add(t_("Delete"), [=] {
			if (!node || !node->owner)
				return;
			if (IsVfsType(*node, AsTypeHash<GeomScene>())) {
				LOG("Delete scene not implemented.");
				return;
		}
		if (e)
			e->PushUndo("Delete node");
		node->owner->Remove(node);
		if (e && e->state) {
			e->state->UpdateObjects();
			e->RefreshData();
		}
		}).Key(K_DELETE);
	if (!is_root)
		bar.Sub(t_("Modify Selection"), [=](Bar& bar) {
			const bool has_mesh = obj && obj->IsModel() && obj->mdl;
			const bool has_asset = obj && obj->IsModel() && !obj->asset_ref.IsEmpty();
			const bool can_copy_behaviors = IsScriptOwnerNode(node);
			const bool can_paste_behaviors = IsScriptOwnerNode(node) && !s_behavior_clipboard.IsEmpty();
			
			auto apply_mesh_change = [&](const String& undo_label, const std::function<void(Model&)>& fn) {
				if (!e || !obj || !obj->IsModel() || !obj->mdl)
					return;
				e->PushUndo(undo_label);
				fn(*obj->mdl);
				if (e->state)
					e->state->UpdateObjects();
				e->RefreshData();
				e->RefrehRenderers();
			};
			
			bar.Add(t_("Set planar texture coordinates for mesh"), [=] {
				apply_mesh_change("Set planar UV", [&](Model& mdl) {
					vec3 mn(DBL_MAX, DBL_MAX, DBL_MAX);
					vec3 mx(-DBL_MAX, -DBL_MAX, -DBL_MAX);
					int vert_count = 0;
					for (Mesh& mesh : mdl.meshes) {
						for (const Vertex& vtx : mesh.vertices) {
							const vec3 p = vtx.position.Splice();
							mn = Min(mn, p);
							mx = Max(mx, p);
							vert_count++;
						}
					}
					if (vert_count == 0)
						return;
					double sx = max((double)(mx[0] - mn[0]), 1e-6);
					double sy = max((double)(mx[1] - mn[1]), 1e-6);
					double sz = max((double)(mx[2] - mn[2]), 1e-6);
					bool use_xy = sz <= 1e-5 && sy > 1e-5;
					for (Mesh& mesh : mdl.meshes) {
						for (Vertex& vtx : mesh.vertices) {
							const vec3 p = vtx.position.Splice();
							double u = (p[0] - mn[0]) / sx;
							double vv = use_xy ? ((p[1] - mn[1]) / sy) : ((p[2] - mn[2]) / sz);
							vtx.tex_coord = vec2((float)u, (float)vv);
						}
					}
				});
			}).Enable(has_mesh);
			bar.Add(t_("Flip direction of all faces"), [=] {
				apply_mesh_change("Flip mesh faces", [&](Model& mdl) {
					for (Mesh& mesh : mdl.meshes)
						mesh.ReverseFaces();
				});
			}).Enable(has_mesh);
			bar.Add(t_("Center pivot point"), [=] {
				apply_mesh_change("Center mesh pivot", [&](Model& mdl) {
					for (Mesh& mesh : mdl.meshes)
						mesh.CenterAnchor();
				});
			}).Enable(has_mesh);
			bar.Add(t_("Free scale and normalize normals"), [=] {
				if (!obj || !obj->mdl)
					return;
				vec3 old_scale(1, 1, 1);
				if (GeomTransform* tr = obj->FindTransform())
					old_scale = tr->scale;
				apply_mesh_change("Freeze scale and normalize normals", [=](Model& mdl) {
					vec3 scale = old_scale;
					for (Mesh& mesh : mdl.meshes) {
						for (Vertex& vtx : mesh.vertices) {
							vec3 p = vtx.position.Splice();
							p[0] *= scale[0];
							p[1] *= scale[1];
							p[2] *= scale[2];
							vtx.position = p.Embed();
							vec3 n = vtx.normal;
							double sx = fabs((double)scale[0]) > 1e-6 ? scale[0] : 1.0;
							double sy = fabs((double)scale[1]) > 1e-6 ? scale[1] : 1.0;
							double sz = fabs((double)scale[2]) > 1e-6 ? scale[2] : 1.0;
							n[0] = (float)(n[0] / sx);
							n[1] = (float)(n[1] / sy);
							n[2] = (float)(n[2] / sz);
							vtx.normal = safe_normalize(n);
						}
						mesh.UpdateBoundingBox();
					}
				});
				if (GeomTransform* tr = obj->FindTransform())
					tr->scale = vec3(1, 1, 1);
				if (e) {
					e->RefreshData();
					e->RefrehRenderers();
				}
			}).Enable(has_mesh);
			bar.Add(t_("Clear vertex colors"), [=] {
				apply_mesh_change("Clear vertex colors", [&](Model& mdl) {
					for (int i = 0; i < mdl.materials.GetCount(); i++)
						mdl.materials[i].params->base_clr_factor = vec4(1, 1, 1, 1);
				});
			}).Enable(has_mesh);
			bar.Add(t_("Set vertex colors"), [=] {
				apply_mesh_change("Set vertex colors", [&](Model& mdl) {
					for (int i = 0; i < mdl.materials.GetCount(); i++)
						mdl.materials[i].params->base_clr_factor = vec4(0.85f, 0.85f, 0.85f, 1.0f);
				});
			}).Enable(has_mesh);
			bar.Add(t_("Recalculate normals and tangents"), [=] {
				apply_mesh_change("Recalculate normals", [&](Model& mdl) {
					for (Mesh& mesh : mdl.meshes)
						mesh.UpdateNormalsAndTangents();
				});
			}).Enable(has_mesh);
			bar.Add(t_("Clone as static animated mesh"), [=] {
				if (!node || !node->owner || !obj || !obj->mdl || !e || !e->prj)
					return;
				e->PushUndo("Clone as static animated mesh");
				String base = node->id.IsEmpty() ? String("node") : node->id;
				String name = base + "_static_anim";
				int idx = 1;
				while (node->owner->Find(name) >= 0)
					name = base + "_static_anim_" + IntStr(idx++);
				VfsValue& dst = node->owner->Add(name, node->type_hash);
				dst.CopyFrom(*node);
				dst.id = name;
				dst.owner = node->owner;
				dst.FixParent();
				GeomObject& clone = dst.GetExt<GeomObject>();
				clone.key = e->prj->NewKey();
				if (clone.mdl && !clone.mdl->meshes.IsEmpty()) {
					GeomMeshAnimation& anim = clone.GetMeshAnimation();
					anim.keyframes.Clear();
					GeomMeshKeyframe& kf = anim.GetAddKeyframe(0);
					kf.points.Clear();
					for (const Vertex& vtx : clone.mdl->meshes[0].vertices)
						kf.points.Add(vtx.position.Splice());
				}
				if (e->state)
					e->state->UpdateObjects();
				e->RefreshData();
				e->RefrehRenderers();
			}).Enable(has_mesh);
			bar.Add(t_("Distribute over terrain"), [=] {
				PromptOK(t_("Distribute over terrain is not implemented yet."));
			}).Enable(false);
			bar.Add(t_("Bake all textures of selection into one"), [=] {
				PromptOK(t_("Texture baking into one map is not implemented yet."));
			}).Enable(false);
			bar.Separator();
			bar.Add(t_("Edit 2D room map"), [] {}).Enable(false);
			bar.Add(t_("Open in animation editor"), [] {}).Enable(false);
			bar.Add(t_("Attach node to animated joint"), [] {}).Enable(false);
			bar.Separator();
			bar.Add(t_("Export mesh as"), [] {}).Enable(false);
			bar.Add(t_("Reload this mesh from disk"), [=] {
				if (!e || !obj || !obj->IsModel())
					return;
				if (obj->asset_ref.IsEmpty()) {
					PromptOK(t_("Selected mesh does not have an asset path."));
					return;
				}
				if (obj->asset_ref.StartsWith("preset:")) {
					PromptOK(t_("Preset meshes cannot be reloaded from disk."));
					return;
				}
				String abs = ResolveAssetPathForReload(*e, obj->asset_ref);
				if (!FileExists(abs)) {
					PromptOK(t_("Asset file not found: ") + abs);
					return;
				}
				ModelLoader loader;
				if (!loader.LoadModel(abs)) {
					PromptOK(t_("Failed to reload mesh from disk."));
					return;
				}
				e->PushUndo("Reload mesh");
				obj->mdl.Create();
				*obj->mdl = loader.Realize();
				if (e->state)
					e->state->UpdateObjects();
				e->RefreshData();
				e->RefrehRenderers();
			}).Enable(has_asset);
			bar.Add(t_("Load new mesh from disk"), [] {}).Enable(false);
			bar.Separator();
			bar.Add(t_("Copy behaviors of this node"), [=] {
				s_behavior_clipboard.Clear();
				if (!node)
					return;
				const hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
				for (VfsValue& sub : node->sub) {
					if (!IsVfsType(sub, script_hash))
						continue;
					GeomScript& src = sub.GetExt<GeomScript>();
					BehaviorClipboardItem& item = s_behavior_clipboard.Add();
					item.id = sub.id;
					item.file = src.file;
					item.enabled = src.enabled;
					item.run_on_load = src.run_on_load;
					item.run_every_frame = src.run_every_frame;
				}
				if (s_behavior_clipboard.IsEmpty())
					PromptOK(t_("No behaviors found on selected node."));
			}).Enable(can_copy_behaviors).Key(K_CTRL|K_B);
			bar.Add(t_("Paste the behaviors to this node"), [=] {
				if (!node || s_behavior_clipboard.IsEmpty() || !e)
					return;
				e->PushUndo("Paste behaviors");
				const hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
				for (const BehaviorClipboardItem& item : s_behavior_clipboard) {
					String id = MakeUniqueScriptNodeId(*node, item.id);
					VfsValue& sc_node = node->Add(id, script_hash);
					sc_node.id = id;
					GeomScript& sc = sc_node.GetExt<GeomScript>();
					sc.file = item.file;
					sc.enabled = item.enabled;
					sc.run_on_load = item.run_on_load;
					sc.run_every_frame = item.run_every_frame;
				}
				e->RefreshData();
			}).Enable(can_paste_behaviors).Key(K_CTRL|K_I);
		});
	bar.Sub(t_("Insert"), [=](Bar& bar) {
		auto add = [&](const char* text, const char* id, bool enabled = true) {
			bar.Add(text, [=] { if (e) e->HandleRibbonAction(id); }).Enable(enabled);
		};
		add(t_("Create a cube"), "create_cube");
		add(t_("Create a sphere"), "create_sphere");
		add(t_("Create a cylinder"), "create_cylinder");
		add(t_("Create a cone"), "create_cone");
		add(t_("Create a plane"), "create_plane");
		add(t_("Create a camera"), "create_camera");
		add(t_("Create a terrain"), "create_terrain");
		add(t_("Create a room mesh from a 2D map"), "create_room_mesh_from_2d_map");
		add(t_("Import a static mesh"), "import_static_mesh");
		add(t_("Import an animated mesh"), "import_animated_mesh");
		add(t_("Create a point light"), "create_point_light");
		add(t_("Create a directional light"), "create_directional_light");
		add(t_("Create a tree (plant)"), "create_tree");
		add(t_("Create a water surface"), "create_water_surface");
		add(t_("Create a billboard"), "create_billboard");
		add(t_("Create a vertical billboard"), "create_vertical_billboard");
		add(t_("Create a skybox"), "create_skybox");
		add(t_("Create a 3D sound"), "create_3d_sound");
		add(t_("Create a particle system"), "create_particle_system");
		add(t_("Create a 2D overlay item"), "create_2d_overlay_item");
		add(t_("Create a 2D touchscreen input item"), "create_2d_touchscreen_input_item");
		add(t_("Create a path"), "create_path");
		add(t_("Create a path node"), "create_path_node", can_create_path_node);
	});
	bar.Add(t_("Go to"), [=] {
		PointcloudPose pose;
		bool ok = false;
		if (obj) {
			const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key);
			if (os) {
				pose.position = os->position;
				pose.orientation = os->orientation;
				ok = true;
			}
		}
		if (!ok && ref) {
			if (ref->kind == TreeNodeRef::K_PROGRAM) {
				GeomCamera& cam = e->state->GetProgram();
				pose.position = cam.position;
				pose.orientation = cam.orientation;
				ok = true;
			}
			if (ref->kind == TreeNodeRef::K_FOCUS) {
				GeomCamera& cam = e->state->GetFocus();
				pose.position = cam.position;
				pose.orientation = cam.orientation;
				ok = true;
			}
		}
		if (ok) {
			GeomCamera& focus = e->state->GetFocus();
			focus.position = pose.position;
			focus.orientation = pose.orientation;
			RefreshAll();
		}
	});
	bar.Add(t_("Focus selected"), [=] { if (e) e->HandleRibbonAction("focus_selected"); }).Key(K_CTRL|K_F);
	if (dir) {
		auto find_dir = [&](GeomDirectory& parent, const String& name) -> GeomDirectory* {
			for (auto& sub : parent.val.sub) {
				if (!IsVfsType(sub, AsTypeHash<GeomDirectory>()))
					continue;
				GeomDirectory& d = sub.GetExt<GeomDirectory>();
				if (d.name == name || d.val.id == name)
					return &d;
			}
			return 0;
		};
		auto unique_name = [&](GeomDirectory& parent, const String& base) {
			String name = base;
			int idx = 1;
			while (parent.FindObject(name) || find_dir(parent, name)) {
				name = base + IntStr(idx++);
			}
			return name;
		};
		bar.Add(t_("Create a folder node"), [=] {
			if (e)
				e->PushUndo("Add directory");
			String name = unique_name(*dir, "dir");
			dir->GetAddDirectory(name);
			e->state->UpdateObjects();
			e->RefreshData();
		});
		bar.Sub(t_("Add"), [=](Bar& bar) {
			bar.Add(t_("Directory"), [=] {
				if (e)
					e->PushUndo("Add directory");
				String name = unique_name(*dir, "dir");
				dir->GetAddDirectory(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
			bar.Add(t_("Camera"), [=] {
				if (e)
					e->PushUndo("Add camera");
				String name = unique_name(*dir, "camera");
				dir->GetAddCamera(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
			bar.Add(t_("Model"), [=] {
				if (e)
					e->PushUndo("Add model");
				String name = unique_name(*dir, "model");
				dir->GetAddModel(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
			bar.Add(t_("Pointcloud"), [=] {
				if (e)
					e->PushUndo("Add pointcloud");
				String name = unique_name(*dir, "pointcloud");
				dir->GetAddOctree(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
			bar.Add(t_("Pointcloud Dataset"), [=] {
				if (e)
					e->PushUndo("Add dataset");
				String name = unique_name(*dir, "dataset");
				dir->GetAddPointcloudDataset(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
			bar.Sub(t_("Preset"), [=](Bar& bar) {
				bar.Add(t_("Sphere"), [=] {
					if (e)
						e->PushUndo("Add sphere");
					String name = unique_name(*dir, "sphere");
					GeomObject& obj = dir->GetAddModel(name);
					ModelBuilder mb;
					mb.AddSphere(vec3(0, 0, 0), 1.0f, 24, 16);
					obj.mdl = mb.Detach();
					obj.asset_ref = "preset:sphere";
					e->state->UpdateObjects();
					e->RefreshData();
				});
			});
		});
	}
	if (obj && obj->IsOctree()) {
		bar.Separator();
		bar.Add(t_("Generate Synthetic Pointcloud"), [=] {
			if (e)
				e->PushUndo("Generate pointcloud");
			e->GenerateSyntheticPointcloudFor(*obj);
		});
		bar.Add(t_("Clear Pointcloud"), [=] {
			if (e)
				e->PushUndo("Clear pointcloud");
			obj->octree.octree.Initialize(-3, 8);
			obj->octree_ptr = 0;
			e->state->UpdateObjects();
			RefreshAll();
		});
	}
}

void GeomProjectCtrl::PropsMenu(Bar& bar) {
	int cursor = tree.GetCursor();
	Value v = (cursor >= 0) ? tree.Get(cursor) : Value();
	GeomObject* obj = GetNodeObject(v);
	GeomPointcloudDataset* ds = GetNodeDataset(v);
	VfsValue* node = v.Is<VfsValue*>() ? ValueTo<VfsValue*>(v) : 0;
	GeomDirectory* dir = 0;
	GeomScene* scene = 0;
	if (node && IsVfsType(*node, AsTypeHash<GeomDirectory>()))
		dir = &node->GetExt<GeomDirectory>();
	if (node && IsVfsType(*node, AsTypeHash<GeomScene>()))
		scene = &node->GetExt<GeomScene>();
	if (!obj && !dir && !scene && !ds)
		return;
	bar.Sub(t_("Add Component"), [=](Bar& bar) {
		bar.Add(t_("Script"), [=] {
			if (e)
				e->PushUndo("Add script");
			if (obj)
				e->AddScriptComponent(*obj);
			else if (dir)
				e->AddScriptComponent(*dir);
			else if (scene)
				e->AddScriptComponent(*scene);
			e->state->UpdateObjects();
			e->RefreshData();
		});
		if (obj) {
			bar.Add(t_("Texture Edit"), [=] {
				if (e)
					e->PushUndo("Add texture edit");
				obj->GetTextureEdit();
				e->UpdateTextureEditor(obj);
				e->state->UpdateObjects();
				e->RefreshData();
			});
		}
		if (obj && obj->IsOctree()) {
			bar.Add(t_("Pointcloud Effect (Transform)"), [=] {
				if (e)
					e->PushUndo("Add pointcloud effect");
				String base = "effect";
				String name = base;
				int idx = 1;
				Vector<GeomPointcloudEffectTransform*> effects;
				obj->GetPointcloudEffects(effects);
				auto exists = [&](const String& n) {
					for (GeomPointcloudEffectTransform* fx : effects) {
						if (!fx)
							continue;
						String fx_name = fx->name.IsEmpty() ? fx->val.id : fx->name;
						if (fx_name == n)
							return true;
					}
					return false;
				};
				while (exists(name))
					name = base + IntStr(idx++);
				obj->GetAddPointcloudEffect(name);
				e->state->UpdateObjects();
				e->RefreshData();
			});
		}
	});
}

void GeomProjectCtrl::PropsData() {
	if (props_refreshing)
		return;
	props_refreshing = true;
	props.Clear();
	props_nodes.Clear();
	props_ctrls.Clear();
	props_transform_pos_ctrl = 0;
	props_transform_ori_ctrl = 0;
	props_transform_pos_id = -1;
	props_transform_ori_id = -1;
	props_selection_center_ctrl = 0;
	props_selection_offset_ctrl = 0;
	props_selection_center_id = -1;
	props_selection_offset_id = -1;
	int cursor = tree.GetCursor();
	Value v = (cursor >= 0) ? tree.Get(cursor) : Value();
	selected_obj = GetNodeObject(v);
	GeomScene* selected_scene = 0;
	if (!selected_obj && v.Is<VfsValue*>()) {
		VfsValue* node = ValueTo<VfsValue*>(v);
		if (node && IsVfsType(*node, AsTypeHash<GeomScene>()))
			selected_scene = &node->GetExt<GeomScene>();
		if (node && IsVfsType(*node, AsTypeHash<GeomBone>())) {
			for (VfsValue* p = node->owner; p; p = p->owner) {
				if (IsVfsType(*p, AsTypeHash<GeomObject>())) {
					selected_obj = &p->GetExt<GeomObject>();
					break;
				}
			}
		}
	}
	selected_ref = GetNodeRef(v);
	selected_dataset = GetNodeDataset(v);
	props.SetRoot(ImagesImg::Root(), "Properties");
	int root = 0;
	int effects = -1;
	struct EffectItem {
		GeomPointcloudEffectTransform* fx = 0;
		int effect_id = -1;
		int pos_id = -1;
		int ori_id = -1;
	};
	Vector<EffectItem> effect_items;
	if (selected_obj && selected_obj->IsOctree()) {
		PropRef& efx = props_nodes.Add();
		efx.kind = PropRef::P_EFFECTS;
		effects = props.Add(root, ImagesImg::Directory(), RawToValue(&efx), t_("Effects"));
		Vector<GeomPointcloudEffectTransform*> fx_list;
		selected_obj->GetPointcloudEffects(fx_list);
		for (GeomPointcloudEffectTransform* fx : fx_list) {
			if (!fx)
				continue;
			String name = fx->name.IsEmpty() ? fx->val.id : fx->name;
			PropRef& fxnode = props_nodes.Add();
			fxnode.kind = PropRef::P_TRANSFORM;
			int fx_id = props.Add(effects, ImagesImg::Object(), RawToValue(&fxnode), name);
			PropRef& epos = props_nodes.Add();
			epos.kind = PropRef::P_POSITION;
			int pos_id = props.Add(fx_id, ImagesImg::Object(), RawToValue(&epos), t_("Position"));
			PropRef& eori = props_nodes.Add();
			eori.kind = PropRef::P_ORIENTATION;
			int ori_id = props.Add(fx_id, ImagesImg::Object(), RawToValue(&eori), t_("Orientation"));
			EffectItem& item = effect_items.Add();
			item.fx = fx;
			item.effect_id = fx_id;
			item.pos_id = pos_id;
			item.ori_id = ori_id;
		}
	}
	PropRef& comps = props_nodes.Add();
	comps.kind = PropRef::P_COMPONENTS;
	int components = props.Add(root, ImagesImg::Directory(), RawToValue(&comps), t_("Components"));
	int materials = -1;
	int meshes = -1;
	Model* model_ptr = 0;
	Geom2DLayer* layer_ptr = 0;
	int layer_material = -1;
	int layer_stroke_id = -1;
	int layer_fill_id = -1;
	int layer_width_id = -1;
	int layer_opacity_id = -1;
	int layer_style_id = -1;
	int layer_texture_id = -1;
	int layer_texture_preview_id = -1;
	int layer_blend_id = -1;
	int layer_tex_offset_x_id = -1;
	int layer_tex_offset_y_id = -1;
	int layer_tex_repeat_x_id = -1;
	int layer_tex_repeat_y_id = -1;
	int layer_tex_rotate_id = -1;
	int layer_tex_wrap_id = -1;
	int layer_stroke_uv_id = -1;
	int layer_stroke_cap_id = -1;
	int layer_stroke_join_id = -1;
	if (selected_obj && selected_obj->IsModel() && selected_obj->mdl) {
		model_ptr = selected_obj->mdl.Get();
		PropRef& mats = props_nodes.Add();
		mats.kind = PropRef::P_MATERIALS;
		materials = props.Add(root, ImagesImg::Directory(), RawToValue(&mats), t_("Materials"));
		PropRef& meshref = props_nodes.Add();
		meshref.kind = PropRef::P_MESH_MATERIAL;
		meshes = props.Add(root, ImagesImg::Directory(), RawToValue(&meshref), t_("Meshes"));
	}
	if (selected_obj)
		layer_ptr = selected_obj->Find2DLayer();
	if (layer_ptr) {
		PropRef& lnode = props_nodes.Add();
		lnode.kind = PropRef::P_MATERIALS;
		layer_material = props.Add(root, ImagesImg::Directory(), RawToValue(&lnode), t_("2D Material"));
		PropRef& sref = props_nodes.Add();
		sref.kind = PropRef::P_MAT_BASE_COLOR;
		layer_stroke_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&sref), t_("Stroke"));
		PropRef& fref = props_nodes.Add();
		fref.kind = PropRef::P_MAT_EMISSIVE;
		layer_fill_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&fref), t_("Fill"));
		PropRef& wref = props_nodes.Add();
		wref.kind = PropRef::P_MAT_ROUGHNESS;
		layer_width_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&wref), t_("Width"));
		PropRef& oref = props_nodes.Add();
		oref.kind = PropRef::P_MAT_METALLIC;
		layer_opacity_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&oref), t_("Opacity"));
		PropRef& uref = props_nodes.Add();
		uref.kind = PropRef::P_MAT_OCCLUSION;
		layer_style_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&uref), t_("Use Layer Style"));
		PropRef& tref = props_nodes.Add();
		tref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_texture_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&tref), t_("Texture"));
		PropRef& tpref = props_nodes.Add();
		tpref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_texture_preview_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&tpref), t_("Texture Preview"));
		PropRef& bref = props_nodes.Add();
		bref.kind = PropRef::P_MAT_BASE_ALPHA;
		layer_blend_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&bref), t_("Blend Mode"));
		PropRef& toxref = props_nodes.Add();
		toxref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_offset_x_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&toxref), t_("Tex Offset X"));
		PropRef& toyref = props_nodes.Add();
		toyref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_offset_y_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&toyref), t_("Tex Offset Y"));
		PropRef& tsxref = props_nodes.Add();
		tsxref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_repeat_x_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&tsxref), t_("Tex Repeat X"));
		PropRef& tsyref = props_nodes.Add();
		tsyref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_repeat_y_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&tsyref), t_("Tex Repeat Y"));
		PropRef& trotref = props_nodes.Add();
		trotref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_rotate_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&trotref), t_("Tex Rotate"));
		PropRef& twref = props_nodes.Add();
		twref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_tex_wrap_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&twref), t_("Tex Wrap"));
		PropRef& suvref = props_nodes.Add();
		suvref.kind = PropRef::P_MAT_NORMAL_SCALE;
		layer_stroke_uv_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&suvref), t_("Stroke UV"));
		PropRef& capref = props_nodes.Add();
		capref.kind = PropRef::P_MAT_STROKE_CAP;
		layer_stroke_cap_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&capref), t_("Stroke Cap"));
		PropRef& joinref = props_nodes.Add();
		joinref.kind = PropRef::P_MAT_STROKE_JOIN;
		layer_stroke_join_id = props.Add(layer_material, ImagesImg::Object(), RawToValue(&joinref), t_("Stroke Join"));
	}
	int scene_timeline = -1;
	int scene_tl_length_id = -1;
	int scene_tl_pos_id = -1;
	int scene_tl_play_id = -1;
	int scene_tl_repeat_id = -1;
	int scene_tl_speed_id = -1;
	GeomSceneTimeline* scene_tl_ptr = 0;
	if (selected_scene) {
		scene_tl_ptr = &selected_scene->GetTimeline();
		PropRef& tl = props_nodes.Add();
		tl.kind = PropRef::P_SCENE_TIMELINE;
		scene_timeline = props.Add(root, ImagesImg::Directory(), RawToValue(&tl), t_("Scene Timeline"));
		PropRef& len = props_nodes.Add();
		len.kind = PropRef::P_SCENE_TIMELINE_LENGTH;
		scene_tl_length_id = props.Add(scene_timeline, ImagesImg::Object(), RawToValue(&len), t_("Length"));
		PropRef& pos = props_nodes.Add();
		pos.kind = PropRef::P_SCENE_TIMELINE_POSITION;
		scene_tl_pos_id = props.Add(scene_timeline, ImagesImg::Object(), RawToValue(&pos), t_("Position"));
		PropRef& play = props_nodes.Add();
		play.kind = PropRef::P_SCENE_TIMELINE_PLAY;
		scene_tl_play_id = props.Add(scene_timeline, ImagesImg::Object(), RawToValue(&play), t_("Play"));
		PropRef& rep = props_nodes.Add();
		rep.kind = PropRef::P_SCENE_TIMELINE_REPEAT;
		scene_tl_repeat_id = props.Add(scene_timeline, ImagesImg::Object(), RawToValue(&rep), t_("Repeat"));
		PropRef& speed = props_nodes.Add();
		speed.kind = PropRef::P_SCENE_TIMELINE_SPEED;
		scene_tl_speed_id = props.Add(scene_timeline, ImagesImg::Object(), RawToValue(&speed), t_("Speed"));
	}
	int pointcloud = -1;
	int dataset_id = -1;
	GeomPointcloudDataset* props_dataset = 0;
	int dataset_name_id = -1;
	int dataset_source_id = -1;
	if (selected_obj && selected_obj->IsOctree()) {
		PropRef& pc = props_nodes.Add();
		pc.kind = PropRef::P_POINTCLOUD;
		pointcloud = props.Add(root, ImagesImg::Directory(), RawToValue(&pc), t_("Pointcloud"));
		PropRef& ds = props_nodes.Add();
		ds.kind = PropRef::P_DATASET;
		dataset_id = props.Add(pointcloud, ImagesImg::Object(), RawToValue(&ds), t_("Dataset"));
	}
	else if (selected_dataset) {
		props_dataset = selected_dataset;
		PropRef& pc = props_nodes.Add();
		pc.kind = PropRef::P_POINTCLOUD;
		pointcloud = props.Add(root, ImagesImg::Directory(), RawToValue(&pc), t_("Pointcloud Dataset"));
		PropRef& name_ref = props_nodes.Add();
		name_ref.kind = PropRef::P_DATASET;
		dataset_name_id = props.Add(pointcloud, ImagesImg::Object(), RawToValue(&name_ref), t_("Name"));
		PropRef& src_ref = props_nodes.Add();
		src_ref.kind = PropRef::P_DATASET;
		dataset_source_id = props.Add(pointcloud, ImagesImg::Object(), RawToValue(&src_ref), t_("Source"));
	}
	PropRef& tnode = props_nodes.Add();
	tnode.kind = PropRef::P_TRANSFORM;
	int transform = props.Add(root, ImagesImg::Object(), RawToValue(&tnode), "Transform");
	PropRef& ppos = props_nodes.Add();
	ppos.kind = PropRef::P_POSITION;
	int pos_id = props.Add(transform, ImagesImg::Object(), RawToValue(&ppos), "Position");
	PropRef& pori = props_nodes.Add();
	pori.kind = PropRef::P_ORIENTATION;
	int ori_id = props.Add(transform, ImagesImg::Object(), RawToValue(&pori), "Orientation");
	int selection = -1;
	int selection_center_id = -1;
	int selection_offset_id = -1;
	bool has_mesh_selection = e && (!e->mesh_sel_points.IsEmpty() || !e->mesh_sel_lines.IsEmpty() || !e->mesh_sel_faces.IsEmpty());
	bool has_2d_selection = e && !e->select_2d_shapes.IsEmpty();
	if (has_mesh_selection || has_2d_selection) {
		PropRef& sel = props_nodes.Add();
		sel.kind = PropRef::P_SELECTION;
		selection = props.Add(root, ImagesImg::Object(), RawToValue(&sel), "Selection");
		PropRef& sc = props_nodes.Add();
		sc.kind = PropRef::P_SELECTION_CENTER;
		selection_center_id = props.Add(selection, ImagesImg::Object(), RawToValue(&sc), "Center");
		PropRef& so = props_nodes.Add();
		so.kind = PropRef::P_SELECTION_OFFSET;
		selection_offset_id = props.Add(selection, ImagesImg::Object(), RawToValue(&so), "Offset");
	}
	struct MaterialItemIds {
		Material* mat = 0;
		int mat_node_id = -1;
		int base_id = -1;
		int alpha_id = -1;
		int metallic_id = -1;
		int roughness_id = -1;
		int emissive_id = -1;
		int normal_id = -1;
		int occlusion_id = -1;
	};
	Vector<MaterialItemIds> material_items;
	Vector<int> mesh_rows;
	if (model_ptr && materials >= 0) {
		for (int i = 0; i < model_ptr->materials.GetCount(); i++) {
			int mat_id = model_ptr->materials.GetKey(i);
			Material& mat = model_ptr->materials[i];
			String name = "Material #" + IntStr(mat_id);
			PropRef& mnode = props_nodes.Add();
			mnode.kind = PropRef::P_MATERIAL;
			mnode.material_id = mat_id;
			int mat_node = props.Add(materials, ImagesImg::Object(), RawToValue(&mnode), name);
			PropRef& bnode = props_nodes.Add();
			bnode.kind = PropRef::P_MAT_BASE_COLOR;
			bnode.material_id = mat_id;
			int base_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&bnode), t_("Base Color"));
			PropRef& anode = props_nodes.Add();
			anode.kind = PropRef::P_MAT_BASE_ALPHA;
			anode.material_id = mat_id;
			int alpha_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&anode), t_("Alpha"));
			PropRef& mtnode = props_nodes.Add();
			mtnode.kind = PropRef::P_MAT_METALLIC;
			mtnode.material_id = mat_id;
			int metallic_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&mtnode), t_("Metallic"));
			PropRef& rnode = props_nodes.Add();
			rnode.kind = PropRef::P_MAT_ROUGHNESS;
			rnode.material_id = mat_id;
			int roughness_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&rnode), t_("Roughness"));
			PropRef& enode = props_nodes.Add();
			enode.kind = PropRef::P_MAT_EMISSIVE;
			enode.material_id = mat_id;
			int emissive_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&enode), t_("Emissive"));
			PropRef& nnode = props_nodes.Add();
			nnode.kind = PropRef::P_MAT_NORMAL_SCALE;
			nnode.material_id = mat_id;
			int normal_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&nnode), t_("Normal Scale"));
			PropRef& ocnode = props_nodes.Add();
			ocnode.kind = PropRef::P_MAT_OCCLUSION;
			ocnode.material_id = mat_id;
			int occlusion_id = props.Add(mat_node, ImagesImg::Object(), RawToValue(&ocnode), t_("Occlusion"));
			MaterialItemIds& ids = material_items.Add();
			ids.mat = &mat;
			ids.mat_node_id = mat_node;
			ids.base_id = base_id;
			ids.alpha_id = alpha_id;
			ids.metallic_id = metallic_id;
			ids.roughness_id = roughness_id;
			ids.emissive_id = emissive_id;
			ids.normal_id = normal_id;
			ids.occlusion_id = occlusion_id;
		}
	}
	if (model_ptr && meshes >= 0) {
		for (int i = 0; i < model_ptr->meshes.GetCount(); i++) {
			PropRef& mrow = props_nodes.Add();
			mrow.kind = PropRef::P_MESH_MATERIAL;
			mrow.mesh_index = i;
			String name = "Mesh #" + IntStr(i);
			int row_id = props.Add(meshes, ImagesImg::Object(), RawToValue(&mrow), name);
			mesh_rows.Add(row_id);
		}
	}

	struct ScriptItemIds {
		GeomScript* script = 0;
		int file_id = -1;
		int enabled_id = -1;
		int run_on_load_id = -1;
		int run_frame_id = -1;
		int edit_id = -1;
		int run_id = -1;
	};
	Vector<ScriptItemIds> script_items;
	if (selected_obj) {
		for (auto& sub : selected_obj->val.sub) {
			hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
			if (!IsVfsType(sub, script_hash))
				continue;
			GeomScript& script = sub.GetExt<GeomScript>();
			String label = script.file.IsEmpty() ? "Script" : GetFileName(script.file);
			PropRef& snode = props_nodes.Add();
			snode.kind = PropRef::P_SCRIPT;
			snode.script = &script;
			int script_id = props.Add(components, ImagesImg::Object(), RawToValue(&snode), label);
			PropRef& sfile = props_nodes.Add();
			sfile.kind = PropRef::P_SCRIPT_FILE;
			sfile.script = &script;
			int file_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sfile), t_("File"));
			PropRef& sen = props_nodes.Add();
			sen.kind = PropRef::P_SCRIPT_ENABLED;
			sen.script = &script;
			int enabled_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sen), t_("Enabled"));
			PropRef& sload = props_nodes.Add();
			sload.kind = PropRef::P_SCRIPT_RUN_ON_LOAD;
			sload.script = &script;
			int run_on_load_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sload), t_("Run on load"));
			PropRef& sframe = props_nodes.Add();
			sframe.kind = PropRef::P_SCRIPT_RUN_EACH_FRAME;
			sframe.script = &script;
			int run_frame_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sframe), t_("Run every frame"));
			PropRef& sedit = props_nodes.Add();
			sedit.kind = PropRef::P_SCRIPT_EDIT;
			sedit.script = &script;
			int edit_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sedit), t_("Edit"));
			PropRef& srun = props_nodes.Add();
			srun.kind = PropRef::P_SCRIPT_RUN;
			srun.script = &script;
			int run_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&srun), t_("Run"));
			ScriptItemIds& ids = script_items.Add();
			ids.script = &script;
			ids.file_id = file_id;
			ids.enabled_id = enabled_id;
			ids.run_on_load_id = run_on_load_id;
			ids.run_frame_id = run_frame_id;
			ids.edit_id = edit_id;
			ids.run_id = run_id;
		}
	}
	if (!selected_obj) {
		int cursor = tree.GetCursor();
		Value v = (cursor >= 0) ? tree.Get(cursor) : Value();
		VfsValue* node = v.Is<VfsValue*>() ? ValueTo<VfsValue*>(v) : 0;
		if (node && (IsVfsType(*node, AsTypeHash<GeomDirectory>()) || IsVfsType(*node, AsTypeHash<GeomScene>()))) {
			for (auto& sub : node->sub) {
				hash_t script_hash = TypedStringHasher<GeomScript>("GeomScript");
				if (!IsVfsType(sub, script_hash))
					continue;
				GeomScript& script = sub.GetExt<GeomScript>();
				String label = script.file.IsEmpty() ? "Script" : GetFileName(script.file);
				PropRef& snode = props_nodes.Add();
				snode.kind = PropRef::P_SCRIPT;
				snode.script = &script;
				int script_id = props.Add(components, ImagesImg::Object(), RawToValue(&snode), label);
				PropRef& sfile = props_nodes.Add();
				sfile.kind = PropRef::P_SCRIPT_FILE;
				sfile.script = &script;
				int file_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sfile), t_("File"));
				PropRef& sen = props_nodes.Add();
				sen.kind = PropRef::P_SCRIPT_ENABLED;
				sen.script = &script;
				int enabled_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sen), t_("Enabled"));
				PropRef& sload = props_nodes.Add();
				sload.kind = PropRef::P_SCRIPT_RUN_ON_LOAD;
				sload.script = &script;
				int run_on_load_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sload), t_("Run on load"));
				PropRef& sframe = props_nodes.Add();
				sframe.kind = PropRef::P_SCRIPT_RUN_EACH_FRAME;
				sframe.script = &script;
				int run_frame_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sframe), t_("Run every frame"));
				PropRef& sedit = props_nodes.Add();
				sedit.kind = PropRef::P_SCRIPT_EDIT;
				sedit.script = &script;
				int edit_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&sedit), t_("Edit"));
				PropRef& srun = props_nodes.Add();
				srun.kind = PropRef::P_SCRIPT_RUN;
				srun.script = &script;
				int run_id = props.Add(script_id, ImagesImg::Object(), RawToValue(&srun), t_("Run"));
				ScriptItemIds& ids = script_items.Add();
				ids.script = &script;
				ids.file_id = file_id;
				ids.enabled_id = enabled_id;
				ids.run_on_load_id = run_on_load_id;
				ids.run_frame_id = run_frame_id;
				ids.edit_id = edit_id;
				ids.run_id = run_id;
			}
		}
	}
	if (effects >= 0)
		props.Open(effects);
	props.Open(components);
	if (materials >= 0)
		props.Open(materials);
	if (meshes >= 0)
		props.Open(meshes);
	if (layer_material >= 0)
		props.Open(layer_material);
	if (pointcloud >= 0)
		props.Open(pointcloud);
	if (scene_timeline >= 0)
		props.Open(scene_timeline);
	if (selection >= 0)
		props.Open(selection);

	vec3 pos(0, 0, 0);
	quat ori = Identity<quat>();
	if (selected_obj) {
		bool use_state = false;
		if (e && e->anim) {
			if (GeomTimeline* tl = selected_obj->FindTimeline())
				use_state = !tl->keypoints.IsEmpty();
		}
		if (use_state) {
			const GeomObjectState* os = e->state->FindObjectStateByKey(selected_obj->key);
			if (os) {
				pos = os->position;
				ori = os->orientation;
			}
		}
		if (!use_state) {
			if (GeomTransform* tr = selected_obj->FindTransform()) {
				pos = tr->position;
				ori = tr->orientation;
			}
			else {
				const GeomObjectState* os = e->state->FindObjectStateByKey(selected_obj->key);
				if (os) {
					pos = os->position;
					ori = os->orientation;
				}
			}
		}
	}
	else if (selected_ref) {
		if (selected_ref->kind == TreeNodeRef::K_PROGRAM) {
			GeomCamera& cam = e->state->GetProgram();
			pos = cam.position;
			ori = cam.orientation;
		}
		if (selected_ref->kind == TreeNodeRef::K_FOCUS) {
			GeomCamera& cam = e->state->GetFocus();
			pos = cam.position;
			ori = cam.orientation;
		}
	}

	props.OpenDeep(root);
	
	auto set_ctrl_col = [&](int id, int col, One<Ctrl>&& c) {
		int line = props.GetLineAtItem(id);
		if (line >= 0)
			props.SetCtrl(line, col, *props_ctrls.Add(pick(c)), false);
	};
	auto set_value_ctrl = [&](int id, One<Ctrl>&& c) {
		set_ctrl_col(id, props_col_value, pick(c));
	};
	auto set_key_ctrl = [&](int id, One<Ctrl>&& c) {
		if (props_col_keyframe >= 0)
			set_ctrl_col(id, props_col_keyframe, pick(c));
	};

	auto clamp01 = [](double v) {
		if (v < 0.0) return 0.0;
		if (v > 1.0) return 1.0;
		return v;
	};
	auto to_color = [&](const vec3& v) -> Color {
		int r = (int)Clamp(v[0] * 255.0f, 0.0f, 255.0f);
		int g = (int)Clamp(v[1] * 255.0f, 0.0f, 255.0f);
		int b = (int)Clamp(v[2] * 255.0f, 0.0f, 255.0f);
		return Color(r, g, b);
	};
	auto update_material = [&](Material* mat, const Function<void(MaterialParameters&)>& fn) {
		if (!mat)
			return;
		mat->params.Set([&](MaterialParameters& p) { fn(p); });
		RefreshAll();
	};
	for (const MaterialItemIds& ids : material_items) {
		Material* mat = ids.mat;
		if (!mat)
			continue;
		if (ids.mat_node_id >= 0) {
			One<MaterialPreviewCtrl> prev = MakeOne<MaterialPreviewCtrl>();
			prev->SetMaterial(*mat);
			set_value_ctrl(ids.mat_node_id, pick(prev));
		}
		vec4 base = mat->params->base_clr_factor;
		vec3 emissive = mat->params->emissive_factor;
		One<ColorPusher> base_ctrl = MakeOne<ColorPusher>();
		base_ctrl->SetData(to_color(base.Splice()));
		ColorPusher* base_ptr = base_ctrl.Get();
		base_ctrl->WhenAction << [=] {
			if (!base_ptr || !mat)
				return;
			RGBA c = (Color)base_ptr->GetData();
			update_material(mat, [&](MaterialParameters& p) {
				p.base_clr_factor = vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, p.base_clr_factor[3]);
			});
		};
		set_value_ctrl(ids.base_id, pick(base_ctrl));

		One<EditDoubleSpin> alpha_ctrl = MakeOne<EditDoubleSpin>();
		alpha_ctrl->Min(0).Max(1);
		alpha_ctrl->SetData(base[3]);
		EditDoubleSpin* alpha_ptr = alpha_ctrl.Get();
		alpha_ctrl->WhenAction << [=] {
			if (!alpha_ptr || !mat)
				return;
			double a = clamp01((double)~*alpha_ptr);
			update_material(mat, [&](MaterialParameters& p) {
				p.base_clr_factor[3] = a;
			});
		};
		set_value_ctrl(ids.alpha_id, pick(alpha_ctrl));

		One<EditDoubleSpin> metallic_ctrl = MakeOne<EditDoubleSpin>();
		metallic_ctrl->Min(0).Max(1);
		metallic_ctrl->SetData(mat->params->metallic_factor);
		EditDoubleSpin* metallic_ptr = metallic_ctrl.Get();
		metallic_ctrl->WhenAction << [=] {
			if (!metallic_ptr || !mat)
				return;
			double v = clamp01((double)~*metallic_ptr);
			update_material(mat, [&](MaterialParameters& p) { p.metallic_factor = v; });
		};
		set_value_ctrl(ids.metallic_id, pick(metallic_ctrl));

		One<EditDoubleSpin> rough_ctrl = MakeOne<EditDoubleSpin>();
		rough_ctrl->Min(0).Max(1);
		rough_ctrl->SetData(mat->params->roughness_factor);
		EditDoubleSpin* rough_ptr = rough_ctrl.Get();
		rough_ctrl->WhenAction << [=] {
			if (!rough_ptr || !mat)
				return;
			double v = clamp01((double)~*rough_ptr);
			update_material(mat, [&](MaterialParameters& p) { p.roughness_factor = v; });
		};
		set_value_ctrl(ids.roughness_id, pick(rough_ctrl));

		One<ColorPusher> emissive_ctrl = MakeOne<ColorPusher>();
		emissive_ctrl->SetData(to_color(emissive));
		ColorPusher* emissive_ptr = emissive_ctrl.Get();
		emissive_ctrl->WhenAction << [=] {
			if (!emissive_ptr || !mat)
				return;
			RGBA c = (Color)emissive_ptr->GetData();
			update_material(mat, [&](MaterialParameters& p) {
				p.emissive_factor = vec3(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
			});
		};
		set_value_ctrl(ids.emissive_id, pick(emissive_ctrl));

		One<EditDoubleSpin> normal_ctrl = MakeOne<EditDoubleSpin>();
		normal_ctrl->Min(0).Max(4);
		normal_ctrl->SetData(mat->params->normal_scale);
		EditDoubleSpin* normal_ptr = normal_ctrl.Get();
		normal_ctrl->WhenAction << [=] {
			if (!normal_ptr || !mat)
				return;
			double v = max(0.0, (double)~*normal_ptr);
			update_material(mat, [&](MaterialParameters& p) { p.normal_scale = v; });
		};
		set_value_ctrl(ids.normal_id, pick(normal_ctrl));

		One<EditDoubleSpin> occ_ctrl = MakeOne<EditDoubleSpin>();
		occ_ctrl->Min(0).Max(1);
		occ_ctrl->SetData(mat->params->occlusion_strength);
		EditDoubleSpin* occ_ptr = occ_ctrl.Get();
		occ_ctrl->WhenAction << [=] {
			if (!occ_ptr || !mat)
				return;
			double v = clamp01((double)~*occ_ptr);
			update_material(mat, [&](MaterialParameters& p) { p.occlusion_strength = v; });
		};
		set_value_ctrl(ids.occlusion_id, pick(occ_ctrl));
	}
	if (model_ptr && meshes >= 0) {
		for (int i = 0; i < mesh_rows.GetCount() && i < model_ptr->meshes.GetCount(); i++) {
			int row_id = mesh_rows[i];
			Mesh* mesh_ptr = &model_ptr->meshes[i];
			One<DropList> dl = MakeOne<DropList>();
			dl->Add(-1, t_("None"));
			for (int k = 0; k < model_ptr->materials.GetCount(); k++) {
				int mat_id = model_ptr->materials.GetKey(k);
				dl->Add(mat_id, "Material #" + IntStr(mat_id));
			}
			dl->SetData(mesh_ptr ? mesh_ptr->material : -1);
			DropList* dl_ptr = dl.Get();
			dl->WhenAction << [=] {
				if (dl_ptr && mesh_ptr) {
					mesh_ptr->material = (int)dl_ptr->GetData();
					RefreshAll();
				}
			};
			set_value_ctrl(row_id, pick(dl));
		}
	}
	if (layer_ptr && layer_material >= 0) {
		One<ColorPusher> stroke_ctrl = MakeOne<ColorPusher>();
		stroke_ctrl->SetData(layer_ptr->stroke);
		ColorPusher* stroke_ptr = stroke_ctrl.Get();
		stroke_ctrl->WhenAction << [=] {
			if (stroke_ptr && layer_ptr) {
				layer_ptr->stroke = (Color)stroke_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_stroke_id, pick(stroke_ctrl));

		One<ColorPusher> fill_ctrl = MakeOne<ColorPusher>();
		fill_ctrl->SetData(layer_ptr->fill);
		ColorPusher* fill_ptr = fill_ctrl.Get();
		fill_ctrl->WhenAction << [=] {
			if (fill_ptr && layer_ptr) {
				layer_ptr->fill = (Color)fill_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_fill_id, pick(fill_ctrl));

		One<EditDoubleSpin> width_ctrl = MakeOne<EditDoubleSpin>();
		width_ctrl->Min(0).Max(20);
		width_ctrl->SetData(layer_ptr->width);
		EditDoubleSpin* width_ptr = width_ctrl.Get();
		width_ctrl->WhenAction << [=] {
			if (width_ptr && layer_ptr) {
				layer_ptr->width = max(0.0, (double)~*width_ptr);
				RefreshAll();
			}
		};
		set_value_ctrl(layer_width_id, pick(width_ctrl));

		One<EditDoubleSpin> opacity_ctrl = MakeOne<EditDoubleSpin>();
		opacity_ctrl->Min(0).Max(1);
		opacity_ctrl->SetData(layer_ptr->opacity);
		EditDoubleSpin* opacity_ptr = opacity_ctrl.Get();
		opacity_ctrl->WhenAction << [=] {
			if (opacity_ptr && layer_ptr) {
				layer_ptr->opacity = clamp01((double)~*opacity_ptr);
				RefreshAll();
			}
		};
		set_value_ctrl(layer_opacity_id, pick(opacity_ctrl));

		One<Option> style_ctrl = MakeOne<Option>();
		style_ctrl->SetData(layer_ptr->use_layer_style);
		Option* style_ptr = style_ctrl.Get();
		style_ctrl->WhenAction << [=] {
			if (style_ptr && layer_ptr) {
				layer_ptr->use_layer_style = (bool)style_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_style_id, pick(style_ctrl));

		One<EditString> tex_ctrl = MakeOne<EditString>();
		tex_ctrl->SetData(layer_ptr->texture_ref);
		EditString* tex_ptr = tex_ctrl.Get();
		One<TexturePreviewCtrl> tex_preview = MakeOne<TexturePreviewCtrl>();
		tex_preview->SetPath(layer_ptr->texture_ref);
		TexturePreviewCtrl* tex_preview_ptr = tex_preview.Get();
		tex_ctrl->WhenAction << [=] {
			if (tex_ptr && layer_ptr) {
				layer_ptr->texture_ref = ~*tex_ptr;
				if (tex_preview_ptr)
					tex_preview_ptr->SetPath(layer_ptr->texture_ref);
			}
		};
		set_value_ctrl(layer_texture_id, pick(tex_ctrl));
		set_value_ctrl(layer_texture_preview_id, pick(tex_preview));

		One<DropList> blend_ctrl = MakeOne<DropList>();
		blend_ctrl->Add(0, t_("Normal"));
		blend_ctrl->Add(1, t_("Add"));
		blend_ctrl->Add(2, t_("Multiply"));
		blend_ctrl->SetData(layer_ptr->blend_mode);
		DropList* blend_ptr = blend_ctrl.Get();
		blend_ctrl->WhenAction << [=] {
			if (blend_ptr && layer_ptr) {
				layer_ptr->blend_mode = (int)blend_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_blend_id, pick(blend_ctrl));

		One<EditDoubleSpin> tex_off_x = MakeOne<EditDoubleSpin>();
		tex_off_x->Min(-10).Max(10);
		tex_off_x->SetData(layer_ptr->tex_offset_x);
		EditDoubleSpin* tex_off_x_ptr = tex_off_x.Get();
		tex_off_x->WhenAction << [=] {
			if (tex_off_x_ptr && layer_ptr) {
				layer_ptr->tex_offset_x = (double)~*tex_off_x_ptr;
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_offset_x_id, pick(tex_off_x));

		One<EditDoubleSpin> tex_off_y = MakeOne<EditDoubleSpin>();
		tex_off_y->Min(-10).Max(10);
		tex_off_y->SetData(layer_ptr->tex_offset_y);
		EditDoubleSpin* tex_off_y_ptr = tex_off_y.Get();
		tex_off_y->WhenAction << [=] {
			if (tex_off_y_ptr && layer_ptr) {
				layer_ptr->tex_offset_y = (double)~*tex_off_y_ptr;
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_offset_y_id, pick(tex_off_y));

		One<EditDoubleSpin> tex_repeat_x = MakeOne<EditDoubleSpin>();
		tex_repeat_x->Min(0.01).Max(100);
		tex_repeat_x->SetData(layer_ptr->tex_repeat_x);
		EditDoubleSpin* tex_repeat_x_ptr = tex_repeat_x.Get();
		tex_repeat_x->WhenAction << [=] {
			if (tex_repeat_x_ptr && layer_ptr) {
				layer_ptr->tex_repeat_x = max(0.01, (double)~*tex_repeat_x_ptr);
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_repeat_x_id, pick(tex_repeat_x));

		One<EditDoubleSpin> tex_repeat_y = MakeOne<EditDoubleSpin>();
		tex_repeat_y->Min(0.01).Max(100);
		tex_repeat_y->SetData(layer_ptr->tex_repeat_y);
		EditDoubleSpin* tex_repeat_y_ptr = tex_repeat_y.Get();
		tex_repeat_y->WhenAction << [=] {
			if (tex_repeat_y_ptr && layer_ptr) {
				layer_ptr->tex_repeat_y = max(0.01, (double)~*tex_repeat_y_ptr);
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_repeat_y_id, pick(tex_repeat_y));

		One<EditDoubleSpin> tex_rotate = MakeOne<EditDoubleSpin>();
		tex_rotate->Min(-360).Max(360);
		tex_rotate->SetData(layer_ptr->tex_rotate);
		EditDoubleSpin* tex_rotate_ptr = tex_rotate.Get();
		tex_rotate->WhenAction << [=] {
			if (tex_rotate_ptr && layer_ptr) {
				layer_ptr->tex_rotate = (double)~*tex_rotate_ptr;
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_rotate_id, pick(tex_rotate));

		One<DropList> tex_wrap = MakeOne<DropList>();
		tex_wrap->Add(0, t_("Clamp"));
		tex_wrap->Add(1, t_("Repeat"));
		tex_wrap->Add(2, t_("Mirror"));
		tex_wrap->SetData(layer_ptr->tex_wrap);
		DropList* tex_wrap_ptr = tex_wrap.Get();
		tex_wrap->WhenAction << [=] {
			if (tex_wrap_ptr && layer_ptr) {
				layer_ptr->tex_wrap = (int)tex_wrap_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_tex_wrap_id, pick(tex_wrap));

		One<DropList> stroke_uv = MakeOne<DropList>();
		stroke_uv->Add(0, t_("BBox"));
		stroke_uv->Add(1, t_("Along Length"));
		stroke_uv->SetData(layer_ptr->stroke_uv_mode);
		DropList* stroke_uv_ptr = stroke_uv.Get();
		stroke_uv->WhenAction << [=] {
			if (stroke_uv_ptr && layer_ptr) {
				layer_ptr->stroke_uv_mode = (int)stroke_uv_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_stroke_uv_id, pick(stroke_uv));

		One<DropList> stroke_cap = MakeOne<DropList>();
		stroke_cap->Add(0, t_("Butt"));
		stroke_cap->Add(1, t_("Round"));
		stroke_cap->Add(2, t_("Square"));
		stroke_cap->SetData(layer_ptr->stroke_cap);
		DropList* stroke_cap_ptr = stroke_cap.Get();
		stroke_cap->WhenAction << [=] {
			if (stroke_cap_ptr && layer_ptr) {
				layer_ptr->stroke_cap = (int)stroke_cap_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_stroke_cap_id, pick(stroke_cap));

		One<DropList> stroke_join = MakeOne<DropList>();
		stroke_join->Add(0, t_("Miter"));
		stroke_join->Add(1, t_("Round"));
		stroke_join->Add(2, t_("Bevel"));
		stroke_join->SetData(layer_ptr->stroke_join);
		DropList* stroke_join_ptr = stroke_join.Get();
		stroke_join->WhenAction << [=] {
			if (stroke_join_ptr && layer_ptr) {
				layer_ptr->stroke_join = (int)stroke_join_ptr->GetData();
				RefreshAll();
			}
		};
		set_value_ctrl(layer_stroke_join_id, pick(stroke_join));
	}
	
	if (selected_obj) {
		One<ToggleRowCtrl> t = MakeOne<ToggleRowCtrl>();
		t->SetVisible(selected_obj->is_visible);
		t->SetLocked(selected_obj->is_locked);
		t->WhenVisible << [=](bool v) {
			selected_obj->is_visible = v;
			tree.SetRowValue(focus_tree_id, tree_col_visible, selected_obj->is_visible);
			RefreshAll();
		};
		t->WhenLocked << [=](bool v) {
			selected_obj->is_locked = v;
			tree.SetRowValue(focus_tree_id, tree_col_locked, selected_obj->is_locked);
		};
		set_value_ctrl(root, pick(t));
		if (selected_obj == e->sim_observation_obj) {
			One<ToggleRowCtrl> tfx = MakeOne<ToggleRowCtrl>();
			tfx->SetVisible(e->sim_observation_effect_visible);
			tfx->SetLocked(e->sim_observation_effect_locked);
			tfx->WhenVisible << [=](bool v) {
				e->sim_observation_effect_visible = v;
				e->RefreshSimObservation();
			};
			tfx->WhenLocked << [=](bool v) {
				e->sim_observation_effect_locked = v;
			};
			set_value_ctrl(effects, pick(tfx));
		}
	}

	if (selected_obj && selected_obj->IsOctree() && dataset_id >= 0) {
		One<DropList> ds_ctrl = MakeOne<DropList>();
		ds_ctrl->Add(String(), t_("None"));
		GeomScene& scene = e->state->GetActiveScene();
		for (auto& s : scene.val.sub) {
			if (!IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
				continue;
			GeomPointcloudDataset& ds = s.GetExt<GeomPointcloudDataset>();
			String id = ds.GetId();
			ds_ctrl->Add(id, id);
		}
		ds_ctrl->SetData(selected_obj->pointcloud_ref);
		DropList* ds_ptr = ds_ctrl.Get();
		ds_ctrl->WhenAction << [=] {
			if (ds_ptr)
				selected_obj->pointcloud_ref = AsString(ds_ptr->GetData());
			e->state->UpdateObjects();
			RefreshAll();
		};
		set_value_ctrl(dataset_id, pick(ds_ctrl));
	}

	if (props_dataset) {
		One<EditString> name_ctrl = MakeOne<EditString>();
		name_ctrl->SetData(props_dataset->GetId());
		EditString* name_ptr = name_ctrl.Get();
		name_ctrl->WhenAction << [=] {
			if (!name_ptr)
				return;
			String old_id = props_dataset->GetId();
			String new_id = ~*name_ptr;
			if (new_id.IsEmpty() || new_id == old_id)
				return;
			props_dataset->name = new_id;
			props_dataset->val.id = new_id;
			GeomScene& scene = e->state->GetActiveScene();
			GeomObjectCollection objs(scene);
			for (GeomObject& o : objs) {
				if (o.pointcloud_ref == old_id)
					o.pointcloud_ref = new_id;
			}
			e->state->UpdateObjects();
			e->RefreshData();
		};
		set_value_ctrl(dataset_name_id, pick(name_ctrl));

		One<EditString> source_ctrl = MakeOne<EditString>();
		source_ctrl->SetData(props_dataset->source_ref);
		EditString* source_ptr = source_ctrl.Get();
		source_ctrl->WhenAction << [=] {
			if (source_ptr)
				props_dataset->source_ref = ~*source_ptr;
			e->state->UpdateObjects();
			e->RefreshData();
		};
		set_value_ctrl(dataset_source_id, pick(source_ctrl));
	}

	for (const EffectItem& item : effect_items) {
		GeomPointcloudEffectTransform* fx_ptr = item.fx;
		if (!fx_ptr)
			continue;
		One<ToggleRowCtrl> tfx = MakeOne<ToggleRowCtrl>();
		tfx->SetVisible(fx_ptr->enabled);
		tfx->SetLocked(fx_ptr->locked);
		tfx->WhenVisible << [=](bool v) {
			if (!fx_ptr)
				return;
			fx_ptr->enabled = v;
			RefreshAll();
		};
		tfx->WhenLocked << [=](bool v) {
			if (!fx_ptr)
				return;
			fx_ptr->locked = v;
		};
		if (item.effect_id >= 0)
			set_value_ctrl(item.effect_id, pick(tfx));
		
		One<Vec3EditCtrl> fpos = MakeOne<Vec3EditCtrl>();
		fpos->SetValue(fx_ptr->position);
		fpos->SetEditable(!fx_ptr->locked);
		Vec3EditCtrl* fpos_ptr = fpos.Get();
		fpos->WhenAction << [=] {
			if (!fx_ptr || !fpos_ptr)
				return;
			fx_ptr->position = fpos_ptr->GetValue();
			RefreshAll();
		};
		set_value_ctrl(item.pos_id, pick(fpos));
		
		One<QuatEditCtrl> fori = MakeOne<QuatEditCtrl>();
		fori->SetValue(fx_ptr->orientation);
		fori->SetEditable(!fx_ptr->locked);
		QuatEditCtrl* fori_ptr = fori.Get();
		fori->WhenAction << [=] {
			if (!fx_ptr || !fori_ptr)
				return;
			fx_ptr->orientation = fori_ptr->GetValue();
			RefreshAll();
		};
		set_value_ctrl(item.ori_id, pick(fori));
	}

	for (const ScriptItemIds& ids : script_items) {
		GeomScript* script_ptr = ids.script;
		if (!script_ptr)
			continue;
		One<EditString> file_ctrl = MakeOne<EditString>();
		file_ctrl->SetData(script_ptr->file);
		EditString* file_ctrl_ptr = file_ctrl.Get();
		file_ctrl->WhenAction << [=] {
			if (file_ctrl_ptr && script_ptr)
				script_ptr->file = ~*file_ctrl_ptr;
			e->EnsureScriptInstances();
		};
		set_value_ctrl(ids.file_id, pick(file_ctrl));

		One<Option> enabled = MakeOne<Option>();
		enabled->SetData(script_ptr->enabled);
		Option* enabled_ptr = enabled.Get();
		enabled->WhenAction << [=] {
			if (enabled_ptr && script_ptr)
				script_ptr->enabled = (bool)enabled_ptr->GetData();
			e->EnsureScriptInstances();
		};
		set_value_ctrl(ids.enabled_id, pick(enabled));

		One<Option> run_on_load = MakeOne<Option>();
		run_on_load->SetData(script_ptr->run_on_load);
		Option* run_on_load_ptr = run_on_load.Get();
		run_on_load->WhenAction << [=] {
			if (run_on_load_ptr && script_ptr)
				script_ptr->run_on_load = (bool)run_on_load_ptr->GetData();
		};
		set_value_ctrl(ids.run_on_load_id, pick(run_on_load));

		One<Option> run_frame = MakeOne<Option>();
		run_frame->SetData(script_ptr->run_every_frame);
		Option* run_frame_ptr = run_frame.Get();
		run_frame->WhenAction << [=] {
			if (run_frame_ptr && script_ptr)
				script_ptr->run_every_frame = (bool)run_frame_ptr->GetData();
		};
		set_value_ctrl(ids.run_frame_id, pick(run_frame));

		One<Button> edit = MakeOne<Button>();
		edit->SetLabel(t_("Edit..."));
		edit->WhenAction << [=] {
			if (script_ptr)
				e->OpenScriptEditor(*script_ptr);
		};
		set_value_ctrl(ids.edit_id, pick(edit));

		One<Button> run = MakeOne<Button>();
		run->SetLabel(t_("Run"));
		run->WhenAction << [=] {
			if (script_ptr)
				e->RunScriptOnce(*script_ptr);
		};
		set_value_ctrl(ids.run_id, pick(run));
	}

	if (scene_tl_ptr) {
		One<EditIntSpin> len_ctrl = MakeOne<EditIntSpin>();
		len_ctrl->Min(0);
		len_ctrl->SetData(scene_tl_ptr->length);
		EditIntSpin* len_ptr = len_ctrl.Get();
		len_ctrl->WhenAction << [=] {
			if (!len_ptr || !scene_tl_ptr || !selected_scene)
				return;
			int len = (int)~*len_ptr;
			if (len < 0)
				len = 0;
			scene_tl_ptr->length = len;
			selected_scene->length = len;
			if (scene_tl_ptr->position >= len && len > 0)
				scene_tl_ptr->position = len - 1;
			scene_tl_ptr->time = scene_tl_ptr->position / (double)e->prj->kps;
			RefreshAll();
		};
		set_value_ctrl(scene_tl_length_id, pick(len_ctrl));

		One<EditIntSpin> pos_ctrl_tl = MakeOne<EditIntSpin>();
		pos_ctrl_tl->Min(0);
		pos_ctrl_tl->SetData(scene_tl_ptr->position);
		EditIntSpin* pos_ptr = pos_ctrl_tl.Get();
		pos_ctrl_tl->WhenAction << [=] {
			if (!pos_ptr || !scene_tl_ptr)
				return;
			int pos = (int)~*pos_ptr;
			if (pos < 0)
				pos = 0;
			if (scene_tl_ptr->length > 0 && pos >= scene_tl_ptr->length)
				pos = scene_tl_ptr->length - 1;
			scene_tl_ptr->position = pos;
			scene_tl_ptr->time = pos / (double)e->prj->kps;
			RefreshAll();
		};
		set_value_ctrl(scene_tl_pos_id, pick(pos_ctrl_tl));

		One<Option> play_ctrl = MakeOne<Option>();
		play_ctrl->SetData(scene_tl_ptr->is_playing);
		Option* play_ptr = play_ctrl.Get();
		play_ctrl->WhenAction << [=] {
			if (!play_ptr || !scene_tl_ptr || !selected_scene)
				return;
			bool on = (bool)play_ptr->GetData();
			if (on)
				scene_tl_ptr->Play(selected_scene->length);
			else
				scene_tl_ptr->Pause();
			RefreshAll();
		};
		set_value_ctrl(scene_tl_play_id, pick(play_ctrl));

		One<Option> repeat_ctrl = MakeOne<Option>();
		repeat_ctrl->SetData(scene_tl_ptr->repeat);
		Option* repeat_ptr = repeat_ctrl.Get();
		repeat_ctrl->WhenAction << [=] {
			if (repeat_ptr && scene_tl_ptr)
				scene_tl_ptr->repeat = (bool)repeat_ptr->GetData();
		};
		set_value_ctrl(scene_tl_repeat_id, pick(repeat_ctrl));

		One<EditDoubleSpin> speed_ctrl = MakeOne<EditDoubleSpin>();
		speed_ctrl->Min(0.01);
		speed_ctrl->SetData(scene_tl_ptr->speed);
		EditDoubleSpin* speed_ptr = speed_ctrl.Get();
		speed_ctrl->WhenAction << [=] {
			if (speed_ptr && scene_tl_ptr) {
				double v = (double)~*speed_ptr;
				if (v < 0.01)
					v = 0.01;
				scene_tl_ptr->speed = v;
			}
		};
		set_value_ctrl(scene_tl_speed_id, pick(speed_ctrl));
	}
	
	One<Vec3EditCtrl> pos_ctrl = MakeOne<Vec3EditCtrl>();
	pos_ctrl->SetValue(pos);
	pos_ctrl->SetEditable(true);
	props_transform_pos_ctrl = pos_ctrl.Get();
	props_transform_pos_id = pos_id;
	pos_ctrl->WhenAction << [=] {
		props.SetCursor(pos_id);
		PropsApply();
	};
	set_value_ctrl(pos_id, pick(pos_ctrl));
	
	One<QuatEditCtrl> ori_ctrl = MakeOne<QuatEditCtrl>();
	ori_ctrl->SetValue(ori);
	ori_ctrl->SetEditable(true);
	props_transform_ori_ctrl = ori_ctrl.Get();
	props_transform_ori_id = ori_id;
	ori_ctrl->WhenAction << [=] {
		props.SetCursor(ori_id);
		PropsApply();
	};
	set_value_ctrl(ori_id, pick(ori_ctrl));
	
	if (selection_center_id >= 0 || selection_offset_id >= 0) {
		vec3 sel_center(0, 0, 0);
		bool have_center = false;
		if (e && has_mesh_selection)
			have_center = e->GetMeshSelectionCenter(sel_center);
		else if (e && has_2d_selection)
			have_center = e->Get2DSelectionCenter(sel_center);
		if (!have_center)
			sel_center = vec3(0, 0, 0);
		vec3 sel_offset(0, 0, 0);
		if (e && has_mesh_selection)
			sel_offset = e->mesh_sel_offset;
		else if (e && has_2d_selection)
			sel_offset = vec3(e->sel2d_offset[0], e->sel2d_offset[1], 0.0f);
		if (selection_center_id >= 0) {
			One<Vec3EditCtrl> sel_center_ctrl = MakeOne<Vec3EditCtrl>();
			sel_center_ctrl->SetValue(sel_center);
			sel_center_ctrl->SetEditable(false);
			props_selection_center_ctrl = sel_center_ctrl.Get();
			props_selection_center_id = selection_center_id;
			set_value_ctrl(selection_center_id, pick(sel_center_ctrl));
		}
		if (selection_offset_id >= 0) {
			One<Vec3EditCtrl> sel_offset_ctrl = MakeOne<Vec3EditCtrl>();
			sel_offset_ctrl->SetValue(sel_offset);
			sel_offset_ctrl->SetEditable(true);
			props_selection_offset_ctrl = sel_offset_ctrl.Get();
			props_selection_offset_id = selection_offset_id;
			sel_offset_ctrl->WhenAction << [=] {
				props.SetCursor(selection_offset_id);
				PropsApply();
			};
			set_value_ctrl(selection_offset_id, pick(sel_offset_ctrl));
		}
	}

	auto set_frame = [&](int frame) {
		if (!e || !e->anim)
			return;
		int max_frame = max(e->state->GetActiveScene().length - 1, 0);
		frame = min(max(frame, 0), max_frame);
		e->anim->position = frame;
		time.SetSelectedColumn(frame);
		time.Refresh();
		RefreshAll();
		PostCallback([=] { PropsData(); });
	};
	auto find_prev_frame = [&](GeomObject* obj, bool pos, bool ori, int frame) -> int {
		if (!obj)
			return 0;
		GeomTimeline* tl = obj->FindTimeline();
		if (!tl || tl->keypoints.IsEmpty())
			return 0;
		int idx = pos ? tl->FindPrePosition(frame) : tl->FindPreOrientation(frame);
		if (!pos && !ori)
			idx = tl->FindPre(frame);
		if (idx < 0)
			return 0;
		return tl->keypoints.GetKey(idx);
	};
	auto find_next_frame = [&](GeomObject* obj, bool pos, bool ori, int frame) -> int {
		int last = max(e->state->GetActiveScene().length - 1, 0);
		if (!obj)
			return last;
		GeomTimeline* tl = obj->FindTimeline();
		if (!tl || tl->keypoints.IsEmpty())
			return last;
		int idx = pos ? tl->FindPostPosition(frame) : tl->FindPostOrientation(frame);
		if (!pos && !ori)
			idx = tl->FindPost(frame);
		if (idx < 0)
			return last;
		return tl->keypoints.GetKey(idx);
	};
	auto has_kp_at = [&](GeomObject* obj, bool pos, bool ori, int frame) -> bool {
		if (!obj)
			return false;
		GeomTimeline* tl = obj->FindTimeline();
		if (!tl)
			return false;
		int idx = tl->keypoints.Find(frame);
		if (idx < 0)
			return false;
		GeomKeypoint& kp = tl->keypoints[idx];
		if (pos && !kp.has_position)
			return false;
		if (ori && !kp.has_orientation)
			return false;
		return true;
	};
	auto toggle_kp = [&](GeomObject* obj, bool pos, bool ori, int frame) {
		if (!obj)
			return;
		GeomTimeline& tl = obj->GetTimeline();
		int idx = tl.keypoints.Find(frame);
		if (idx >= 0) {
			GeomKeypoint& kp = tl.keypoints[idx];
			if (pos) kp.has_position = !kp.has_position;
			if (ori) kp.has_orientation = !kp.has_orientation;
			if (pos && kp.has_position && obj->FindTransform())
				kp.position = obj->FindTransform()->position;
			if (ori && kp.has_orientation && obj->FindTransform())
				kp.orientation = obj->FindTransform()->orientation;
			if (!kp.has_position && !kp.has_orientation)
				tl.keypoints.Remove(idx);
		}
		else {
			GeomKeypoint& kp = tl.GetAddKeypoint(frame);
			kp.frame_id = frame;
			if (GeomTransform* tr = obj->FindTransform()) {
				if (pos) kp.position = tr->position;
				if (ori) kp.orientation = tr->orientation;
			}
			kp.has_position = pos;
			kp.has_orientation = ori;
		}
		obj->read_enabled = true;
		tree.SetRowValue(focus_tree_id, tree_col_read, obj->read_enabled);
		TimelineData();
		time.Refresh();
		RefreshAll();
		PostCallback([=] { PropsData(); });
	};

	if (selected_obj) {
		int frame = e->anim ? e->anim->position : 0;
		One<KeyframeCtrl> kp_pos = MakeOne<KeyframeCtrl>();
		kp_pos->SetKeyed(has_kp_at(selected_obj, true, false, frame));
		kp_pos->WhenPrev = [=] { set_frame(find_prev_frame(selected_obj, true, false, frame)); };
		kp_pos->WhenNext = [=] { set_frame(find_next_frame(selected_obj, true, false, frame)); };
		kp_pos->WhenToggle = [=] { toggle_kp(selected_obj, true, false, frame); };
		set_key_ctrl(pos_id, pick(kp_pos));

		One<KeyframeCtrl> kp_ori = MakeOne<KeyframeCtrl>();
		kp_ori->SetKeyed(has_kp_at(selected_obj, false, true, frame));
		kp_ori->WhenPrev = [=] { set_frame(find_prev_frame(selected_obj, false, true, frame)); };
		kp_ori->WhenNext = [=] { set_frame(find_next_frame(selected_obj, false, true, frame)); };
		kp_ori->WhenToggle = [=] { toggle_kp(selected_obj, false, true, frame); };
		set_key_ctrl(ori_id, pick(kp_ori));
	}
	
	props_refreshing = false;
}

void GeomProjectCtrl::PropsApply() {
	int id = props.GetCursor();
	if (id < 0)
		return;
	int linei = props.GetLineAtItem(id);
	Value v = props.Get(id);
	if (!v.Is<PropRef*>())
		return;
	PropRef* pr = ValueTo<PropRef*>(v);
	if (!pr)
		return;

	auto get_vec = [&](int line) {
		if (Ctrl* c = props.GetCtrl(line, props_col_value)) {
			if (Vec3EditCtrl* vc = dynamic_cast<Vec3EditCtrl*>(c))
				return vc->GetValue();
		}
		return vec3(0, 0, 0);
	};
	auto get_quat = [&](int line) {
		if (Ctrl* c = props.GetCtrl(line, props_col_value)) {
			if (QuatEditCtrl* qc = dynamic_cast<QuatEditCtrl*>(c))
				return qc->GetValue();
		}
		return Identity<quat>();
	};

	bool allow_write = true;
	if (selected_obj)
		allow_write = !selected_obj->is_locked;
	if (selected_ref) {
		if (selected_ref->kind == TreeNodeRef::K_PROGRAM)
			allow_write = program_write;
		if (selected_ref->kind == TreeNodeRef::K_FOCUS)
			allow_write = focus_write;
	}
	if (!allow_write) {
		PropsData();
		return;
	}

	if (pr->kind == PropRef::P_SELECTION_CENTER) {
		PropsData();
		return;
	}
	if (pr->kind == PropRef::P_SELECTION_OFFSET) {
		vec3 new_offset = get_vec(linei);
		if (e) {
			bool has_mesh_sel = !e->mesh_sel_points.IsEmpty() || !e->mesh_sel_lines.IsEmpty() || !e->mesh_sel_faces.IsEmpty();
			bool has_2d_sel = !e->select_2d_shapes.IsEmpty();
			if (has_mesh_sel) {
				vec3 delta = new_offset - e->mesh_sel_offset;
				e->ApplyMeshSelectionDelta(delta);
			}
			else if (has_2d_sel) {
				vec2 delta((float)(new_offset[0] - e->sel2d_offset[0]),
				           (float)(new_offset[1] - e->sel2d_offset[1]));
				e->Apply2DSelectionDelta(delta);
			}
		}
		return;
	}

	if (pr->kind == PropRef::P_EFFECT_POSITION || pr->kind == PropRef::P_EFFECT_ORIENTATION) {
		PropsData();
		return;
	}
	if (selected_obj) {
		if (e)
			e->PushUndo("Props edit");
		vec3 pos = pr->kind == PropRef::P_POSITION ? get_vec(linei) : vec3(0);
		quat ori = pr->kind == PropRef::P_ORIENTATION ? get_quat(linei) : Identity<quat>();
		if (GeomTransform* tr = selected_obj->FindTransform()) {
			if (pr->kind == PropRef::P_POSITION)
				tr->position = pos;
			if (pr->kind == PropRef::P_ORIENTATION)
				tr->orientation = ori;
		}
		bool do_key = selected_obj->write_enabled;
		bool auto_key_arm = e->auto_key && e->timeline_object_key == selected_obj->key &&
		    (e->timeline_component == Edit3D::TC_TRANSFORM || e->timeline_component == Edit3D::TC_NONE);
		if (auto_key_arm) {
			if (pr->kind == PropRef::P_POSITION && e->auto_key_position)
				do_key = true;
			if (pr->kind == PropRef::P_ORIENTATION && e->auto_key_orientation)
				do_key = true;
		}
		if (do_key) {
			GeomTimeline& tl = selected_obj->GetTimeline();
			int frame = e->anim ? e->anim->position : 0;
			int idx = tl.keypoints.Find(frame);
			bool existed = idx >= 0;
			GeomKeypoint& kp = existed ? tl.keypoints[idx] : tl.GetAddKeypoint(frame);
			if (!existed) {
				kp.has_position = false;
				kp.has_orientation = false;
			}
			if (pr->kind == PropRef::P_POSITION) {
				kp.position = pos;
				kp.has_position = true;
			}
			if (pr->kind == PropRef::P_ORIENTATION) {
				kp.orientation = ori;
				kp.has_orientation = true;
			}
			selected_obj->read_enabled = true;
			tree.SetRowValue(focus_tree_id, tree_col_read, selected_obj->read_enabled);
		}
		else {
			selected_obj->read_enabled = false;
			tree.SetRowValue(focus_tree_id, tree_col_read, selected_obj->read_enabled);
		}
		e->state->UpdateObjects();
		RefreshAll();
		return;
	}
	if (selected_ref) {
		if (e)
			e->PushUndo("Props edit");
		if (selected_ref->kind == TreeNodeRef::K_PROGRAM) {
			GeomCamera& cam = e->state->GetProgram();
			if (pr->kind == PropRef::P_POSITION)
				cam.position = get_vec(linei);
			if (pr->kind == PropRef::P_ORIENTATION)
				cam.orientation = get_quat(linei);
		}
		if (selected_ref->kind == TreeNodeRef::K_FOCUS) {
			GeomCamera& cam = e->state->GetFocus();
			if (pr->kind == PropRef::P_POSITION)
				cam.position = get_vec(linei);
			if (pr->kind == PropRef::P_ORIENTATION)
				cam.orientation = get_quat(linei);
		}
		RefreshAll();
	}
}

void GeomProjectCtrl::BuildViewMenu(Bar& bar, int i) {
	if (i < 0 || i >= 4)
		return;
	bar.Add(t_("View: YZ"), [=] { if (rends[i]) rends[i]->SetViewMode(VIEWMODE_YZ); RefreshRenderer(i); });
	bar.Add(t_("View: XZ"), [=] { if (rends[i]) rends[i]->SetViewMode(VIEWMODE_XZ); RefreshRenderer(i); });
	bar.Add(t_("View: XY"), [=] { if (rends[i]) rends[i]->SetViewMode(VIEWMODE_XY); RefreshRenderer(i); });
	bar.Add(t_("View: Perspective"), [=] { if (rends[i]) rends[i]->SetViewMode(VIEWMODE_PERSPECTIVE); RefreshRenderer(i); });
	bar.Separator();
	bar.Add(t_("Camera: Focus"), [=] { if (rends[i]) rends[i]->SetCameraSource(CAMSRC_FOCUS); RefreshRenderer(i); });
	bar.Add(t_("Camera: Program"), [=] { if (rends[i]) rends[i]->SetCameraSource(CAMSRC_PROGRAM); RefreshRenderer(i); });
	bar.Add(t_("Camera: Selected"), [=] {
		if (!e || !rends[i])
			return;
		GeomObject* obj = e->v0.selected_obj;
		if (!obj || !obj->IsCamera())
			return;
		rends[i]->SetCameraSource(CAMSRC_OBJECT);
		rends[i]->SetCameraObjectKey(obj->key);
		RefreshRenderer(i);
	});
	bar.Separator();
	bar.Add(t_("Renderer: V1"), [=] { SetRendererVersion(i, 1); }).Check(rend_version[i] == 1);
	bar.Add(t_("Renderer: V2"), [=] { SetRendererVersion(i, 2); }).Check(rend_version[i] == 2);
	bar.Add(t_("Renderer: V2 Ogl"), [=] { SetRendererVersion(i, 3); }).Check(rend_version[i] == 3);
	bar.Add(t_("Renderer: Wireframe"), [=] {
		if (rends[i]) {
			rends[i]->SetWireframeOnly(!rends[i]->IsWireframeOnly());
			RefreshRenderer(i);
		}
	}).Check(rends[i] && rends[i]->IsWireframeOnly());
	bar.Separator();
	bar.Add(t_("Reset Camera"), [=] {
		if (!rends[i])
			return;
		GeomCamera& cam = rends[i]->GetGeomCamera();
		cam.position = vec3(0, 0, 0);
		cam.orientation = Identity<quat>();
		cam.scale = 1.0f;
		RefreshRenderer(i);
	});
}

void GeomProjectCtrl::SetRendererVersion(int i, int version) {
	if (i < 0 || i >= 4)
		return;
	if (version != 1 && version != 2 && version != 3)
		return;
	EditRendererBase* prev = rends[i];
	rend_version[i] = version;
	EditRendererBase* next = (version == 1) ? static_cast<EditRendererBase*>(&rends_v1[i])
	                                        : (version == 2) ? static_cast<EditRendererBase*>(&rends_v2[i])
	                                                         : static_cast<EditRendererBase*>(&rends_v2_ogl[i]);
	if (prev && next) {
		next->view_mode = prev->view_mode;
		next->cam_src = prev->cam_src;
		next->cam_object_key = prev->cam_object_key;
		next->ctx = prev->ctx;
	}
	rends[i] = next;
	RebuildGrid();
	RefreshRenderer(i);
}

void GeomProjectCtrl::RebuildGrid() {
	for (int i = 0; i < 4; i++) {
		grid.RemoveChild(&rends_v1[i]);
		grid.RemoveChild(&rends_v2[i]);
		grid.RemoveChild(&rends_v2_ogl[i]);
	}
	for (int i = 0; i < 4; i++) {
		if (rends[i])
			grid.Add(*rends[i]);
	}
	grid.Layout();
}

void GeomProjectCtrl::OnCursor(int i) {
	if (!e)
		return;
	if (e->timeline_scope == Edit3D::TS_SCENE && e->state && e->state->HasActiveScene()) {
		GeomScene& scene = e->state->GetActiveScene();
		GeomSceneTimeline& tl = scene.GetTimeline();
		tl.position = i;
		if (e->prj && e->prj->kps > 0) {
			double frame_time = 1.0 / (double)e->prj->kps;
			tl.time = frame_time * i;
		}
		tl.ApplyAtPosition(*e->state, tl.position, tl.time);
	}
	else if (e->anim) {
		e->anim->position = i;
		if (e->prj && e->prj->kps > 0) {
			double frame_time = 1.0 / (double)e->prj->kps;
			e->anim->time = frame_time * i;
			e->anim->ApplyAtPosition(i, e->anim->time);
		}
	}
	last_props_frame = i;
	SyncPropsValues();
	RefreshAll();
}

void GeomProjectCtrl::SyncPropsValues() {
	if (props_refreshing)
		return;
	if (!e || !e->state)
		return;
	vec3 pos(0, 0, 0);
	quat ori = Identity<quat>();
	if (selected_obj) {
		bool use_timeline = false;
		int frame = e->anim ? e->anim->position : 0;
		double time = e->anim ? e->anim->time : 0.0;
		int kps = (e && e->prj) ? e->prj->kps : 0;
		if (e && e->state && e->state->HasActiveScene()) {
			GeomScene& scene = e->state->GetActiveScene();
			GeomSceneTimeline& tl_scene = scene.GetTimeline();
			if (e->timeline_scope == Edit3D::TS_SCENE || tl_scene.is_playing) {
				frame = tl_scene.position;
				if (kps > 0)
					time = frame / (double)kps;
				use_timeline = true;
			}
		}
		if (GeomTimeline* tl = selected_obj->FindTimeline()) {
			if (use_timeline && !tl->keypoints.IsEmpty() && kps > 0) {
				int pre_i = tl->FindPrePosition(frame);
				int post_i = tl->FindPostPosition(frame);
				if (pre_i >= 0 && post_i >= 0) {
					GeomKeypoint& pre = tl->keypoints[pre_i];
					GeomKeypoint& post = tl->keypoints[post_i];
					float pre_time = pre.frame_id / (float)kps;
					float post_time = post.frame_id / (float)kps;
					float f = (post_time > pre_time) ? (time - pre_time) / (post_time - pre_time) : 0.0f;
					f = min(max(f, 0.0f), 1.0f);
					pos = Lerp(pre.position, post.position, f);
				}
				else if (pre_i >= 0) {
					GeomKeypoint& pre = tl->keypoints[pre_i];
					pos = pre.position;
				}
				else if (post_i >= 0) {
					GeomKeypoint& post = tl->keypoints[post_i];
					pos = post.position;
				}
				
				pre_i = tl->FindPreOrientation(frame);
				post_i = tl->FindPostOrientation(frame);
				if (pre_i >= 0 && post_i >= 0) {
					GeomKeypoint& pre = tl->keypoints[pre_i];
					GeomKeypoint& post = tl->keypoints[post_i];
					float pre_time = pre.frame_id / (float)kps;
					float post_time = post.frame_id / (float)kps;
					float f = (post_time > pre_time) ? (time - pre_time) / (post_time - pre_time) : 0.0f;
					f = min(max(f, 0.0f), 1.0f);
					ori = Slerp(pre.orientation, post.orientation, f);
				}
				else if (pre_i >= 0) {
					GeomKeypoint& pre = tl->keypoints[pre_i];
					ori = pre.orientation;
				}
				else if (post_i >= 0) {
					GeomKeypoint& post = tl->keypoints[post_i];
					ori = post.orientation;
				}
			}
			else if (!tl->keypoints.IsEmpty()) {
				const GeomObjectState* os = e->state->FindObjectStateByKey(selected_obj->key);
				if (os) {
					pos = os->position;
					ori = os->orientation;
				}
			}
			else if (GeomTransform* tr = selected_obj->FindTransform()) {
				pos = tr->position;
				ori = tr->orientation;
			}
			else {
				const GeomObjectState* os = e->state->FindObjectStateByKey(selected_obj->key);
				if (os) {
					pos = os->position;
					ori = os->orientation;
				}
			}
		}
		else if (GeomTransform* tr = selected_obj->FindTransform()) {
			pos = tr->position;
			ori = tr->orientation;
		}
		else {
			const GeomObjectState* os = e->state->FindObjectStateByKey(selected_obj->key);
			if (os) {
				pos = os->position;
				ori = os->orientation;
			}
		}
	}
	else if (selected_ref) {
		if (selected_ref->kind == TreeNodeRef::K_PROGRAM) {
			GeomCamera& cam = e->state->GetProgram();
			pos = cam.position;
			ori = cam.orientation;
		}
		if (selected_ref->kind == TreeNodeRef::K_FOCUS) {
			GeomCamera& cam = e->state->GetFocus();
			pos = cam.position;
			ori = cam.orientation;
		}
	}
	if (props_transform_pos_ctrl) {
		if (Vec3EditCtrl* vc = dynamic_cast<Vec3EditCtrl*>(props_transform_pos_ctrl))
			vc->SetValue(pos);
	}
	if (props_transform_ori_ctrl) {
		if (QuatEditCtrl* qc = dynamic_cast<QuatEditCtrl*>(props_transform_ori_ctrl))
			qc->SetValue(ori);
	}
	if (props_selection_center_ctrl) {
		vec3 sel_center(0, 0, 0);
		bool have = false;
		if (e) {
			bool has_mesh_sel = !e->mesh_sel_points.IsEmpty() || !e->mesh_sel_lines.IsEmpty() || !e->mesh_sel_faces.IsEmpty();
			bool has_2d_sel = !e->select_2d_shapes.IsEmpty();
			if (has_mesh_sel)
				have = e->GetMeshSelectionCenter(sel_center);
			else if (has_2d_sel)
				have = e->Get2DSelectionCenter(sel_center);
		}
		if (!have)
			sel_center = vec3(0, 0, 0);
		if (Vec3EditCtrl* vc = dynamic_cast<Vec3EditCtrl*>(props_selection_center_ctrl))
			vc->SetValue(sel_center);
	}
	if (props_selection_offset_ctrl) {
		vec3 sel_offset(0, 0, 0);
		if (e) {
			bool has_mesh_sel = !e->mesh_sel_points.IsEmpty() || !e->mesh_sel_lines.IsEmpty() || !e->mesh_sel_faces.IsEmpty();
			bool has_2d_sel = !e->select_2d_shapes.IsEmpty();
			if (has_mesh_sel)
				sel_offset = e->mesh_sel_offset;
			else if (has_2d_sel)
				sel_offset = vec3(e->sel2d_offset[0], e->sel2d_offset[1], 0.0f);
		}
		if (Vec3EditCtrl* vc = dynamic_cast<Vec3EditCtrl*>(props_selection_offset_ctrl))
			vc->SetValue(sel_offset);
	}
	if (e && e->verbose_debug) {
		LOG(Format("SyncPropsValues obj=%d frame=%d time=%.3f pos=(%.3f, %.3f, %.3f) ori=(%.3f, %.3f, %.3f, %.3f)",
			(int)(selected_obj ? selected_obj->key : 0), e->timeline_scope == Edit3D::TS_SCENE ? (e->state && e->state->HasActiveScene() ? e->state->GetActiveScene().GetTimeline().position : 0) : (e->anim ? e->anim->position : 0),
			e->timeline_scope == Edit3D::TS_SCENE ? (e->state && e->state->HasActiveScene() ? e->state->GetActiveScene().GetTimeline().time : 0.0) : (e->anim ? e->anim->time : 0.0),
			(double)pos[0], (double)pos[1], (double)pos[2],
			(double)ori[0], (double)ori[1], (double)ori[2], (double)ori[3]));
	}
}

void GeomProjectCtrl::TreeValue(int id, VfsValue& node) {
	auto warn_unknown = [&](const VfsValue& n) {
		hash_t type_hash = n.ext ? n.ext->GetTypeHash() : n.type_hash;
		if (warned_tree_types.Find(type_hash) >= 0)
			return;
		warned_tree_types.Add(type_hash);
		LOG("GeomProjectCtrl: unexpected VfsValue type in tree: " + n.GetTypeString());
	};
	auto add_bone = [&](auto&& add_bone, int parent_id, VfsValue& bone_node) -> void {
		GeomBone& bone = bone_node.GetExt<GeomBone>();
		String name = bone.name.IsEmpty() ? bone_node.id : bone.name;
		int j = tree.Add(parent_id, ImagesImg::Object(), RawToValue(&bone_node), name);
		tree.SetRowValue(j, tree_col_visible, Null);
		tree.SetRowValue(j, tree_col_locked, Null);
		tree.SetRowValue(j, tree_col_read, Null);
		tree.SetRowValue(j, tree_col_write, Null);
		for (auto& sub : bone_node.sub) {
			if (IsVfsType(sub, AsTypeHash<GeomBone>()))
				add_bone(add_bone, j, sub);
		}
	};
	Vector<VfsValue*> dirs;
	Vector<VfsValue*> objs;
	Vector<VfsValue*> datasets;
	for (auto& s : node.sub) {
		if (IsVfsType(s, AsTypeHash<GeomDirectory>()))
			dirs.Add(&s);
		else if (IsVfsType(s, AsTypeHash<GeomObject>()))
			objs.Add(&s);
		else if (IsVfsType(s, AsTypeHash<GeomPointcloudDataset>()))
			datasets.Add(&s);
		else
			warn_unknown(s);
	}
	for (VfsValue* s : dirs) {
		GeomDirectory& dir = s->GetExt<GeomDirectory>();
		String name = dir.name.IsEmpty() ? dir.val.id : dir.name;
		int j = tree.Add(id, ImagesImg::Directory(), RawToValue(s), name);
		tree.SetRowValue(j, tree_col_visible, Null);
		tree.SetRowValue(j, tree_col_locked, Null);
		tree.SetRowValue(j, tree_col_read, Null);
		tree.SetRowValue(j, tree_col_write, Null);
		TreeValue(j, *s);
	}
	for (VfsValue* s : objs) {
		GeomObject& o = s->GetExt<GeomObject>();
		Image img;
		switch (o.type) {
			case GeomObject::O_CAMERA: img = ImagesImg::Camera(); break;
			case GeomObject::O_MODEL:  img = ImagesImg::Model(); break;
			case GeomObject::O_OCTREE: img = ImagesImg::Octree(); break;
			default: img = ImagesImg::Object();
		}
		String name = o.name.IsEmpty() ? s->id : o.name;
		int j = tree.Add(id, img, RawToValue(s), name);
		tree.SetRowValue(j, tree_col_visible, o.is_visible);
		tree.SetRowValue(j, tree_col_locked, o.is_locked);
		tree.SetRowValue(j, tree_col_read, o.read_enabled);
		tree.SetRowValue(j, tree_col_write, o.write_enabled);
		for (auto& sub : s->sub) {
			if (!IsVfsType(sub, AsTypeHash<GeomSkeleton>()))
				continue;
			GeomSkeleton& sk = sub.GetExt<GeomSkeleton>();
			String sk_name = sk.name.IsEmpty() ? sub.id : sk.name;
			int sk_id = tree.Add(j, ImagesImg::Directory(), RawToValue(&sub), sk_name);
			tree.SetRowValue(sk_id, tree_col_visible, Null);
			tree.SetRowValue(sk_id, tree_col_locked, Null);
			tree.SetRowValue(sk_id, tree_col_read, Null);
			tree.SetRowValue(sk_id, tree_col_write, Null);
			for (auto& b : sub.sub) {
				if (IsVfsType(b, AsTypeHash<GeomBone>()))
					add_bone(add_bone, sk_id, b);
			}
		}
	}
	for (VfsValue* s : datasets) {
		GeomPointcloudDataset& ds = s->GetExt<GeomPointcloudDataset>();
		String name = ds.name.IsEmpty() ? s->id : ds.name;
		int j = tree.Add(id, ImagesImg::Octree(), RawToValue(s), name);
		tree.SetRowValue(j, tree_col_visible, Null);
		tree.SetRowValue(j, tree_col_locked, Null);
		tree.SetRowValue(j, tree_col_read, Null);
		tree.SetRowValue(j, tree_col_write, Null);
	}
}

void GeomProjectCtrl::TimelineData() {
	GeomProject& prj = *e->prj;
	GeomScene& scene = e->state->GetActiveScene();
	Vector<GeomObject*> objects;
	GeomObjectCollection collection(scene);
	for (GeomObject& o : collection)
		objects.Add(&o);

	timeline_rows.Clear();
	
	Vector<TimelineRowInfo> rows;
	TimelineRowInfo scene_row;
	scene_row.kind = TimelineRowInfo::R_SCENE;
	scene_row.indent = 0;
	scene_row.has_children = true;
	scene_row.expanded = true;
	scene_row.active = (e->timeline_scope == Edit3D::TS_SCENE);
	rows.Add(scene_row);

	for (GeomObject* obj : objects) {
		TimelineRowInfo obj_row;
		obj_row.kind = TimelineRowInfo::R_OBJECT;
		obj_row.object_key = obj->key;
		obj_row.indent = 1;
		obj_row.has_children = true;
		int idx = timeline_expanded.Find(obj->key);
		obj_row.expanded = (idx >= 0);
		obj_row.active = (e->timeline_scope != Edit3D::TS_SCENE && e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_NONE);
		int midx = timeline_muted.Find(obj->key);
		if (midx >= 0)
			obj_row.muted = timeline_muted[midx];
		int sidx = timeline_solo.Find(obj->key);
		if (sidx >= 0)
			obj_row.solo = timeline_solo[sidx];
		int cidx = timeline_row_color.Find(obj->key);
		if (cidx >= 0)
			obj_row.tag_color = timeline_row_color[cidx];
		rows.Add(obj_row);

		if (obj_row.has_children && obj_row.expanded) {
			TimelineRowInfo tr;
			tr.kind = TimelineRowInfo::R_TRANSFORM;
			tr.object_key = obj->key;
			tr.indent = 2;
			tr.active = (e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_TRANSFORM && e->timeline_transform_field == Edit3D::TT_NONE);
			tr.muted = obj_row.muted;
			tr.solo = obj_row.solo;
			tr.tag_color = obj_row.tag_color;
			rows.Add(tr);
			TimelineRowInfo pr;
			pr.kind = TimelineRowInfo::R_POSITION;
			pr.object_key = obj->key;
			pr.indent = 3;
			pr.active = (e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_TRANSFORM && e->timeline_transform_field == Edit3D::TT_POSITION);
			pr.muted = obj_row.muted;
			pr.solo = obj_row.solo;
			pr.tag_color = obj_row.tag_color;
			rows.Add(pr);
			TimelineRowInfo orow;
			orow.kind = TimelineRowInfo::R_ORIENTATION;
			orow.object_key = obj->key;
			orow.indent = 3;
			orow.active = (e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_TRANSFORM && e->timeline_transform_field == Edit3D::TT_ORIENTATION);
			orow.muted = obj_row.muted;
			orow.solo = obj_row.solo;
			orow.tag_color = obj_row.tag_color;
			rows.Add(orow);
			if (obj->FindEditableMesh()) {
				TimelineRowInfo mr;
				mr.kind = TimelineRowInfo::R_MESH;
				mr.object_key = obj->key;
				mr.indent = 2;
				mr.active = (e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_MESH);
				mr.muted = obj_row.muted;
				mr.solo = obj_row.solo;
				mr.tag_color = obj_row.tag_color;
				rows.Add(mr);
			}
			if (obj->Find2DLayer()) {
				TimelineRowInfo r2d;
				r2d.kind = TimelineRowInfo::R_2D;
				r2d.object_key = obj->key;
				r2d.indent = 2;
				r2d.active = (e->timeline_object_key == obj->key && e->timeline_component == Edit3D::TC_2D);
				r2d.muted = obj_row.muted;
				r2d.solo = obj_row.solo;
				r2d.tag_color = obj_row.tag_color;
				rows.Add(r2d);
			}
		}
	}
	timeline_rows = pick(rows);
	time.SetCount(timeline_rows.GetCount());
	time.SetKeypointRate(prj.kps);
	time.SetLength(scene.length);
	time.SetKeypointColumnWidth(13);
	
	for (int i = 0; i < timeline_rows.GetCount(); i++) {
		TimelineRowInfo& info = timeline_rows[i];
		TimelineRowCtrl& row = time.GetRowIndex(i);
		row.SetIndent(info.indent);
		row.SetHasChildren(info.has_children);
		row.SetExpanded(info.expanded);
		row.SetActive(info.active);
		row.SetTagColor(info.tag_color);
		Index<int> keys;
		String name;
		if (info.kind == TimelineRowInfo::R_SCENE) {
			name = scene.name.IsEmpty() ? "Scene" : scene.name;
		}
		else if (GeomObject* obj = e->state->FindObjectByKey(info.object_key)) {
			name = obj->name.IsEmpty() ? "Object" : obj->name;
			if (info.kind == TimelineRowInfo::R_TRANSFORM) name << " / Transform";
			if (info.kind == TimelineRowInfo::R_POSITION) name << " / Position";
			if (info.kind == TimelineRowInfo::R_ORIENTATION) name << " / Orientation";
			if (info.kind == TimelineRowInfo::R_MESH) name << " / Mesh";
			if (info.kind == TimelineRowInfo::R_2D) name << " / 2D";
			if (info.kind == TimelineRowInfo::R_OBJECT) {
				if (GeomTimeline* tl = obj->FindTimeline()) {
					for (int k = 0; k < tl->keypoints.GetCount(); k++) {
						int key = tl->keypoints.GetKey(k);
						if ((tl->keypoints[k].has_position || tl->keypoints[k].has_orientation) && keys.Find(key) < 0)
							keys.Add(key);
					}
				}
				if (GeomMeshAnimation* ma = obj->FindMeshAnimation()) {
					for (int k = 0; k < ma->keyframes.GetCount(); k++) {
						int key = ma->keyframes.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
				if (Geom2DAnimation* a2d = obj->Find2DAnimation()) {
					for (int k = 0; k < a2d->keyframes.GetCount(); k++) {
						int key = a2d->keyframes.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
			else if (info.kind == TimelineRowInfo::R_TRANSFORM) {
				if (GeomTimeline* tl = obj->FindTimeline()) {
					for (int k = 0; k < tl->keypoints.GetCount(); k++) {
						int key = tl->keypoints.GetKey(k);
						if ((tl->keypoints[k].has_position || tl->keypoints[k].has_orientation) && keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
			else if (info.kind == TimelineRowInfo::R_POSITION) {
				if (GeomTimeline* tl = obj->FindTimeline()) {
					for (int k = 0; k < tl->keypoints.GetCount(); k++) {
						if (!tl->keypoints[k].has_position)
							continue;
						int key = tl->keypoints.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
			else if (info.kind == TimelineRowInfo::R_ORIENTATION) {
				if (GeomTimeline* tl = obj->FindTimeline()) {
					for (int k = 0; k < tl->keypoints.GetCount(); k++) {
						if (!tl->keypoints[k].has_orientation)
							continue;
						int key = tl->keypoints.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
			else if (info.kind == TimelineRowInfo::R_MESH) {
				if (GeomMeshAnimation* ma = obj->FindMeshAnimation()) {
					for (int k = 0; k < ma->keyframes.GetCount(); k++) {
						int key = ma->keyframes.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
			else if (info.kind == TimelineRowInfo::R_2D) {
				if (Geom2DAnimation* a2d = obj->Find2DAnimation()) {
					for (int k = 0; k < a2d->keyframes.GetCount(); k++) {
						int key = a2d->keyframes.GetKey(k);
						if (keys.Find(key) < 0)
							keys.Add(key);
					}
				}
			}
		}
		row.SetTitle(name);
		Vector<int> row_keys;
		row_keys.SetCount(keys.GetCount());
		for (int k = 0; k < keys.GetCount(); k++)
			row_keys[k] = keys[k];
		Sort(row_keys);
		row.SetKeypoints(row_keys);
		row.Refresh();
	}
	time.Refresh();
	if (e) {
		String detail;
		if (e->auto_key) {
			Vector<String> parts;
			if (e->auto_key_position) parts.Add("P");
			if (e->auto_key_orientation) parts.Add("O");
			if (e->auto_key_mesh) parts.Add("M");
			if (e->auto_key_2d) parts.Add("2D");
			if (parts.IsEmpty())
				detail = t_("None");
			else
				detail = Join(parts, " ");
		}
		time.SetAutoKeyIndicator(e->auto_key, detail);
	}
}

void GeomProjectCtrl::TimelineRowMenu(Bar& bar, int row) {
	if (!e || !e->state)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE) {
		bar.Add(t_("Make Active Timeline"), [=] {
			e->timeline_scope = Edit3D::TS_SCENE;
			e->timeline_component = Edit3D::TC_NONE;
			e->timeline_object_key = 0;
			TimelineData();
		});
		bar.Add(t_("Auto-key"), [=] { e->auto_key = !e->auto_key; }).Check(e->auto_key);
		bar.Sub(t_("Auto-key Channels"), [=](Bar& bar) {
			bar.Add(t_("Position"), [=] { e->auto_key_position = !e->auto_key_position; TimelineData(); })
				.Check(e->auto_key_position);
			bar.Add(t_("Orientation"), [=] { e->auto_key_orientation = !e->auto_key_orientation; TimelineData(); })
				.Check(e->auto_key_orientation);
			bar.Add(t_("Mesh"), [=] { e->auto_key_mesh = !e->auto_key_mesh; TimelineData(); })
				.Check(e->auto_key_mesh);
			bar.Add(t_("2D"), [=] { e->auto_key_2d = !e->auto_key_2d; TimelineData(); })
				.Check(e->auto_key_2d);
		});
		bar.Separator();
		bar.Add(t_("Copy Keyframes"), [=] { TimelineCopySelection(); }).Key(K_CTRL|K_C);
		bar.Add(t_("Paste Keyframes"), [=] {
		int frame = e->timeline_scope == Edit3D::TS_SCENE && e->state && e->state->HasActiveScene()
			? e->state->GetActiveScene().GetTimeline().position
			: (e->anim ? e->anim->position : 0);
		TimelinePasteSelection(frame);
	}).Key(K_CTRL|K_V);
		return;
	}
	hash_t key = info.object_key;
	GeomObject* obj = e->state->FindObjectByKey(key);
	if (!obj)
		return;
	bar.Add(t_("Mute Row"), [=] { TimelineToggleMuteRow(row); }).Check(info.muted);
	bar.Add(t_("Solo Row"), [=] { TimelineToggleSoloRow(row); }).Check(info.solo);
	bar.Separator();
	timeline_menu_row = row;
	bar.Add(t_("Row Color"), THISBACK(TimelineRowColorMenu));
	bar.Separator();
	int frame = e->anim ? e->anim->position : 0;
	bar.Add(t_("Make Active Timeline"), [=] {
		e->timeline_scope = Edit3D::TS_OBJECT;
		e->timeline_object_key = obj->key;
		e->timeline_component = Edit3D::TC_NONE;
		e->timeline_transform_field = Edit3D::TT_NONE;
		if (info.kind == TimelineRowInfo::R_TRANSFORM) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
		}
		else if (info.kind == TimelineRowInfo::R_POSITION) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
			e->timeline_transform_field = Edit3D::TT_POSITION;
		}
		else if (info.kind == TimelineRowInfo::R_ORIENTATION) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
			e->timeline_transform_field = Edit3D::TT_ORIENTATION;
		}
		else if (info.kind == TimelineRowInfo::R_MESH) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_MESH;
		}
		else if (info.kind == TimelineRowInfo::R_2D) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_2D;
		}
		TimelineData();
	});
	bar.Add(t_("Auto-key"), [=] { e->auto_key = !e->auto_key; }).Check(e->auto_key);
	bar.Sub(t_("Auto-key Channels"), [=](Bar& bar) {
		bar.Add(t_("Position"), [=] { e->auto_key_position = !e->auto_key_position; TimelineData(); })
			.Check(e->auto_key_position);
		bar.Add(t_("Orientation"), [=] { e->auto_key_orientation = !e->auto_key_orientation; TimelineData(); })
			.Check(e->auto_key_orientation);
		bar.Add(t_("Mesh"), [=] { e->auto_key_mesh = !e->auto_key_mesh; TimelineData(); })
			.Check(e->auto_key_mesh);
		bar.Add(t_("2D"), [=] { e->auto_key_2d = !e->auto_key_2d; TimelineData(); })
			.Check(e->auto_key_2d);
	});
	bar.Separator();
	bar.Add(t_("Copy Keyframes"), [=] { TimelineCopySelection(); }).Key(K_CTRL|K_C);
	bar.Add(t_("Paste Keyframes"), [=] {
		int frame = e->timeline_scope == Edit3D::TS_SCENE && e->state && e->state->HasActiveScene()
			? e->state->GetActiveScene().GetTimeline().position
			: (e->anim ? e->anim->position : 0);
		TimelinePasteSelection(frame);
	}).Key(K_CTRL|K_V);
	bar.Separator();
	auto add_transform_kp = [&](bool pos, bool ori) {
		if (!e || !e->state)
			return;
		e->PushUndo("Add keyframe");
		GeomTimeline& tl = obj->GetTimeline();
		GeomKeypoint& kp = tl.GetAddKeypoint(frame);
		kp.frame_id = frame;
		if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key)) {
			if (pos) kp.position = os->position;
			if (ori) kp.orientation = os->orientation;
		}
		else if (GeomTransform* tr = obj->FindTransform()) {
			if (pos) kp.position = tr->position;
			if (ori) kp.orientation = tr->orientation;
		}
		if (pos) kp.has_position = true;
		if (ori) kp.has_orientation = true;
		TimelineData();
	};
	auto set_transform_ease = [&](int mode, bool pos, bool ori) {
		if (!e || !e->state)
			return;
		GeomTimeline& tl = obj->GetTimeline();
		int idx = tl.keypoints.Find(frame);
		bool existed = idx >= 0;
		GeomKeypoint& kp = existed ? tl.keypoints[idx] : tl.GetAddKeypoint(frame);
		kp.frame_id = frame;
		if ((!existed || !kp.has_position) && pos) {
			if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key))
				kp.position = os->position;
			else if (GeomTransform* tr = obj->FindTransform())
				kp.position = tr->position;
			kp.has_position = true;
		}
		if ((!existed || !kp.has_orientation) && ori) {
			if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key))
				kp.orientation = os->orientation;
			else if (GeomTransform* tr = obj->FindTransform())
				kp.orientation = tr->orientation;
			kp.has_orientation = true;
		}
		if (pos)
			kp.position_ease = mode;
		if (ori)
			kp.orientation_ease = mode;
		TimelineData();
	};
	auto get_transform_ease = [&](bool pos) -> int {
		if (GeomTimeline* tl = obj->FindTimeline()) {
			int idx = tl->keypoints.Find(frame);
			if (idx >= 0) {
				const GeomKeypoint& kp = tl->keypoints[idx];
				return pos ? kp.position_ease : kp.orientation_ease;
			}
		}
		return 0;
	};
	auto ease_check = [&](int mode, bool pos, bool ori) -> bool {
		if (pos && ori)
			return get_transform_ease(true) == mode && get_transform_ease(false) == mode;
		if (pos)
			return get_transform_ease(true) == mode;
		return get_transform_ease(false) == mode;
	};
	auto ease_menu = [&](Bar& bar, bool pos, bool ori) {
		bar.Add(t_("Linear"), [=] { set_transform_ease(0, pos, ori); }).Check(ease_check(0, pos, ori));
		bar.Add(t_("Step"), [=] { set_transform_ease(1, pos, ori); }).Check(ease_check(1, pos, ori));
		bar.Add(t_("Smooth"), [=] { set_transform_ease(2, pos, ori); }).Check(ease_check(2, pos, ori));
	};
	auto clear_transform_kp = [&](bool pos, bool ori) {
		if (e)
			e->PushUndo("Clear keyframes");
		if (GeomTimeline* tl = obj->FindTimeline()) {
			for (int i = tl->keypoints.GetCount() - 1; i >= 0; i--) {
				GeomKeypoint& kp = tl->keypoints[i];
				if (pos) kp.has_position = false;
				if (ori) kp.has_orientation = false;
				if (!kp.has_position && !kp.has_orientation)
					tl->keypoints.Remove(i);
			}
		}
		TimelineData();
	};
	if (info.kind == TimelineRowInfo::R_POSITION) {
		bar.Add(t_("Add Position Keyframe"), [=] { add_transform_kp(true, false); });
		bar.Sub(t_("Easing"), [=](Bar& bar) { ease_menu(bar, true, false); });
		bar.Add(t_("Clear Position Keyframes"), [=] { clear_transform_kp(true, false); });
	}
	else if (info.kind == TimelineRowInfo::R_ORIENTATION) {
		bar.Add(t_("Add Orientation Keyframe"), [=] { add_transform_kp(false, true); });
		bar.Sub(t_("Easing"), [=](Bar& bar) { ease_menu(bar, false, true); });
		bar.Add(t_("Clear Orientation Keyframes"), [=] { clear_transform_kp(false, true); });
	}
	else {
		bar.Add(t_("Add Transform Keyframe"), [=] { add_transform_kp(true, true); });
		if (info.kind == TimelineRowInfo::R_TRANSFORM) {
			bar.Add(t_("Add Position Keyframe"), [=] { add_transform_kp(true, false); });
			bar.Add(t_("Add Orientation Keyframe"), [=] { add_transform_kp(false, true); });
		}
		if (info.kind == TimelineRowInfo::R_TRANSFORM || info.kind == TimelineRowInfo::R_OBJECT) {
			bar.Sub(t_("Easing"), [=](Bar& bar) {
				bar.Sub(t_("Position"), [=](Bar& bar) { ease_menu(bar, true, false); });
				bar.Sub(t_("Orientation"), [=](Bar& bar) { ease_menu(bar, false, true); });
				bar.Sub(t_("Transform"), [=](Bar& bar) { ease_menu(bar, true, true); });
			});
		}
		if (obj->FindEditableMesh()) {
			bar.Add(t_("Add Mesh Keyframe"), [=] {
				if (GeomEditableMesh* mesh = obj->FindEditableMesh()) {
					if (e)
						e->PushUndo("Add mesh keyframe");
					GeomMeshAnimation& anim = obj->GetMeshAnimation();
					GeomMeshKeyframe& kf = anim.GetAddKeyframe(frame);
					kf.frame_id = frame;
					kf.points.SetCount(mesh->points.GetCount());
					for (int i = 0; i < mesh->points.GetCount(); i++)
						kf.points[i] = mesh->points[i];
					TimelineData();
				}
			});
		}
		if (obj->Find2DLayer()) {
			bar.Add(t_("Add 2D Keyframe"), [=] {
				if (Geom2DLayer* layer = obj->Find2DLayer()) {
					if (e)
						e->PushUndo("Add 2D keyframe");
					Geom2DAnimation& anim = obj->Get2DAnimation();
					Geom2DKeyframe& kf = anim.GetAddKeyframe(frame);
					kf.frame_id = frame;
					kf.shapes.SetCount(layer->shapes.GetCount());
					for (int i = 0; i < layer->shapes.GetCount(); i++) {
						const Geom2DShape& s = layer->shapes[i];
						Geom2DShape& d = kf.shapes[i];
						d.type = s.type;
						d.radius = s.radius;
						d.stroke = s.stroke;
						d.width = s.width;
						d.closed = s.closed;
						d.points.SetCount(s.points.GetCount());
						for (int k = 0; k < s.points.GetCount(); k++)
							d.points[k] = s.points[k];
					}
					TimelineData();
				}
			});
		}
	}
	bar.Separator();
	if (info.kind == TimelineRowInfo::R_POSITION) {
		bar.Add(t_("Clear Position Keyframes"), [=] { clear_transform_kp(true, false); });
	}
	else if (info.kind == TimelineRowInfo::R_ORIENTATION) {
		bar.Add(t_("Clear Orientation Keyframes"), [=] { clear_transform_kp(false, true); });
	}
	else {
		bar.Add(t_("Clear Transform Keyframes"), [=] { clear_transform_kp(true, true); });
	}
	if (GeomMeshAnimation* ma = obj->FindMeshAnimation()) {
		bar.Add(t_("Clear Mesh Keyframes"), [=] {
			if (e)
				e->PushUndo("Clear mesh keyframes");
			ma->keyframes.Clear();
			TimelineData();
		});
	}
	if (Geom2DAnimation* a2d = obj->Find2DAnimation()) {
		bar.Add(t_("Clear 2D Keyframes"), [=] {
			if (e)
				e->PushUndo("Clear 2D keyframes");
			a2d->keyframes.Clear();
			TimelineData();
		});
	}
	bar.Add(t_("Clear All Keyframes"), [=] {
		if (e)
			e->PushUndo("Clear all keyframes");
		if (GeomTimeline* tl = obj->FindTimeline()) {
			for (int i = tl->keypoints.GetCount() - 1; i >= 0; i--)
				tl->keypoints.Remove(i);
		}
		if (GeomMeshAnimation* ma = obj->FindMeshAnimation())
			ma->keyframes.Clear();
		if (Geom2DAnimation* a2d = obj->Find2DAnimation())
			a2d->keyframes.Clear();
		TimelineData();
	});
}

void GeomProjectCtrl::TimelineToggleKeyframe(int row, int frame) {
	if (!e || !e->state)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	e->PushUndo("Toggle keyframe");
	Vector<int> rows = time.GetSelectedRows();
	if (rows.IsEmpty())
		rows.Add(row);
	bool has_range = time.HasSelectionRange();
	int range_start = has_range ? time.GetRangeStart() : frame;
	int range_end = has_range ? time.GetRangeEnd() : frame;
	auto set_kp_values = [&](GeomObject* obj, GeomKeypoint& kp, bool pos, bool ori) {
		if (const GeomObjectState* os = e->state->FindObjectStateByKey(obj->key)) {
			if (pos) kp.position = os->position;
			if (ori) kp.orientation = os->orientation;
		}
		else if (GeomTransform* tr = obj->FindTransform()) {
			if (pos) kp.position = tr->position;
			if (ori) kp.orientation = tr->orientation;
		}
	};
	auto add_transform = [&](GeomObject* obj, int f, bool pos, bool ori) {
		GeomTimeline& tl = obj->GetTimeline();
		GeomKeypoint& kp = tl.GetAddKeypoint(f);
		kp.has_position = kp.has_position || pos;
		kp.has_orientation = kp.has_orientation || ori;
		set_kp_values(obj, kp, pos, ori);
	};
	auto toggle_transform = [&](GeomObject* obj, int f, bool pos, bool ori, bool bulk) {
		if (bulk) {
			add_transform(obj, f, pos, ori);
			return;
		}
		GeomTimeline& tl = obj->GetTimeline();
		int idx = tl.keypoints.Find(f);
		if (idx < 0) {
			add_transform(obj, f, pos, ori);
			return;
		}
		GeomKeypoint& kp = tl.keypoints[idx];
		bool has_all = (!pos || kp.has_position) && (!ori || kp.has_orientation);
		if (has_all) {
			if (pos) kp.has_position = false;
			if (ori) kp.has_orientation = false;
		}
		else {
			if (pos && !kp.has_position) {
				kp.has_position = true;
				set_kp_values(obj, kp, true, false);
			}
			if (ori && !kp.has_orientation) {
				kp.has_orientation = true;
				set_kp_values(obj, kp, false, true);
			}
		}
		if (!kp.has_position && !kp.has_orientation)
			tl.keypoints.Remove(idx);
	};
	auto add_mesh_kf = [&](GeomObject* obj, int f) {
		if (GeomEditableMesh* mesh = obj->FindEditableMesh()) {
			GeomMeshAnimation& anim = obj->GetMeshAnimation();
			GeomMeshKeyframe& kf = anim.GetAddKeyframe(f);
			kf.frame_id = f;
			kf.points.SetCount(mesh->points.GetCount());
			for (int i = 0; i < mesh->points.GetCount(); i++)
				kf.points[i] = mesh->points[i];
		}
	};
	auto add_2d_kf = [&](GeomObject* obj, int f) {
		if (Geom2DLayer* layer = obj->Find2DLayer()) {
			Geom2DAnimation& anim = obj->Get2DAnimation();
			Geom2DKeyframe& kf = anim.GetAddKeyframe(f);
			kf.frame_id = f;
			kf.shapes.SetCount(layer->shapes.GetCount());
			for (int i = 0; i < layer->shapes.GetCount(); i++) {
				const Geom2DShape& s = layer->shapes[i];
				Geom2DShape& d = kf.shapes[i];
				d.type = s.type;
				d.radius = s.radius;
				d.stroke = s.stroke;
				d.width = s.width;
				d.closed = s.closed;
				d.points.SetCount(s.points.GetCount());
				for (int k = 0; k < s.points.GetCount(); k++)
					d.points[k] = s.points[k];
			}
		}
	};
	for (int r = 0; r < rows.GetCount(); r++) {
		int row_id = rows[r];
		if (row_id < 0 || row_id >= timeline_rows.GetCount())
			continue;
		const TimelineRowInfo& info = timeline_rows[row_id];
		if (info.kind == TimelineRowInfo::R_SCENE)
			continue;
		GeomObject* obj = e->state->FindObjectByKey(info.object_key);
		if (!obj)
			continue;
		for (int f = range_start; f <= range_end; f++) {
			if (info.kind == TimelineRowInfo::R_POSITION)
				toggle_transform(obj, f, true, false, has_range);
			else if (info.kind == TimelineRowInfo::R_ORIENTATION)
				toggle_transform(obj, f, false, true, has_range);
			else if (info.kind == TimelineRowInfo::R_TRANSFORM || info.kind == TimelineRowInfo::R_OBJECT)
				toggle_transform(obj, f, true, true, has_range);
			else if (info.kind == TimelineRowInfo::R_MESH)
				add_mesh_kf(obj, f);
			else if (info.kind == TimelineRowInfo::R_2D)
				add_2d_kf(obj, f);
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelineRemoveKeyframe(int row, int frame) {
	if (!e || !e->state)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	e->PushUndo("Remove keyframe");
	Vector<int> rows = time.GetSelectedRows();
	if (rows.IsEmpty())
		rows.Add(row);
	bool has_range = time.HasSelectionRange();
	int range_start = has_range ? time.GetRangeStart() : frame;
	int range_end = has_range ? time.GetRangeEnd() : frame;
	for (int r = 0; r < rows.GetCount(); r++) {
		int row_id = rows[r];
		if (row_id < 0 || row_id >= timeline_rows.GetCount())
			continue;
		const TimelineRowInfo& info = timeline_rows[row_id];
		if (info.kind == TimelineRowInfo::R_SCENE)
			continue;
		GeomObject* obj = e->state->FindObjectByKey(info.object_key);
		if (!obj)
			continue;
		for (int f = range_start; f <= range_end; f++) {
			auto remove_transform = [&](bool pos, bool ori) {
				if (GeomTimeline* tl = obj->FindTimeline()) {
					int idx = tl->keypoints.Find(f);
					if (idx < 0)
						return;
					GeomKeypoint& kp = tl->keypoints[idx];
					if (pos) kp.has_position = false;
					if (ori) kp.has_orientation = false;
					if (!kp.has_position && !kp.has_orientation)
						tl->keypoints.Remove(idx);
				}
			};
			if (info.kind == TimelineRowInfo::R_POSITION)
				remove_transform(true, false);
			else if (info.kind == TimelineRowInfo::R_ORIENTATION)
				remove_transform(false, true);
			else if (info.kind == TimelineRowInfo::R_TRANSFORM || info.kind == TimelineRowInfo::R_OBJECT)
				remove_transform(true, true);
			else if (info.kind == TimelineRowInfo::R_MESH) {
				if (GeomMeshAnimation* anim = obj->FindMeshAnimation()) {
					int idx = anim->keyframes.Find(f);
					if (idx >= 0)
						anim->keyframes.Remove(idx);
				}
			}
			else if (info.kind == TimelineRowInfo::R_2D) {
				if (Geom2DAnimation* anim = obj->Find2DAnimation()) {
					int idx = anim->keyframes.Find(f);
					if (idx >= 0)
						anim->keyframes.Remove(idx);
				}
			}
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelineToggleAutoKey() {
	if (!e)
		return;
	e->auto_key = !e->auto_key;
	TimelineData();
}

void GeomProjectCtrl::TimelineCopySelection() {
	timeline_clipboard.Clear();
	if (!e || !e->state)
		return;
	Vector<int> rows = time.GetSelectedRows();
	if (rows.IsEmpty())
		return;
	int base_frame = e->timeline_scope == Edit3D::TS_SCENE && e->state->HasActiveScene()
		? e->state->GetActiveScene().GetTimeline().position
		: (e->anim ? e->anim->position : 0);
	bool has_range = time.HasSelectionRange();
	int range_start = has_range ? time.GetRangeStart() : base_frame;
	int range_end = has_range ? time.GetRangeEnd() : base_frame;
	timeline_clipboard.base_frame = range_start;
	timeline_clipboard.has_range = has_range;
	auto copy_shape = [&](Geom2DShape& dst, const Geom2DShape& src) {
		dst.type = src.type;
		dst.radius = src.radius;
		dst.stroke = src.stroke;
		dst.width = src.width;
		dst.closed = src.closed;
		dst.tex_wrap = src.tex_wrap;
		dst.stroke_uv_mode = src.stroke_uv_mode;
		dst.tex_repeat_x = src.tex_repeat_x;
		dst.tex_repeat_y = src.tex_repeat_y;
		dst.points.SetCount(src.points.GetCount());
		for (int i = 0; i < src.points.GetCount(); i++)
			dst.points[i] = src.points[i];
	};
	for (int r = 0; r < rows.GetCount(); r++) {
		int row_id = rows[r];
		if (row_id < 0 || row_id >= timeline_rows.GetCount())
			continue;
		const TimelineRowInfo& info = timeline_rows[row_id];
		if (info.kind == TimelineRowInfo::R_SCENE)
			continue;
		GeomObject* obj = e->state->FindObjectByKey(info.object_key);
		if (!obj)
			continue;
		auto add_transform = [&](GeomTimeline* tl, int idx) {
			if (!tl)
				return;
			int frame = tl->keypoints.GetKey(idx);
			if (frame < range_start || frame > range_end)
				return;
			const GeomKeypoint& kp = tl->keypoints[idx];
			if (info.kind == TimelineRowInfo::R_POSITION && !kp.has_position)
				return;
			if (info.kind == TimelineRowInfo::R_ORIENTATION && !kp.has_orientation)
				return;
			TimelineClipboard::Item it;
			it.kind = info.kind;
			it.object_key = info.object_key;
			it.frame_offset = frame - range_start;
			it.keypoint = kp;
			timeline_clipboard.items.Add(pick(it));
		};
		if (GeomTimeline* tl = obj->FindTimeline()) {
			for (int i = 0; i < tl->keypoints.GetCount(); i++)
				add_transform(tl, i);
		}
		if (info.kind == TimelineRowInfo::R_MESH || info.kind == TimelineRowInfo::R_OBJECT) {
			if (GeomMeshAnimation* ma = obj->FindMeshAnimation()) {
				for (int i = 0; i < ma->keyframes.GetCount(); i++) {
					int frame = ma->keyframes.GetKey(i);
					if (frame < range_start || frame > range_end)
						continue;
					TimelineClipboard::Item it;
					it.kind = TimelineRowInfo::R_MESH;
					it.object_key = info.object_key;
					it.frame_offset = frame - range_start;
					it.mesh_kf.frame_id = frame;
					it.mesh_kf.points.SetCount(ma->keyframes[i].points.GetCount());
					for (int k = 0; k < ma->keyframes[i].points.GetCount(); k++)
						it.mesh_kf.points[k] = ma->keyframes[i].points[k];
					timeline_clipboard.items.Add(pick(it));
				}
			}
		}
		if (info.kind == TimelineRowInfo::R_2D || info.kind == TimelineRowInfo::R_OBJECT) {
			if (Geom2DAnimation* a2d = obj->Find2DAnimation()) {
				for (int i = 0; i < a2d->keyframes.GetCount(); i++) {
					int frame = a2d->keyframes.GetKey(i);
					if (frame < range_start || frame > range_end)
						continue;
					TimelineClipboard::Item it;
					it.kind = TimelineRowInfo::R_2D;
					it.object_key = info.object_key;
					it.frame_offset = frame - range_start;
					it.a2d_kf.frame_id = frame;
					it.a2d_kf.shapes.SetCount(a2d->keyframes[i].shapes.GetCount());
					for (int k = 0; k < a2d->keyframes[i].shapes.GetCount(); k++)
						copy_shape(it.a2d_kf.shapes[k], a2d->keyframes[i].shapes[k]);
					timeline_clipboard.items.Add(pick(it));
				}
			}
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelinePasteSelection(int frame) {
	if (!e || !e->state)
		return;
	if (timeline_clipboard.items.IsEmpty())
		return;
	e->PushUndo("Paste keyframes");
	auto copy_shape = [&](Geom2DShape& dst, const Geom2DShape& src) {
		dst.type = src.type;
		dst.radius = src.radius;
		dst.stroke = src.stroke;
		dst.width = src.width;
		dst.closed = src.closed;
		dst.tex_wrap = src.tex_wrap;
		dst.stroke_uv_mode = src.stroke_uv_mode;
		dst.tex_repeat_x = src.tex_repeat_x;
		dst.tex_repeat_y = src.tex_repeat_y;
		dst.points.SetCount(src.points.GetCount());
		for (int i = 0; i < src.points.GetCount(); i++)
			dst.points[i] = src.points[i];
	};
	for (const TimelineClipboard::Item& it : timeline_clipboard.items) {
		GeomObject* obj = e->state->FindObjectByKey(it.object_key);
		if (!obj)
			continue;
		int dst_frame = frame + it.frame_offset;
		if (dst_frame < 0)
			continue;
		if (it.kind == TimelineRowInfo::R_POSITION || it.kind == TimelineRowInfo::R_ORIENTATION || it.kind == TimelineRowInfo::R_TRANSFORM || it.kind == TimelineRowInfo::R_OBJECT) {
			GeomTimeline& tl = obj->GetTimeline();
			GeomKeypoint& kp = tl.GetAddKeypoint(dst_frame);
			if (it.kind == TimelineRowInfo::R_POSITION || it.kind == TimelineRowInfo::R_TRANSFORM || it.kind == TimelineRowInfo::R_OBJECT) {
				kp.position = it.keypoint.position;
				kp.has_position = (it.kind == TimelineRowInfo::R_POSITION || it.kind == TimelineRowInfo::R_TRANSFORM || it.kind == TimelineRowInfo::R_OBJECT) && it.keypoint.has_position;
			}
			if (it.kind == TimelineRowInfo::R_ORIENTATION || it.kind == TimelineRowInfo::R_TRANSFORM || it.kind == TimelineRowInfo::R_OBJECT) {
				kp.orientation = it.keypoint.orientation;
				kp.has_orientation = (it.kind == TimelineRowInfo::R_ORIENTATION || it.kind == TimelineRowInfo::R_TRANSFORM || it.kind == TimelineRowInfo::R_OBJECT) && it.keypoint.has_orientation;
			}
		}
		if (it.kind == TimelineRowInfo::R_MESH) {
			GeomMeshAnimation& ma = obj->GetMeshAnimation();
			GeomMeshKeyframe& kf = ma.GetAddKeyframe(dst_frame);
			kf.frame_id = dst_frame;
			kf.points.SetCount(it.mesh_kf.points.GetCount());
			for (int k = 0; k < it.mesh_kf.points.GetCount(); k++)
				kf.points[k] = it.mesh_kf.points[k];
		}
		if (it.kind == TimelineRowInfo::R_2D) {
			Geom2DAnimation& a2d = obj->Get2DAnimation();
			Geom2DKeyframe& kf = a2d.GetAddKeyframe(dst_frame);
			kf.frame_id = dst_frame;
			kf.shapes.SetCount(it.a2d_kf.shapes.GetCount());
			for (int k = 0; k < it.a2d_kf.shapes.GetCount(); k++)
				copy_shape(kf.shapes[k], it.a2d_kf.shapes[k]);
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelineMoveKeyframe(int row, int from_frame, int to_frame) {
	if (!e || !e->state)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE)
		return;
	GeomObject* obj = e->state->FindObjectByKey(info.object_key);
	if (!obj)
		return;
	if (from_frame == to_frame)
		return;
	e->PushUndo("Move keyframe");
	auto move_transform = [&](bool pos, bool ori) {
		if (GeomTimeline* tl = obj->FindTimeline()) {
			int idx = tl->keypoints.Find(from_frame);
			if (idx < 0)
				return;
			GeomKeypoint src = tl->keypoints[idx];
			if (pos && !src.has_position)
				return;
			if (ori && !src.has_orientation)
				return;
			GeomKeypoint& dst = tl->GetAddKeypoint(to_frame);
			if (pos) {
				dst.position = src.position;
				dst.has_position = true;
			}
			if (ori) {
				dst.orientation = src.orientation;
				dst.has_orientation = true;
			}
			if (pos) tl->keypoints[idx].has_position = false;
			if (ori) tl->keypoints[idx].has_orientation = false;
			if (!tl->keypoints[idx].has_position && !tl->keypoints[idx].has_orientation)
				tl->keypoints.Remove(idx);
		}
	};
	if (info.kind == TimelineRowInfo::R_POSITION)
		move_transform(true, false);
	else if (info.kind == TimelineRowInfo::R_ORIENTATION)
		move_transform(false, true);
	else if (info.kind == TimelineRowInfo::R_TRANSFORM || info.kind == TimelineRowInfo::R_OBJECT)
		move_transform(true, true);
	else if (info.kind == TimelineRowInfo::R_MESH) {
		if (GeomMeshAnimation* anim = obj->FindMeshAnimation()) {
			int idx = anim->keyframes.Find(from_frame);
			if (idx < 0)
				return;
			GeomMeshKeyframe src = pick(anim->keyframes[idx]);
			anim->keyframes.Remove(idx);
			GeomMeshKeyframe& dst = anim->GetAddKeyframe(to_frame);
			dst.frame_id = to_frame;
			dst.points = pick(src.points);
		}
	}
	else if (info.kind == TimelineRowInfo::R_2D) {
		if (Geom2DAnimation* anim = obj->Find2DAnimation()) {
			int idx = anim->keyframes.Find(from_frame);
			if (idx < 0)
				return;
			Geom2DKeyframe src = pick(anim->keyframes[idx]);
			anim->keyframes.Remove(idx);
			Geom2DKeyframe& dst = anim->GetAddKeyframe(to_frame);
			dst.frame_id = to_frame;
			dst.shapes = pick(src.shapes);
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelineToggleMuteRow(int row) {
	if (!e)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE)
		return;
	e->PushUndo("Toggle row mute");
	bool v = !info.muted;
	timeline_muted.GetAdd(info.object_key) = v;
	if (GeomObject* obj = e->state ? e->state->FindObjectByKey(info.object_key) : 0)
		obj->timeline_muted = v;
	TimelineData();
}

void GeomProjectCtrl::TimelineToggleSoloRow(int row) {
	if (!e)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE)
		return;
	e->PushUndo("Toggle row solo");
	bool v = !info.solo;
	timeline_solo.GetAdd(info.object_key) = v;
	timeline_has_solo = false;
	for (int i = 0; i < timeline_solo.GetCount(); i++)
		if (timeline_solo[i])
			timeline_has_solo = true;
	if (GeomObject* obj = e->state ? e->state->FindObjectByKey(info.object_key) : 0)
		obj->timeline_solo = v;
	TimelineData();
}

void GeomProjectCtrl::TimelineSetRowColor(int row, Color c) {
	if (!e)
		return;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE)
		return;
	e->PushUndo("Set row color");
	if (IsNull(c))
		timeline_row_color.RemoveKey(info.object_key);
	else
		timeline_row_color.GetAdd(info.object_key) = c;
	TimelineData();
}

void GeomProjectCtrl::TimelineRowColorMenu(Bar& bar) {
	int row = timeline_menu_row;
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE)
		return;
	struct { const char* label; Color color; } colors[] = {
		{ "Red", Color(220, 80, 80) },
		{ "Orange", Color(220, 140, 80) },
		{ "Yellow", Color(220, 200, 80) },
		{ "Green", Color(120, 200, 120) },
		{ "Cyan", Color(80, 180, 200) },
		{ "Blue", Color(100, 140, 220) },
		{ "Purple", Color(160, 120, 220) },
		{ "Gray", Color(160, 160, 160) },
	};
	for (int i = 0; i < 8; i++)
		bar.Add(colors[i].label, [=] { TimelineSetRowColor(row, colors[i].color); }).Check(info.tag_color == colors[i].color);
	bar.Separator();
	bar.Add(t_("Custom..."), [=] {
		String text;
		if (!IsNull(info.tag_color))
			text = ColorToHtml(info.tag_color);
		EditText(text, t_("Set Row Color"), t_("Color value"));
		Color nc = ColorFromText(text);
		if (!IsNull(nc))
			TimelineSetRowColor(row, nc);
	});
	bar.Add(t_("Clear"), [=] { TimelineSetRowColor(row, Null); }).Enable(!IsNull(info.tag_color));
}

void GeomProjectCtrl::TimelineRowSelect(int row) {
	if (!e || row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind == TimelineRowInfo::R_SCENE) {
		e->timeline_scope = Edit3D::TS_SCENE;
		e->timeline_component = Edit3D::TC_NONE;
		e->timeline_transform_field = Edit3D::TT_NONE;
		e->timeline_object_key = 0;
	}
	else {
		e->timeline_scope = Edit3D::TS_OBJECT;
		e->timeline_object_key = info.object_key;
		e->timeline_component = Edit3D::TC_NONE;
		e->timeline_transform_field = Edit3D::TT_NONE;
		if (info.kind == TimelineRowInfo::R_TRANSFORM) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
		}
		else if (info.kind == TimelineRowInfo::R_POSITION) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
			e->timeline_transform_field = Edit3D::TT_POSITION;
		}
		else if (info.kind == TimelineRowInfo::R_ORIENTATION) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_TRANSFORM;
			e->timeline_transform_field = Edit3D::TT_ORIENTATION;
		}
		else if (info.kind == TimelineRowInfo::R_MESH) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_MESH;
		}
		else if (info.kind == TimelineRowInfo::R_2D) {
			e->timeline_scope = Edit3D::TS_COMPONENT;
			e->timeline_component = Edit3D::TC_2D;
		}
	}
	TimelineData();
}

void GeomProjectCtrl::TimelineRowToggle(int row) {
	if (row < 0 || row >= timeline_rows.GetCount())
		return;
	const TimelineRowInfo& info = timeline_rows[row];
	if (info.kind != TimelineRowInfo::R_OBJECT)
		return;
	int idx = timeline_expanded.Find(info.object_key);
	if (idx >= 0)
		timeline_expanded.Remove(idx);
	else
		timeline_expanded.Add(info.object_key);
	TimelineData();
}

END_UPP_NAMESPACE
