#pragma once

import "config.h";
import "lib/protocol.h";
import "lib/string.h";
import "game/items/itemlist.h";
import "lib/types.h";
import "lib/math/vector.h";
import "lib/savables.h";
import "lib/types/enum.h";
import "entity_types.h";

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
	FL_MECHANICAL = 1 << 12,	// entity is mechanical, use sparks not blood
	FL_DAMAGEABLE = 1 << 13,	// damageable, even if it normally couldn't be
#ifdef SINGLE_PLAYER
	FL_DISGUISED = 1 << 14,	// entity is in disguise, monsters will not recognize.
#endif
	FL_NOGIB = 1 << 15,		// player has been vaporized by a nuke, drop no gibs
#endif
	
	FL_RESPAWN = 1 << 16

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
using painfunc = void(*)(entity &, entity &, float, int32_t);
using diefunc = void(*)(entity &, entity &, entity &, int32_t, vector);

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

using mendfunc = void(*)(entity &);

// global monster functions
#if defined(SINGLE_PLAYER)
using aifunc = void(*)(entity &, float);

struct mframe_t
{
	aifunc		aifunc;
	float		dist;
	thinkfunc	thinkfunc;
};

struct mmove_t
{
	int			firstframe;
	int			lastframe;
	mframe_t	*frame;
	mendfunc	endfunc;

	constexpr mmove_t(int first, int last, mframe_t *frame, mendfunc endfunc = nullptr) :
		firstframe(first),
		lastframe(last),
		frame(frame),
		endfunc(endfunc)
	{
	}
};

//monster ai flags
enum ai_flags : int32_t
{
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
	AI_RESURRECTING			= 1 << 14
	
#ifdef GROUND_ZERO
	, AI_WALK_WALLS			= 1 << 15,
	AI_MANUAL_STEERING		= 1 << 16,
	AI_TARGET_ANGER			= 1 << 17,
	AI_DODGING				= 1 << 18,
	AI_CHARGING				= 1 << 19,
	AI_HINT_PATH			= 1 << 20,
	AI_IGNORE_SHOTS			= 1 << 21,
	AI_DO_NOT_COUNT			= 1 << 22,	// set for healed monsters
	AI_SPAWNED_CARRIER		= 1 << 23,	//
	AI_SPAWNED_MEDIC_C		= 1 << 24,	// both do_not_count and spawned are set for spawned monsters
	AI_SPAWNED_WIDOW		= 1 << 25,	//
	AI_BLOCKED				= 1 << 26,	// used by blocked_checkattack: set to say I'm attacking while blocked (prevents run-attacks)

	AI_GOOD_GUY_MASK		= AI_GOOD_GUY | AI_DO_NOT_COUNT,
	AI_SPAWNED_MASK			= AI_SPAWNED_CARRIER | AI_SPAWNED_MEDIC_C | AI_SPAWNED_WIDOW	// mask to catch all three flavors of spawned
#else
	, AI_GOOD_GUY_MASK		= AI_GOOD_GUY
#endif
};

MAKE_ENUM_BITWISE(ai_flags);

//monster attack state
enum ai_attack_state : int
{
	AS_STRAIGHT = 1,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE

#ifdef GROUND_ZERO
	, AS_BLIND	// PMM - used by boss code to do nasty things even if it can't see you
#endif
};

#ifdef GROUND_ZERO
using mdodgefunc = void(*)(entity &, entity &, float, trace &);
#else
using mdodgefunc = void(*)(entity &, entity &, float);
#endif

using monster_func = void(*)(entity &);
using monster_sightfunc = void(*)(entity &, entity &);
using monster_checkattack = bool(*)(entity &);
#endif

// entity is inherited from the server entity, and contains game-local
// data.
struct entity : public server_entity
{
friend server_entity;
friend void WipeEntities();
friend void G_InitEdict(entity &);
friend void G_FreeEdict(entity &);

private:
	// an error here means you're using entity as a value type. Always use entity&
	// to pass things around.
	entity() { }
	// Entities can also not be copied; use a.copy(b)
	// to do a proper copy, which resets members that would break the game.
	entity(entity&) { };

	// move constructor not allowed; entities can't be "deleted"
	// so move constructor would be weird
	entity(entity&&) = delete;

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
	savable_function<thinkfunc>	prethink;
	savable_function<thinkfunc>	think;

	savable_function<blockedfunc>	blocked;
	savable_function<touchfunc>		touch;
	savable_function<usefunc>		use;
	savable_function<painfunc>		pain;
	savable_function<diefunc>		die;

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

		savable_function<mendfunc>	endfunc;
	} moveinfo;

#ifdef SINGLE_PLAYER
	struct {
		savable_data<mmove_t>	currentmove;
		ai_flags	aiflags;
		int         nextframe;
		float       scale;
	
		savable_function<monster_func>			stand;
		savable_function<monster_func>			idle;
		savable_function<monster_func>			search;
		savable_function<monster_func>			walk;
		savable_function<monster_func>			run;
		savable_function<mdodgefunc>			dodge;
		savable_function<monster_func>			attack;
		savable_function<monster_func>			melee;
		savable_function<monster_sightfunc>		sight;
		savable_function<monster_checkattack>	checkattack;
	
		gtime	pause_framenum;
		gtime	attack_finished;
	
		vector				saved_goal;
		gtime				search_framenum;
		gtime				trail_framenum;
		vector				last_sighting;
		ai_attack_state		attack_state;
		bool				lefty;
		gtime				idle_framenum;
		int					linkcount;
	
		gitem_id	power_armor_type;
		int			power_armor_power;
	
	#ifdef GROUND_ZERO
		bool(entity self, float dist)	blocked;
		gtime		last_hint_framenum;		// last time the monster checked for hintpaths.
		entity	goal_hint;			// which hint_path we're trying to get to
		int		medicTries;
		entity	badMedic1, badMedic2;	// these medics have declared this monster "unhealable"
		entity	healer;	// this is who is healing this monster
		void(entity self, float eta)	duck;
		void(entity self)				unduck;
		void(entity self)				sidestep;
		float	base_height;
		gtime		next_duck_framenum;
		gtime		duck_wait_framenum;
		entity	last_player_enemy;
		// blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
		// (set in the monster) of the next shot
		bool	blindfire;		// will the monster blindfire?
		gtime		blind_fire_framedelay;
		vector	blind_fire_target;
		// used by the spawners to not spawn too much and keep track of #s of monsters spawned
		int		monster_slots;
		int		monster_used;
		entity	commander;
		// powerup timers, used by widow, our friend
		gtime	quad_framenum;
		gtime	invincible_framenum;
		gtime	double_framenum;
	#endif
	} monsterinfo;

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
	int	orders;
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
};

// fetch an entity by their number.
entity &itoe(size_t index);

// fetch an entity's number
constexpr size_t etoi(const entity &ent)
{
	return ent.s.number;
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

import "game/client.h";