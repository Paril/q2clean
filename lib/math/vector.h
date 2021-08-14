#pragma once

#include <exception>

#include "lib/math.h"
#include "lib/types.h"

// thrown on an invalid vector dereference
class vector_out_of_range : public std::exception
{
public:
	vector_out_of_range() :
		std::exception("attempted to fetch out-of-range element on a vector", 1)
	{
	}
};

template<typename T>
concept is_numeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

struct vector;

// the vector type represents a 3d float array. this is designed for maximum
// compatibility with QC, with the exception of member functions designed to
// make using the class simpler/more functional. this supports most modern QC-style
// accessors (.x/.y/.z/[0]/[1]/[2]); _x/_y/_z simply need to be re-named to .x/.y/.z.
struct vector
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
	template<is_numeric T>
	constexpr vector operator*(const T &rhs) const { return { (float) (x * rhs), (float) (y * rhs), (float) (z * rhs) }; }
	template<is_numeric T>
	constexpr vector operator/(const T &rhs) const { return { (float) (x / rhs), (float) (y / rhs), float (z / rhs) }; }

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

	template<is_numeric T>
	vector &operator*=(const T &rhs) { return *this = *this * rhs; }
	template<is_numeric T>
	vector &operator/=(const T &rhs) { return *this = *this / rhs; }
	vector &operator+=(const vector &rhs) { return *this = *this + rhs; }
	vector &operator-=(const vector &rhs) { return *this = *this - rhs; }

	// equality

	constexpr bool operator==(const vector &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	constexpr bool operator!=(const vector &rhs) const { return x != rhs.x && y != rhs.y && z != rhs.z; }

	constexpr explicit operator bool() const { return x || y || z; }

	// member functions; global versions are provided too
	// fetch the squared length of this vector
	[[nodiscard]] constexpr float length_squared() const
	{
		return *this * *this;
	}

	// fetch the length of this vector
	[[nodiscard]] inline float length() const
	{
		return sqrt(length_squared());
	}

	// normalize this vector and return its old length
	inline float normalize()
	{
		float length = length_squared();

		if (length)
		{
			length = sqrt(length);
			*this *= 1.f / length;
		}

		return length;
	}

	// fetch the squared distance between this and rhs
	[[nodiscard]] constexpr float distance_squared(const vector &rhs) const
	{
		return (*this - rhs).length_squared();
	}

	// fetch the distance between this and rhs
	[[nodiscard]] inline float distance(const vector &rhs) const
	{
		return (*this - rhs).length();
	}
};

template<is_numeric T>
constexpr vector operator*(const T &lhs, const vector &rhs) { return { (float) (lhs * rhs.x), (float) (lhs * rhs.y), (float) (lhs * rhs.z) }; }
template<is_numeric T>
constexpr vector operator/(const T &lhs, const vector &rhs) { return { (float) (lhs / rhs.x), (float) (lhs / rhs.y), (float) (lhs / rhs.z) }; }

// the origin point (all zeroes)
constexpr vector vec3_origin { 0, 0, 0 };

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
	return lhs.length_squared();
}

// fetch the squared length of lhs
[[nodiscard]] inline float VectorLength(const vector &lhs)
{
	return lhs.length();
}

// fetch the squared distance between lhs and rhs
[[nodiscard]] constexpr float VectorDistanceSquared(const vector &lhs, const vector &rhs)
{
	return lhs.distance_squared(rhs);
}

// fetch the distance between lhs and rhs
[[nodiscard]] inline float VectorDistance(const vector &lhs, const vector &rhs)
{
	return lhs.distance(rhs);
}

// normalize lhs and return its old length
inline float VectorNormalize(vector &lhs)
{
	return lhs.normalize();
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
concept is_vecdir_pointer_or_null_pointer = std::is_same_v<T, vector *> || std::is_null_pointer_v<T>;

// convert Euler angles to vectors
template<is_vecdir_pointer_or_null_pointer TF, is_vecdir_pointer_or_null_pointer TR, is_vecdir_pointer_or_null_pointer TU>
inline void AngleVectors(const vector &angles, TF forward, TR right, TU up)
{
	float angle = deg2rad(angles[YAW]);
	float sy = sin(angle);
	float cy = cos(angle);
	angle = deg2rad(angles[PITCH]);
	float sp = sin(angle);
	float cp = cos(angle);

	if constexpr (!std::is_null_pointer_v<TF>)
		*forward = { cp * cy, cp * sy, -sp };

	if constexpr (!std::is_null_pointer_v<TR> || !std::is_null_pointer_v<TU>)
	{
		angle = deg2rad(angles[ROLL]);
		float sr = sin(angle);
		float cr = cos(angle);

		if constexpr (!std::is_null_pointer_v<TR>)
			*right = {
				(-1 * sr * sp * cy + -1 * cr * -sy),
				(-1 * sr * sp * sy + -1 * cr * cy),
				-1 * sr * cp
		};

		if constexpr (!std::is_null_pointer_v<TU>)
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
	return { v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x };
}