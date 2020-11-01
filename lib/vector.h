#pragma once

#include "types.h"

// thrown on an invalid vector dereference
class vector_out_of_range : public std::exception
{
public:
	vector_out_of_range() :
		std::exception("attempted to fetch out-of-range element on a vector", 1)
	{
	}
};

// the vector type represents a 3d float array. this is designed for maximum
// compatibility with QC, with the exception of member functions designed to
// make using the class simpler/more functional. this supports most modern QC-style
// accessors (.x/.y/.z/[0]/[1]/[2]); _x/_y/_z simply need to be re-named to .x/.y/.z.
extern "C" struct vector
{
	// number of elements in a vector, just for reference sake.
	static constexpr size_t size = 3;

	// the elements
	float x, y, z;

	// support for iteration
	
	constexpr float *begin() { return &x; }
	constexpr float *end() { return &x + size; }
	constexpr const float *begin() const { return &x; }
	constexpr const float *end() const { return &x + size; }

	// support for indexing
	
	constexpr float &operator[](const size_t &index) { return index < size ? *((&x) + index) : (throw vector_out_of_range()); }
	constexpr const float &operator[](const size_t &index) const { return index < size ? *((&x) + index) : (throw vector_out_of_range()); }

	// QuakeC operators

	// dot product - *not* channel-wise scale!
	constexpr float operator*(const vector &rhs) const { return (x * rhs.x) + (y * rhs.y) + (z * rhs.z); }

	// channel-wise value operators
	
	constexpr vector operator*(const float &rhs) const { return { x * rhs, y * rhs, z * rhs }; }
	constexpr vector operator*(const int32_t &rhs) const { return { x * rhs, y * rhs, z * rhs }; }
	constexpr vector operator/(const float &rhs) const { return { x / rhs, y / rhs, z / rhs }; }
	constexpr vector operator/(const int32_t &rhs) const { return { x / rhs, y / rhs, z / rhs }; }
	
	constexpr vector operator+(const vector &rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
	constexpr vector operator-(const vector &rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z }; }

	constexpr vector operator-() const { return { -x, -y, -z }; }

	using callback_func = float(*)(float);

	inline vector each(callback_func f)
	{
		return {
			f(x),
			f(y),
			f(z)
		};
	}

	// channel-wise self operators
	
	vector &operator*=(const float &rhs) { return *this = *this * rhs; }
	vector &operator*=(const int32_t &rhs) { return *this = *this * rhs; }
	vector &operator/=(const float &rhs) { return *this = *this / rhs; }
	vector &operator/=(const int32_t &rhs) { return *this = *this / rhs; }
	vector &operator+=(const vector &rhs) { return *this = *this + rhs; }
	vector &operator-=(const vector &rhs) { return *this = *this - rhs; }

	// equality

	constexpr bool operator==(const vector &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	constexpr bool operator!=(const vector &rhs) const { return x != rhs.x && y != rhs.y && z != rhs.z; }

	constexpr operator bool() const { return x || y || z; }

	// member functions; global versions are provided too

	// fetch the squared length of this vector
	[[nodiscard]] constexpr float LengthSquared() const
	{
		return *this * *this;
	}

	// fetch the length of this vector
	[[nodiscard]] inline float Length() const
	{
		return sqrt(LengthSquared());
	}

	// fetch the squared distance between this and rhs
	[[nodiscard]] constexpr float DistanceSquared(const vector &rhs) const
	{
		return (*this - rhs).LengthSquared();
	}

	// fetch the distance between this and rhs
	[[nodiscard]] inline float Distance(const vector &rhs) const
	{
		return (*this - rhs).Length();
	}

	// normalize this vector and return its old length
	inline float Normalize()
	{
		float length = LengthSquared();

		if (length)
		{
			length = sqrt(length);
			*this *= 1.f / length;
		}

		return length;
	}
};

// the origin point (all zeroes)
static constexpr vector vec3_origin { 0, 0, 0 };

// globals for pitch/yaw/roll
enum : size_t
{
	PITCH,
	YAW,
	ROLL
};

// fetch the squared length of lhs
[[nodiscard]] constexpr float VectorLengthSquared(const vector &lhs)
{
	return lhs.LengthSquared();
}

// fetch the squared length of lhs
[[nodiscard]] inline float VectorLength(const vector &lhs)
{
	return lhs.Length();
}

// fetch the squared distance between lhs and rhs
[[nodiscard]] constexpr float VectorDistanceSquared(const vector &lhs, const vector &rhs)
{
	return lhs.DistanceSquared(rhs);
}

// fetch the distance between lhs and rhs
[[nodiscard]] inline float VectorDistance(const vector &lhs, const vector &rhs)
{
	return lhs.Distance(rhs);
}

// normalize lhs and return its old length
inline float VectorNormalize(vector &lhs)
{
	return lhs.Normalize();
}

// This constant is used for how steep a ground plane is for
// bouncing things.
constexpr float STOP_EPSILON = 0.1f;

// Slide off of the impacting object
[[nodiscard]] constexpr vector ClipVelocity(const vector &in, const vector &normal, const float &overbounce)
{
	const float backoff = (in * normal) * overbounce;
	vector out = in - (normal * backoff);

	for (float &v : out)
		if (v > -STOP_EPSILON && v < STOP_EPSILON)
			v = 0;
	
	return out;
}

template<typename T>
constexpr bool is_vector_pointer_or_null_pointer_v = std::is_same_v<T, vector*> || std::is_null_pointer_v<T>;

// convert Euler angles to vectors
template<typename TF, typename TR, typename TU, typename = std::enable_if_t<is_vector_pointer_or_null_pointer_v<TF> && is_vector_pointer_or_null_pointer_v<TR> && is_vector_pointer_or_null_pointer_v<TU>>>
inline void AngleVectors(const vector &angles, TF forward, TR right, TU up)
{
	float angle = deg2rad(angles[YAW]);
	float sy = sin(angle);
	float cy = cos(angle);
	angle = deg2rad(angles[PITCH]);
	float sp = sin(angle);
	float cp = cos(angle);

	if constexpr(!std::is_null_pointer_v<TF>)
		*forward = { cp * cy, cp * sy, -sp };
	
	if constexpr(!std::is_null_pointer_v<TR> || !std::is_null_pointer_v<TU>)
	{
		angle = deg2rad(angles[ROLL]);
		float sr = sin(angle);
		float cr = cos(angle);

		if constexpr(!std::is_null_pointer_v<TR>)
			*right = {
				(-1 * sr * sp * cy + -1 * cr * -sy),
				(-1 * sr * sp * sy + -1 * cr * cy),
				-1 * sr * cp
			};

		if constexpr(!std::is_null_pointer_v<TU>)
			*up = {
				(cr * sp * cy + -sr * -sy),
				(cr * sp * sy + -sr * cy),
				cr * cp
			};
	}
}

// convert a vector to Euler angles
[[nodiscard]] inline vector vectoangles(const vector &value1)
{
	if (value1.y == 0 && value1.x == 0)
	{
		if (value1.z > 0)
			return { -90.f, 0, 0 };

		return { -270.f, 0, 0 };
	}

	float yaw;

	if (value1.x)
	{
		yaw = rad2deg(atan2(value1.y, value1.x));

		if (yaw < 0)
			yaw += 360;
	}
	else if (value1.y > 0)
		yaw = 90.f;
	else
		yaw = 270.f;

	const float forward = sqrt(value1.x * value1.x + value1.y * value1.y);
	float pitch = rad2deg(atan2(value1.z, forward));
	if (pitch < 0)
		pitch += 360;

	return { -pitch, yaw, 0 };
}

// convert a vector to an Euler yaw
[[nodiscard]] inline float vectoyaw(const vector &vec)
{
	if (vec.x == 0)
	{
		if (vec.y > 0)
			return 90.f;
		else if (vec.y < 0)
			return -90.f;

		return 0;
	}

	float yaw = rad2deg(atan2(vec.y, vec.x));

	if (yaw < 0)
		yaw += 360.f;

	return yaw;
}

[[nodiscard]] constexpr vector CrossProduct(const vector &v1, const vector &v2)
{
	return {v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x};
}

constexpr void AddPointToBounds(const vector &v, vector &mins, vector &maxs)
{
	mins[0] = min(mins[0], v[0]);
	maxs[0] = max(maxs[0], v[0]);
	mins[1] = min(mins[1], v[1]);
	maxs[1] = max(maxs[1], v[1]);
	mins[2] = min(mins[2], v[2]);
	maxs[2] = max(maxs[2], v[2]);
}

constexpr static vector operator*(const float &lhs, const vector &rhs) { return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z }; }
constexpr static vector operator*(const int32_t &lhs, const vector &rhs) { return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z }; }
constexpr static vector operator/(const float &lhs, const vector &rhs) { return { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z }; }
constexpr static vector operator/(const int32_t &lhs, const vector &rhs) { return { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z }; }
