#ifndef _Rt3D_Octree_h_
#define _Rt3D_Octree_h_

// 6-directional octree map data format, ported from Tesseract-Sauerbraten
// src/engine/octa.h. Layout is kept byte-exact (word/byte/dword fields, the
// faces/edges union) because phase 013 Task 2 loads real .ogz files through
// this structure - a U++-idiomatic re-layout would break the on-disk format.

#include "Math.h"

class Rt3ElementSet
{
public:
	word texture, envmap;
	union {
		struct { byte orient, layer; };
		word reuse;
	};
	word length, minvert, maxvert;
};

enum {
	RT3_EMID_NONE = 0,
	RT3_EMID_CUSTOM,
	RT3_EMID_SKY,
	RT3_EMID_RESERVED
};

class Rt3MaterialSurface
{
public:
	Rt3IVec o;
	word csize, rsize;
	word material, skip;
	byte orient, visible;
	union {
		word envmap;
		byte ends;
	};
};

class Rt3VertInfo
{
public:
	word x, y, z, norm;
	word u = 0, v = 0; // Optional fields for older maps

	void SetXYZ(word a, word b, word c) { x = a; y = b; z = c; }
	void SetXYZ(const Rt3IVec& v) { SetXYZ(v.x, v.y, v.z); }

	// For newer maps
	void Set(word a, word b, word c, word n = 0) { SetXYZ(a, b, c); norm = n; }
	void Set(const Rt3IVec& v, word n = 0) { Set(v.x, v.y, v.z, n); }

	// For older maps
	void Set(word a, word b, word c, word s, word t, word n = 0)
	{
		SetXYZ(a, b, c);
		u = s;
		v = t;
		norm = n;
	}

	void Set(const Rt3IVec& v, word s, word t, word n = 0)
	{
		Set(v.x, v.y, v.z, s, t, n);
	}

	Rt3IVec GetXYZ() const { return Rt3IVec(x, y, z); }
};

enum { LMID_AMBIENT = 0, LMID_AMBIENT1, LMID_BRIGHT, LMID_BRIGHT1, LMID_DARK, LMID_DARK1, LMID_RESERVED };

enum {
	LAYER_TOP    = (1<<5),
	LAYER_BOTTOM = (1<<6),
	LAYER_DUP    = (1<<7),

	LAYER_BLEND  = LAYER_TOP|LAYER_BOTTOM,

	MAXFACEVERTS = 15
};

class Rt3SurfaceInfo
{
public:
	byte lmid[2];
	byte verts, numverts;

	int TotalVerts() const { return (numverts & LAYER_DUP) ? (numverts & MAXFACEVERTS) * 2 : numverts & MAXFACEVERTS; }
	bool Used() const { return lmid[0] != LMID_AMBIENT || lmid[1] != LMID_AMBIENT || numverts & ~LAYER_TOP; }
	void Clear() { lmid[0] = LMID_AMBIENT; lmid[1] = LMID_AMBIENT; numverts = (numverts & MAXFACEVERTS) | LAYER_TOP; }
	// Ported verbatim, including its own apparent self-defect: every field it
	// sets is immediately overwritten by the Clear() on the same line, so
	// the net effect equals Clear() alone. Kept faithful rather than
	// "corrected" - see Rt3D/AGENTS.md on deliberate quirks.
	void Brighten() { lmid[0] = LMID_BRIGHT; lmid[1] = LMID_AMBIENT; numverts = (numverts & MAXFACEVERTS) | LAYER_TOP; Clear(); }
};

inline Rt3SurfaceInfo Rt3TopSurface() { Rt3SurfaceInfo s; s.lmid[0] = 0; s.lmid[1] = 0; s.verts = 0; s.numverts = LAYER_TOP; return s; }
inline Rt3SurfaceInfo Rt3BottomSurface() { Rt3SurfaceInfo s; s.lmid[0] = 0; s.lmid[1] = 0; s.verts = 0; s.numverts = LAYER_BOTTOM; return s; }
inline Rt3SurfaceInfo Rt3AmbientSurface() { Rt3SurfaceInfo s; s.lmid[0] = LMID_AMBIENT; s.lmid[1] = LMID_AMBIENT; s.verts = 0; s.numverts = LAYER_TOP; return s; }
inline Rt3SurfaceInfo Rt3BrightSurface() { Rt3SurfaceInfo s; s.lmid[0] = LMID_BRIGHT; s.lmid[1] = LMID_AMBIENT; s.verts = 0; s.numverts = LAYER_TOP; return s; }
inline Rt3SurfaceInfo Rt3BrightBottomSurface() { Rt3SurfaceInfo s; s.lmid[0] = LMID_AMBIENT; s.lmid[1] = LMID_BRIGHT; s.verts = 0; s.numverts = LAYER_BOTTOM; return s; }

class Rt3TJoint
{
public:
	int next;
	word offset;
	byte edge;
};

class Rt3CubeExt
{
public:
	void* va;           // vertex array for children, or NULL (rt3d/va types come later)
	void* ents;         // map entities inside cube (octaentities, ported later)
	Rt3SurfaceInfo surfaces[6]; // render info for each surface
	int tjoints;        // linked list of t-joints
	byte maxverts;      // allocated space for verts

	Rt3VertInfo* Verts() { return (Rt3VertInfo*)(this + 1); }
};

class Rt3Cube
{
public:
	Rt3Cube* children;  // points to 8 cube structures which are its children, or NULL. -Z first, then -Y, -X
	Rt3CubeExt* ext;    // extended info for the cube
	union {
		byte edges[12];  // edges of the cube, each byte is 2 4bit values denoting the range.
		dword faces[3];  // 4 edges of each dimension together representing 2 perpendicular faces
	};
	word texture[6];    // one for each face. same order as orient.
	word material;      // empty-space material
	byte merged;        // merged faces of the cube
	union {
		byte escaped;    // mask of which children have escaped merges
		byte visible;    // visibility info for faces
	};
};

const dword RT3_F_EMPTY = 0;          // all edges in the range (0,0)
const dword RT3_F_SOLID = 0x80808080; // all edges in the range (0,8)

inline bool Rt3IsEmpty(const Rt3Cube& c) { return c.faces[0] == RT3_F_EMPTY; }
inline bool Rt3IsEntirelySolid(const Rt3Cube& c)
{
	return c.faces[0] == RT3_F_SOLID && c.faces[1] == RT3_F_SOLID && c.faces[2] == RT3_F_SOLID;
}
inline void Rt3SetFaces(Rt3Cube& c, dword face) { c.faces[0] = c.faces[1] = c.faces[2] = face; }
inline void Rt3SolidFaces(Rt3Cube& c) { Rt3SetFaces(c, RT3_F_SOLID); }
inline void Rt3EmptyFaces(Rt3Cube& c) { Rt3SetFaces(c, RT3_F_EMPTY); }

inline byte Rt3EdgeMake(byte a, byte b) { return (b << 4) | a; }
inline byte Rt3EdgeGet(byte edge, int coord) { return coord ? (edge >> 4) : (edge & 0xF); }
inline void Rt3EdgeSet(byte& edge, int coord, int val)
{
	edge = coord ? (byte)((edge & 0xF) | (val << 4)) : (byte)((edge & 0xF0) | val);
}

inline byte& Rt3CubeEdge(Rt3Cube& c, int d, int x, int y) { return c.edges[(d << 2) + (y << 1) + x]; }
inline byte Rt3CubeEdge(const Rt3Cube& c, int d, int x, int y) { return c.edges[(d << 2) + (y << 1) + x]; }

// octant bit helpers. D/R/C (dimension/row/col) come from cube.h and are
// defined in Octree.cpp; the axis order must stay exactly as upstream.
extern const int RT3_D[3], RT3_R[3], RT3_C[3];

inline int OctaDim(int d) { return 1 << d; }                  // creates mask for bit of given dimension
inline int OctaCoord(int d, int i) { return (i & OctaDim(d)) >> d; }
inline int OppositeOcta(int d, int i) { return i ^ OctaDim(RT3_D[d]); }
inline int OctaIndex(int d, int x, int y, int z)
{
	return (z << RT3_D[d]) + (y << RT3_C[d]) + (x << RT3_R[d]);
}
inline int OctaStep(int x, int y, int z, int scale)
{
	return (((z >> scale) & 1) << 2) | (((y >> scale) & 1) << 1) | ((x >> scale) & 1);
}

// Returns a bitmask of the 8 octants that can possibly collide with the
// given box (bit 0 = octant 0, etc). 0xFF = all 8 possible.
inline byte OctaBoxOverlap(const Rt3IVec& o, int size, const Rt3IVec& bbmin, const Rt3IVec& bbmax)
{
	byte p = 0xFF; // bitmask of possible collisions with octants. 0 bit = 0 octant, etc
	Rt3IVec mid = o + Rt3IVec(size);
	if(mid.z <= bbmin.z) p &= 0xF0;        // not in a -ve Z octant
	else if(mid.z >= bbmax.z) p &= 0x0F;   // not in a +ve Z octant
	if(mid.y <= bbmin.y) p &= 0xCC;        // not in a -ve Y octant
	else if(mid.y >= bbmax.y) p &= 0x33;   // etc..
	if(mid.x <= bbmin.x) p &= 0xAA;
	else if(mid.x >= bbmax.x) p &= 0x55;
	return p;
}

enum {
	O_LEFT = 0,
	O_RIGHT,
	O_BACK,
	O_FRONT,
	O_BOTTOM,
	O_TOP,
	O_ANY
};

// The `Rt3` prefix here is not decoration: X11's `X.h` defines `Opposite` as
// a macro (`#define Opposite 4`), and a GUI build - SweetSandy, which draws
// the pedestrians - includes it. The first version of this port used upstream's
// bare names and only compiled in the headless test.
inline int Rt3Dimension(int orient) { return orient >> 1; }
inline int Rt3DimCoord(int orient) { return orient & 1; }
inline int Rt3Opposite(int orient) { return orient ^ 1; }

enum {
	RT3_OCCLUDE_NOTHING = 0,
	RT3_OCCLUDE_GEOM,
	RT3_OCCLUDE_BB,
	RT3_OCCLUDE_PARENT
};

enum {
	RT3_MERGE_ORIGIN = 1<<0,
	RT3_MERGE_PART   = 1<<1,
	RT3_MERGE_USE    = 1<<2
};

// Default material/texture ids (cube.h upstream). Ports as constants here
// because the editor-only material system is out of scope for phase 013.
enum {
	RT3_MAT_AIR = 0,
	RT3_DEFAULT_GEOM = 0,
	RT3_DEFAULT_SKY = 1
};

// Cube tree allocation, from octa.cpp.
int Rt3FamilySize(const Rt3Cube& c);
Rt3Cube* Rt3NewCubes(dword face, int mat = RT3_MAT_AIR);
void Rt3FreeOcta(Rt3Cube* c);
void Rt3DiscardChildren(Rt3Cube& c, int depth = 0);
Rt3CubeExt* Rt3NewCubeExt(Rt3Cube& c, int maxverts, bool init);
void Rt3FreeCubeExt(Rt3Cube& c);

void Rt3GetCubeVector(const Rt3Cube& c, int d, int x, int y, int z, Rt3IVec& p);
void Rt3SetCubeVector(Rt3Cube& c, int d, int x, int y, int z, const Rt3IVec& p);
void Rt3GetOctantVector(const Rt3Cube& c, int i, Rt3IVec& p);
void Rt3SetOctantVector(Rt3Cube& c, int i, const Rt3IVec& p);

// octa.cpp's `genfaceverts`: the four corners of face `orient` of a cube, in
// the order upstream emits them (the winding order of the face's triangle
// fan). Corner k is the {0,1} coordinate triple from RT3_FACE_COORDS; the
// stored value of each component comes from the cube's edge nibbles.
void Rt3GenFaceVerts(const Rt3Cube& c, int orient, Rt3IVec v[4]);

// Corner coordinates per face orientation, upstream's generated
// `facecoords` table (octarender.cpp).
extern const Rt3IVec RT3_FACE_COORDS[6][4];

// octa.cpp's `validatec`: post-load repair pass. Cubes at the bottom of the
// tree that still have children are forced solid, and leaves whose edge
// nibbles are degenerate (equal or inverted across a face word) are emptied.
struct Rt3ValidateReport
{
	int forcedSolid = 0; // cubes that had children but were at size <= 1
	// Leaves the pass emptied. Note this counts every already-empty leaf too:
	// F_EMPTY is 0, so all-zero nibbles are "equal" and trip the check, and
	// emptying an already empty leaf is a no-op. On a real map this equals the
	// empty-leaf count unless a leaf was genuinely degenerate.
	int emptied = 0;
};

// worldsize is the edge length of the world in cube units; the world spans
// [0, worldsize) on every axis (shared/iengine.h's `insideworld`).
bool Rt3InsideWorld(const Rt3IVec& o, int worldsize);

void Rt3ValidateCubes(Rt3Cube* c, int size, Rt3ValidateReport& rep);

// Returns a reference to the shared immutable "empty" cube used for
// out-of-world lookups (octa.cpp's static emptycube).
const Rt3Cube& Rt3EmptyCube();

// Number of cube nodes currently allocated (upstream's allocnodes counter).
int Rt3AllocNodes();
void Rt3ResetAllocNodes();

#endif
