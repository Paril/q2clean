#pragma once

import "config.h";
import <exception>;
import "lib/string.h";
import "lib/math/vector.h";
import "lib/types/array.h";
import "lib/types/enum.h";
import "game/entity_types.h";

// Q2's engine uses an int32-wide value to represent booleans.
using qboolean = int32_t;

// coordinate/angle compression interop
constexpr float coord2short = 8.f;
constexpr float angle2short = 65536.f / 360.f;
constexpr float short2coord = 1.0f / 8;
constexpr float short2angle = 360.0f / 65536;

// an opaque index that is a BSP area
enum class area_index : int32_t { };

// lower bits are stronger, and will eat weaker brushes completely
enum content_flags : uint32_t
{
	CONTENTS_NONE = 0,
	// an eye is never valid in a solid
	CONTENTS_SOLID = 1 << 0,
	// translucent, but not watery
	CONTENTS_WINDOW = 1 << 1,
	CONTENTS_AUX = 1 << 2,
	CONTENTS_LAVA = 1 << 3,
	CONTENTS_SLIME = 1 << 4,
	CONTENTS_WATER = 1 << 5,
	CONTENTS_MIST = 1 << 6,
	LAST_VISIBLE_CONTENTS = CONTENTS_MIST,

	// remaining contents are non-visible, and don't eat brushes

	CONTENTS_AREAPORTAL = 1 << 15,

	CONTENTS_PLAYERCLIP = 1 << 16,
	CONTENTS_MONSTERCLIP = 1 << 17,

	// currents can be added to any other contents, and may be mixed

	CONTENTS_CURRENT_0 = 1 << 18,
	CONTENTS_CURRENT_90 = 1 << 19,
	CONTENTS_CURRENT_180 = 1 << 20,
	CONTENTS_CURRENT_270 = 1 << 21,
	CONTENTS_CURRENT_UP = 1 << 22,
	CONTENTS_CURRENT_DOWN = 1 << 23,

	// removed before bsping an entity
	CONTENTS_ORIGIN = 1 << 24,

	// should never be on a brush, only in game
	CONTENTS_MONSTER = 1 << 25,
	CONTENTS_DEADMONSTER = 1 << 26,
	// brushes to be added after vis leafs
	CONTENTS_DETAIL = 1 << 27,
	// auto set if any surface has translucency
	CONTENTS_TRANSLUCENT = 1 << 28,
	CONTENTS_LADDER = 1 << 29,

	MASK_ALL = (content_flags) -1,

	// content masks

	MASK_SOLID = CONTENTS_SOLID | CONTENTS_WINDOW,
	MASK_PLAYERSOLID = CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER,
	MASK_DEADSOLID = CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW,
	MASK_MONSTERSOLID = CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER,
	MASK_WATER = CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME,
	MASK_OPAQUE = CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA,
	MASK_SHOT = CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_WINDOW | CONTENTS_DEADMONSTER,
	MASK_CURRENT = CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN
};

MAKE_ENUM_BITWISE(content_flags, export);

// special thin types for indexes
class sound_index
{
private:
	int32_t index;

public:
	constexpr sound_index() : index(0) { }
	constexpr explicit sound_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() const { return (bool) index; }
	constexpr explicit operator const int32_t &() const { return index; }
	constexpr bool operator==(const sound_index &r) { return index == r.index; }
	constexpr bool operator!=(const sound_index &r) { return index != r.index; }
};

constexpr sound_index SOUND_NONE(0);

class model_index
{
private:
	int32_t index;

public:
	constexpr model_index() : index(0) { }
	constexpr explicit model_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() const { return (bool) index; }
	constexpr explicit operator const int32_t &() const { return index; }
	constexpr bool operator==(const model_index &r) { return index == r.index; }
	constexpr bool operator!=(const model_index &r) { return index != r.index; }
};

constexpr model_index MODEL_NONE(0);
constexpr model_index MODEL_WORLD(1);
constexpr model_index MODEL_PLAYER(255);

class image_index
{
private:
	int32_t index;

public:
	constexpr image_index() : index(0) { }
	constexpr explicit image_index(const int32_t &value) : index(value) { }
	constexpr explicit operator bool() const { return (bool) index; }
	constexpr explicit operator const int32_t &() const { return index; }
	constexpr bool operator==(const image_index &r) { return index == r.index; }
	constexpr bool operator!=(const image_index &r) { return index != r.index; }
};

constexpr image_index IMAGE_NONE(0);

// pmove_state_t is the information necessary for client side movement
// prediction
enum pmtype : int32_t
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,     // different bounding box
	PM_FREEZE
};

// pmove->pm_flags
enum pmflags : uint8_t
{
	PMF_NONE,
	PMF_DUCKED = 1 << 0,
	PMF_JUMP_HELD = 1 << 1,
	PMF_ON_GROUND = 1 << 2,
	// pm_time is waterjump
	PMF_TIME_WATERJUMP = 1 << 3,
	// pm_time is time before rejump
	PMF_TIME_LAND = 1 << 4,
	// pm_time is non-moving time
	PMF_TIME_TELEPORT = 1 << 5,
	// temporarily disables prediction (used for grappling hook)
	PMF_NO_PREDICTION = 1 << 6,
	// used by q2pro
	PMF_TELEPORT_BIT = 1 << 7
};

MAKE_ENUM_BITWISE(pmflags, export);

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
extern "C" struct pmove_state
{
	pmtype	pm_type;

private:
#ifdef KMQUAKE2_ENGINE_MOD
	array<int32_t, 3>	origin;		// 12.3
#else
	array<int16_t, 3>	origin;		// 12.3
#endif
	array<int16_t, 3>	velocity;	// 12.3
public:
	void set_origin(const vector &v);
	vector get_origin() const;
	void set_velocity(const vector &v);
	vector get_velocity() const;

	// ducked, jump_held, etc
	pmflags	pm_flags;
	// each unit = 8 ms
	uint8_t	pm_time;
	int16_t	gravity;

private:
	// add to command angles to get view direction
	// changed by spawns, rotating objects, and teleporters
	array<int16_t, 3>	delta_angles;

public:
	void set_delta_angles(const vector &v);
	vector get_delta_angles() const;
};

enum cvar_flags : int32_t
{
	CVAR_NONE,
	// set to cause it to be saved to config.cfg
	CVAR_ARCHIVE = 1 << 0,
	// added to userinfo  when changed
	CVAR_USERINFO = 1 << 1,
	// added to serverinfo when changed
	CVAR_SERVERINFO = 1 << 2,
	// don't allow change from console at all,
	// but can be set from the command line
	CVAR_NOSET = 1 << 3,
	// save changes until server restart
	CVAR_LATCH = 1 << 4
};

MAKE_ENUM_BITWISE(cvar_flags, export);

// console variables
struct cvar
{
	cvar() = delete;

	stringlit	name;
	stringlit	string;
	// for CVAR_LATCH vars, this is the current value
	stringlit	latched_string;
	const cvar_flags	flags;
	// set each time the cvar is changed
	qboolean			modified;
	const float			value;
};

// a wrapper for cvar that can perform conversions automatically.
struct cvarref
{
private:
	cvar *cv;
	static constexpr cvar_flags empty_flags = CVAR_NONE;
	static qboolean empty_modified;
	static constexpr float empty_value = 0.f;

public:
	stringlit			name;
	stringlit			string;
	stringlit			latched_string;
	const cvar_flags &flags;
	qboolean &modified;
	const float &value;

	cvarref() :
		cv(nullptr),
		name(nullptr),
		string(nullptr),
		latched_string(nullptr),
		flags(empty_flags),
		modified(empty_modified),
		value(empty_value)
	{
	}

	cvarref(nullptr_t) :
		cvarref()
	{
	}

	cvarref(cvar &v) :
		cv(&v),
		name(v.name),
		string(v.string),
		latched_string(v.latched_string),
		flags(v.flags),
		modified(v.modified),
		value(v.value)
	{
	}

	cvarref &operator=(const cvarref &tr)
	{
		new(this) cvarref(tr);
		return *this;
	}

	// string literal conversion
	inline explicit operator stringlit() const
	{
		return string;
	}

	// bool conversion catches empty strings and "0" as false, everything else
	// as true
	inline explicit operator bool() const
	{
		return *string && !(string[0] == '0' && string[1] == '\0');
	}

	// everything else is templated
	template<typename T>
	inline explicit operator T() const
	{
		return (T) value;
	}

	inline bool operator==(stringlit lit) const { return (stringref) string == lit; }
	inline bool operator!=(stringlit lit) const { return (stringref) string != lit; }

	inline bool operator==(const stringref &ref) const { return ref == string; }
	inline bool operator!=(const stringref &ref) const { return ref != string; }
};

// print level
enum print_level : uint8_t
{
	// pickup messages
	PRINT_LOW,
	// death messages
	PRINT_MEDIUM,
	// critical messages
	PRINT_HIGH,
	// chat messages
	PRINT_CHAT
};

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
enum sound_channel : uint8_t
{
	CHAN_AUTO,
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	// 3 unused IDs

	// bitflags; can be added/OR'd with other channels

	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
	CHAN_NO_PHS_ADD = 1 << 3,
	// send by reliable message, not datagram
	CHAN_RELIABLE = 1 << 4
};

MAKE_ENUM_BITWISE(sound_channel, export);

// sound attenuation values
enum sound_attn : uint8_t
{
	// full volume the entire level
	ATTN_NONE,
	ATTN_NORM,
	ATTN_IDLE,
	// diminish very rapidly with distance
	ATTN_STATIC
};

// destination class for gi.multicast()
enum multicast_destination : uint8_t
{
	// send to all
	MULTICAST_ALL,
	// send only if inside phs
	MULTICAST_PHS,
	// send only if inside pvs
	MULTICAST_PVS,

	// reliable version of all
	MULTICAST_ALL_R,
	// reliable version of phs
	MULTICAST_PHS_R,
	// reliable version of pvs
	MULTICAST_PVS_R
};

// which type of objects to pull in from gi.BoxEdicts
enum box_edicts_area : uint8_t
{
	AREA_SOLID = 1,
	AREA_TRIGGERS = 2
};

// surface flags returned from traces.
enum surface_flags : uint32_t
{
	SURF_NONE = 0,
	// value will hold the light strength
	SURF_LIGHT = 1 << 0,
	// icey surface
	SURF_SLICK = 1 << 1,
	// don't draw, but add to skybox
	SURF_SKY = 1 << 2,
	// turbulent water warp
	SURF_WARP = 1 << 3,
	SURF_TRANS33 = 1 << 4,
	SURF_TRANS66 = 1 << 5,
	// scroll towards angle
	SURF_FLOWING = 1 << 6,
	// don't bother referencing the texture
	SURF_NODRAW = 1 << 7,
	// used by kmquake2 for alphatest-y surfaces
	SURF_ALPHATEST = 1 << 25
};

MAKE_ENUM_BITWISE(surface_flags, export);

struct surface
{
	// surface name
	stringarray<16>	name;
	// flags of this surface
	surface_flags	flags;
	// value for light surfaces
	int32_t			value;
};

constexpr surface null_surface { stringarray<16>({ '\0' }), SURF_NONE, 0 };

// a trace is returned when a box is swept through the world
struct trace
{
	inline trace();

	inline trace(const trace &tr) :
		allsolid(tr.allsolid),
		startsolid(tr.startsolid),
		fraction(tr.fraction),
		endpos(tr.endpos),
		normal(tr.normal),
		surface(tr.surface),
		contents(tr.contents),
		ent(tr.ent)
	{
	}

	inline trace &operator=(const trace &tr)
	{
		new(this) trace(tr);
		return *this;
	}

	// if true, plane is not valid
	qboolean		allsolid;
	// if true, the initial point was in a solid area
	qboolean		startsolid;
	// time completed, 1.0 = didn't hit anything
	float			fraction;
	// final position
	vector			endpos;
	// surface normal at impact
	vector			normal;
private:
	int32_t			plane_padding[2] = { 0, 0 };
public:
	// surface hit
	const surface &surface;
	// contents on other side of surface hit
	content_flags	contents;
	// entity hit
	entity &ent;
};

// max bytes in a single message packet
constexpr size_t MESSAGE_LIMIT = 1400;

// max size of a Quake path
constexpr size_t MAX_QPATH = 64;

//
// muzzle flashes / player effects
//
enum muzzleflash : uint8_t
{
	MZ_BLASTER,
	MZ_MACHINEGUN,
	MZ_SHOTGUN,
	MZ_CHAINGUN1,
	MZ_CHAINGUN2,
	MZ_CHAINGUN3,
	MZ_RAILGUN,
	MZ_ROCKET,
	MZ_GRENADE,
	MZ_LOGIN,
	MZ_LOGOUT,
	MZ_RESPAWN,
	MZ_BFG,
	MZ_SSHOTGUN,
	MZ_HYPERBLASTER,
	MZ_ITEMRESPAWN,
	// RAFAEL
	MZ_IONRIPPER,
	MZ_BLUEHYPERBLASTER,
	MZ_PHALANX,
	// RAFAEL

	//ROGUE
	MZ_ETF_RIFLE = 30,
	MZ_UNUSED,
	MZ_SHOTGUN2,
	MZ_HEATBEAM,
	MZ_BLASTER2,
	MZ_TRACKER,
	MZ_NUKE1,
	MZ_NUKE2,
	MZ_NUKE4,
	MZ_NUKE8,
	//ROGUE

	MZ_SILENCED = 128,	// bit flag ORed with one of the above numbers,
	MZ_NONE = 0
};

MAKE_ENUM_BITWISE(muzzleflash, export)

//
// monster muzzle flashes
//
enum monster_muzzleflash : uint8_t
{
	MZ2_TANK_BLASTER_1 = 1,
	MZ2_TANK_BLASTER_2,
	MZ2_TANK_BLASTER_3,
	MZ2_TANK_MACHINEGUN_1,
	MZ2_TANK_MACHINEGUN_2,
	MZ2_TANK_MACHINEGUN_3,
	MZ2_TANK_MACHINEGUN_4,
	MZ2_TANK_MACHINEGUN_5,
	MZ2_TANK_MACHINEGUN_6,
	MZ2_TANK_MACHINEGUN_7,
	MZ2_TANK_MACHINEGUN_8,
	MZ2_TANK_MACHINEGUN_9,
	MZ2_TANK_MACHINEGUN_10,
	MZ2_TANK_MACHINEGUN_11,
	MZ2_TANK_MACHINEGUN_12,
	MZ2_TANK_MACHINEGUN_13,
	MZ2_TANK_MACHINEGUN_14,
	MZ2_TANK_MACHINEGUN_15,
	MZ2_TANK_MACHINEGUN_16,
	MZ2_TANK_MACHINEGUN_17,
	MZ2_TANK_MACHINEGUN_18,
	MZ2_TANK_MACHINEGUN_19,
	MZ2_TANK_ROCKET_1,
	MZ2_TANK_ROCKET_2,
	MZ2_TANK_ROCKET_3,

	MZ2_INFANTRY_MACHINEGUN_1,
	MZ2_INFANTRY_MACHINEGUN_2,
	MZ2_INFANTRY_MACHINEGUN_3,
	MZ2_INFANTRY_MACHINEGUN_4,
	MZ2_INFANTRY_MACHINEGUN_5,
	MZ2_INFANTRY_MACHINEGUN_6,
	MZ2_INFANTRY_MACHINEGUN_7,
	MZ2_INFANTRY_MACHINEGUN_8,
	MZ2_INFANTRY_MACHINEGUN_9,
	MZ2_INFANTRY_MACHINEGUN_10,
	MZ2_INFANTRY_MACHINEGUN_11,
	MZ2_INFANTRY_MACHINEGUN_12,
	MZ2_INFANTRY_MACHINEGUN_13,

	MZ2_SOLDIER_BLASTER_1,
	MZ2_SOLDIER_BLASTER_2,
	MZ2_SOLDIER_SHOTGUN_1,
	MZ2_SOLDIER_SHOTGUN_2,
	MZ2_SOLDIER_MACHINEGUN_1,
	MZ2_SOLDIER_MACHINEGUN_2,

	MZ2_GUNNER_MACHINEGUN_1,
	MZ2_GUNNER_MACHINEGUN_2,
	MZ2_GUNNER_MACHINEGUN_3,
	MZ2_GUNNER_MACHINEGUN_4,
	MZ2_GUNNER_MACHINEGUN_5,
	MZ2_GUNNER_MACHINEGUN_6,
	MZ2_GUNNER_MACHINEGUN_7,
	MZ2_GUNNER_MACHINEGUN_8,
	MZ2_GUNNER_GRENADE_1,
	MZ2_GUNNER_GRENADE_2,
	MZ2_GUNNER_GRENADE_3,
	MZ2_GUNNER_GRENADE_4,

	MZ2_CHICK_ROCKET_1,

	MZ2_FLYER_BLASTER_1,
	MZ2_FLYER_BLASTER_2,

	MZ2_MEDIC_BLASTER_1,

	MZ2_GLADIATOR_RAILGUN_1,

	MZ2_HOVER_BLASTER_1,

	MZ2_ACTOR_MACHINEGUN_1,

	MZ2_SUPERTANK_MACHINEGUN_1,
	MZ2_SUPERTANK_MACHINEGUN_2,
	MZ2_SUPERTANK_MACHINEGUN_3,
	MZ2_SUPERTANK_MACHINEGUN_4,
	MZ2_SUPERTANK_MACHINEGUN_5,
	MZ2_SUPERTANK_MACHINEGUN_6,
	MZ2_SUPERTANK_ROCKET_1,
	MZ2_SUPERTANK_ROCKET_2,
	MZ2_SUPERTANK_ROCKET_3,

	MZ2_BOSS2_MACHINEGUN_L1,
	MZ2_BOSS2_MACHINEGUN_L2,
	MZ2_BOSS2_MACHINEGUN_L3,
	MZ2_BOSS2_MACHINEGUN_L4,
	MZ2_BOSS2_MACHINEGUN_L5,
	MZ2_BOSS2_ROCKET_1,
	MZ2_BOSS2_ROCKET_2,
	MZ2_BOSS2_ROCKET_3,
	MZ2_BOSS2_ROCKET_4,

	MZ2_FLOAT_BLASTER_1,

	MZ2_SOLDIER_BLASTER_3,
	MZ2_SOLDIER_SHOTGUN_3,
	MZ2_SOLDIER_MACHINEGUN_3,
	MZ2_SOLDIER_BLASTER_4,
	MZ2_SOLDIER_SHOTGUN_4,
	MZ2_SOLDIER_MACHINEGUN_4,
	MZ2_SOLDIER_BLASTER_5,
	MZ2_SOLDIER_SHOTGUN_5,
	MZ2_SOLDIER_MACHINEGUN_5,
	MZ2_SOLDIER_BLASTER_6,
	MZ2_SOLDIER_SHOTGUN_6,
	MZ2_SOLDIER_MACHINEGUN_6,
	MZ2_SOLDIER_BLASTER_7,
	MZ2_SOLDIER_SHOTGUN_7,
	MZ2_SOLDIER_MACHINEGUN_7,
	MZ2_SOLDIER_BLASTER_8,
	MZ2_SOLDIER_SHOTGUN_8,
	MZ2_SOLDIER_MACHINEGUN_8,

	// Xian
	MZ2_MAKRON_BFG,
	MZ2_MAKRON_BLASTER_1,
	MZ2_MAKRON_BLASTER_2,
	MZ2_MAKRON_BLASTER_3,
	MZ2_MAKRON_BLASTER_4,
	MZ2_MAKRON_BLASTER_5,
	MZ2_MAKRON_BLASTER_6,
	MZ2_MAKRON_BLASTER_7,
	MZ2_MAKRON_BLASTER_8,
	MZ2_MAKRON_BLASTER_9,
	MZ2_MAKRON_BLASTER_10,
	MZ2_MAKRON_BLASTER_11,
	MZ2_MAKRON_BLASTER_12,
	MZ2_MAKRON_BLASTER_13,
	MZ2_MAKRON_BLASTER_14,
	MZ2_MAKRON_BLASTER_15,
	MZ2_MAKRON_BLASTER_16,
	MZ2_MAKRON_BLASTER_17,
	MZ2_MAKRON_RAILGUN_1,
	MZ2_JORG_MACHINEGUN_L1,
	MZ2_JORG_MACHINEGUN_L2,
	MZ2_JORG_MACHINEGUN_L3,
	MZ2_JORG_MACHINEGUN_L4,
	MZ2_JORG_MACHINEGUN_L5,
	MZ2_JORG_MACHINEGUN_L6,
	MZ2_JORG_MACHINEGUN_R1,
	MZ2_JORG_MACHINEGUN_R2,
	MZ2_JORG_MACHINEGUN_R3,
	MZ2_JORG_MACHINEGUN_R4,
	MZ2_JORG_MACHINEGUN_R5,
	MZ2_JORG_MACHINEGUN_R6,
	MZ2_JORG_BFG_1,
	MZ2_BOSS2_MACHINEGUN_R1,
	MZ2_BOSS2_MACHINEGUN_R2,
	MZ2_BOSS2_MACHINEGUN_R3,
	MZ2_BOSS2_MACHINEGUN_R4,
	MZ2_BOSS2_MACHINEGUN_R5,
	// Xian

	//ROGUE
	MZ2_CARRIER_MACHINEGUN_L1,
	MZ2_CARRIER_MACHINEGUN_R1,
	MZ2_CARRIER_GRENADE,
	MZ2_TURRET_MACHINEGUN,
	MZ2_TURRET_ROCKET,
	MZ2_TURRET_BLASTER,
	MZ2_STALKER_BLASTER,
	MZ2_DAEDALUS_BLASTER,
	MZ2_MEDIC_BLASTER_2,
	MZ2_CARRIER_RAILGUN,
	MZ2_WIDOW_DISRUPTOR,
	MZ2_WIDOW_BLASTER,
	MZ2_WIDOW_RAIL,
	MZ2_WIDOW_PLASMABEAM,			// PMM - not used
	MZ2_CARRIER_MACHINEGUN_L2,
	MZ2_CARRIER_MACHINEGUN_R2,
	MZ2_WIDOW_RAIL_LEFT,
	MZ2_WIDOW_RAIL_RIGHT,
	MZ2_WIDOW_BLASTER_SWEEP1,
	MZ2_WIDOW_BLASTER_SWEEP2,
	MZ2_WIDOW_BLASTER_SWEEP3,
	MZ2_WIDOW_BLASTER_SWEEP4,
	MZ2_WIDOW_BLASTER_SWEEP5,
	MZ2_WIDOW_BLASTER_SWEEP6,
	MZ2_WIDOW_BLASTER_SWEEP7,
	MZ2_WIDOW_BLASTER_SWEEP8,
	MZ2_WIDOW_BLASTER_SWEEP9,
	MZ2_WIDOW_BLASTER_100,
	MZ2_WIDOW_BLASTER_90,
	MZ2_WIDOW_BLASTER_80,
	MZ2_WIDOW_BLASTER_70,
	MZ2_WIDOW_BLASTER_60,
	MZ2_WIDOW_BLASTER_50,
	MZ2_WIDOW_BLASTER_40,
	MZ2_WIDOW_BLASTER_30,
	MZ2_WIDOW_BLASTER_20,
	MZ2_WIDOW_BLASTER_10,
	MZ2_WIDOW_BLASTER_0,
	MZ2_WIDOW_BLASTER_10L,
	MZ2_WIDOW_BLASTER_20L,
	MZ2_WIDOW_BLASTER_30L,
	MZ2_WIDOW_BLASTER_40L,
	MZ2_WIDOW_BLASTER_50L,
	MZ2_WIDOW_BLASTER_60L,
	MZ2_WIDOW_BLASTER_70L,
	MZ2_WIDOW_RUN_1,
	MZ2_WIDOW_RUN_2,
	MZ2_WIDOW_RUN_3,
	MZ2_WIDOW_RUN_4,
	MZ2_WIDOW_RUN_5,
	MZ2_WIDOW_RUN_6,
	MZ2_WIDOW_RUN_7,
	MZ2_WIDOW_RUN_8,
	MZ2_CARRIER_ROCKET_1,
	MZ2_CARRIER_ROCKET_2,
	MZ2_CARRIER_ROCKET_3,
	MZ2_CARRIER_ROCKET_4,
	MZ2_WIDOW2_BEAMER_1,
	MZ2_WIDOW2_BEAMER_2,
	MZ2_WIDOW2_BEAMER_3,
	MZ2_WIDOW2_BEAMER_4,
	MZ2_WIDOW2_BEAMER_5,
	MZ2_WIDOW2_BEAM_SWEEP_1,
	MZ2_WIDOW2_BEAM_SWEEP_2,
	MZ2_WIDOW2_BEAM_SWEEP_3,
	MZ2_WIDOW2_BEAM_SWEEP_4,
	MZ2_WIDOW2_BEAM_SWEEP_5,
	MZ2_WIDOW2_BEAM_SWEEP_6,
	MZ2_WIDOW2_BEAM_SWEEP_7,
	MZ2_WIDOW2_BEAM_SWEEP_8,
	MZ2_WIDOW2_BEAM_SWEEP_9,
	MZ2_WIDOW2_BEAM_SWEEP_10,
	MZ2_WIDOW2_BEAM_SWEEP_11
	// ROGUE
};

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
enum temp_event : uint8_t
{
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,	// used as '22' in a map, so DON'T RENUMBER!!!
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
	//ROGUE
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE,
	//ROGUE

	TE_NUM_ENTITIES
};

// color for TE_SPLASH
enum splash_type : uint8_t
{
	SPLASH_UNKNOWN,
	SPLASH_SPARKS,
	SPLASH_BLUE_WATER,
	SPLASH_BROWN_WATER,
	SPLASH_SLIME,
	SPLASH_LAVA,
	SPLASH_BLOOD
};

// messages that can be sent to players
enum svc_ops : uint8_t
{
	svc_bad,

	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	svc_sound = 9,			// <see code>
	svc_print,			// [byte] id [string] null terminated string
	svc_stufftext,			// [string] stuffed into client's console buffer, should be \n terminated
	svc_configstring = 13,		// [short] [string]
	svc_centerprint = 15		// [string] to put in center of the screen
};

//
// button bits
//
enum button_bits : uint8_t
{
	BUTTON_NONE,
	BUTTON_ATTACK = 1,
	BUTTON_USE = 2,
	// any key whatsoever
	BUTTON_ANY = 0xFF
};

MAKE_ENUM_BITWISE(button_bits, export);

// usercmd_t is sent to the server each client frame
extern "C" struct usercmd
{
	uint8_t				msec;
	button_bits			buttons;
private:
	array<int16_t, 3>	angles;
public:
	vector get_angles() const;
	void set_angles(vector v);

	int16_t				forwardmove, sidemove, upmove;
	// these are used, but bad
private:
	uint8_t				impulse;
	uint8_t				lightlevel;
};

constexpr size_t MAX_TOUCH = 32;

extern "C" struct pmove_t
{
	// state (in / out)
	pmove_state	s;

	// command (in)
	usercmd	cmd;

	// if s has been changed outside pmove
	qboolean	snapinitial;

	// results (out)

	int32_t						numtouch;
	array<entityref, MAX_TOUCH>	touchents;

	vector	viewangles;         // clamped
	float	viewheight;

	vector	mins, maxs;         // bounding box size

	entityref		groundentity;
	content_flags	watertype;
	int32_t			waterlevel;

	// callbacks to test the world
	trace(*trace)(const vector &start, const vector &mins, const vector &maxs, const vector &end);
	content_flags(*pointcontents)(const vector &point);
};

//
// per-level limits
//
constexpr size_t MAX_CLIENTS = 256;	// absolute limit
constexpr size_t MAX_LIGHTSTYLES = 256;
#ifdef KMQUAKE2_ENGINE_MOD
constexpr size_t MAX_EDICTS = 8192;	// must change protocol to increase more
constexpr size_t MAX_MODELS = 8192;	// these are sent over the net as bytes
constexpr size_t MAX_SOUNDS = 8192;	// so they cannot be blindly increased
constexpr size_t MAX_IMAGES = 2048;
#else
constexpr size_t MAX_EDICTS = 1024;	// must change protocol to increase more
constexpr size_t MAX_MODELS = 256;	// these are sent over the net as bytes
constexpr size_t MAX_SOUNDS = 256;	// so they cannot be blindly increased
constexpr size_t MAX_IMAGES = 256;
#endif
constexpr size_t MAX_ITEMS = 256;
constexpr size_t MAX_GENERAL = (MAX_CLIENTS * 2); // general config strings

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
enum config_string : int32_t
{
	CS_NAME,
	CS_CDTRACK,
	CS_SKY,
	CS_SKYAXIS,	// %f %f %f format
	CS_SKYROTATE,
	CS_STATUSBAR,	// display program string

	CS_AIRACCEL = 29,	// air acceleration control
	CS_MAXCLIENTS,
	CS_MAPCHECKSUM,			// for catching cheater maps

	CS_MODELS,
	CS_SOUNDS = CS_MODELS + MAX_MODELS,
	CS_IMAGES = CS_SOUNDS + MAX_SOUNDS,
	CS_LIGHTS = CS_IMAGES + MAX_IMAGES,
	CS_ITEMS = CS_LIGHTS + MAX_LIGHTSTYLES,
	CS_PLAYERSKINS = CS_ITEMS + MAX_ITEMS,
	CS_GENERAL = CS_PLAYERSKINS + MAX_CLIENTS,
	MAX_CONFIGSTRINGS = CS_GENERAL + MAX_GENERAL
};

// fetch the max size you are able to write to the specific configstring index.
// some mods actually exploit CS_STATUSBAR to take space up to CS_AIRACCEL.
constexpr size_t CS_SIZE(const config_string &cs)
{
	if (cs >= CS_STATUSBAR && cs < CS_AIRACCEL)
		return MAX_QPATH * (CS_AIRACCEL - cs);

	return MAX_QPATH;
}

// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
enum entity_effects : uint32_t
{
	EF_NONE,
	EF_ROTATE = 1 << 0,	// rotate (bonus items)
	EF_GIB = 1 << 1,		// leave a trail
	EF_UNUSED = 1 << 2,
	EF_BLASTER = 1 << 3,	// redlight + trail
	EF_ROCKET = 1 << 4,	// redlight + trail
	EF_GRENADE = 1 << 5,
	EF_HYPERBLASTER = 1 << 6,
	EF_BFG = 1 << 7,
	EF_COLOR_SHELL = 1 << 8,
	EF_POWERSCREEN = 1 << 9,
	EF_ANIM01 = 1 << 10,		// automatically cycle between frames 0 and 1 at 2 hz
	EF_ANIM23 = 1 << 11,		// automatically cycle between frames 2 and 3 at 2 hz
	EF_ANIM_ALL = 1 << 12,		// automatically cycle through all frames at 2hz
	EF_ANIM_ALLFAST = 1 << 13,	// automatically cycle through all frames at 10hz
	EF_FLIES = 1 << 14,
	EF_QUAD = 1 << 15,
	EF_PENT = 1 << 16,
	EF_TELEPORTER = 1 << 17,		// particle fountain
	EF_FLAG1 = 1 << 18,
	EF_FLAG2 = 1 << 19,
	// RAFAEL
	EF_IONRIPPER = 1 << 20,
	EF_GREENGIB = 1 << 21,
	EF_BLUEHYPERBLASTER = 1 << 22,
	EF_SPINNINGLIGHTS = 1 << 23,
	EF_PLASMA = 1 << 24,
	EF_TRAP = 1 << 25,
	// RAFAEL

	//ROGUE
	EF_TRACKER = 1 << 26,
	EF_DOUBLE = 1 << 27,
	EF_SPHERETRANS = 1 << 28,
	EF_TAGTRAIL = 1 << 29,
	EF_HALF_DAMAGE = 1 << 30,
	//ROGUE

	EF_TRACKERTRAIL = 1u << 31
};

MAKE_ENUM_BITWISE(entity_effects, export);

// render effects affect the rendering of an entity
enum render_effects : uint32_t
{
	RF_MINLIGHT = 1 << 0,	// always have some light (viewmodel)
	RF_VIEWERMODEL = 1 << 1,	// don't draw through eyes, only mirrors
	RF_WEAPONMODEL = 1 << 2,	// only draw through eyes
	RF_FULLBRIGHT = 1 << 3,	// always draw full intensity
	RF_DEPTHHACK = 1 << 4,	// for view weapon Z crunching
	RF_TRANSLUCENT = 1 << 5,
	RF_FRAMELERP = 1 << 6,
	RF_BEAM = 1 << 7,
	RF_CUSTOMSKIN = 1 << 8,	// skin is an index in image_precache
	RF_GLOW = 1 << 9,		// pulse lighting for bonus items
	RF_SHELL_RED = 1 << 10,
	RF_SHELL_GREEN = 1 << 11,
	RF_SHELL_BLUE = 1 << 12,

	//ROGUE
	RF_IR_VISIBLE = 1 << 15,
	RF_SHELL_DOUBLE = 1 << 16,
	RF_SHELL_HALF_DAM = 1 << 17,
	RF_USE_DISGUISE = 1 << 18
	//ROGUE
};

MAKE_ENUM_BITWISE(render_effects, export);

// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
enum entity_event : uint32_t
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
};

// entity state is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
struct entity_state
{
	// edict index; do not touch
	uint32_t	number;

	vector	origin;
	vector	angles;
	// for lerping
	vector	old_origin;

	model_index	modelindex;
	model_index	modelindex2, modelindex3, modelindex4;  // weapons, CTF flags, etc
#ifdef KMQUAKE2_ENGINE_MOD
	model_index	modelindex5, modelindex6;	//more attached models
#endif
	int32_t	frame;
	int32_t	skinnum;
#ifdef KMQUAKE2_ENGINE_MOD
	float	alpha;	//entity transparency
#endif
	entity_effects	effects;
	render_effects	renderfx;
private:
	// solidity value sent over to clients. not used by game DLL.
	int32_t	solid [[maybe_unused]];
public:
	sound_index	sound;			// for looping sounds, to guarantee shutoff
#ifdef KMQUAKE2_ENGINE_MOD // Knightmare- added sound attenuation
	float		attenuation;
#endif
	// impulse events -- muzzle flashes, footsteps, etc
	// events only go out for a single frame, they
	// are automatically cleared each frame
	entity_event	event;
};

// area connection
struct area_list
{
	area_list *next, *prev;
};

constexpr size_t MAX_ENT_CLUSTERS = 16;

// entity server flags; the engine uses these, so don't change this.
enum server_flags : uint32_t
{
	SVF_NONE,
	// don't send entity to clients, even if it has effects
	SVF_NOCLIENT = 1 << 0,
	// treat as CONTENTS_DEADMONSTER for collision
	SVF_DEADMONSTER = 1 << 1,
	// treat as CONTENTS_MONSTER for collision
	SVF_MONSTER = 1 << 2
};

MAKE_ENUM_BITWISE(server_flags, export);

enum solidity : uint32_t
{
	// no interaction with other objects
	SOLID_NOT,
	// only touch when inside, after moving
	SOLID_TRIGGER,
	// touch on edge
	SOLID_BBOX,
	// bsp clip, touch on edge
	SOLID_BSP
};

// this is a reference to the world entity.
extern entityref world;

// player_state_t->refdef flags
enum refdef_flags
{
	RDF_NONE,

	RDF_UNDERWATER = 1 << 0,	// warp the screen as apropriate
	RDF_NOWORLDMODEL = 1 << 1,	// used for player configuration screen

	//ROGUE
	RDF_IRGOGGLES = 1 << 2,
	RDF_UVGOGGLES = 1 << 3
	//ROGUE
};

MAKE_ENUM_BITWISE(refdef_flags, export);

// player_state->stats[] indexes
struct player_stat
{
	uint16_t	value;

	player_stat() :
		value(0)
	{
	}

	template<typename T>
	player_stat(const T &v) :
		value((uint16_t) v)
	{
	}

	player_stat(const image_index &v) :
		player_stat((int32_t) v)
	{
	}

	inline operator uint16_t &() { return value; }
	inline operator const uint16_t &() const { return value; }
	inline explicit operator image_index() const { return image_index(value); }
};

// For engine compatibility, these 18 IDs should remain the same
// and keep their described usage.
// These are macros so they can be embedded in the static statusbar strings.
enum
{
	STAT_HEALTH_ICON,
	STAT_HEALTH,
	STAT_AMMO_ICON,
	STAT_AMMO,
	STAT_ARMOR_ICON,
	STAT_ARMOR,
	STAT_SELECTED_ICON,
	STAT_PICKUP_ICON,
	STAT_PICKUP_STRING,
	STAT_TIMER_ICON,
	STAT_TIMER,
	STAT_HELPICON,
	STAT_SELECTED_ITEM,
	STAT_LAYOUTS,
	STAT_FRAGS,
	STAT_FLASHES, // cleared each frame, 1 = health, 2 = armor
	STAT_CHASE,
	STAT_SPECTATOR,
	// Bits from here to (MAX_STATS - 1) are free for mods
#ifdef CTF
	#define STAT_CTF_TEAM1_PIC			18
	#define STAT_CTF_TEAM1_CAPS			19
	#define STAT_CTF_TEAM2_PIC			20
	#define STAT_CTF_TEAM2_CAPS			21
	#define STAT_CTF_FLAG_PIC			22
	#define STAT_CTF_JOINED_TEAM1_PIC	23
	#define STAT_CTF_JOINED_TEAM2_PIC	24
	#define STAT_CTF_TEAM1_HEADER		25
	#define STAT_CTF_TEAM2_HEADER		26
	#define STAT_CTF_TECH				27
	#define STAT_CTF_ID_VIEW			28
	#define STAT_CTF_ID_VIEW_COLOR		29
	#define STAT_CTF_TEAMINFO			30
#endif

#ifdef KMQUAKE2_ENGINE_MOD
	#define MAX_STATS					256
#else
	MAX_STATS		= 32
#endif
};

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
struct player_state
{
	pmove_state	pmove;      // for prediction

	// for fixed views
	vector	viewangles;
	// add to pmovestate->origin
	vector	viewoffset;
	// add to view direction to get render angles
	// set by weapon kicks, pain effects, etc
	vector	kick_angles;

	vector		gunangles;
	vector		gunoffset;
	model_index	gunindex;
	int32_t		gunframe;

#ifdef KMQUAKE2_ENGINE_MOD //Knightmare added
	int32_t		gunskin;		// for animated weapon skins
	model_index	gunindex2;		// for a second weapon model (boot)
	int32_t		gunframe2;
	int32_t		gunskin2;

	// server-side speed control!
	int32_t	maxspeed;
	int32_t	duckspeed;
	int32_t	waterspeed;
	int32_t	accel;
	int32_t	stopspeed;
#endif

	array<float, 4>	blend;	// rgba full screen effect

	float	fov;	// horizontal field of view

	refdef_flags	rdflags;        // refdef flags

	array<player_stat, MAX_STATS>	stats;       // fast status bar updates
};

// client data, only stored in entities between 1 and maxclients inclusive
struct server_client
{
	// shared between client and server
	player_state	ps;
	int32_t			ping;
	int32_t			clientNum;
};

// this is the structure used by all entities in Q2++.
struct server_entity
{
	friend struct entity;

private:
	// an error here means you're using entity as a value type. Always use entity&
	// to pass things around.
	server_entity() { }

	// Entities can also not be copied; use a.copy(b)
	// to do a proper copy, which resets members that would break the game.
	server_entity(server_entity &) { };

	// move constructor not allowed; entities can't be "deleted"
	// so move constructor would be weird
	server_entity(server_entity &&) = delete;

public:
	// this data is shared between the game and engine

	entity_state	s;
	client			*client;
	qboolean		inuse;
	int32_t			linkcount;

private:
	// this stuff is private to the engine
	area_list	area;				// linked to a division node or leaf

	int32_t								num_clusters;		// if -1, use headnode instead
	array<int32_t, MAX_ENT_CLUSTERS>	clusternums;

	int32_t		headnode;			// unused if num_clusters != -1

	// back to shared members
public:
	area_index		areanum, areanum2;

	server_flags	svflags;
	vector			mins, maxs;
	vector			absmin, absmax, size;
	solidity		solid;
	content_flags	clipmask;
	entityref		owner;

	// check whether this entity is the world entity.
	// for QC compatibility mostly.
	inline bool is_world() const
	{
		return s.number == 0;
	}

	// check whether this entity is a client and
	// has a client field that can be dereferenced.
	inline bool is_client() const
	{
		return client;
	}

	// check whether this entity is currently linked
	inline bool is_linked() const
	{
		return area.prev;
	}

	inline bool operator==(const server_entity &rhs) const { return s.number == rhs.s.number; }
	inline bool operator!=(const server_entity &rhs) const { return s.number != rhs.s.number; }
};