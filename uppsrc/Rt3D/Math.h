#ifndef _Rt3D_Math_h_
#define _Rt3D_Math_h_

// Integer and float vector math for the Tesseract-Sauerbraten port.
//
// Source: src/shared/geom.h (`ivec`, `vec`), ported to U++ value types and
// prefixed `Rt3` (see Rt3D/AGENTS.md - every ported symbol is prefixed
// because everything shares the flat Upp namespace).

class Rt3IVec
{
public:
	int x, y, z;

	Rt3IVec() : x(0), y(0), z(0) {}
	Rt3IVec(int x, int y, int z) : x(x), y(y), z(z) {}
	explicit Rt3IVec(int v) : x(v), y(v), z(v) {}

	// geom.h's `ivec(int i, const ivec &co, int size)`: the origin of octant i
	// of the cube at co, each octant being a cube of edge length size.
	Rt3IVec(int i, const Rt3IVec& co, int size)
		: x(co.x + ((i & 1) >> 0) * size)
		, y(co.y + ((i & 2) >> 1) * size)
		, z(co.z + ((i & 4) >> 2) * size) {}

	Rt3IVec& Set(int x, int y, int z) { this->x = x; this->y = y; this->z = z; return *this; }
	Rt3IVec& Set(int v) { x = y = z = v; return *this; }
	Rt3IVec& Set(const Rt3IVec& v) { return Set(v.x, v.y, v.z); }

	int& operator[](int i) { return (&x)[i]; }
	int operator[](int i) const { return (&x)[i]; }

	Rt3IVec operator+(const Rt3IVec& v) const { return Rt3IVec(x + v.x, y + v.y, z + v.z); }
	Rt3IVec operator-(const Rt3IVec& v) const { return Rt3IVec(x - v.x, y - v.y, z - v.z); }
	Rt3IVec operator*(int n) const { return Rt3IVec(x * n, y * n, z * n); }
	Rt3IVec operator-() const { return Rt3IVec(-x, -y, -z); }

	Rt3IVec& operator+=(const Rt3IVec& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Rt3IVec& operator-=(const Rt3IVec& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Rt3IVec& operator*=(int n) { x *= n; y *= n; z *= n; return *this; }

	// Componentwise min/max, matching geom.h's static helpers.
	Rt3IVec Min(const Rt3IVec& v) const { return Rt3IVec(min(x, v.x), min(y, v.y), min(z, v.z)); }
	Rt3IVec Max(const Rt3IVec& v) const { return Rt3IVec(max(x, v.x), max(y, v.y), max(z, v.z)); }

	int Abs() const { return Upp::abs(x) > Upp::abs(y) ? (Upp::abs(x) > Upp::abs(z) ? Upp::abs(x) : Upp::abs(z)) : (Upp::abs(y) > Upp::abs(z) ? Upp::abs(y) : Upp::abs(z)); }

	int Dot(const Rt3IVec& v) const { return x * v.x + y * v.y + z * v.z; }

	// Bitwise ops - the octree stores coordinates in fixed-point-like
	// low bits and uses masks to round an origin down to a cube boundary.
	Rt3IVec operator&(int n) const { return Rt3IVec(x & n, y & n, z & n); }
	Rt3IVec operator&(const Rt3IVec& v) const { return Rt3IVec(x & v.x, y & v.y, z & v.z); }
	Rt3IVec operator|(int n) const { return Rt3IVec(x | n, y | n, z | n); }
	Rt3IVec operator<<(int n) const { return Rt3IVec(x << n, y << n, z << n); }
	Rt3IVec operator>>(int n) const { return Rt3IVec(x >> n, y >> n, z >> n); }

	int Compare(const Rt3IVec& v) const
	{
		if(x != v.x) return x < v.x ? -1 : 1;
		if(y != v.y) return y < v.y ? -1 : 1;
		if(z != v.z) return z < v.z ? -1 : 1;
		return 0;
	}

	bool operator==(const Rt3IVec& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const Rt3IVec& v) const { return !operator==(v); }
	bool operator<(const Rt3IVec& v) const { return Compare(v) < 0; }

	bool IsZero() const { return !x && !y && !z; }

	String ToString() const { return Format("(%d, %d, %d)", x, y, z); }
};

// geom.h's `ivec::cross(a, b)` overwrites its receiver with a x b and never
// reads the old value, so it is a free function here rather than a member -
// a member that ignores its receiver is a trap for readers.
inline Rt3IVec Rt3Cross(const Rt3IVec& a, const Rt3IVec& b)
{
	return Rt3IVec(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// A 4-component integer vector, ported from geom.h's `ivec(d, x, y, z)`
// constructor. Kept as its own type rather than a 4-arg ctor on Rt3IVec so
// Rt3IVec stays a strict 3-vector (its operator[] and layout are relied on
// everywhere). Only the octant's "dimension + 3 corner coords" accessors
// need this.
class Rt3IVec4
{
public:
	int d, x, y, z;

	Rt3IVec4() : d(0), x(0), y(0), z(0) {}
	Rt3IVec4(int d, int x, int y, int z) : d(d), x(x), y(y), z(z) {}

	int& operator[](int i) { return (&d)[i]; }
	int operator[](int i) const { return (&d)[i]; }

	String ToString() const { return Format("(%d: %d, %d, %d)", d, x, y, z); }
};

class Rt3Vec3
{
public:
	double x, y, z;

	Rt3Vec3() : x(0), y(0), z(0) {}
	Rt3Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
	explicit Rt3Vec3(double v) : x(v), y(v), z(v) {}
	explicit Rt3Vec3(const Rt3IVec& v) : x(v.x), y(v.y), z(v.z) {}

	Rt3Vec3& Set(double x, double y, double z) { this->x = x; this->y = y; this->z = z; return *this; }

	double& operator[](int i) { return (&x)[i]; }
	double operator[](int i) const { return (&x)[i]; }

	Rt3Vec3 operator+(const Rt3Vec3& v) const { return Rt3Vec3(x + v.x, y + v.y, z + v.z); }
	Rt3Vec3 operator-(const Rt3Vec3& v) const { return Rt3Vec3(x - v.x, y - v.y, z - v.z); }
	Rt3Vec3 operator*(double n) const { return Rt3Vec3(x * n, y * n, z * n); }
	Rt3Vec3 operator/(double n) const { return Rt3Vec3(x / n, y / n, z / n); }
	Rt3Vec3 operator-() const { return Rt3Vec3(-x, -y, -z); }

	Rt3Vec3& operator+=(const Rt3Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Rt3Vec3& operator*=(double n) { x *= n; y *= n; z *= n; return *this; }

	// geom.h's `add`/`sub`/`mul` spelled out, because the quaternion and
	// matrix types below are written in that member style.
	Rt3Vec3& Add(const Rt3Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Rt3Vec3& Sub(const Rt3Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Rt3Vec3& Mul(double n) { x *= n; y *= n; z *= n; return *this; }
	Rt3Vec3& Mul(const Rt3Vec3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }

	// geom.h's `madd`/`msub`: fused multiply-add over a vector, which every
	// matrix and quaternion product in geom.h is written with. Kept as
	// separate statements here - the compiler fuses them just as well.
	Rt3Vec3& Madd(const Rt3Vec3& v, double n) { x += v.x * n; y += v.y * n; z += v.z * n; return *this; }
	Rt3Vec3& Msub(const Rt3Vec3& v, double n) { x -= v.x * n; y -= v.y * n; z -= v.z * n; return *this; }

	double Dot(const Rt3Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
	Rt3Vec3 Cross(const Rt3Vec3& v) const
	{
		return Rt3Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
	double Length() const { return sqrt(Dot(*this)); }
	double SquaredLength() const { return Dot(*this); }
	Rt3Vec3 Normalized() const
	{
		double len = Length();
		return len > 0 ? *this / len : *this;
	}
	Rt3Vec3 Lerp(const Rt3Vec3& to, double t) const { return *this + (to - *this) * t; }

	// geom.h's `vec::project(n)`: remove this vector's component along n.
	Rt3Vec3& Project(const Rt3Vec3& n)
	{
		double k = Dot(n);
		x -= k * n.x;
		y -= k * n.y;
		z -= k * n.z;
		return *this;
	}

	bool IsZero() const { return !x && !y && !z; }

	bool operator==(const Rt3Vec3& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const Rt3Vec3& v) const { return !operator==(v); }

	String ToString() const { return Sprintf("(%.3f, %.3f, %.3f)", x, y, z); }
};

class Rt3Matrix4x3;

// geom.h's `quat`. Upstream's `quat` derives from `vec4` and gets its
// component arithmetic from it; `Rt3Vec4` is not ported, so the four
// components are members of `Rt3Quat` and the inherited mixins (add/sub/mul/
// madd/msub/normalize/neg3) became explicit members with the same names.
// `double` follows `Rt3Vec3` above rather than upstream's `float`.
class Rt3Quat
{
public:
	double x, y, z, w;

	Rt3Quat() : x(0), y(0), z(0), w(0) {}
	Rt3Quat(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}

	// geom.h: `w = cos(angle/2)`, the vector part is `sin(angle/2) * axis`.
	// The axis is not normalized by upstream either.
	Rt3Quat(const Rt3Vec3& axis, double angle)
	{
		w = cos(angle / 2);
		double s = sin(angle / 2);
		x = s * axis.x;
		y = s * axis.y;
		z = s * axis.z;
	}

	// geom.h: the shortest-arc rotation taking u onto v, then normalized.
	// The `cross(u, v)` is written into the quaternion's own vector part.
	Rt3Quat(const Rt3Vec3& u, const Rt3Vec3& v)
	{
		w = sqrt(u.SquaredLength() * v.SquaredLength()) + u.Dot(v);
		x = u.y * v.z - u.z * v.y;
		y = u.z * v.x - u.x * v.z;
		z = u.x * v.y - u.y * v.x;
		Normalize();
	}

	// geom.h: a pure-vector quaternion - the "from direction" idiom. Note
	// `RestoreW` computes a *negative* root, and 0 when the radicand is
	// negative; that is upstream, not a transcription slip.
	explicit Rt3Quat(const Rt3Vec3& v) : x(v.x), y(v.y), z(v.z) { RestoreW(); }

	// Defined after Rt3Matrix4x3 (needs its rows).
	explicit Rt3Quat(const Rt3Matrix4x3& m);
	void ConvertMatrix(const Rt3Matrix4x3& m);

	void RestoreW() { w = 1.0 - x * x - y * y - z * z; w = w < 0 ? 0 : -sqrt(w); }

	Rt3Quat& Add(const Rt3Quat& o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	Rt3Quat& Sub(const Rt3Quat& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	Rt3Quat& Mul(double k) { x *= k; y *= k; z *= k; w *= k; return *this; }
	// geom.h's `vec4::mul3`: scale the vector part and leave `w` alone, which
	// is what a tangent quaternion needs (its `w` is the bitangent's sign).
	Rt3Quat& Mul3(double k) { x *= k; y *= k; z *= k; return *this; }
	Rt3Quat& Madd(const Rt3Quat& o, double k) { x += o.x * k; y += o.y * k; z += o.z * k; w += o.w * k; return *this; }
	Rt3Quat& Msub(const Rt3Quat& o, double k) { x -= o.x * k; y -= o.y * k; z -= o.z * k; w -= o.w * k; return *this; }

	// geom.h's product: `this = p * o` (apply o, then p).
	Rt3Quat& Mul(const Rt3Quat& p, const Rt3Quat& o)
	{
		x = p.w * o.x + p.x * o.w + p.y * o.z - p.z * o.y;
		y = p.w * o.y - p.x * o.z + p.y * o.w + p.z * o.x;
		z = p.w * o.z + p.x * o.y - p.y * o.x + p.z * o.w;
		w = p.w * o.w - p.x * o.x - p.y * o.y - p.z * o.z;
		return *this;
	}
	Rt3Quat& Mul(const Rt3Quat& o) { Rt3Quat self = *this; return Mul(self, o); }

	Rt3Quat& Invert() { x = -x; y = -y; z = -z; return *this; }
	Rt3Quat& Neg() { x = -x; y = -y; z = -z; w = -w; return *this; }
	Rt3Quat& Normalize()
	{
		double invlen = 1 / Magnitude();
		return Mul(invlen);
	}

	// geom.h's `quat::rotate` is `2 * cross(q, cross(q, v) + w*v) + v`, the
	// branch-free Rodrigues form; `CrossQuat` is `vec3(q.xyz) x o`, which is
	// what `vec().cross(quat, o)` computes for the vector part.
	Rt3Vec3 CrossQuat(const Rt3Vec3& o) const
	{
		return Rt3Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
	}
	Rt3Vec3 Rotate(const Rt3Vec3& v) const
	{
		Rt3Vec3 t = CrossQuat(v);
		t.x += v.x * w; t.y += v.y * w; t.z += v.z * w;
		return CrossQuat(t).Mul(2).Add(v);
	}
	Rt3Vec3 InvertedRotate(const Rt3Vec3& v) const
	{
		Rt3Vec3 t = CrossQuat(v);
		t.x -= v.x * w; t.y -= v.y * w; t.z -= v.z * w;
		return CrossQuat(t).Mul(2).Add(v);
	}

	void CalcAngleAxis(double& angle, Rt3Vec3& axis) const
	{
		double rr = Dot3(*this);
		if(rr > 0)
		{
			angle = 2 * acos(clamp01(w));
			axis = Rt3Vec3(x, y, z).Mul(1 / rr);
		}
		else { angle = 0; axis = Rt3Vec3(0, 0, 1); }
	}

	// geom.h's `calcangles`: the three intrinsic-ish Euler angles, with the
	// gimbal-lock branch tested against `0.49999*rr`.
	Rt3Vec3 CalcAngles() const
	{
		double qx = x * x, qy = y * y, qz = z * z, qw = w * w,
		       rr = qx + qy + qz + qw,
		       t = x * y + z * w;
		if(Upp::abs(t) > 0.49999 * rr)
			return t < 0 ? Rt3Vec3(-2 * atan2(x, w), -M_PI / 2, 0)
			            : Rt3Vec3(2 * atan2(x, w), M_PI / 2, 0);
		return Rt3Vec3(atan2(2 * (y * w - x * z), qx - qy - qz + qw),
		               asin(2 * t / rr),
		               atan2(2 * (x * w - y * z), -qx + qy - qz + qw));
	}

	double Dot(const Rt3Quat& o) const { return x * o.x + y * o.y + z * o.z + w * o.w; }
	double Dot3(const Rt3Quat& o) const { return x * o.x + y * o.y + z * o.z; }
	double Magnitude() const { return sqrt(Dot(*this)); }
	double SquaredLength() const { return Dot(*this); }

	String ToString() const { return Sprintf("(%.3f, %.3f, %.3f, %.3f)", x, y, z, w); }

private:
	// geom.h clamps with a 2-argument clamp only where a range is known; the
	// acos here sees a unit quaternion's w, so a stray drift above 1 would
	// make acos return NaN - upstream has the same exposure, but clamping to
	// 1 keeps the data-core test from producing NaN.
	static double clamp01(double v) { return v < -1 ? -1 : v > 1 ? 1 : v; }
};

// geom.h's `dualquat`: a rotation quaternion plus the quaternion part of a
// translation, so a whole rigid transform multiplies and interpolates
// without ever building a matrix.
class Rt3DualQuat
{
public:
	Rt3Quat real, dual;

	Rt3DualQuat() {}
	Rt3DualQuat(const Rt3Quat& q, const Rt3Vec3& p) : real(q)
	{
		dual.x =  0.5 * ( p.x * q.w + p.y * q.z - p.z * q.y);
		dual.y =  0.5 * (-p.x * q.z + p.y * q.w + p.z * q.x);
		dual.z =  0.5 * ( p.x * q.y - p.y * q.x + p.z * q.w);
		dual.w = -0.5 * ( p.x * q.x + p.y * q.y + p.z * q.z);
	}
	explicit Rt3DualQuat(const Rt3Quat& q) : real(q), dual(0, 0, 0, 0) {}
	explicit Rt3DualQuat(const Rt3Matrix4x3& m);

	Rt3DualQuat& Mul(double k) { real.Mul(k); dual.Mul(k); return *this; }
	Rt3DualQuat& Add(const Rt3DualQuat& d) { real.Add(d.real); dual.Add(d.dual); return *this; }

	// geom.h: the shortest-arc blend, which flips `k` when the two reals are
	// antipodal so the interpolation does not take the long way round.
	Rt3DualQuat& Lerp(const Rt3DualQuat& to, double t)
	{
		double k = real.Dot(to.real) < 0 ? -t : t;
		real.Mul(1 - t).Madd(to.real, k);
		dual.Mul(1 - t).Madd(to.dual, k);
		return *this;
	}
	Rt3DualQuat& Lerp(const Rt3DualQuat& from, const Rt3DualQuat& to, double t)
	{
		double k = from.real.Dot(to.real) < 0 ? -t : t;
		real = from.real;
		real.Mul(1 - t).Madd(to.real, k);
		dual = from.dual;
		dual.Mul(1 - t).Madd(to.dual, k);
		return *this;
	}

	// geom.h's `invert` has a real quirk: the dual part is negated and then
	// the real*dot(real,dual) term is *subtracted*. Ported as written.
	Rt3DualQuat& Invert()
	{
		real.Invert();
		dual.Invert();
		dual.Msub(real, 2 * real.Dot(dual));
		return *this;
	}

	void Mul(const Rt3DualQuat& p, const Rt3DualQuat& o)
	{
		real.Mul(p.real, o.real);
		dual.Mul(p.real, o.dual).Add(Rt3Quat().Mul(p.dual, o.real));
	}
	void Mul(const Rt3DualQuat& o)
	{
		Rt3DualQuat self = *this;
		Mul(self, o);
	}

	// Conjugate a rotation out of the rigid part: `this = q * this * ~q`.
	// Upstream copies its own components before multiplying because the
	// product writes in place; the copies here are the same guard, and
	// dropping them would corrupt `y`/`z`/`w` from an already-updated `x`.
	void MulOrient(const Rt3Quat& q)
	{
		Rt3Quat r = real, d = dual, qi = q;
		real.Mul(q, r);
		dual.Mul(qi.Invert(), d);
	}

	// geom.h's two-argument `mulorient(q, base)`: remap this transform from
	// `base`'s bone space into `q`'s, keeping the translation consistent.
	void MulOrient(const Rt3Quat& q, const Rt3DualQuat& base)
	{
		Rt3Quat trans;
		Rt3Quat invbase = base.real;
		invbase.Invert();
		trans.Mul(base.dual, invbase);

		Rt3Quat tmp = real, qi = q;
		qi.Invert();
		dual.Mul(qi, Rt3Quat().Mul(tmp, trans).Add(dual));

		real.Mul(q, tmp);
		trans.Invert();
		dual.Add(Rt3Quat().Mul(real, trans)).Msub(real, 2 * base.real.Dot(base.dual));
	}

	void Normalize()
	{
		double invlen = 1 / real.Magnitude();
		real.Mul(invlen);
		dual.Mul(invlen);
	}

	void Translate(const Rt3Vec3& p)
	{
		dual.x +=  0.5 * ( p.x * real.w + p.y * real.z - p.z * real.y);
		dual.y +=  0.5 * (-p.x * real.z + p.y * real.w + p.z * real.x);
		dual.z +=  0.5 * ( p.x * real.y - p.y * real.x + p.z * real.w);
		dual.w += -0.5 * ( p.x * real.x + p.y * real.y + p.z * real.z);
	}

	// geom.h: same antipodal flip as `Lerp`, which is what makes summing many
	// poses (the animation blend) safe.
	void Accumulate(const Rt3DualQuat& d, double k)
	{
		if(real.Dot(d.real) < 0) k = -k;
		real.Madd(d.real, k);
		dual.Madd(d.dual, k);
	}

	void FixAntipodal(const Rt3DualQuat& d)
	{
		if(real.Dot(d.real) < 0) Negate();
	}

	void Negate() { real.Neg(); dual.Neg(); }

	Rt3Vec3 Transform(const Rt3Vec3& v) const
	{
		Rt3Vec3 t = real.CrossQuat(v);
		t.x += v.x * real.w; t.y += v.y * real.w; t.z += v.z * real.w;
		t.Add(Rt3Vec3(dual.x, dual.y, dual.z));
		Rt3Vec3 r = real.CrossQuat(t);
		r.Madd(Rt3Vec3(dual.x, dual.y, dual.z), real.w);
		r.Msub(Rt3Vec3(real.x, real.y, real.z), dual.w);
		return r.Mul(2).Add(v);
	}

	Rt3Quat Transform(const Rt3Quat& q) const { return Rt3Quat().Mul(real, q); }

	Rt3Vec3 TransposedTransform(const Rt3Vec3& v) const
	{
		Rt3DualQuat copy = *this;
		return copy.Invert().Transform(v);
	}

	// A dualquat's real part carries the rotation, so a direction transform
	// is just the rotation - this is what `calcdeviation` measures with.
	Rt3Vec3 TransformNormal(const Rt3Vec3& v) const { return real.Rotate(v); }
	Rt3Vec3 TransposedTransformNormal(const Rt3Vec3& v) const { return real.InvertedRotate(v); }

	Rt3Vec3 GetTranslation() const
	{
		Rt3Vec3 r = real.CrossQuat(Rt3Vec3(dual.x, dual.y, dual.z));
		r.Madd(Rt3Vec3(dual.x, dual.y, dual.z), real.w);
		r.Msub(Rt3Vec3(real.x, real.y, real.z), dual.w);
		return r.Mul(2);
	}
};

// geom.h's `matrix4x3`, reduced to what the skeleton data core and the model
// loaders use. The full 4x4, the projection members and `matrix3` are not
// ported - see Rt3D/AGENTS.md ("Deliberately out of scope").
class Rt3Matrix4x3
{
public:
	Rt3Vec3 a, b, c, d;

	Rt3Matrix4x3() {}
	Rt3Matrix4x3(const Rt3Vec3& a, const Rt3Vec3& b, const Rt3Vec3& c, const Rt3Vec3& d)
		: a(a), b(b), c(c), d(d) {}
	Rt3Matrix4x3(const Rt3DualQuat& dq)
	{
		// geom.h: `r` is the real part scaled to unit length and `rr` its
		// componentwise square, both taken *before* `r` is doubled - the
		// doubling is only for the off-diagonal terms below. The squaring is
		// `vec4::mul(vec4)`, which is componentwise; `quat::mul(quat)` hides
		// it with the quaternion product, so the components are spelled out.
		Rt3Quat r = dq.real;
		r.Mul(1 / r.SquaredLength());
		Rt3Quat rr;
		rr.x = r.x * dq.real.x;
		rr.y = r.y * dq.real.y;
		rr.z = r.z * dq.real.z;
		rr.w = r.w * dq.real.w;
		r.Mul(2);
		double xy = r.x * dq.real.y, xz = r.x * dq.real.z, yz = r.y * dq.real.z,
		       wx = r.w * dq.real.x, wy = r.w * dq.real.y, wz = r.w * dq.real.z;
		a = Rt3Vec3(rr.w + rr.x - rr.y - rr.z, xy + wz, xz - wy);
		b = Rt3Vec3(xy - wz, rr.w + rr.y - rr.x - rr.z, yz + wx);
		c = Rt3Vec3(xz + wy, yz - wx, rr.w + rr.z - rr.x - rr.y);
		d = Rt3Vec3(-(dq.dual.w * r.x - dq.dual.x * r.w + dq.dual.y * r.z - dq.dual.z * r.y),
		            -(dq.dual.w * r.y - dq.dual.x * r.z - dq.dual.y * r.w + dq.dual.z * r.x),
		            -(dq.dual.w * r.z + dq.dual.x * r.y - dq.dual.y * r.x - dq.dual.z * r.w));
	}

	void Identity()
	{
		a = Rt3Vec3(1, 0, 0);
		b = Rt3Vec3(0, 1, 0);
		c = Rt3Vec3(0, 0, 1);
		d = Rt3Vec3(0, 0, 0);
	}

	void SetTranslation(const Rt3Vec3& p) { d = p; }
	void SetTranslation(double x, double y, double z) { d = Rt3Vec3(x, y, z); }
	void SetScale(double x, double y, double z) { a.x = x; b.y = y; c.z = z; }
	void SetScale(const Rt3Vec3& v) { SetScale(v.x, v.y, v.z); }
	void SetScale(double n) { SetScale(n, n, n); }
	void Scale(double x, double y, double z) { a.Mul(x); b.Mul(y); c.Mul(z); }
	void Scale(const Rt3Vec3& v) { Scale(v.x, v.y, v.z); }
	void Scale(double n) { Scale(n, n, n); }

	void Translate(const Rt3Vec3& p) { d.Madd(a, p.x).Madd(b, p.y).Madd(c, p.z); }
	void Translate(double x, double y, double z) { Translate(Rt3Vec3(x, y, z)); }
	void PostTranslate(const Rt3Vec3& p) { d.Add(p); }
	void PostTranslate(double x, double y, double z) { PostTranslate(Rt3Vec3(x, y, z)); }

	void Accumulate(const Rt3Matrix4x3& m, double k)
	{
		a.Madd(m.a, k);
		b.Madd(m.b, k);
		c.Madd(m.c, k);
		d.Madd(m.d, k);
	}

	void Normalize()
	{
		a = a.Normalized();
		b = b.Normalized();
		c = c.Normalized();
	}

	void Lerp(const Rt3Matrix4x3& to, double t)
	{
		a = a.Lerp(to.a, t);
		b = b.Lerp(to.b, t);
		c = c.Lerp(to.c, t);
		d = d.Lerp(to.d, t);
	}
	void Lerp(const Rt3Matrix4x3& from, const Rt3Matrix4x3& to, double t)
	{
		a = from.a.Lerp(to.a, t);
		b = from.b.Lerp(to.b, t);
		c = from.c.Lerp(to.c, t);
		d = from.d.Lerp(to.d, t);
	}

	// geom.h's `mul(m, n)` computes `m * n` (apply n, then m). Upstream starts
	// each row from a copy (`vec(m.a).mul(...)`) because the row belongs to a
	// const operand; the copy is what the temporaries below are.
	void Mul(const Rt3Matrix4x3& m, const Rt3Matrix4x3& n)
	{
		a = Rt3Vec3(m.a).Mul(n.a.x).Madd(m.b, n.a.y).Madd(m.c, n.a.z);
		b = Rt3Vec3(m.a).Mul(n.b.x).Madd(m.b, n.b.y).Madd(m.c, n.b.z);
		c = Rt3Vec3(m.a).Mul(n.c.x).Madd(m.b, n.c.y).Madd(m.c, n.c.z);
		d = Rt3Vec3(m.d).Madd(m.a, n.d.x).Madd(m.b, n.d.y).Madd(m.c, n.d.z);
	}
	void Mul(const Rt3Matrix4x3& n)
	{
		Rt3Matrix4x3 self = *this;
		Mul(self, n);
	}

	void Transpose()
	{
		d = Rt3Vec3(a.Dot(d), b.Dot(d), c.Dot(d)).Mul(-1);
		double t;
		t = a.y; a.y = b.x; b.x = t;
		t = a.z; a.z = c.x; c.x = t;
		t = b.z; b.z = c.y; c.y = t;
	}
	void Transpose(const Rt3Matrix4x3& o)
	{
		a = Rt3Vec3(o.a.x, o.b.x, o.c.x);
		b = Rt3Vec3(o.a.y, o.b.y, o.c.y);
		c = Rt3Vec3(o.a.z, o.b.z, o.c.z);
		d = Rt3Vec3(o.a.Dot(o.d), o.b.Dot(o.d), o.c.Dot(o.d)).Mul(-1);
	}

	// geom.h's `transposemul(m, n)` = `m^T * n`.
	void TransposeMul(const Rt3Matrix4x3& m, const Rt3Matrix4x3& n)
	{
		Rt3Vec3 t(m.a.Dot(m.d), m.b.Dot(m.d), m.c.Dot(m.d));
		a = Rt3Vec3(m.a.Dot(n.a), m.b.Dot(n.a), m.c.Dot(n.a));
		b = Rt3Vec3(m.a.Dot(n.b), m.b.Dot(n.b), m.c.Dot(n.b));
		c = Rt3Vec3(m.a.Dot(n.c), m.b.Dot(n.c), m.c.Dot(n.c));
		d = Rt3Vec3(m.a.Dot(n.d), m.b.Dot(n.d), m.c.Dot(n.d)) - t;
	}

	// geom.h's `multranspose(m, n)` = `m * n^T`.
	void MulTranspose(const Rt3Matrix4x3& m, const Rt3Matrix4x3& n)
	{
		Rt3Vec3 t(n.a.Dot(n.d), n.b.Dot(n.d), n.c.Dot(n.d));
		a = Rt3Vec3(m.a).Mul(n.a.x).Madd(m.b, n.b.x).Madd(m.c, n.c.x);
		b = Rt3Vec3(m.a).Mul(n.a.y).Madd(m.b, n.b.y).Madd(m.c, n.c.y);
		c = Rt3Vec3(m.a).Mul(n.a.z).Madd(m.b, n.b.z).Madd(m.c, n.c.z);
		d = Rt3Vec3(m.d).Msub(m.a, t.x).Msub(m.b, t.y).Msub(m.c, t.z);
	}

	// geom.h's `invert(o)`: transpose and undo the per-row scale, which is
	// all an orthonormal rotation matrix needs.
	void Invert(const Rt3Matrix4x3& o)
	{
		Rt3Vec3 unscale(1 / o.a.SquaredLength(), 1 / o.b.SquaredLength(), 1 / o.c.SquaredLength());
		Transpose(o);
		a.Mul(unscale);
		b.Mul(unscale);
		c.Mul(unscale);
		d.Mul(unscale);
	}
	void Invert()
	{
		Rt3Matrix4x3 self = *this;
		Invert(self);
	}

	Rt3Vec3 Transform(const Rt3Vec3& o) const
	{
		Rt3Vec3 r = d;
		return r.Madd(a, o.x).Madd(b, o.y).Madd(c, o.z);
	}
	Rt3Vec3 TransposedTransform(const Rt3Vec3& o) const
	{
		Rt3Vec3 p = o - d;
		return Rt3Vec3(a.Dot(p), b.Dot(p), c.Dot(p));
	}
	Rt3Vec3 TransformNormal(const Rt3Vec3& o) const
	{
		Rt3Vec3 r = a;
		return r.Mul(o.x).Madd(b, o.y).Madd(c, o.z);
	}
	Rt3Vec3 TransposedTransformNormal(const Rt3Vec3& o) const { return Rt3Vec3(a.Dot(o), b.Dot(o), c.Dot(o)); }
};

inline Rt3Quat::Rt3Quat(const Rt3Matrix4x3& m) { ConvertMatrix(m); }

// geom.h's `quat::convertmatrix`: Shepperd's method - pick the largest of
// the four candidate divisors so the square root stays well conditioned.
inline void Rt3Quat::ConvertMatrix(const Rt3Matrix4x3& m)
{
	double trace = m.a.x + m.b.y + m.c.z;
	if(trace > 0)
	{
		double r = sqrt(1 + trace), inv = 0.5 / r;
		w = 0.5 * r;
		x = (m.b.z - m.c.y) * inv;
		y = (m.c.x - m.a.z) * inv;
		z = (m.a.y - m.b.x) * inv;
	}
	else if(m.a.x > m.b.y && m.a.x > m.c.z)
	{
		double r = sqrt(1 + m.a.x - m.b.y - m.c.z), inv = 0.5 / r;
		x = 0.5 * r;
		y = (m.a.y + m.b.x) * inv;
		z = (m.c.x + m.a.z) * inv;
		w = (m.b.z - m.c.y) * inv;
	}
	else if(m.b.y > m.c.z)
	{
		double r = sqrt(1 + m.b.y - m.a.x - m.c.z), inv = 0.5 / r;
		x = (m.a.y + m.b.x) * inv;
		y = 0.5 * r;
		z = (m.b.z + m.c.y) * inv;
		w = (m.c.x - m.a.z) * inv;
	}
	else
	{
		double r = sqrt(1 + m.c.z - m.a.x - m.b.y), inv = 0.5 / r;
		x = (m.c.x + m.a.z) * inv;
		y = (m.b.z + m.c.y) * inv;
		z = 0.5 * r;
		w = (m.a.y - m.b.x) * inv;
	}
}

// geom.h's inline `dualquat::dualquat(const matrix4x3 &m)`, with the
// `real(m)` sub-ctor resolving to `Rt3Quat(Rt3Matrix4x3)`.
inline Rt3DualQuat::Rt3DualQuat(const Rt3Matrix4x3& m) : real(m)
{
	dual.x =  0.5 * ( m.d.x * real.w + m.d.y * real.z - m.d.z * real.y);
	dual.y =  0.5 * (-m.d.x * real.z + m.d.y * real.w + m.d.z * real.x);
	dual.z =  0.5 * ( m.d.x * real.y - m.d.y * real.x + m.d.z * real.w);
	dual.w = -0.5 * ( m.d.x * real.x + m.d.y * real.y + m.d.z * real.z);
}

#endif
