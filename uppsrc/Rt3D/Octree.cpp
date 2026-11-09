#include "Rt3D.h"

NAMESPACE_UPP

const int RT3_D[3] = {0, 1, 2}; // depth
const int RT3_R[3] = {1, 2, 0}; // row
const int RT3_C[3] = {2, 0, 1}; // col

static int allocnodes = 0;

int Rt3AllocNodes() { return allocnodes; }
void Rt3ResetAllocNodes() { allocnodes = 0; }

const Rt3Cube& Rt3EmptyCube()
{
	static Rt3Cube emptycube;
	static bool inited = false;
	if(!inited)
	{
		inited = true;
		emptycube.children = nullptr;
		emptycube.ext = nullptr;
		emptycube.visible = 0;
		emptycube.merged = 0;
		emptycube.material = RT3_MAT_AIR;
		Rt3SetFaces(emptycube, RT3_F_EMPTY);
		for(int i = 0; i < 6; i++) emptycube.texture[i] = RT3_DEFAULT_SKY;
	}
	return emptycube;
}

int Rt3FamilySize(const Rt3Cube& c)
{
	int size = 1;
	if(c.children)
		for(int i = 0; i < 8; i++) size += Rt3FamilySize(c.children[i]);
	return size;
}

Rt3Cube* Rt3NewCubes(dword face, int mat)
{
	Rt3Cube* c = new Rt3Cube[8];
	for(int i = 0; i < 8; i++)
	{
		c[i].children = nullptr;
		c[i].ext = nullptr;
		c[i].visible = 0;
		c[i].merged = 0;
		Rt3SetFaces(c[i], face);
		for(int l = 0; l < 6; l++) c[i].texture[l] = RT3_DEFAULT_GEOM;
		c[i].material = word(mat);
	}
	allocnodes++;
	return c;
}

void Rt3FreeOcta(Rt3Cube* c)
{
	if(!c) return;
	// Upstream loops discardchildren(c[i]) here, not delete[] c[i].children:
	// the discard recurses, so the whole subtree (and every cubeext in it) is
	// released. Freeing the direct child arrays only would leak all deeper
	// levels.
	for(int i = 0; i < 8; i++) Rt3DiscardChildren(c[i]);
	delete[] c;
	allocnodes--;
}

void Rt3FreeCubeExt(Rt3Cube& c)
{
	if(c.ext)
	{
		delete[] (byte*)c.ext;
		c.ext = nullptr;
	}
}

void Rt3DiscardChildren(Rt3Cube& c, int depth)
{
	c.material = RT3_MAT_AIR;
	c.visible = 0;
	c.merged = 0;
	if(c.ext)
	{
		c.ext->va = nullptr;
		c.ext->tjoints = -1;
		Rt3FreeCubeExt(c);
	}
	if(c.children)
	{
		dword filled = RT3_F_EMPTY;
		for(int i = 0; i < 8; i++)
		{
			Rt3DiscardChildren(c.children[i], depth + 1);
			filled |= c.children[i].faces[0];
		}
		// Upstream's fixtex path (getmippedtexture, added in a later task)
		// is not ported here: it needs the mip/tex cache, not just the
		// octree. The depth-based face fill-up is kept, it is pure octree.
		if(depth > 0 && filled != RT3_F_EMPTY) c.faces[0] = RT3_F_SOLID;
		delete[] c.children;
		c.children = nullptr;
		allocnodes--;
	}
}

static void setcubeext(Rt3Cube& c, Rt3CubeExt* ext)
{
	Rt3CubeExt* old = c.ext;
	if(old == ext) return;
	c.ext = ext;
	if(old) delete[] (byte*)old;
}

Rt3CubeExt* Rt3NewCubeExt(Rt3Cube& c, int maxverts, bool init)
{
	if(c.ext && c.ext->maxverts >= maxverts) return c.ext;
	Rt3CubeExt* ext = (Rt3CubeExt*)new byte[sizeof(Rt3CubeExt) + maxverts * sizeof(Rt3VertInfo)];
	// Ported from growcubeext(): va/ents/tjoints always carry over, the
	// rest is not initialized here.
	if(c.ext)
	{
		ext->va = c.ext->va;
		ext->ents = c.ext->ents;
		ext->tjoints = c.ext->tjoints;
	}
	else
	{
		ext->va = nullptr;
		ext->ents = nullptr;
		ext->tjoints = -1;
	}
	ext->maxverts = byte(maxverts);
	if(init)
	{
		if(c.ext)
		{
			memcpy(ext->surfaces, c.ext->surfaces, sizeof(ext->surfaces));
			memcpy(ext->Verts(), c.ext->Verts(), c.ext->maxverts * sizeof(Rt3VertInfo));
		}
		else memset(ext->surfaces, 0, sizeof(ext->surfaces));
	}
	setcubeext(c, ext);
	return ext;
}

void Rt3GetCubeVector(const Rt3Cube& c, int d, int x, int y, int z, Rt3IVec& p)
{
	Rt3IVec4 v(d, x, y, z);
	for(int i = 0; i < 3; i++)
		p[i] = Rt3EdgeGet(Rt3CubeEdge(c, i, v[RT3_R[i]], v[RT3_C[i]]), v[RT3_D[i]]);
}

void Rt3SetCubeVector(Rt3Cube& c, int d, int x, int y, int z, const Rt3IVec& p)
{
	Rt3IVec4 v(d, x, y, z);
	for(int i = 0; i < 3; i++)
		Rt3EdgeSet(Rt3CubeEdge(c, i, v[RT3_R[i]], v[RT3_C[i]]), v[RT3_D[i]], p[i]);
}

void Rt3GetOctantVector(const Rt3Cube& c, int i, Rt3IVec& p)
{
	p.x = Rt3EdgeGet(Rt3CubeEdge(c, 0, (i >> RT3_R[0]) & 1, (i >> RT3_C[0]) & 1), (i >> RT3_D[0]) & 1);
	p.y = Rt3EdgeGet(Rt3CubeEdge(c, 1, (i >> RT3_R[1]) & 1, (i >> RT3_C[1]) & 1), (i >> RT3_D[1]) & 1);
	p.z = Rt3EdgeGet(Rt3CubeEdge(c, 2, (i >> RT3_R[2]) & 1, (i >> RT3_C[2]) & 1), (i >> RT3_D[2]) & 1);
}

void Rt3SetOctantVector(Rt3Cube& c, int i, const Rt3IVec& p)
{
	Rt3EdgeSet(Rt3CubeEdge(c, 0, (i >> RT3_R[0]) & 1, (i >> RT3_C[0]) & 1), (i >> RT3_D[0]) & 1, p.x);
	Rt3EdgeSet(Rt3CubeEdge(c, 1, (i >> RT3_R[1]) & 1, (i >> RT3_C[1]) & 1), (i >> RT3_D[1]) & 1, p.y);
	Rt3EdgeSet(Rt3CubeEdge(c, 2, (i >> RT3_R[2]) & 1, (i >> RT3_C[2]) & 1), (i >> RT3_D[2]) & 1, p.z);
}

// octarender.cpp's generated `facecoords` table, expanded by hand from the
// GENFACEVERTS macros in octa.h. Each entry is a {0,1} corner triple of the
// face; the winding order per orientation is upstream's, and the renderer
// depends on it.
const Rt3IVec RT3_FACE_COORDS[6][4] = {
	{ Rt3IVec(0, 1, 1), Rt3IVec(0, 1, 0), Rt3IVec(0, 0, 0), Rt3IVec(0, 0, 1) }, // O_LEFT
	{ Rt3IVec(1, 1, 1), Rt3IVec(1, 0, 1), Rt3IVec(1, 0, 0), Rt3IVec(1, 1, 0) }, // O_RIGHT
	{ Rt3IVec(1, 0, 1), Rt3IVec(0, 0, 1), Rt3IVec(0, 0, 0), Rt3IVec(1, 0, 0) }, // O_BACK
	{ Rt3IVec(0, 1, 0), Rt3IVec(0, 1, 1), Rt3IVec(1, 1, 1), Rt3IVec(1, 1, 0) }, // O_FRONT
	{ Rt3IVec(0, 0, 0), Rt3IVec(0, 1, 0), Rt3IVec(1, 1, 0), Rt3IVec(1, 0, 0) }, // O_BOTTOM
	{ Rt3IVec(0, 0, 1), Rt3IVec(1, 0, 1), Rt3IVec(1, 1, 1), Rt3IVec(0, 1, 1) }  // O_TOP
};

void Rt3GenFaceVerts(const Rt3Cube& c, int orient, Rt3IVec v[4])
{
	ASSERT_(orient >= 0 && orient < 6, "face orientation must be 0..5");
	for(int i = 0; i < 4; i++)
	{
		const Rt3IVec& corner = RT3_FACE_COORDS[orient][i];
		int x = corner.x, y = corner.y, z = corner.z;
		v[i] = Rt3IVec(Rt3EdgeGet(Rt3CubeEdge(c, 0, y, z), x),
		               Rt3EdgeGet(Rt3CubeEdge(c, 1, z, x), y),
		               Rt3EdgeGet(Rt3CubeEdge(c, 2, x, y), z));
	}
}

bool Rt3InsideWorld(const Rt3IVec& o, int worldsize)
{
	return (unsigned int)(o.x) < (unsigned int)(worldsize) && (unsigned int)(o.y) < (unsigned int)(worldsize) && (unsigned int)(o.z) < (unsigned int)(worldsize);
}

// octa.cpp's `validatec`, with one branch deliberately not ported: upstream
// subdivides any leaf larger than 0x1000 cube units (subdividecube, which
// needs the clip-plane machinery ported later). That branch is unreachable
// for maps with worldsize <= 8192 - the root's children are worldsize/2 and
// no leaf can be bigger - so we assert instead of silently skipping a repair
// we cannot perform.
void Rt3ValidateCubes(Rt3Cube* c, int size, Rt3ValidateReport& rep)
{
	for(int i = 0; i < 8; i++)
	{
		Rt3Cube& cube = c[i];
		if(cube.children)
		{
			if(size <= 1)
			{
				Rt3SolidFaces(cube);
				Rt3DiscardChildren(cube);
				rep.forcedSolid++;
			}
			else Rt3ValidateCubes(cube.children, size >> 1, rep);
		}
		else
		{
			ASSERT_(size <= 0x1000, "a leaf larger than 0x1000 cube units needs subdividecube, which is not ported yet (worldsize must be <= 8192)");
			for(int j = 0; j < 3; j++)
			{
				dword f = cube.faces[j];
				dword e0 = f & 0x0F0F0F0FU, e1 = (f >> 4) & 0x0F0F0F0FU;
				if(e0 == e1 || ((e1 + 0x07070707U) | (e1 - e0)) & 0xF0F0F0F0U)
				{
					Rt3EmptyFaces(cube);
					rep.emptied++;
					break;
				}
			}
		}
	}
}

END_UPP_NAMESPACE
