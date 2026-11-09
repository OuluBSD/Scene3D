#include "Rt3D.h"

NAMESPACE_UPP

// ---------------------------------------------------------------------------
// Little-endian reader over the decompressed map payload.
//
// Upstream reads through `stream` with `getlil<T>()`; this is the same thing
// without a FILE*. Assembling the bytes by hand keeps the port endian-correct
// on a big-endian host, where upstream's `lilswap` becomes a real swap.
// ---------------------------------------------------------------------------

class Rt3Reader
{
	const byte* p;
	int n;
	int pos = 0;
	String error;

public:
	explicit Rt3Reader(const String& data) : p((const byte*)(~data)), n(data.GetCount()) {}

	int GetCount() const { return n; }
	int Position() const { return pos; }
	int Remaining() const { return n - pos; }
	const String& Error() const { return error; }
	bool Failed() const { return !error.IsEmpty(); }

	void Fail(const String& e)
	{
		if(error.IsEmpty()) error = e;
	}

	void Skip(int64 count)
	{
		int64 to = int64(pos) + count;
		if(count < 0 || to > n)
		{
			Fail(Format("skip of %lld bytes at offset %d runs past the end of the %d byte map payload",
				(long long)count, pos, n));
			pos = n;
		}
		else pos = int(to);
	}

	byte U8()
	{
		if(pos >= n)
		{
			Fail(Format("unexpected end of the map payload at offset %d", pos));
			return 0;
		}
		return p[pos++];
	}

	word U16()
	{
		byte lo = U8(), hi = U8();
		return word(word(lo) | (word(hi) << 8));
	}

	short I16() { return short(U16()); }

	dword U32()
	{
		dword lo = U16(), hi = U16();
		return dword(lo | (hi << 16));
	}

	int I32() { return int(U32()); }

	float F32()
	{
		byte b[4];
		for(int i = 0; i < 4; i++) b[i] = U8();
		float f = 0;
		memcpy(&f, b, 4);
		return f;
	}

	void Read(void* dest, int count)
	{
		if(count < 0 || pos + count > n)
		{
			Fail(Format("read of %d bytes at offset %d runs past the end of the %d byte map payload", count, pos, n));
			memset(dest, 0, count > 0 ? count : 0);
			pos = n;
			return;
		}
		memcpy(dest, p + pos, count);
		pos += count;
	}

	// Reads a counted string, keeping at most `limit` characters but always
	// consuming exactly `count` bytes - upstream reads min(count, limit) and
	// then seeks whatever is left.
	String ReadString(int count, int limit)
	{
		if(count <= 0) return String();
		if(pos + count > n)
		{
			Fail(Format("string of %d bytes at offset %d runs past the end of the %d byte map payload", count, pos, n));
			pos = n;
			return String();
		}
		String s((const char*)p + pos, min(count, limit));
		pos += count;
		return s;
	}

	// A NUL-terminated string counted by a leading length byte, as the
	// gametype is stored: upstream reads len+1 bytes (the NUL included), so
	// that is what this consumes too.
	String ReadCString(int len)
	{
		if(len < 0) return String();
		String s = ReadString(len, RT3_MAXSTRLEN - 1);
		U8(); // trailing NUL
		return s;
	}

	Vector<byte> ReadBytes(int count)
	{
		if(count <= 0) return Vector<byte>();
		if(pos + count > n)
		{
			Fail(Format("block of %d bytes at offset %d runs past the end of the %d byte map payload", count, pos, n));
			pos = n;
			return Vector<byte>();
		}
		Vector<byte> v(count);
		memcpy(v.Begin(), p + pos, count);
		pos += count;
		return v;
	}
};

void Rt3MapData::FreeTree()
{
	if(root)
	{
		Rt3FreeOcta(root);
		root = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Octree (worldio.cpp's `loadc`/`loadchildren`, the non-oldmap path)
// ---------------------------------------------------------------------------

// `mapversion <= 0` is not dead code upstream: the loader rewrites an "OCTA"
// header to "TMAP" version 0 (loadmapheader), so every map shipped in
// packages/map today loads through these branches. Only the pre-33 `loadc_old`
// reader (a different cube encoding, with the old texture/normal/merge fixups)
// is out of scope.
static void Rt3LoadCube(Rt3Reader& r, Rt3Cube& c, const Rt3IVec& co, int size, int mapversion, Rt3MapData& d);

static Rt3Cube* Rt3LoadChildren(Rt3Reader& r, const Rt3IVec& co, int size, int mapversion, Rt3MapData& d)
{
	Rt3Cube* c = Rt3NewCubes(RT3_F_SOLID);
	for(int i = 0; i < 8; i++)
	{
		Rt3LoadCube(r, c[i], Rt3IVec(i, co, size), size, mapversion, d);
		if(d.failed || r.Failed()) break;
	}
	return c;
}

static void Rt3LoadCube(Rt3Reader& r, Rt3Cube& c, const Rt3IVec& co, int size, int mapversion, Rt3MapData& d)
{
	int octsav = r.U8();
	switch(octsav & 0x7)
	{
	case 0: // OCTSAV_CHILDREN
		c.children = Rt3LoadChildren(r, co, size >> 1, mapversion, d);
		return;
	case 1: // OCTSAV_EMPTY
		Rt3EmptyFaces(c);
		break;
	case 2: // OCTSAV_SOLID
		Rt3SolidFaces(c);
		break;
	case 3: // OCTSAV_NORMAL
		r.Read(c.edges, 12);
		break;
	default:
		d.failed = true;
		return;
	}

	for(int i = 0; i < 6; i++) c.texture[i] = r.U16();
	if(octsav & 0x40) c.material = r.U16();
	if(octsav & 0x80) c.merged = r.U8();
	if(!(octsav & 0x20)) return;

	int surfmask = r.U8();
	int totalverts = int(r.U8()); // upstream: max(f->getchar(), 0)
	Rt3NewCubeExt(c, totalverts, false);
	memset(c.ext->surfaces, 0, sizeof(c.ext->surfaces));
	if(totalverts > 0) memset(c.ext->Verts(), 0, totalverts * sizeof(Rt3VertInfo));
	int offset = 0;
	for(int i = 0; i < 6; i++)
	{
		if(!(surfmask & (1 << i))) continue;

		Rt3SurfaceInfo& surf = c.ext->surfaces[i];
		if(mapversion <= 0)
		{
			// worldio.cpp's polysurfacecompat: {lmid[2], verts, numverts},
			// which is the same 4 bytes as the current surfaceinfo. Ported
			// field by field as upstream does, to keep the two formats
			// visibly separate.
			byte compat[4];
			r.Read(compat, 4);
			surf.lmid[0] = compat[0];
			surf.lmid[1] = compat[1];
			surf.verts = compat[2];
			surf.numverts = compat[3];
		}
		else r.Read(&surf, sizeof(surf));

		int vertmask = surf.verts, numverts = surf.TotalVerts();
		if(!numverts)
		{
			surf.verts = 0;
			continue;
		}
		surf.verts = offset;
		Rt3VertInfo* verts = c.ext->Verts() + offset;
		offset += numverts;
		Rt3IVec v[4], n, vo = (co & 0xFFF) << 3; // ivec(co).mask(0xFFF).shl(3)
		int layerverts = surf.numverts & MAXFACEVERTS, dim = Rt3Dimension(i), vc = RT3_C[dim], vr = RT3_R[dim], bias = 0;
		Rt3GenFaceVerts(c, i, v);
		bool hasxyz = (vertmask & 0x04) != 0;
		bool hasuv = mapversion <= 0 && (vertmask & 0x40) != 0;
		bool hasnorm = (vertmask & 0x80) != 0;
		if(hasxyz)
		{
			Rt3IVec e1, e2, e3;
			n = Rt3Cross(e1 = v[1] - v[0], e2 = v[2] - v[0]);
			if(n.IsZero()) n = Rt3Cross(e2, e3 = v[3] - v[0]);
			bias = -n.Dot(v[0] * size + vo);
		}
		else
		{
			int vis = layerverts < 4 ? (vertmask & 0x02 ? 2 : 1) : 3, order = vertmask & 0x01 ? 1 : 0, k = 0;
			verts[k++].SetXYZ((v[order] * size) + vo);
			if(vis & 1) verts[k++].SetXYZ((v[order + 1] * size) + vo);
			verts[k++].SetXYZ((v[order + 2] * size) + vo);
			if(vis & 2) verts[k++].SetXYZ((v[(order + 3) & 3] * size) + vo);
		}
		if(layerverts == 4)
		{
			if(hasxyz && (vertmask & 0x01))
			{
				// A planar quad is stored as its two bounding corner pairs; the
				// remaining axis comes out of the face's plane equation.
				int c1 = r.U16(), r1 = r.U16(), c2 = r.U16(), r2 = r.U16();
				Rt3IVec xyz;
				xyz[vc] = c1; xyz[vr] = r1; xyz[dim] = n[dim] ? -(bias + n[vc] * xyz[vc] + n[vr] * xyz[vr]) / n[dim] : vo[dim];
				verts[0].SetXYZ(xyz);
				xyz[vc] = c1; xyz[vr] = r2; xyz[dim] = n[dim] ? -(bias + n[vc] * xyz[vc] + n[vr] * xyz[vr]) / n[dim] : vo[dim];
				verts[1].SetXYZ(xyz);
				xyz[vc] = c2; xyz[vr] = r2; xyz[dim] = n[dim] ? -(bias + n[vc] * xyz[vc] + n[vr] * xyz[vr]) / n[dim] : vo[dim];
				verts[2].SetXYZ(xyz);
				xyz[vc] = c2; xyz[vr] = r1; xyz[dim] = n[dim] ? -(bias + n[vc] * xyz[vc] + n[vr] * xyz[vr]) / n[dim] : vo[dim];
				verts[3].SetXYZ(xyz);
				hasxyz = false;
			}
			if(hasuv && (vertmask & 0x02))
			{
				// All four vertices share one set of texture coordinates, stored
				// once. A duplicated layer stores a second set.
				for(int k = 0; k < 4; k++) r.U16();
				if(surf.numverts & LAYER_DUP) for(int k = 0; k < 4; k++) r.U16();
				hasuv = false;
			}
		}
		if(hasnorm && (vertmask & 0x08))
		{
			word norm = r.U16();
			for(int k = 0; k < layerverts; k++) verts[k].norm = norm;
			hasnorm = false;
		}
		if(hasxyz || hasuv || hasnorm)
		{
			for(int k = 0; k < layerverts; k++)
			{
				Rt3VertInfo& vv = verts[k];
				if(hasxyz)
				{
					Rt3IVec xyz;
					xyz[vc] = r.U16();
					xyz[vr] = r.U16();
					xyz[dim] = n[dim] ? -(bias + n[vc] * xyz[vc] + n[vr] * xyz[vr]) / n[dim] : vo[dim];
					vv.SetXYZ(xyz);
				}
				// Old-format (mapversion <= 0) maps carry explicit per-vertex
				// texture coordinates here; they are not kept, but the two
				// shorts must still be consumed.
				if(hasuv)
				{
					r.U16();
					r.U16();
				}
				if(hasnorm) vv.norm = r.U16();
			}
		}
		if(hasuv && (surf.numverts & LAYER_DUP))
		{
			for(int k = 0; k < layerverts; k++)
			{
				r.U16();
				r.U16();
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Vslots (worldio.cpp's `loadvslot`/`loadvslots`)
// ---------------------------------------------------------------------------

static void Rt3LoadVSlot(Rt3Reader& r, Rt3VSlot& vs, int changed)
{
	vs.changed = changed;
	if(vs.changed & (1 << RT3_VSLOT_SHPARAM))
	{
		int numparams = r.U16();
		for(int i = 0; i < numparams; i++)
		{
			Rt3ShaderParam& p = vs.params.Add();
			int nlen = r.U16();
			p.name = r.ReadString(nlen, RT3_MAXSTRLEN - 1);
			for(int k = 0; k < 4; k++) p.val[k] = r.F32();
		}
	}
	if(vs.changed & (1 << RT3_VSLOT_SCALE)) vs.scale = r.F32();
	if(vs.changed & (1 << RT3_VSLOT_ROTATION)) vs.rotation = clamp(r.I32(), 0, 7);
	if(vs.changed & (1 << RT3_VSLOT_OFFSET))
		for(int k = 0; k < 2; k++) vs.offset[k] = r.I32();
	if(vs.changed & (1 << RT3_VSLOT_SCROLL))
		for(int k = 0; k < 2; k++) vs.scroll[k] = r.F32();
	if(vs.changed & (1 << RT3_VSLOT_LAYER)) vs.layer = r.I32();
	if(vs.changed & (1 << RT3_VSLOT_ALPHA))
	{
		vs.alphafront = r.F32();
		vs.alphaback = r.F32();
	}
	if(vs.changed & (1 << RT3_VSLOT_COLOR))
		for(int k = 0; k < 3; k++) vs.colorscale[k] = r.F32();
	if(vs.changed & (1 << RT3_VSLOT_REFRACT))
	{
		vs.refractscale = r.F32();
		for(int k = 0; k < 3; k++) vs.refractcolor[k] = r.F32();
	}
	if(vs.changed & (1 << RT3_VSLOT_DETAIL)) vs.detail = r.I32();
}

static void Rt3LoadVSlots(Rt3Reader& r, int numvslots, Vector<Rt3VSlot>& vslots)
{
	Vector<int> prev;
	int remaining = numvslots;
	while(remaining > 0)
	{
		int changed = r.I32();
		if(changed < 0)
		{
			// A run of unchanged slots: only their existence is stored.
			for(int i = 0; i < -changed; i++) vslots.Add();
			remaining += changed;
		}
		else
		{
			int previndex = r.I32();
			Rt3VSlot& vs = vslots.Add();
			vs.index = vslots.GetCount() - 1;
			vs.prev = previndex;
			prev.Add(previndex);
			Rt3LoadVSlot(r, vs, changed);
			remaining--;
		}
		if(r.Failed()) break;
	}
}

// ---------------------------------------------------------------------------
// Header (worldio.cpp's `loadmapheader`)
// ---------------------------------------------------------------------------

// Returns the map's version in `mapversion` and fills `ohdr` (an "OCTA"
// header, or the "TMAP" one normalised into the same fields, exactly as
// upstream's loadmapheader does).
static bool Rt3LoadMapHeader(Rt3Reader& r, Rt3OctaHeader& ohdr, int& mapversion, String& error)
{
	memset(&ohdr, 0, sizeof(ohdr));
	char magic[4];
	for(int i = 0; i < 4; i++) magic[i] = char(r.U8());
	int version = r.I32();
	int headersize = r.I32();
	if(r.Failed())
	{
		error = "map payload is too short to hold a header";
		return false;
	}
	memcpy(ohdr.magic, "OCTA", 4);
	ohdr.version = version;
	ohdr.headersize = headersize;
	if(memcmp(magic, "TMAP", 4) == 0)
	{
		if(version > RT3_MAP_VERSION)
		{
			error = Format("map version %d is newer than the supported MAPVERSION %d", version, int(RT3_MAP_VERSION));
			return false;
		}
		ohdr.worldsize = r.I32();
		ohdr.numents = r.I32();
		ohdr.numpvs = r.I32();
		ohdr.blendmap = r.I32();
		ohdr.numvars = r.I32();
		ohdr.numvslots = r.I32();
		// A TMAP header has no lightmap count: upstream leaves it zero.
		mapversion = version;
	}
	else if(memcmp(magic, "OCTA", 4) == 0)
	{
		if(version != RT3_OCTA_VERSION)
		{
			error = Format("OCTA map version %d is not the supported OCTAVERSION %d", version, int(RT3_OCTA_VERSION));
			return false;
		}
		ohdr.worldsize = r.I32();
		ohdr.numents = r.I32();
		ohdr.numpvs = r.I32();
		ohdr.lightmaps = r.I32();
		ohdr.blendmap = r.I32();
		ohdr.numvars = r.I32();
		ohdr.numvslots = r.I32();
		// Upstream rewrites the header to TMAP version 0 here, which is what
		// makes the mapversion <= 0 branches in loadc live. Keep the same
		// version the loader will use.
		mapversion = 0;
	}
	else
	{
		error = Format("map magic is '%c%c%c%c', expected TMAP or OCTA", magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	if(r.Failed())
	{
		error = "map header is truncated";
		return false;
	}
	if(ohdr.worldsize <= 0 || ohdr.numents < 0)
	{
		error = Format("map header has a non-positive worldsize (%d) or negative entity count (%d)", ohdr.worldsize, ohdr.numents);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Top level (worldio.cpp's `trynewmap`)
// ---------------------------------------------------------------------------

static bool Rt3LoadMap(const String& data, Rt3MapData& out, String& error)
{
	out.Clear();
	Rt3Reader r(data);
	int mapversion = 0;
	if(!Rt3LoadMapHeader(r, out.hdr, mapversion, error)) return false;

	// vars
	for(int i = 0; i < out.hdr.numvars; i++)
	{
		Rt3MapVar& var = out.vars.Add();
		var.type = r.U8();
		int ilen = r.U16();
		var.name = r.ReadString(ilen, RT3_MAXSTRLEN - 1);
		switch(var.type)
		{
		case RT3_ID_VAR: var.num = r.I32(); break;
		case RT3_ID_FVAR: var.num = r.F32(); break;
		case RT3_ID_SVAR:
		{
			int slen = r.U16();
			var.str = r.ReadString(slen, RT3_MAXSTRLEN - 1);
			break;
		}
		default: continue; // upstream: no ident, so no value read either
		}
		if(r.Failed()) break;
	}

	// gametype: a length byte, then that many characters and a NUL
	out.gametype = r.ReadCString(r.U8());

	// game data + per-entity extra info size
	out.extraEntityInfo = r.U16();
	int extrasize = r.U16();
	out.gameExtras = r.ReadBytes(extrasize);

	// texture MRU
	int nummru = r.U16();
	for(int i = 0; i < nummru; i++) out.texmru.Add(r.U16());

	// entities. `eif` is the extra bytes per entity *as stored in the file*,
	// which is what upstream's own entity-only reader (loadents) skips; the
	// game-specific `einfosize` in trynewmap depends on a loaded game, which
	// we are not.
	int numents = out.hdr.numents < int(RT3_MAXENTS) ? out.hdr.numents : int(RT3_MAXENTS);
	for(int i = 0; i < numents; i++)
	{
		Rt3Entity& e = out.ents.Add();
		for(int k = 0; k < 3; k++) e.o[k] = r.F32();
		for(int k = 0; k < 5; k++) e.attr[k] = r.I16();
		e.type = r.U8();
		e.reserved = r.U8();
		if(out.extraEntityInfo > 0) r.Skip(out.extraEntityInfo);
	}
	if(out.hdr.numents > RT3_MAXENTS)
		r.Skip(int64(out.hdr.numents - RT3_MAXENTS) * out.extraEntityInfo);

	if(r.Failed())
	{
		error = "map payload ends inside its entity list: " + r.Error();
		return false;
	}

	// texture slots
	Rt3LoadVSlots(r, out.hdr.numvslots, out.vslots);
	if(r.Failed())
	{
		error = "map payload ends inside its vslot list: " + r.Error();
		return false;
	}

	// the octree
	int octreeStart = r.Position();
	out.octreeOffset = octreeStart;
	out.root = Rt3LoadChildren(r, Rt3IVec(0, 0, 0), out.hdr.worldsize >> 1, mapversion, out);
	if(out.failed || r.Failed())
	{
		// Drop the half-built tree: a failed load must leave nothing behind.
		out.FreeTree();
		out.octreeOffset = -1;
		error = Format("garbage in map at byte %d of the payload (octree started at %d): %s",
			r.Position(), octreeStart, out.failed ? "unknown cube record" : ~r.Error());
		return false;
	}

	// post-load repair pass. Upstream continues with lightmaps/PVS/blendmap
	// after this; we stop here - see the scope note in MapIO.h.
	Rt3ValidateCubes(out.root, out.hdr.worldsize >> 1, out.validate);
	return true;
}

bool Rt3LoadMapData(const String& data, Rt3MapData& out, String& error)
{
	return Rt3LoadMap(data, out, error);
}

bool Rt3LoadOgz(const String& path, Rt3MapData& out, String& error)
{
	out.Clear();
	String raw = LoadFile(path);
	if(raw.IsEmpty())
	{
		error = "could not read map file '" + path + "'";
		return false;
	}
	String data = GZDecompress(raw);
	if(data.IsEmpty())
	{
		error = Format("'%s' (%lld bytes) did not decompress as a gzip stream", path, (long long)raw.GetCount());
		return false;
	}
	return Rt3LoadMap(data, out, error);
}

END_UPP_NAMESPACE
