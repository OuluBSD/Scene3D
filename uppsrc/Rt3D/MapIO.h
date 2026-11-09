#ifndef _Rt3D_MapIO_h_
#define _Rt3D_MapIO_h_

// .ogz map loading, ported from Tesseract-Sauerbraten src/engine/worldio.cpp
// (the modern `trynewmap` path only) and src/engine/world.h's header structs.
//
// The format: a gzip stream whose payload is, in order, a map header, the map
// vars, the gametype string, game-specific extras, the texture MRU, the entity
// list, the vslot list, and finally the octree. Everything is little endian.
//
// Scope (deliberate, see Rt3D/AGENTS.md):
//   - mapversion > 0 (MAPVERSION 33) only. The pre-33 "old map" reader
//     (`tryoldmap`/`loadc_old`/`convertoldsurfaces`, ~1000 lines) is not
//     ported; the loader fails with a clear message on such maps.
//   - no PVS, lightmaps, blendmap, lighting or material work: the reader stops
//     after the octree and reports what it found.
//   - entity records are read verbatim (they are game data); the per-entity
//     extra bytes are skipped using the file's own `eif` count, which is what
//     upstream's own entity-only reader (`loadents`) does.

class Rt3SubOctree;

enum {	RT3_OCTA_VERSION = 33,   // engine/world.h's OCTAVERSION
	RT3_MAP_VERSION  = 33,   // engine/world.h's MAPVERSION
	RT3_MAXENTS     = 10000, // shared/ents.h's MAXENTS
	RT3_MAXSTRLEN    = 260   // shared/tools.h's MAXSTRLEN
};

// Variable types in the map's var table (shared/ident.h's ID_*).
enum { RT3_ID_VAR = 0, RT3_ID_FVAR, RT3_ID_SVAR, RT3_ID_ENABLED, RT3_ID_CURRENT };

// engine/world.h. Byte-exact: these are read field by field, and the whole
// 40-byte block is what "headersize" counts.
class Rt3OctaHeader
{
public:
	char magic[4]; // "OCTA"
	int version;
	int headersize;
	int worldsize;
	int numents;
	int numpvs;
	int lightmaps;
	int blendmap;
	int numvars;
	int numvslots;
};

class Rt3MapHeader
{
public:
	char magic[4]; // "TMAP"
	int version;
	int headersize;
	int worldsize;
	int numents;
	int numpvs;
	int blendmap;
	int numvars;
	int numvslots;
};

// shared/ents.h's `entity` as stored on disk: 3 floats of position, five
// shorts of attributes, then type/reserved. 24 bytes.
class Rt3Entity
{
public:
	float o[3] = {0, 0, 0};
	short attr[5] = {0, 0, 0, 0, 0};
	byte type = 0;
	byte reserved = 0;

	Rt3Vec3 Pos() const { return Rt3Vec3(o[0], o[1], o[2]); }
};

// engine/texture.h's VSLOT_* bit positions - the `changed` mask of a vslot.
enum {
	RT3_VSLOT_SHPARAM = 0,
	RT3_VSLOT_SCALE,
	RT3_VSLOT_ROTATION,
	RT3_VSLOT_OFFSET,
	RT3_VSLOT_SCROLL,
	RT3_VSLOT_LAYER,
	RT3_VSLOT_ALPHA,
	RT3_VSLOT_COLOR,
	RT3_VSLOT_RESERVED, // used by RE
	RT3_VSLOT_REFRACT,
	RT3_VSLOT_DETAIL
};

struct Rt3ShaderParam : Moveable<Rt3ShaderParam>
{
	String name;
	float val[4] = {0, 0, 0, 0};
};

// A texture slot as stored in the map. Upstream's VSlot also points at a Slot
// (the texture itself) and links to the next modified vslot; neither is in
// the file, so this only carries what `loadvslot` reads.
struct Rt3VSlot : Moveable<Rt3VSlot>
{
	int index = 0;
	int changed = 0;
	int prev = -1;
	Vector<Rt3ShaderParam> params;
	float scale = 0;
	int rotation = 0;
	int offset[2] = {0, 0};
	float scroll[2] = {0, 0};
	int layer = 0;
	float alphafront = 1, alphaback = 1;
	float colorscale[3] = {1, 1, 1};
	float refractscale = 0;
	float refractcolor[3] = {0, 0, 0};
	int detail = 0;
};

struct Rt3MapVar : Moveable<Rt3MapVar>
{
	int type = RT3_ID_VAR;
	String name;
	double num = 0;   // ID_VAR and ID_FVAR
	String str;       // ID_SVAR
};

struct Rt3MapData
{
	Rt3OctaHeader hdr;
	String gametype;
	int extraEntityInfo = 0;              // upstream's `eif`
	Vector<Rt3MapVar> vars;
	Vector<Rt3Entity> ents;
	Vector<int> texmru;
	Vector<Rt3VSlot> vslots;
	Vector<byte> gameExtras;               // upstream's `extras`
	Rt3ValidateReport validate;
	bool failed = false;                   // upstream's "garbage in map" flag

	Rt3Cube* root = nullptr;               // the octree, owned by this struct
	int octreeOffset = -1;                // byte offset of the first cube record, for diagnostics

	Rt3MapData() {}
	~Rt3MapData() { FreeTree(); }
	void FreeTree();

	void Clear()
	{
		FreeTree();
		hdr = Rt3OctaHeader();
		gametype.Clear();
		extraEntityInfo = 0;
		vars.Clear();
		ents.Clear();
		texmru.Clear();
		vslots.Clear();
		gameExtras.Clear();
		validate = Rt3ValidateReport();
		failed = false;
		octreeOffset = -1;
	}
};

// Reads `path` (a gzipped .ogz map) into `out`, including the octree. On
// failure returns false, sets `error` and leaves `out` cleared.
bool Rt3LoadOgz(const String& path, Rt3MapData& out, String& error);

// Same, for an already decompressed map payload.
bool Rt3LoadMapData(const String& data, Rt3MapData& out, String& error);

#endif
