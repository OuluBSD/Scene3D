#ifndef _Rt3D_SubOctree_h_
#define _Rt3D_SubOctree_h_

// A loaded Tesseract map (see MapIO.h) placed in SweetSandy world space.
//
// This is the "sub-octree" of the phase plan: a self-contained octree map
// that can be dropped anywhere in the driving demo's world, with its own
// origin, yaw and scale. It owns no renderer and no physics - it exposes
// geometry queries and a leaf traversal that the later renderer/collision
// tasks consume.
//
// Coordinates, spelled out once:
//   - Tesseract map space: integer "cube units", the world spans
//     [0, worldsize) on each axis, +x east, +y north, +z up, 1 unit == 1
//     Sauerbraten game unit (see physics.cpp's octacollide, which passes
//     game coordinates straight through as octree coordinates).
//   - `vertinfo` coordinates are in 1/8 of a cube unit (upstream's
//     `ivec(co).mask(0xFFF).shl(3)` vertex origin), so multiply by 8 and add
//     the cube's own origin to land in cube units.
//   - SweetSandy world space: meters, +x east, +y up, +z south (see
//     game/SweetSandy/Terrain.cpp's heightfield grid). `ToMeters` does the
//     yaw + axis swap + scale.

struct Rt3SubOctreeStats
{
	int nodes = 0;          // cube structs in the tree
	int leaves = 0;         // cubes without children
	int empties = 0;        // leaves whose faces are all F_EMPTY
	int solids = 0;         // leaves whose faces are all F_SOLID
	int extends = 0;        // leaves carrying a cubeext
	int surfaces = 0;       // used surfaces across all cubeexts
	int vertices = 0;       // total vertices across all cubeexts
	int minSize = 0;        // smallest leaf edge, cube units
	int maxSize = 0;        // largest leaf edge, cube units
	int maxDepth = 0;
};

class Rt3SubOctreeVisitor
{
public:
	virtual ~Rt3SubOctreeVisitor() {}
	// Return false to stop the traversal early.
	virtual bool VisitLeaf(const Rt3IVec& origin, int size, const Rt3Cube& cube) = 0;
};

class Rt3SubOctree
{
public:
	Rt3SubOctree() {}
	~Rt3SubOctree() {}

	// ---- loading -----------------------------------------------------------

	// Loads a gzipped .ogz map. On failure returns false, sets `error` and
	// leaves the sub-octree cleared.
	bool Load(const String& path, String& error);
	// Same, for an already decompressed map payload.
	bool LoadData(const String& data, String& error);
	void Clear() { data.Clear(); }
	bool IsLoaded() const { return data.root != nullptr; }
	const String& GetLastError() const { return lastError; }

	const Rt3MapData& Data() const { return data; }

	// ---- placement ---------------------------------------------------------

	// Tesseract units to meters. The default is 0.25 m per unit, which makes
	// a 4096-unit map (Tesseract's largest) 1024 m across - a city sized for
	// a driving demo.
	static constexpr double DEFAULT_METERS_PER_UNIT = 0.25;

	void SetOrigin(const Rt3Vec3& meters) { origin = meters; }
	void SetYawDegrees(double degrees) { yaw = degrees * M_PI / 180.0; }
	void SetScale(double metersPerUnit) { scale = metersPerUnit; }
	Rt3Vec3 GetOrigin() const { return origin; }
	double GetYawDegrees() const { return yaw * 180.0 / M_PI; }
	double GetYawRadians() const { return yaw; }
	double GetScale() const { return scale; }

	// Point transform. Yaw is applied about the up axis first; positive yaw
	// rotates the map clockwise seen from above.
	Rt3Vec3 ToMeters(const Rt3IVec& units) const
	{
		double c = cos(yaw), s = sin(yaw);
		double east = units.x * c + units.y * s;
		double north = -units.x * s + units.y * c;
		return Rt3Vec3(origin.x + east * scale, origin.y + units.z * scale, origin.z - north * scale);
	}

	// Direction transform, for normals: same rotation, no origin/scale.
	Rt3Vec3 DirectionToMeters(const Rt3IVec& units) const
	{
		double c = cos(yaw), s = sin(yaw);
		double east = units.x * c + units.y * s;
		double north = -units.x * s + units.y * c;
		return Rt3Vec3(east, units.z, -north);
	}

	// The world box [0, worldsize] in Tesseract units, as a meters AABB that
	// encloses the rotated map.
	Rt3Vec3 MinMeters() const;
	Rt3Vec3 MaxMeters() const;

	// ---- map facts ---------------------------------------------------------

	int GetWorldSize() const { return data.hdr.worldsize; }
	int GetOctreeSize() const { return data.hdr.worldsize >> 1; }
	int GetWorldScale() const;
	String GetGametype() const { return data.gametype; }
	const Rt3OctaHeader& GetHeader() const { return data.hdr; }
	int GetEntityCount() const { return data.ents.GetCount(); }
	int GetVSlotCount() const { return data.vslots.GetCount(); }

	// ---- geometry queries --------------------------------------------------

	bool InsideWorld(const Rt3IVec& p) const { return Rt3InsideWorld(p, data.hdr.worldsize); }

	// The leaf cube containing p, or NULL if p is outside the world. Mirrors
	// `lookupcube`'s descent, minus its on-demand subdivision: a leaf is
	// returned as-is even if a smaller cube would be needed.
	const Rt3Cube* LeafAt(const Rt3IVec& p) const;

	// True if p is inside a fully solid leaf. A leaf with an octant-local
	// clip would need ray casting (not ported), so anything that is not
	// entirely solid and not entirely empty counts as not solid here.
	bool SolidAt(const Rt3IVec& p) const;

	// Visits every leaf, root's children first. Returns the number visited.
	int VisitLeaves(Rt3SubOctreeVisitor& visitor) const;

	Rt3SubOctreeStats GetStats() const;

private:
	Rt3MapData data;
	String lastError;
	Rt3Vec3 origin;
	double yaw = 0;
	double scale = DEFAULT_METERS_PER_UNIT;

	int VisitLeaves(Rt3Cube* c, const Rt3IVec& o, int size, Rt3SubOctreeVisitor& visitor) const;
	void CollectStats(Rt3Cube* c, int size, int depth, Rt3SubOctreeStats& stats) const;

	// Non-copyable: the octree is a raw heap tree owned by Rt3MapData.
	Rt3SubOctree(const Rt3SubOctree&);
	Rt3SubOctree& operator=(const Rt3SubOctree&);
};

#endif
