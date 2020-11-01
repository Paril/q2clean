#pragma once

#include "../lib/types.h"
#include "../lib/pmove_state.h"
#include "../lib/surface.h"
#include "../lib/sound_attn.h"
#include "spawn_flag.h"
#include "items.h"
#include "itemref.h"
#ifdef BOTS
#include "ai/ai.h"
#endif

// entity type; this is to replace classname comparisons. Every entity spawnable by
// classnames must have their own ID. You can assign IDs to other entities if you're
// wanting to search for them, or anything like that.
enum entity_type : uint32_t
{
	// entity doesn't have a type assigned
	ET_UNKNOWN,

	// freed entity
	ET_FREED,

	// items spawned in the world
	ET_ITEM,
	
	// special values for run-time entities
	ET_BODYQUEUE,
	ET_BLASTER_BOLT,
	ET_GRENADE,
	ET_HANDGRENADE,
	ET_ROCKET,
	ET_BFG_BLAST,
	ET_DEBRIS,
	ET_PLAYER,
	ET_DISCONNECTED_PLAYER,
	ET_DELAYED_USE,

	// spawnable entities
	ET_FUNC_PLAT,
	ET_FUNC_ROTATING,
	ET_FUNC_BUTTON,
	ET_FUNC_DOOR,
	ET_FUNC_DOOR_ROTATING,
	ET_FUNC_KILLBOX,
	ET_FUNC_TRAIN,
	ET_TRIGGER_ELEVATOR,
	ET_FUNC_TIMER,
	ET_FUNC_CONVEYOR,
	ET_FUNC_GROUP,
	ET_FUNC_AREAPORTAL,
	ET_PATH_CORNER,
	ET_INFO_NOTNULL,
	ET_LIGHT,
	ET_FUNC_WALL,
	ET_FUNC_OBJECT,
	ET_MISC_BLACKHOLE,
	ET_MISC_EASTERTANK,
	ET_MISC_EASTERCHICK,
	ET_MISC_EASTERCHICK2,
	ET_MONSTER_COMMANDER_BODY,
	ET_MISC_BANNER,
	ET_MISC_VIPER,
	ET_MISC_BIGVIPER,
	ET_MISC_VIPER_BOMB,
	ET_MISC_STROGG_SHIP,
	ET_MISC_SATELLITE_DISH,
	ET_LIGHT_MINE1,
	ET_LIGHT_MINE2,
	ET_MISC_GIB_ARM,
	ET_MISC_GIB_LEG,
	ET_MISC_GIB_HEAD,
	ET_TARGET_CHARACTER,
	ET_TARGET_STRING,
	ET_FUNC_CLOCK,
	ET_MISC_TELEPORTER,
	ET_MISC_TELEPORTER_DEST,
	ET_INFO_PLAYER_START,
	ET_INFO_PLAYER_DEATHMATCH,
	ET_INFO_PLAYER_INTERMISSION,
	ET_WORLDSPAWN,
	ET_TARGET_TEMP_ENTITY,
	ET_TARGET_SPEAKER,
	ET_TARGET_EXPLOSION,
	ET_TARGET_CHANGELEVEL,
	ET_TARGET_SPLASH,
	ET_TARGET_SPAWNER,
	ET_TARGET_BLASTER,
	ET_TARGET_LASER,
	ET_TARGET_EARTHQUAKE,
	ET_TRIGGER_MULTIPLE,
	ET_TRIGGER_ONCE,
	ET_TRIGGER_RELAY,
	ET_TRIGGER_COUNTER,
	ET_TRIGGER_ALWAYS,
	ET_TRIGGER_PUSH,
	ET_TRIGGER_HURT,
	ET_TRIGGER_GRAVITY,

	// special index for all entity types. save games use this
	// to check for compatibility.
	ET_TOTAL
};

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

#ifdef SINGLE_PLAYER
	FL_FLY = 1 << 0,
	FL_SWIM = 1 << 1,		// implied immunity to drowning
#endif
#if defined(SINGLE_PLAYER) || defined(BOTS)
	FL_NOTARGET = 1 << 2,
#endif
#ifdef SINGLE_PLAYER
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
	FL_MECHANICAL = 1 << 12,	// entity is mechanical, use sparks not blood
	FL_DAMAGEABLE = 1 << 13,	// damageable, even if it normally couldn't be
#ifdef SINGLE_PLAYER
	FL_DISGUISED = 1 << 14,	// entity is in disguise, monsters will not recognize.
#endif
	FL_NOGIB = 1 << 15,		// player has been vaporized by a nuke, drop no gibs
#endif
	
	FL_RESPAWN = 1 << 16,

#ifdef SINGLE_PLAYER
#ifdef GROUND_ZERO
	, FL_VISIBLE_MASK = FL_NOTARGET | FL_DISGUISED
#else
	, FL_VISIBLE_MASK = FL_NOTARGET
#endif
#endif
};

MAKE_ENUM_BITWISE(entity_flags);

using thinkfunc = void(*)(entity &);
using blockedfunc = void(*)(entity &, entity &);
using touchfunc = void(*)(entity &, entity &, vector, const surface &);
using usefunc = void(*)(entity &, entity &, entity &);
using painfunc = void(*)(entity &, entity &, float, int);
using diefunc = void(*)(entity &, entity &, entity &, int, vector);

enum dead_flag : uint8_t
{
	DEAD_NO,
	DEAD_DYING,
	DEAD_DEAD,
	DEAD_RESPAWNABLE 
};

enum move_state : uint8_t
{
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
};

using endfuncfunc = void(*)(entity &);

// the gentity is game-local entity data. Every entity holds an instance of this under
// the entity::g member.
struct gentity
{
	move_type	movetype;

	entity_flags	flags;

	string	model;
	gtime	freeframenum;

	string		message;
	entity_type	type;
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
	gtime	air_finished_framenum;
	float	gravity;

	entityref	goalentity;
	entityref	movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	gtime		nextthink;
	thinkfunc	prethink;
	thinkfunc	think;

	blockedfunc	blocked;
	touchfunc	touch;
	usefunc		use;
	painfunc	pain;
	diefunc		die;

	gtime	touch_debounce_framenum;
	gtime	pain_debounce_framenum;
	gtime	damage_debounce_framenum;
	gtime	fly_sound_debounce_framenum;
	gtime	last_move_framenum;

	int32_t	health;
	int32_t	max_health;
	int32_t	gib_health;

	dead_flag	deadflag;

	gtime	show_hostile;

	gtime	powerarmor_framenum;

	string	map;
	int32_t	viewheight;
	bool	takedamage;
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

	float	wait;
	float	delay;
	float	rand;

	gtime	last_sound_framenum;

	content_flags	watertype;
	int32_t			waterlevel;

	vector	move_origin;
	vector	move_angles;

	int32_t	style;
	itemref	item;

#ifdef GROUND_ZERO
	uint32_t	plat2flags;
	vector		offset;
	vector		gravityVector;
	entityref	bad_area;
	entityref	hint_chain;
	entityref	monster_hint_chain;
	entityref	target_hint_chain;
	size_t		hint_chain_id;
	gtime		lastMoveFrameNum;
#endif

	struct
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
	
		float	wait;
	
		// state data

		move_state	state;
		vector		dir;
		float		current_speed;
		float		move_speed;
		float		next_speed;
		float		remaining_distance;
		float		decel_distance;
		endfuncfunc	endfunc;
	} moveinfo;

#ifdef SINGLE_PLAYER
struct mframe_t
{
	void(entity, float)	aifunc;
	float				dist;
	void(entity)		thinkfunc;
};

struct mmove_t
{
	int				firstframe;
	int				lastframe;
	void(entity)	endfunc;
	mframe_t		*frame;
};

//monster ai flags
typedef enumflags : int
{
	AI_STAND_GROUND,
	AI_TEMP_STAND_GROUND,
	AI_SOUND_TARGET,
	AI_LOST_SIGHT,
	AI_PURSUIT_LAST_SEEN,
	AI_PURSUE_NEXT,
	AI_PURSUE_TEMP,
	AI_HOLD_FRAME,
	AI_GOOD_GUY,
	AI_BRUTAL,
	AI_NOSTEP,
	AI_DUCKED,
	AI_COMBAT_POINT,
	AI_MEDIC,
	AI_RESURRECTING
	
#ifdef GROUND_ZERO
	, AI_WALK_WALLS,
	AI_MANUAL_STEERING,
	AI_TARGET_ANGER,
	AI_DODGING,
	AI_CHARGING,
	AI_HINT_PATH,
	AI_IGNORE_SHOTS,
	AI_DO_NOT_COUNT,	// set for healed monsters
	AI_SPAWNED_CARRIER,	//
	AI_SPAWNED_MEDIC_C,	// both do_not_count and spawned are set for spawned monsters
	AI_SPAWNED_WIDOW,	//
	AI_BLOCKED,			// used by blocked_checkattack: set to say I'm attacking while blocked (prevents run-attacks)

	AI_GOOD_GUY_MASK = AI_GOOD_GUY | AI_DO_NOT_COUNT,
	AI_SPAWNED_MASK = AI_SPAWNED_CARRIER | AI_SPAWNED_MEDIC_C | AI_SPAWNED_WIDOW	// mask to catch all three flavors of spawned
#else
	, AI_GOOD_GUY_MASK = AI_GOOD_GUY
#endif
} ai_flags_t;

//monster attack state
typedef enum : int
{
	AS_STRAIGHT = 1,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE

#ifdef GROUND_ZERO
	, AS_BLIND	// PMM - used by boss code to do nasty things even if it can't see you
#endif
} ai_attack_state_t;

#ifdef GROUND_ZERO
typedef void(entity, entity, float, trace_t *) mdodgefunc;
#else
typedef void(entity, entity, float) mdodgefunc;
#endif

struct monsterinfo_t
{
	mmove_t     *currentmove;
	ai_flags_t  aiflags;
	int         nextframe;
	float       scale;
	
	void(entity)			stand;
	void(entity)			idle;
	void(entity)			search;
	void(entity)			walk;
	void(entity)			run;
	mdodgefunc				dodge;
	void(entity)			attack;
	void(entity)			melee;
	void(entity, entity)	sight;
	bool(entity)			checkattack;
	
	int	pause_framenum;
	int	attack_finished;
	
	vector				saved_goal;
	int					search_framenum;
	int					trail_framenum;
	vector				last_sighting;
	ai_attack_state_t	attack_state;
	bool				lefty;
	int					idle_framenum;
	int					linkcount;
	
	int	power_armor_type;
	int	power_armor_power;
	
#ifdef GROUND_ZERO
	bool(entity self, float dist)	blocked;
	int		last_hint_framenum;		// last time the monster checked for hintpaths.
	entity	goal_hint;			// which hint_path we're trying to get to
	int		medicTries;
	entity	badMedic1, badMedic2;	// these medics have declared this monster "unhealable"
	entity	healer;	// this is who is healing this monster
	void(entity self, float eta)	duck;
	void(entity self)				unduck;
	void(entity self)				sidestep;
	float	base_height;
	int		next_duck_framenum;
	int		duck_wait_framenum;
	entity	last_player_enemy;
	// blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
	// (set in the monster) of the next shot
	bool	blindfire;		// will the monster blindfire?
	int		blind_fire_framedelay;
	vector	blind_fire_target;
	// used by the spawners to not spawn too much and keep track of #s of monsters spawned
	int		monster_slots;
	int		monster_used;
	entity	commander;
	// powerup timers, used by widow, our friend
	int	quad_framenum;
	int	invincible_framenum;
	int	double_framenum;
#endif
};

.monsterinfo_t	monsterinfo;

#ifdef GROUND_ZERO
// this determines how long to wait after a duck to duck again.  this needs to be longer than
// the time after the monster_duck_up in all of the animation sequences
const float DUCK_INTERVAL	= 0.5;

// this is for the count of monsters
inline int(entity self) SELF_SLOTS_LEFT =
{
	return (self.monsterinfo.monster_slots - self.monsterinfo.monster_used);
};
#endif

#ifdef THE_RECKONING
.int	orders;
#endif
#endif

	// used to store pre-push state
	struct
	{
		vector	origin, angles;
	#ifdef USE_SMOOTH_DELTA_ANGLES
		float	deltayaw;
	#endif
	} pushed;

	#ifdef BOTS
	ai_t	ai;
	#endif
};