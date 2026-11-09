#include "Rt3D.h"

// Inter-Quake Model reader. See Iqm.h for what IQM is and why this fork needs
// it; the load order mirrors iqm.h: read the file, decode the shared arrays,
// then let the caller build the skeleton, the mesh and the animations.

NAMESPACE_UPP

// How many channels a pose's mask accounts for, which is how many values the
// reader has to consume for that bone's share of a frame. The mask tops out at
// bit 9: the three position bits, the four orientation bits and the three size
// bits that are in the stream but not skinned with.
static int IqmMaskBits(dword mask)
{
	int n = 0;
	for(int k = 0; k < 10; k++) if(mask & (1 << k)) n++;
	return n;
}

// 16 bytes of magic plus 27 dwords.
static const dword IQM_HEADER_SIZE = 16 + 27 * 4;

// Upstream caps IQM files at 16 MB (`if(hdr.filesize > (16<<20)) goto error`).
static const dword IQM_MAX_FILESIZE = 16 << 20;

// Little-endian component readers. memcpy rather than a reinterpret_cast,
// because the buffer has no alignment guarantee at these offsets.
static word IqmU16(const String& f, dword ofs)
{
	const byte* p = (const byte*)f + ofs;
	return (word)(p[0] | (p[1] << 8));
}

static dword IqmU32(const String& f, dword ofs)
{
	const byte* p = (const byte*)f + ofs;
	return (dword)p[0] | ((dword)p[1] << 8) | ((dword)p[2] << 16) | ((dword)p[3] << 24);
}

static int IqmI32(const String& f, dword ofs)
{
	return (int)IqmU32(f, ofs);
}

static double IqmF32(const String& f, dword ofs)
{
	float v;
	memcpy(&v, (const char*)f + ofs, 4);
	return v;
}

// Every array the file declares must lie inside the payload. Upstream trusts
// the header (it has just read the file into a buffer of exactly
// `filesize` bytes, so an over-long array reads out of bounds - the only
// reason it survives is that a real exporter never writes one).
static bool IqmFits(int64 ofs, int64 count, int64 stride, dword filesize)
{
	if(ofs < 0 || ofs > (int64)filesize) return false;
	if(stride <= 0) return false;
	return count <= ((int64)filesize - ofs) / stride;
}

bool Rt3IqmModel::Load(const String& path, Rt3IqmModel& out, String& error)
{
	out.path = path;
	if(!out.ReadFile(path, error)) return false;
	if(!out.ParseHeader(error)) return false;
	if(!out.ParseVertexArrays(error)) return false;
	return true;
}

bool Rt3IqmModel::ReadFile(const String& path, String& error)
{
	FileIn f;
	if(!f.Open(path))
	{
		error = "cannot open '" + path + "'";
		return false;
	}
	int64 len = f.GetSize();
	if(len < IQM_HEADER_SIZE)
	{
		error = Format("'%s' is %lld bytes, too short to be an IQM file", path, (long long)len);
		return false;
	}
	Vector<byte> buf((int)len);
	if(!f.GetAll(buf.Begin(), (int)len))
	{
		error = Format("short read on '%s': wanted %lld bytes", path, (long long)len);
		return false;
	}
	file = String((const char*)buf.Begin(), (int)len);
	f.Close();
	if(memcmp((const char*)file, "INTERQUAKEMODEL", 16))
	{
		error = Format("'%s' is not an IQM file (bad magic)", path);
		return false;
	}
	hdr.version = IqmU32(file, 16);
	hdr.filesize = IqmU32(file, 20);
	if(hdr.version != 2)
	{
		error = Format("'%s' is IQM version %d, only version 2 is supported",
		               path, (int)hdr.version);
		return false;
	}
	if(hdr.filesize < IQM_HEADER_SIZE || hdr.filesize > IQM_MAX_FILESIZE)
	{
		error = Format("'%s' declares a %d byte file, which is not a plausible IQM",
		               path, (int)hdr.filesize);
		return false;
	}
	if((int64)hdr.filesize > len)
	{
		error = Format("'%s' declares %d bytes but is only %lld long",
		               path, (int)hdr.filesize, (long long)len);
		return false;
	}
	// The header's own `filesize` decides how much of the file belongs to the
	// model, and every offset below is validated against it - so trailing
	// bytes are simply never read, as upstream ignores them too.
	return true;
}

bool Rt3IqmModel::ParseHeader(String& error)
{
	// The 27 header dwords past the magic, in upstream's declaration order.
	// A section's count always precedes its offset, which is the whole
	// convention the reader relies on below.
	dword at = 16;
	hdr.version = IqmU32(file, at); at += 4;
	hdr.filesize = IqmU32(file, at); at += 4;
	hdr.flags = IqmU32(file, at); at += 4;
	hdr.numtext = IqmU32(file, at); at += 4; hdr.ofstext = IqmU32(file, at); at += 4;
	hdr.nummeshes = IqmU32(file, at); at += 4; hdr.ofsmeshes = IqmU32(file, at); at += 4;
	hdr.numvertexarrays = IqmU32(file, at); at += 4;
	hdr.numvertexes = IqmU32(file, at); at += 4;
	hdr.ofsvertexarrays = IqmU32(file, at); at += 4;
	hdr.numtriangles = IqmU32(file, at); at += 4;
	hdr.ofstriangles = IqmU32(file, at); at += 4;
	hdr.ofsadjacency = IqmU32(file, at); at += 4;
	hdr.numjoints = IqmU32(file, at); at += 4; hdr.ofsjoints = IqmU32(file, at); at += 4;
	hdr.numposes = IqmU32(file, at); at += 4; hdr.ofsposes = IqmU32(file, at); at += 4;
	hdr.numanims = IqmU32(file, at); at += 4; hdr.ofsanims = IqmU32(file, at); at += 4;
	hdr.numframes = IqmU32(file, at); at += 4;
	hdr.numframechannels = IqmU32(file, at); at += 4;
	hdr.ofsframes = IqmU32(file, at); at += 4; hdr.ofsbounds = IqmU32(file, at); at += 4;
	hdr.numcomment = IqmU32(file, at); at += 4; hdr.ofscomment = IqmU32(file, at); at += 4;
	hdr.numextensions = IqmU32(file, at); at += 4; hdr.ofsextensions = IqmU32(file, at);

	// Text: the block holds `numtext` NUL-terminated strings back to back, and
	// everything else in the file refers to a string by its byte offset in
	// here. The block is not sized by the header, so it is read by counting
	// terminators - and a string that is not terminated inside the file is a
	// corrupt file, not a long name.
	if(hdr.numtext)
	{
		if(hdr.ofstext >= hdr.filesize)
		{
			error = Format("IQM text block at %d is outside the %d byte file",
			               (int)hdr.ofstext, (int)hdr.filesize);
			return false;
		}
		dword p = hdr.ofstext;
		for(dword t = 0; t < hdr.numtext; t++)
		{
			while(p < hdr.filesize && file[p]) p++;
			if(p >= hdr.filesize)
			{
				error = Format("IQM text string %d is not terminated", (int)t);
				return false;
			}
			p++;  // the terminator
		}
		text = String((const char*)file + hdr.ofstext, (int)(p - hdr.ofstext));
	}

	if(!IqmFits(hdr.ofsmeshes, hdr.nummeshes, 24, hdr.filesize) ||
	   !IqmFits(hdr.ofsvertexarrays, hdr.numvertexarrays, 20, hdr.filesize) ||
	   !IqmFits(hdr.ofstriangles, hdr.numtriangles, 12, hdr.filesize) ||
	   !IqmFits(hdr.ofsjoints, hdr.numjoints, 48, hdr.filesize) ||
	   !IqmFits(hdr.ofsposes, hdr.numposes, 88, hdr.filesize) ||
	   !IqmFits(hdr.ofsanims, hdr.numanims, 20, hdr.filesize) ||
	   !IqmFits(hdr.ofsframes, (int64)hdr.numframes * hdr.numframechannels, 2, hdr.filesize))	{
		error = Format("IQM header points outside the %d byte file", (int)hdr.filesize);
		return false;
	}

	meshes.SetCount(hdr.nummeshes);
	for(dword m = 0; m < hdr.nummeshes; m++)
	{
		dword o = hdr.ofsmeshes + 24 * m;
		meshes[m].name = IqmU32(file, o);
		meshes[m].material = IqmU32(file, o + 4);
		meshes[m].firstvertex = IqmU32(file, o + 8);
		meshes[m].numvertexes = IqmU32(file, o + 12);
		meshes[m].firsttriangle = IqmU32(file, o + 16);
		meshes[m].numtriangles = IqmU32(file, o + 20);
		// A mesh's own ranges have to be inside the shared arrays too, or the
		// per-mesh loops below would read whatever follows them.
		if(meshes[m].firstvertex + (int64)meshes[m].numvertexes > (int64)hdr.numvertexes ||
		   meshes[m].firsttriangle + (int64)meshes[m].numtriangles > (int64)hdr.numtriangles)
		{
			error = Format("IQM mesh %d reaches outside the shared vertex/triangle arrays", (int)m);
			return false;
		}
	}

	vertexarrays.SetCount(hdr.numvertexarrays);
	for(dword a = 0; a < hdr.numvertexarrays; a++)
	{
		dword o = hdr.ofsvertexarrays + 20 * a;
		vertexarrays[a].type = IqmU32(file, o);
		vertexarrays[a].flags = IqmU32(file, o + 4);
		vertexarrays[a].format = IqmU32(file, o + 8);
		vertexarrays[a].size = IqmU32(file, o + 12);
		vertexarrays[a].offset = IqmU32(file, o + 16);
		// Only the two formats upstream accepts: the geometric arrays are
		// 32-bit floats, the two skinning arrays are bytes. Anything else it
		// refuses outright, because its `lilswap` would leave such an array
		// byte-swapped on a big-endian host and read as noise.
		int esize = vertexarrays[a].format == RT3_IQM_FLOAT ? 4 :
		            vertexarrays[a].format == RT3_IQM_UBYTE ? 1 : 0;
		if(!esize || !vertexarrays[a].size)
		{
			error = Format("IQM vertex array %d has an unsupported format %d or a zero size",
			               (int)a, (int)vertexarrays[a].format);
			return false;
		}
		if(!IqmFits(vertexarrays[a].offset,
		            (int64)hdr.numvertexes * vertexarrays[a].size, esize, hdr.filesize))
		{
			error = Format("IQM vertex array %d reaches outside the %d byte file",
			               (int)a, (int)hdr.filesize);
			return false;
		}
	}

	joints.SetCount(hdr.numjoints);
	for(dword j = 0; j < hdr.numjoints; j++)
	{
		dword o = hdr.ofsjoints + 48 * j;
		joints[j].name = IqmU32(file, o);
		joints[j].parent = IqmI32(file, o + 4);
		joints[j].pos = Rt3Vec3(IqmF32(file, o + 8), IqmF32(file, o + 12), IqmF32(file, o + 16));
		joints[j].orient = Rt3Quat(IqmF32(file, o + 20), IqmF32(file, o + 24),
		                           IqmF32(file, o + 28), IqmF32(file, o + 32));
		joints[j].size = Rt3Vec3(IqmF32(file, o + 36), IqmF32(file, o + 40), IqmF32(file, o + 44));
		if(joints[j].parent >= (int)j)
		{
			error = Format("IQM joint %d has parent %d, which is not an earlier joint",
			               (int)j, joints[j].parent);
			return false;
		}
	}

	poses.SetCount(hdr.numposes);
	for(dword p = 0; p < hdr.numposes; p++)
	{
		dword o = hdr.ofsposes + 88 * p;
		poses[p].parent = IqmI32(file, o);
		poses[p].mask = IqmU32(file, o + 4);
		poses[p].offsetpos = Rt3Vec3(IqmF32(file, o + 8), IqmF32(file, o + 12), IqmF32(file, o + 16));
		poses[p].offsetorient = Rt3Quat(IqmF32(file, o + 20), IqmF32(file, o + 24),
		                                IqmF32(file, o + 28), IqmF32(file, o + 32));
		poses[p].offsetsize = Rt3Vec3(IqmF32(file, o + 36), IqmF32(file, o + 40), IqmF32(file, o + 44));
		poses[p].scalepos = Rt3Vec3(IqmF32(file, o + 48), IqmF32(file, o + 52), IqmF32(file, o + 56));
		poses[p].scaleorient = Rt3Quat(IqmF32(file, o + 60), IqmF32(file, o + 64),
		                                IqmF32(file, o + 68), IqmF32(file, o + 72));
		poses[p].scalesize = Rt3Vec3(IqmF32(file, o + 76), IqmF32(file, o + 80), IqmF32(file, o + 84));
	}

	anims.SetCount(hdr.numanims);
	for(dword n = 0; n < hdr.numanims; n++)
	{
		dword o = hdr.ofsanims + 20 * n;
		anims[n].name = IqmU32(file, o);
		anims[n].firstframe = IqmU32(file, o + 4);
		anims[n].numframes = IqmU32(file, o + 8);
		anims[n].framerate = IqmF32(file, o + 12);
		anims[n].flags = IqmU32(file, o + 16);
		if(anims[n].firstframe + (int64)anims[n].numframes > (int64)hdr.numframes)
		{
			error = Format("IQM animation %d covers frames %d..%d of the %d the file has",
			               (int)n, (int)anims[n].firstframe,
			               (int)(anims[n].firstframe + anims[n].numframes), (int)hdr.numframes);
			return false;
		}
	}

	framechannels.SetCount((int)((int64)hdr.numframes * hdr.numframechannels));
	for(int i = 0; i < framechannels.GetCount(); i++)
		framechannels[i] = IqmU16(file, hdr.ofsframes + 2 * i);
	return true;
}

// One per-vertex attribute, decoded into a flat list of components. Upstream
// requires an exact format and component count for each attribute it knows
// and rejects the file otherwise; a file that simply has none of an attribute
// is legal, and `present` says which case this is.
bool Rt3IqmModel::ReadDecodedArray(dword type, dword format, dword want, Vector<double>& out,
                                    bool& present, String& error)
{
	present = false;
	for(int a = 0; a < vertexarrays.GetCount(); a++)
	{
		if(vertexarrays[a].type != type) continue;
		const Rt3IqmVertexArray& va = vertexarrays[a];
		if(va.format != format || va.size != want)
		{
			error = Format("IQM vertex array of type %d is format %d with %d components, "
			               "expected format %d with %d", (int)type, (int)va.format,
			               (int)va.size, (int)format, (int)want);
			return false;
		}
		int n = (int)((int64)hdr.numvertexes * want);
		out.SetCount(n);
		if(format == RT3_IQM_FLOAT)
		{
			// memcpy per element, because `ofs` in a real file need not be
			// 4-byte aligned inside the buffer.
			for(int i = 0; i < n; i++)
			{
				float v;
				memcpy(&v, (const char*)file + va.offset + 4 * i, 4);
				out[i] = v;
			}
		}
		else
			for(int i = 0; i < n; i++) out[i] = (double)((const byte*)file)[va.offset + i];
		present = true;
		return true;
	}
	return true;
}

bool Rt3IqmModel::ParseVertexArrays(String& error)
{
	// The shared arrays, decoded once into component lists. Missing arrays are
	// legal and mean "the file has none of that" - a mesh without normals
	// skinned rigidly, for instance - so `present` decides what the mesh
	// builder may assume, not whether the file is valid.
	Vector<double> tpos, tnorm, ttan, ttanw, ttc, tindex, tweight;
	bool ppos, pnorm, ptan, ptc, pindex, pweight;
	if(!ReadDecodedArray(RT3_IQM_POSITION, RT3_IQM_FLOAT, 3, tpos, ppos, error)) return false;
	if(!ReadDecodedArray(RT3_IQM_NORMAL, RT3_IQM_FLOAT, 3, tnorm, pnorm, error)) return false;
	if(!ReadDecodedArray(RT3_IQM_TANGENT, RT3_IQM_FLOAT, 4, ttan, ptan, error)) return false;
	if(!ReadDecodedArray(RT3_IQM_TEXCOORD, RT3_IQM_FLOAT, 2, ttc, ptc, error)) return false;
	if(!ReadDecodedArray(RT3_IQM_BLENDINDEXES, RT3_IQM_UBYTE, 4, tindex, pindex, error)) return false;
	if(!ReadDecodedArray(RT3_IQM_BLENDWEIGHTS, RT3_IQM_UBYTE, 4, tweight, pweight, error)) return false;
	if(pindex != pweight)
	{
		error = "IQM file has blend indexes without blend weights (or the other way round)";
		return false;
	}
	if(ptan && !pnorm)
	{
		error = "IQM file has tangents without normals";
		return false;
	}

	if(ppos)
		for(int i = 0; i < hdr.numvertexes; i++)
			vpos.Add(Rt3Vec3(tpos[3 * i], tpos[3 * i + 1], tpos[3 * i + 2]));
	if(pnorm)
		for(int i = 0; i < hdr.numvertexes; i++)
			vnorm.Add(Rt3Vec3(tnorm[3 * i], tnorm[3 * i + 1], tnorm[3 * i + 2]));
	if(ptan)
	{
		for(int i = 0; i < hdr.numvertexes; i++)
		{
			vtan.Add(Rt3Vec3(ttan[4 * i], ttan[4 * i + 1], ttan[4 * i + 2]));
			vtanw.Add(ttan[4 * i + 3]);
		}
	}
	if(ptc)
		for(int i = 0; i < hdr.numvertexes; i++)
		{
			vtc.Add(ttc[2 * i]);
			vtc.Add(ttc[2 * i + 1]);
		}
	if(pindex)
		for(int i = 0; i < 4 * (int)hdr.numvertexes; i++)
			vindex.Add(tindex[i]);
	if(pweight)
		for(int i = 0; i < 4 * (int)hdr.numvertexes; i++)
			vweight.Add(tweight[i]);

	rawtris.SetCount(hdr.numtriangles);
	for(dword t = 0; t < hdr.numtriangles; t++)
	{
		dword o = hdr.ofstriangles + 12 * t;
		rawtris[t].vert[0] = IqmU32(file, o);
		rawtris[t].vert[1] = IqmU32(file, o + 4);
		rawtris[t].vert[2] = IqmU32(file, o + 8);
		for(int k = 0; k < 3; k++)
			if((dword)rawtris[t].vert[k] >= hdr.numvertexes)
			{
				error = Format("IQM triangle %d references vertex %d of %d",
				               (int)t, rawtris[t].vert[k], (int)hdr.numvertexes);
				return false;
			}
	}
	return true;
}

String Rt3IqmModel::Text(dword offset) const
{
	if(offset >= text.GetLength()) return "";
	int end = text.Find('\0', offset);
	return end < 0 ? "" : text.Mid(offset, end - offset);
}

bool Rt3IqmModel::BuildSkeleton(Rt3Skeleton& skel, String& error) const
{
	// iqm.h: a skeleton that already has bones is shared between meshes, and
	// this file contributes nothing to it.
	if(skel.numbones > 0) return true;
	if(joints.IsEmpty())
	{
		error = "IQM file has no joints, so it has no skeleton";
		return false;
	}
	skel.bones.SetCount(joints.GetCount());
	for(int i = 0; i < joints.GetCount(); i++)
	{
		const Rt3IqmJoint& j = joints[i];
		if(j.parent >= joints.GetCount())
		{
			error = Format("IQM joint %d has parent %d of %d joints", i, j.parent, joints.GetCount());
			return false;
		}
		Rt3BoneInfo& b = skel.bones[i];
		b.name = Text(j.name);
		b.parent = j.parent;
		// IQM is y-up and Tesseract is z-up. Upstream flips the y of every
		// position and the x/z of every orientation (a 90-degree turn about y
		// composed with the handedness flip, which a quaternion sign change
		// and a component mirroring together give), then normalizes because
		// mirroring is not a unit operation on the raw numbers.
		Rt3Vec3 pos(j.pos.x, -j.pos.y, j.pos.z);
		Rt3Quat orient(-j.orient.x, j.orient.y, -j.orient.z, j.orient.w);
		orient.Normalize();
		b.base = Rt3DualQuat(orient, pos);
		// `Rt3DualQuat::Mul(p, o)` writes in place, so `o` must not be this
		// same object: the product's real part reads the previous w after it
		// has already overwritten x. Upstream spells the copy out
		// (`b.base.mul(parent, dualquat(b.base))`) and so does this.
		if(b.parent >= 0) b.base.Mul(skel.bones[b.parent].base, Rt3DualQuat(b.base));
		b.invbase = b.base;
		b.invbase.Invert();
	}
	skel.numbones = joints.GetCount();
	skel.LinkChildren();
	return true;
}

// animmodel.h's `calctangent`: the tangent frame as a quaternion, built from
// the triangle's own normal and the file's tangent direction, with the
// bitangent taken as `n x t` and its sign forced to the file's. The rows are
// laid out the way `quat::convertmatrix` reads them.
static Rt3Quat IqmCalcTangent(const Rt3Vec3& n, const Rt3Vec3& t, double bt)
{
	Rt3Matrix4x3 m(t, n.Cross(t), n, Rt3Vec3(0, 0, 0));
	Rt3Quat q(m);
	Rt3FixQuatTangent(q, bt);
	return q;
}


bool Rt3IqmModel::BuildMesh(Rt3Skeleton& skel, String& error)
{
	// The blend weights are the file's raw bytes, 0..255, handed to
	// `Rt3BlendCombo::Finalize` unscaled - which is upstream's behavior, and
	// why a file stores 255 for "all of this bone" rather than 1.
	bool hasblend = !vindex.IsEmpty();
	bool havenorm = !vnorm.IsEmpty();

	// The blend indexes name the model's joints, so a weight pointing outside
	// the bone table would skin with a bone that does not exist - a corrupt
	// file rather than a tolerable one. Every mesh is checked before any
	// vertex is added, so a rejected file leaves the model untouched.
	if(hasblend)
		for(int m = 0; m < meshes.GetCount(); m++)
		{
			const Rt3IqmMesh& im = meshes[m];
			for(int v = (int)im.firstvertex; v < (int)(im.firstvertex + im.numvertexes); v++)
				for(int k = 0; k < 4; k++)
					if(vweight[4 * v + k] > 1e-3 && (int)vindex[4 * v + k] >= skel.numbones)
					{
						error = Format("IQM mesh '%s' weights vertex %d to bone %d of %d",
						               Text(im.name), v, (int)vindex[4 * v + k], skel.numbones);
						return false;
					}
		}

	for(int m = 0; m < meshes.GetCount(); m++)
	{
		const Rt3IqmMesh& im = meshes[m];
		// iqm.h drops a mesh with no vertices or no triangles, warning about
		// it. The warning matters - an exporter that emitted one is broken -
		// but the mesh itself is not an error.
		if(!im.numvertexes || !im.numtriangles)
		{
			LOG("IQM: dropping mesh '" << Text(im.name) << "' with " << im.numvertexes
			    << " vertices and " << im.numtriangles << " triangles in " << path);
			continue;
		}
		int fv = (int)im.firstvertex;
		// Where this mesh lands in the flat lists, so `BuildSkelMeshes` can
		// split them back apart - a multi-material model has one skin per
		// material, and that is the granularity a draw call works at.
		Rt3IqmMeshPart part;
		part.name = Text(im.name);
		part.material = Text(im.material);
		part.vertbase = verts.GetCount();
		part.tribase = tris.GetCount();

		// A file with no blend arrays has an unskinned mesh: one combination
		// with no influences at all, shared by every vertex.
		int noblend = -1;
		if(!hasblend)
		{
			Rt3BlendCombo c;
			c.Finalize(0);
			noblend = skel.AddBlendCombo(c);
		}

		for(int j = 0; j < (int)im.numvertexes; j++)
		{
			int v = fv + j;
			Rt3SkelVert& out = verts.Add();
			out.pos = Rt3Vec3(vpos[v].x, -vpos[v].y, vpos[v].z);
			if(vtc.GetCount() >= 2 * (v + 1))
			{
				out.u = vtc[2 * v];
				out.v = vtc[2 * v + 1];
			}
			if(havenorm)
			{
				out.norm = Rt3Vec3(vnorm[v].x, -vnorm[v].y, vnorm[v].z);
				// A tangent array is optional; without one the vertex keeps the
				// identity frame, which is what upstream does too
				// (`v.tangent = quat(0,0,0,1)`). Deriving tangents from the
				// triangles is `calctangents`' job and belongs with the
				// renderer.
				if(!vtan.IsEmpty())
					out.tangent = IqmCalcTangent(out.norm,
					                            Rt3Vec3(vtan[v].x, -vtan[v].y, vtan[v].z),
					                            vtanw[v]);
			}
			if(noblend < 0)
			{
				Rt3BlendCombo c;
				int sorted = 0;
				for(int k = 0; k < 4; k++)
					sorted = c.AddWeight(sorted, vweight[4 * v + k], (int)vindex[4 * v + k]);
				c.Finalize(sorted);
				out.blend = skel.AddBlendCombo(c);
			}
			else out.blend = noblend;
		}

		// Upstream rebases each mesh's triangles onto its own first vertex, so
		// the appended mesh is contiguous and self-contained.
		for(int t = 0; t < (int)im.numtriangles; t++)
		{
			const Rt3SkelTri& rt = rawtris[im.firsttriangle + t];
			Rt3SkelTri& out = tris.Add();
			for(int k = 0; k < 3; k++) out.vert[k] = rt.vert[k] - fv;
		}
		part.numverts = verts.GetCount() - part.vertbase;
		part.numtris = tris.GetCount() - part.tribase;
		meshparts.Add(part);
	}
	return true;
}

void Rt3IqmModel::SortBlendCombos(Rt3Skeleton& skel)
{
	// skelmodel.h's `sortblendcombos`: sort the list into shader order, then
	// renumber each vertex's combination through the old -> new map.
	// `vector::sort` upstream, which is a stable sort - and stability matters,
	// because two combinations that compare equal keep the order their
	// vertices were added in.
	std::stable_sort(skel.blendcombos.begin(), skel.blendcombos.begin() + skel.blendcombos.GetCount(),
	                 Rt3BlendCombo::SortCmp);
	int n = skel.blendcombos.GetCount();
	Vector<int> remap(n);
	for(int i = 0; i < n; i++) remap[skel.blendcombos[i].interpindex] = i;
	for(int v = 0; v < verts.GetCount(); v++)
	{
		int b = verts[v].blend;
		if(b >= 0 && b < n) verts[v].blend = remap[b];
	}
}

void Rt3IqmModel::RemapBlendVertices(Rt3Skeleton& skel)
{
	for(int i = 0; i < verts.GetCount(); i++)
		verts[i].interpindex = skel.RemapBlend(verts[i].blend);
}


void Rt3IqmModel::BuildSkelMeshes(const Rt3Skeleton& skel, Vector<Rt3SkelMesh>& out) const
{
	for(int i = 0; i < meshparts.GetCount(); i++)
	{
		const Rt3IqmMeshPart& p = meshparts[i];
		Rt3SkelMesh& m = out.Add();
		m.name = p.name;
		m.material = p.material;
		for(int j = 0; j < p.numverts; j++)
		{
			const Rt3SkelVert& v = verts[p.vertbase + j];
			m.verts.Add(v);
			// skelmesh's `addblendcombo` also raised the mesh's `maxweights` to
			// the number of influences of the widest combination it holds,
			// which is what picks the shader variant to draw it with.
			if(v.blend >= 0 && v.blend < skel.blendcombos.GetCount())
				m.maxweights = max(m.maxweights, skel.blendcombos[v.blend].Size());
		}
		for(int j = 0; j < p.numtris; j++)
			m.tris.Add(tris[p.tribase + j]);
	}
}

bool Rt3IqmModel::AppendAnims(Rt3Skeleton& skel, const String& path, String& error)
{
	if(anims.IsEmpty()) return true;
	if(!skel.numbones)
	{
		error = "IQM file has animations but no skeleton to put them on";
		return false;
	}
	if(poses.GetCount() < skel.numbones)
	{
		error = Format("IQM file has %d poses for a %d bone skeleton",
		               poses.GetCount(), skel.numbones);
		return false;
	}
	// A frame's channels are exactly the ones the poses' masks call for, read
	// bone by bone and bit by bit. If the two do not add up, the reader would
	// walk the stream out of step and quietly produce a pose that drifts away
	// from the animation - so it is checked before anything is appended.
	int channels = 0;
	for(int k = 0; k < skel.numbones; k++) channels += IqmMaskBits(poses[k].mask);
	if(channels != (int)hdr.numframechannels)
	{
		error = Format("IQM file has %d frame channels, but its %d poses account for %d",
		               (int)hdr.numframechannels, skel.numbones, channels);
		return false;
	}

	for(int a = 0; a < anims.GetCount(); a++)
	{
		const Rt3IqmAnim& ia = anims[a];
		// Upstream names an animation "<file>:<anim>", which is what makes
		// `findskelanim`'s ':' separator match: a caller that knows the model
		// path can ask for the animation without knowing the file layout.
		String name = path + ":" + Text(ia.name);
		if(skel.FindSkelAnim(name) >= 0) continue;

		int startframe = skel.numframes;
		Rt3SkelAnimSpec& sa = skel.AddSkelAnim(name);
		sa.frame = startframe;
		sa.range = (int)ia.numframes;
		// The keyframe array grows by this animation's frames; the frames
		// already there are the previous animations' and are kept.
		skel.framebones.SetCount((startframe + (int)ia.numframes) * skel.numbones);
		skel.numframes = startframe + (int)ia.numframes;

		// The frame channels are consumed in mask order per bone, and the
		// cursor is a pointer into the file's channel stream - which is why
		// the unused size channels still have to be stepped over.
		const word* animdata = &framechannels[(int64)ia.firstframe * hdr.numframechannels];
		for(int j = 0; j < (int)ia.numframes; j++)
		{
			Rt3DualQuat* frame = &skel.framebones[(startframe + j) * skel.numbones];
			for(int k = 0; k < skel.numbones; k++)
			{
				const Rt3IqmPose& p = poses[k];
				Rt3Vec3 pos;
				Rt3Quat orient;
				pos.x = p.offsetpos.x; if(p.mask & 0x01) pos.x += *animdata++ * p.scalepos.x;
				pos.y = -p.offsetpos.y; if(p.mask & 0x02) pos.y -= *animdata++ * p.scalepos.y;
				pos.z = p.offsetpos.z; if(p.mask & 0x04) pos.z += *animdata++ * p.scalepos.z;
				orient.x = -p.offsetorient.x; if(p.mask & 0x08) orient.x -= *animdata++ * p.scaleorient.x;
				orient.y = p.offsetorient.y; if(p.mask & 0x10) orient.y += *animdata++ * p.scaleorient.y;
				orient.z = -p.offsetorient.z; if(p.mask & 0x20) orient.z -= *animdata++ * p.scaleorient.z;
				orient.w = p.offsetorient.w; if(p.mask & 0x40) orient.w += *animdata++ * p.scaleorient.w;
				orient.Normalize();
				// The three size channels: not skinned with, but present in the
				// stream, so they have to be walked past.
				if(p.mask & 0x380)
				{
					if(p.mask & 0x80) animdata++;
					if(p.mask & 0x100) animdata++;
					if(p.mask & 0x200) animdata++;
				}
				Rt3DualQuat dq(orient, pos);
				Rt3BoneInfo& b = skel.bones[k];
				// A pose is stored in bone space; the interpolation wants it
				// relative to the bind pose, and a child composes onto its
				// parent's bind transform.
				dq.Mul(b.invbase);
				if(p.parent < 0) frame[k] = dq;
				else frame[k].Mul(skel.bones[p.parent].base, dq);
				// Keep the sign continuous with the previous frame, or the
				// interpolation between them takes the long way round. The
				// first frame of an animation has no predecessor to compare
				// with - upstream compares it against uninitialized memory
				// here, which is a bug rather than a behavior worth keeping.
				if(j > 0) frame[k].FixAntipodal(frame[k - skel.numbones]);
			}
		}
	}
	return true;
}

END_UPP_NAMESPACE
