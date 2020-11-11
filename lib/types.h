#pragma once

#include "../game/config.h"

// Q2++ uses explicitly-typed integers so that there's no ambiguity as to the
// meaning of any sizes.
#include <cinttypes>
// cstddef used for nullptr
#include <cstddef>
// Q2++ uses exceptions
#include <exception>
// array is used for some built-in stuff
#include <array>
// expose as array globally
using std::array;
// varargs are used in a few places
#include <cstdarg>

// Math!
#include "math.h"

// Allocator for STL containers
#include "allocator.h"

// type name for a string literal
using stringlit = const char *;

// Strings are used in a lot of places
#include "string.h"

// allows lengthof(array) for C-style arrays
template<typename T, size_t S>
inline size_t lengthof(T (&)[S]) { return S; }

// Time is stored as an unsigned 64-bit integer
using gtime = uint64_t;

// certain enums used in the code are bitwise. this allows bitwise
// operators to work. stupid but functional.
#define MAKE_ENUM_BITWISE_R(enumtype, prefix) \
	prefix constexpr enumtype operator|(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l | (std::underlying_type<enumtype>::type)r); } \
	prefix constexpr enumtype operator&(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l & (std::underlying_type<enumtype>::type)r); } \
	prefix constexpr enumtype operator^(const enumtype &l, const enumtype &r) { return (enumtype)((std::underlying_type<enumtype>::type)l ^ (std::underlying_type<enumtype>::type)r); } \
	prefix constexpr enumtype operator~(const enumtype &v) { return (enumtype)(~(std::underlying_type<enumtype>::type)v); } \
	prefix constexpr enumtype &operator|=(enumtype &l, const enumtype &r) { return l = l | r; } \
	prefix constexpr enumtype &operator&=(enumtype &l, const enumtype &r) { return l = l & r; } \
	prefix constexpr enumtype &operator^=(enumtype &l, const enumtype &r) { return l = l ^ r; }

#define MAKE_ENUM_BITWISE(enumtype) \
	MAKE_ENUM_BITWISE_R(enumtype, )

#define MAKE_ENUM_BITWISE_EXPORT(enumtype) \
	MAKE_ENUM_BITWISE_R(enumtype, export)

// default server FPS
constexpr gtime		BASE_FRAMERATE		= 10;
// the amount of time, in s, that a single frame lasts for
constexpr float		BASE_1_FRAMETIME	= 1.0f / BASE_FRAMERATE;
// the amount of time, in ms, that a single frame lasts for
constexpr gtime		BASE_FRAMETIME		= (gtime)(BASE_1_FRAMETIME * 1000);
// the amount of time, in s, that a single frame lasts for
constexpr float		BASE_FRAMETIME_1000	= BASE_1_FRAMETIME;
// the amount of time, in s, that a single frame lasts for
constexpr float		FRAMETIME			= BASE_1_FRAMETIME;

constexpr stringlit GAMEVERSION = "clean";

// extended features
enum game_features : uint32_t
{
	GMF_CLIENTNUM				= 1 << 0,
	GMF_PROPERINUSE				= 1 << 1,
	GMF_MVDSPEC					= 1 << 2,
	GMF_WANT_ALL_DISCONNECTS	= 1 << 3,
	
	GMF_ENHANCED_SAVEGAMES		= 1 << 10,
	GMF_VARIABLE_FPS			= 1 << 11,
	GMF_EXTRA_USERINFO			= 1 << 12,
	GMF_IPV6_ADDRESS_AWARE		= 1 << 13
};

MAKE_ENUM_BITWISE(game_features);

// features this game supports
constexpr game_features G_FEATURES = (GMF_PROPERINUSE | GMF_WANT_ALL_DISCONNECTS | GMF_ENHANCED_SAVEGAMES);

// Q2's engine uses an int32-wide value to represent booleans.
using qboolean = int32_t;

// more interop stuff

constexpr float coord2short = 8.f;
constexpr float angle2short = 65536.f / 360.f;
constexpr float short2coord = 1.0f / 8;
constexpr float short2angle = 360.0f / 65536;

// an opaque index that is a BSP area
enum class area_index : int32_t { };

// lower bits are stronger, and will eat weaker brushes completely
enum content_flags : uint32_t
{
	CONTENTS_NONE			= 0,
	// an eye is never valid in a solid
	CONTENTS_SOLID			= 1 << 0,
	// translucent, but not watery
	CONTENTS_WINDOW			= 1 << 1,
	CONTENTS_AUX			= 1 << 2,
	CONTENTS_LAVA			= 1 << 3,
	CONTENTS_SLIME			= 1 << 4,
	CONTENTS_WATER			= 1 << 5,
	CONTENTS_MIST			= 1 << 6,
	LAST_VISIBLE_CONTENTS	= CONTENTS_MIST,

	// remaining contents are non-visible, and don't eat brushes

	CONTENTS_AREAPORTAL		= 1 << 15,

	CONTENTS_PLAYERCLIP		= 1 << 16,
	CONTENTS_MONSTERCLIP	= 1 << 17,

	// currents can be added to any other contents, and may be mixed

	CONTENTS_CURRENT_0		= 1 << 18,
	CONTENTS_CURRENT_90		= 1 << 19,
	CONTENTS_CURRENT_180	= 1 << 20,
	CONTENTS_CURRENT_270	= 1 << 21,
	CONTENTS_CURRENT_UP		= 1 << 22,
	CONTENTS_CURRENT_DOWN	= 1 << 23,
	
	// removed before bsping an entity
	CONTENTS_ORIGIN			= 1 << 24,

	// should never be on a brush, only in game
	CONTENTS_MONSTER		= 1 << 25,
	CONTENTS_DEADMONSTER	= 1 << 26,
	// brushes to be added after vis leafs
	CONTENTS_DETAIL			= 1 << 27,
	// auto set if any surface has translucency
	CONTENTS_TRANSLUCENT	= 1 << 28,
	CONTENTS_LADDER			= 1 << 29,

	MASK_ALL				= (content_flags)-1,

	// content masks

	MASK_SOLID			= CONTENTS_SOLID | CONTENTS_WINDOW,
	MASK_PLAYERSOLID	= CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER,
	MASK_DEADSOLID		= CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW,
	MASK_MONSTERSOLID	= CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER,
	MASK_WATER			= CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME,
	MASK_OPAQUE			= CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA,
	MASK_SHOT			= CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_WINDOW | CONTENTS_DEADMONSTER,
	MASK_CURRENT		= CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN
};

MAKE_ENUM_BITWISE(content_flags);

// forward declarations

struct client;
struct entity;

#include "entityref.h"

// special thin types for indexes
class sound_index
{
private:
	int32_t index;

public:
	constexpr sound_index() : index(0) { }
	constexpr explicit sound_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() { return (bool)index; }
	constexpr explicit operator int32_t() { return index; }
	constexpr bool operator==(const sound_index &r) { return index == r.index; }
	constexpr bool operator!=(const sound_index &r) { return index != r.index; }
};

constexpr sound_index SOUND_NONE = sound_index(0);

class model_index
{
private:
	int32_t index;

public:
	constexpr model_index() : index(0) { }
	constexpr explicit model_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() { return (bool)index; }
	constexpr explicit operator int32_t() { return index; }
	constexpr bool operator==(const model_index &r) { return index == r.index; }
	constexpr bool operator!=(const model_index &r) { return index != r.index; }
};

constexpr model_index MODEL_NONE = model_index(0);
constexpr model_index MODEL_WORLD = model_index(1);
constexpr model_index MODEL_PLAYER = model_index(255);

class image_index
{
private:
	int32_t index;

public:
	constexpr image_index() : index(0) { }
	constexpr explicit image_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() const { return (bool)index; }
	constexpr explicit operator int32_t() const { return index; }
	constexpr bool operator==(const image_index &r) { return index == r.index; }
	constexpr bool operator!=(const image_index &r) { return index != r.index; }
};

constexpr image_index IMAGE_NONE = image_index(0);

#include "random.h"