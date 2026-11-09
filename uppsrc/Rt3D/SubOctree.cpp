#include "Rt3D.h"

NAMESPACE_UPP

bool Rt3SubOctree::Load(const String& path, String& error)
{
	lastError.Clear();
	if(!Rt3LoadOgz(path, data, error))
	{
		lastError = error;
		return false;
	}
	return true;
}

bool Rt3SubOctree::LoadData(const String& payload, String& error)
{
	lastError.Clear();
	if(!Rt3LoadMapData(payload, data, error))
	{
		lastError = error;
		return false;
	}
	return true;
}

int Rt3SubOctree::GetWorldScale() const
{
	int worldsize = data.hdr.worldsize, worldscale = 0;
	if(worldsize <= 0) return 0;
	while((1 << worldscale) < worldsize) worldscale++;
	return worldscale;
}

Rt3Vec3 Rt3SubOctree::MinMeters() const
{
	Rt3Vec3 lo(1e30, 1e30, 1e30);
	int w = data.hdr.worldsize;
	if(w <= 0) return ToMeters(Rt3IVec(0, 0, 0));
	for(int i = 0; i < 8; i++)
	{
		// All 8 corners: the y (up) corners come from z=0 and z=worldsize.
		Rt3Vec3 p = ToMeters(Rt3IVec((i & 1) ? w : 0, (i & 2) ? w : 0, (i & 4) ? w : 0));
		lo = Rt3Vec3(min(lo.x, p.x), min(lo.y, p.y), min(lo.z, p.z));
	}
	return lo;
}

Rt3Vec3 Rt3SubOctree::MaxMeters() const
{
	Rt3Vec3 hi(-1e30, -1e30, -1e30);
	int w = data.hdr.worldsize;
	if(w <= 0) return ToMeters(Rt3IVec(0, 0, 0));
	for(int i = 0; i < 8; i++)
	{
		Rt3Vec3 p = ToMeters(Rt3IVec((i & 1) ? w : 0, (i & 2) ? w : 0, (i & 4) ? w : 0));
		hi = Rt3Vec3(max(hi.x, p.x), max(hi.y, p.y), max(hi.z, p.z));
	}
	return hi;
}

const Rt3Cube* Rt3SubOctree::LeafAt(const Rt3IVec& p) const
{
	if(!data.root || !InsideWorld(p)) return nullptr;
	int worldsize = data.hdr.worldsize;
	int scale2 = GetWorldScale() - 1;
	// worldsize is a power of two in every map format version we read, so the
	// first descent step below is well defined.
	if(scale2 < 0) return nullptr;
	const Rt3Cube* c = &data.root[OctaStep(p.x, p.y, p.z, scale2)];
	while(c->children)
	{
		scale2--;
		if(scale2 < 0) break;
		c = &c->children[OctaStep(p.x, p.y, p.z, scale2)];
	}
	return c;
}

bool Rt3SubOctree::SolidAt(const Rt3IVec& p) const
{
	const Rt3Cube* c = LeafAt(p);
	return c && Rt3IsEntirelySolid(*c);
}

int Rt3SubOctree::VisitLeaves(Rt3Cube* c, const Rt3IVec& o, int size, Rt3SubOctreeVisitor& visitor) const
{
	if(!c) return 0;
	int visited = 0;
	for(int i = 0; i < 8; i++)
	{
		Rt3Cube& cube = c[i];
		Rt3IVec co(i, o, size);
		if(cube.children) visited += VisitLeaves(cube.children, co, size >> 1, visitor);
		else
		{
			visited++;
			if(!visitor.VisitLeaf(co, size, cube)) return visited;
		}
	}
	return visited;
}

int Rt3SubOctree::VisitLeaves(Rt3SubOctreeVisitor& visitor) const
{
	if(!data.root) return 0;
	return VisitLeaves(data.root, Rt3IVec(0, 0, 0), data.hdr.worldsize >> 1, visitor);
}

void Rt3SubOctree::CollectStats(Rt3Cube* c, int size, int depth, Rt3SubOctreeStats& stats) const
{
	for(int i = 0; i < 8; i++)
	{
		Rt3Cube& cube = c[i];
		stats.nodes++;
		if(cube.children)
		{
			CollectStats(cube.children, size >> 1, depth + 1, stats);
			continue;
		}
		stats.leaves++;
		stats.maxDepth = max(stats.maxDepth, depth);
		if(stats.minSize == 0 || size < stats.minSize) stats.minSize = size;
		stats.maxSize = max(stats.maxSize, size);
		if(Rt3IsEmpty(cube)) stats.empties++;
		if(Rt3IsEntirelySolid(cube)) stats.solids++;
		if(!cube.ext) continue;
		stats.extends++;
		for(int s = 0; s < 6; s++)
		{
			const Rt3SurfaceInfo& surf = cube.ext->surfaces[s];
			if(!surf.Used()) continue;
			stats.surfaces++;
			stats.vertices += surf.TotalVerts();
		}
	}
}

Rt3SubOctreeStats Rt3SubOctree::GetStats() const
{
	Rt3SubOctreeStats stats;
	if(data.root) CollectStats(data.root, data.hdr.worldsize >> 1, 0, stats);
	return stats;
}

END_UPP_NAMESPACE
