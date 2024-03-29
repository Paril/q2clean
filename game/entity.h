#pragma once

#include "config.h"
#include "lib/protocol.h"
#include "lib/string.h"
#include "game/items/itemlist.h"
#include "lib/types.h"
#include "lib/math/vector.h"
#include "savables.h"
#include "lib/types/enum.h"
#include "entity_types.h"
#include "game_types.h"

// entity move type
enum move_type : uint8_t
{
	MOVETYPE_NONE,          // never moves
	MOVETYPE_NOCLIP,        // origin and angles change with no interaction
	MOVETYPE_PUSH,          // no clip to world, push on box contact
	MOVETYPE_STOP,          // no clip to world, stops on box contact

	MOVETYPE_WALK,          // gravity
#ifdef SINGLE_PLAYER
	MOVETYPE_STEP,          // gravity, special edge handling
#endif
	MOVETYPE_FLY,
	MOVETYPE_TOSS,          // gravity
	MOVETYPE_FLYMISSILE,    // extra size to monsters
	MOVETYPE_BOUNCE

#ifdef THE_RECKONING
	, MOVETYPE_WALLBOUNCE
#endif
};

// entity flags
enum entity_flags : uint32_t
{
	FL_NONE,

#if defined(SINGLE_PLAYER)
	FL_FLY = 1 << 0,
	FL_SWIM = 1 << 1,		// implied immunity to drowning
	FL_NOTARGET = 1 << 2,
	FL_PARTIALGROUND = 1 << 3,
#endif

	FL_IMMUNE_LASER = 1 << 4,
	FL_INWATER = 1 << 5,
	FL_GODMODE = 1 << 6,
	FL_IMMUNE_SLIME = 1 << 7,
	FL_IMMUNE_LAVA = 1 << 8,
	FL_TEAMSLAVE = 1 << 9,
	FL_NO_KNOCKBACK = 1 << 10,
	FL_POWER_ARMOR = 1 << 11,

#ifdef GROUND_ZERO
	FL_DAMAGEABLE = 1 << 12,	// damageable, even if it normally couldn't be
#ifdef SINGLE_PLAYER
	FL_DISGUISED = 1 << 13,	// entity is in disguise, monsters will not recognize.
#endif
	FL_NOGIB = 1 << 14,		// player has been vaporized by a nuke, drop no gibs
#endif
	
	FL_RESPAWN = 1 << 15

#ifdef SINGLE_PLAYER
#ifdef GROUND_ZERO
	, FL_VISIBLE_MASK = FL_NOTARGET | FL_DISGUISED
#else
	, FL_VISIBLE_MASK = FL_NOTARGET
#endif
#endif
};

MAKE_ENUM_BITWISE(entity_flags);

using ethinkfunc = void(*)(entity &);
using blockedfunc = void(*)(entity &, entity &);
using touchfunc = void(*)(entity &, entity &, vector, const surface &);
using usefunc = void(*)(entity &, entity &, entity &);
using painfunc = void(*)(entity &, entity &, float, int32_t);
using diefunc = void(*)(entity &, entity &, entity &, int32_t, vector);

using mendfunc = void(*)(entity &);

// global monster functions
#if defined(SINGLE_PLAYER)
using maifunc = void(*)(entity &, float);

struct mframe_t
{
	maifunc		aifunc;
	float		dist;
	ethinkfunc	thinkfunc;

	consteval mframe_t(maifunc aifunc, float dist = 0, ethinkfunc thinkfunc = nullptr) :
		aifunc(aifunc),
		dist(dist),
		thinkfunc(thinkfunc)
	{
	}

	consteval mframe_t(maifunc aifunc, ethinkfunc thinkfunc) :
		mframe_t(aifunc, 0.f, thinkfunc)
	{
	}
};

struct mmove_t
{
	uint16_t		firstframe;
	uint16_t		lastframe;
	const mframe_t	*frames;
	mendfunc		endfunc;

	template<size_t N>
	constexpr mmove_t(uint32_t first, uint32_t last, const mframe_t (&frames) [N], mendfunc endfunc = nullptr) :
		firstframe(first),
		lastframe(last),
		frames(frames),
		endfunc(endfunc)
	{
		if (N != (uint32_t) ((first <= last) ? (last - first + 1) : (first - last + 1)))
			throw std::exception("invalid frames");
	}
};

//monster ai flags
enum ai_flags : int32_t
{
	AI_NONE,

	AI_STAND_GROUND			= 1 << 0,
	AI_TEMP_STAND_GROUND	= 1 << 1,
	AI_SOUND_TARGET			= 1 << 2,
	AI_LOST_SIGHT			= 1 << 3,
	AI_PURSUIT_LAST_SEEN	= 1 << 4,
	AI_PURSUE_NEXT			= 1 << 5,
	AI_PURSUE_TEMP			= 1 << 6,
	AI_HOLD_FRAME			= 1 << 7,
	AI_GOOD_GUY				= 1 << 8,
	AI_BRUTAL				= 1 << 9,
	AI_NOSTEP				= 1 << 10,
	AI_DUCKED				= 1 << 11,
	AI_COMBAT_POINT			= 1 << 12,
	AI_MEDIC				= 1 << 13,
	AI_RESURRECTING			= 1 << 14,
	AI_DO_NOT_COUNT			= 1 << 15,	// set for healed monsters

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	AI_MANUAL_STEERING		= 1 << 16,
	AI_IGNORE_SHOTS			= 1 << 17,
	AI_BLOCKED				= 1 << 18,
#endif
	
#ifdef ROGUE_AI
	AI_DODGING				= 1 << 19,
	AI_CHARGING				= 1 << 20,
	AI_HINT_PATH			= 1 << 21,
#endif

#ifdef GROUND_ZERO
	AI_TARGET_ANGER			= 1 << 22,
	AI_SPAWNED_CARRIER		= 1 << 23,	//
	AI_SPAWNED_MEDIC_C		= 1 << 24,	// both do_not_count and spawned are set for spawned monsters
	AI_SPAWNED_WIDOW		= 1 << 25,	//

	AI_SPAWNED_MASK			= AI_SPAWNED_CARRIER | AI_SPAWNED_MEDIC_C | AI_SPAWNED_WIDOW,	// mask to catch all three flavors of spawned
#endif

	AI_GOOD_GUY_MASK		= AI_GOOD_GUY | AI_DO_NOT_COUNT
};

MAKE_ENUM_BITWISE(ai_flags);

//monster attack state
enum ai_attack_state : int
{
	AS_STRAIGHT = 1,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE,

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
	AS_BLIND
#endif
};

#ifdef ROGUE_AI
using mdodgefunc = void(*)(entity &, entity &, gtimef, trace &);
using mblockedfunc = bool(*)(entity &, float);
using mduckfunc = void(*)(entity &, gtimef);
using munduckfunc = void(*)(entity &);
using msidestepfunc = void(*)(entity &);
#else
using mdodgefunc = void(*)(entity &, entity &, gtimef);
#endif

using monster_func = void(*)(entity &);
using monster_sightfunc = void(*)(entity &, entity &);
using monster_checkattack = bool(*)(entity &);

using monster_reacttodamage = void(*)(entity &, entity &, entity &, int32_t, int32_t);
#endif

enum move_state : uint8_t
{
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
};

struct moveinfo
{
	// fixed data
	vector	start_origin;
	vector	start_angles;
	vector	end_origin;
	vector	end_angles;

	sound_index	sound_start;
	sound_index	sound_middle;
	sound_index	sound_end;

	float	accel;
	float	speed;
	float	decel;
	float	distance;

	gtimef	wait;

	// state data

	move_state	state;
	vector		dir;
	float		current_speed;
	float		move_speed;
	float		next_speed;
	float		remaining_distance;
	float		decel_distance;

	savable<mendfunc>	endfunc;
};

#ifdef SINGLE_PLAYER
struct monsterinfo
{
	savable<const mmove_t>	currentmove;
	ai_flags	aiflags;
	int         nextframe;
	float       scale;

	savable<monster_func> stand;
	savable<monster_func> idle;
	savable<monster_func> search;
	savable<monster_func> walk;
	savable<monster_func> run;
	savable<monster_func> attack;
	savable<monster_func> melee;

	savable<mdodgefunc>	dodge;

	savable<monster_sightfunc>		sight;
	savable<monster_checkattack>	checkattack;

	savable<monster_reacttodamage>	reacttodamage;

	gtime	pause_time;
	gtime	attack_finished;

	vector			saved_goal;
	gtime			search_time;
	gtime			trail_time;
	vector			last_sighting;
	ai_attack_state	attack_state;
	bool			lefty;
	gtime			idle_time;
	int				linkcount;

	gitem_id	power_armor_type;
	int			power_armor_power;

	gtime		surprise_time;

#ifdef ROGUE_AI
	savable<mblockedfunc> blocked;
	gtime		last_hint_time;		// last time the monster checked for hintpaths.
	entityref	goal_hint;			// which hint_path we're trying to get to
	int32_t		medicTries;
	entityref	badMedic1, badMedic2;	// these medics have declared this monster "unhealable"
	entityref	healer;	// this is who is healing this monster
	savable<mduckfunc>		duck;
	savable<munduckfunc>	unduck;
	savable<msidestepfunc>	sidestep;
	float	base_height;
	gtime	next_duck_time;
	gtime	duck_wait_time;
	entityref	last_player_enemy;
	// blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
	// (set in the monster) of the next shot
	bool	blindfire;		// will the monster blindfire?
	gtime	blind_fire_framedelay;
	vector	blind_fire_target;
#endif
#ifdef GROUND_ZERO
	// used by the spawners to not spawn too much and keep track of #s of monsters spawned
	uint32_t	monster_slots;
	uint32_t	monster_used;
	entityref	commander;
	int32_t		summon_type;
	// powerup timers, used by widow, our friend
	gtime	quad_time;
	gtime	invincible_time;
	gtime	double_time;
#endif
};
#endif

#ifdef GROUND_ZERO
enum plat2flags : uint8_t
{
	PLAT2_NONE		= 0,
	PLAT2_CALLED	= 1,
	PLAT2_MOVING	= 2,
	PLAT2_WAITING	= 4
};

MAKE_ENUM_BITWISE(plat2flags);
#endif

// this enum describes how this entity responds to damage.
enum bleed_style : uint8_t
{
	// use the default damage visual response specified
	// by the damage type
	BLEED_DEFAULT,
	// always use sparks for damage
	BLEED_MECHANICAL,
	// always use green blood for damage
	BLEED_GREEN
};

// entity is inherited from the server entity, and contains game-local
// data.
struct entity : public server_entity
{
friend server_entity;
friend void WipeEntities();
friend void G_InitEdict(entity &);
friend void G_FreeEdict(entity &);
friend void InitGame();
#ifdef SAVING
friend uint32_t ReadLevelStream(stringlit filename);
#endif

private:
	// an error here means you're using entity as a value type. Always use entity&
	// to pass things around.
	using server_entity::server_entity;

	// internal use only
	void __init();
	// internal use only
	void __free();

public:
	move_type	movetype;

	entity_flags	flags;

	string	model;
	gtime	freeframenum;

	string		message;
	entity_type_ref	type;
	spawn_flag	spawnflags;

	gtime		timestamp;
	float		angle;
	string		target;
	string		targetname;
	string		killtarget;
	string		team;
	string		pathtarget;
	string		deathtarget;
	string		combattarget;
	entityref	target_ent;

	float	speed, accel, decel;
	vector	movedir;
	vector	pos1, pos2;

	vector	velocity;
	vector	avelocity;
	int32_t	mass;
	gtime	air_finished_time;
	float	gravity;

	entityref	goalentity;
	entityref	movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	gtime		nextthink;
	savable<ethinkfunc>	prethink;
	savable<ethinkfunc>	think;

	savable<blockedfunc>	blocked;
	savable<touchfunc>		touch;
	savable<usefunc>		use;
	savable<painfunc>		pain;
	savable<diefunc>		die;

	gtime	touch_debounce_time;
	gtime	pain_debounce_time;
	gtime	damage_debounce_time;
	gtime	fly_sound_debounce_time;
	gtime	last_move_time;

	int32_t	health;
	int32_t	max_health;
	int32_t	gib_health;

	bool	deadflag;

	gtime	show_hostile;

	gtime	powerarmor_time;

	string	map;
	float	viewheight;
	bool	takedamage;
	bleed_style	bleed_style;
	int32_t	dmg;
	int32_t	radius_dmg;
	float	dmg_radius;
	int32_t	sounds;
	int32_t	count;

	entityref	chain;
	entityref	enemy;
	entityref	oldenemy;
	entityref	activator;
	entityref	groundentity;
	int32_t		groundentity_linkcount;
	entityref	teamchain;
	entityref	teammaster;

	entityref	mynoise;
	entityref	mynoise2;

	sound_index	noise_index;
	sound_index	noise_index2;
	float	volume;
	sound_attn	attenuation;

	gtimef	wait;
	gtimef	delay;
	gtimef	rand;

	gtime	last_sound_time;

	content_flags	watertype;
	water_level		waterlevel;

	vector	move_origin;
	vector	move_angles;

	int32_t	style;
	itemref	item;

#ifdef GROUND_ZERO
	plat2flags	plat2flags;
	vector		offset;
#endif
#ifdef ROGUE_AI
	vector		gravityVector;
	entityref	bad_area;
	entityref	hint_chain;
	entityref	monster_hint_chain;
	entityref	target_hint_chain;
	size_t		hint_chain_id;
#endif

	moveinfo moveinfo;

#ifdef SINGLE_PLAYER
	monsterinfo monsterinfo;
	uint8_t power_cube_id;
#endif

	// used to store pre-push state
	struct
	{
		vector	origin, angles;
	#ifdef USE_SMOOTH_DELTA_ANGLES
		float	deltayaw;
	#endif
	} pushed;
};

constexpr bool entityref::is_world() const
{
	return has_value() && _ptr->is_world();
}

// fetch an entity by their number.
entity &itoe(size_t index);

// fetch an entity's number
constexpr size_t etoi(const entity &ent)
{
	return ent.number;
}

// fetch the entity that follows this one
inline entity &next_ent(const entity &e)
{
	return itoe(etoi(e) + 1);
}

class entity_range_iterable
{
private:
	entity	*first, *last;

public:
	entity_range_iterable();
	entity_range_iterable(size_t first, size_t last);

	entity *begin();
	entity *end();
};

// fetch an entity range, for iteration. Both
// values are inclusive.
entity_range_iterable entity_range(size_t start, size_t end);

// reference to the live number of entities
extern uint32_t &num_entities;

// reference to max entities
extern const uint32_t &max_entities;

// internal function, mainly for save/load code.
void WipeEntities();

#include "game/client.h"

// formatter for entity
#include "lib/string/format.h"

template<>
struct std::formatter<entity>
{
	template <typename FormatParseContext>
	auto parse(FormatParseContext &pc)
	{
		return pc.end();
	}

	template<typename FormatContext>
	auto format(const ::entity &v, FormatContext &ctx)
	{
		return std::vformat_to(ctx.out(), "{}[{}] @ {}", std::make_format_args(v.type.type->id, v.number, v.solid == SOLID_BSP ? v.absbounds.center() : v.origin));
	}
};

template<>
struct std::formatter<entityref> : std::formatter<entity>
{
	template<typename FormatContext>
	auto format(const ::entityref &v, FormatContext &ctx)
	{
		if (!v.has_value())
			return std::vformat_to(ctx.out(), "null entity", std::make_format_args());
		return std::formatter<entity>::format(v, ctx);
	}
};