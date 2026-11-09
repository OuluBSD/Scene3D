#include "Rt3D.h"

NAMESPACE_UPP

// animmodel.h's `fixqtangent`: a tangent quaternion's `w` is the bitangent's
// sign, and building or skinning one can flip it. Declared in SkelMesh.h
// because both the tangent frame construction (Iqm.cpp's `calctangent`) and the
// skinning pass (below) need it, and it must be one function, not two copies.
void Rt3FixQuatTangent(Rt3Quat& q, double bt)
{
	// Both constants are upstream's: a negative bitangent biases `w` to just
	// below zero rather than to exactly -1, which is what the shader's own
	// convention expects, and the rest is the sign the file asked for.
	const double bias = -1.5 / 65535, biasscale = sqrt(1 - bias * bias);
	if(bt < 0)
	{
		if(q.w >= 0) q.Neg();
		if(q.w > bias)
		{
			q.Mul3(biasscale);
			q.w = bias;
		}
	}
	else if(q.w < 0) q.Neg();
}

int Rt3SkelCacheEntry::lastversion = 0;

int Rt3SkelMesh::GenVbo(Vector<int>& idxs, int offset)
{
	voffset = offset;
	eoffset = idxs.GetCount();
	for(int i = 0; i < tris.GetCount(); i++)
		for(int j = 0; j < 3; j++)
			idxs.Add(voffset + tris[i].vert[j]);
	elen = idxs.GetCount() - eoffset;
	// Upstream's are `ushort`, so a model with more than 65536 vertices would
	// wrap; the element list here is an `int` and the range is the true one.
	minvert = voffset;
	maxvert = voffset + verts.GetCount() - 1;
	return verts.GetCount();
}

void Rt3SkelMesh::InterpVerts(const Vector<Rt3DualQuat>& bdata, const Vector<Rt3DualQuat>& blend,
                              int numgpubones, Vector<Rt3SkelVert>& out) const
{
	for(int i = 0; i < verts.GetCount(); i++)
	{
		const Rt3SkelVert& src = verts[i];
		int index = src.interpindex;
		const Rt3DualQuat* b = index < numgpubones ? &bdata[index] : &blend[index - numgpubones];
		Rt3SkelVert& dst = out[voffset + i];
		dst.pos = b->Transform(src.pos);
		dst.norm = b->TransformNormal(src.norm);
		// The tangent frame is a quaternion, so skinning it is a single
		// rotation - and then its `w` has to be forced back to the bitangent's
		// sign, exactly as upstream's `interpverts` does.
		dst.tangent = b->Transform(src.tangent);
		Rt3FixQuatTangent(dst.tangent, src.tangent.w);
		dst.u = src.u;
		dst.v = src.v;
	}
}

void Rt3SkelModel::GenVbo()
{
	elems.Clear();
	vdata.Clear();
	vlen = 0;
	for(int i = 0; i < meshes.GetCount(); i++)
	{
		vlen += meshes[i].GenVbo(elems, vlen);
		// The bind pose fills the shared array, so the model draws correctly
		// before anything animates it. Upstream copies only the parts a pose
		// cannot move (`fillverts` writes just the texture coordinates, because
		// its vertex buffer's positions are overwritten before every draw);
		// here the whole vertex is one value, and `InterpVerts` overwrites the
		// three attributes a pose does move.
		Rt3SkelMesh& m = meshes[i];
		for(int j = 0; j < m.verts.GetCount(); j++)
			vdata.Add(m.verts[j]);
	}
}

void Rt3SkelModel::Animate(const Rt3AnimState* as, Rt3SkelCacheEntry& sc, double pitch,
                           const Rt3Vec3& axis, const Rt3Vec3& forward, int numanimparts,
                           const byte* partmask)
{
	skel.InterpBones(as, pitch, axis, forward, numanimparts, partmask, sc.bdata);
	sc.as = *as;
	sc.pitch = pitch;
	sc.NextVersion();

	Vector<Rt3DualQuat> blend;
	skel.BlendBones(sc.bdata, blend);
	for(int i = 0; i < meshes.GetCount(); i++)
		meshes[i].InterpVerts(sc.bdata, blend, skel.numgpubones, vdata);
}

END_UPP_NAMESPACE
