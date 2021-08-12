#include "config.h"

#ifdef SINGLE_PLAYER
#include "../entity.h"
#include "../game.h"
#include "../move.h"
#include "../ai.h"
#include "../monster.h"
#include "../misc.h"
#include "../spawn.h"

#include "lib/gi.h"
#include "flash.h"
#include "game/util.h"
#include "flyer_model.h"
#include "game/ballistics/hit.h"

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif

#ifdef GROUND_ZERO
#include "game/combat.h"
#endif

static sound_index  sound_sight;
static sound_index  sound_idle;
static sound_index  sound_pain1;
static sound_index  sound_pain2;
static sound_index  sound_slash;
static sound_index  sound_sproing;
static sound_index  sound_die;

static void flyer_sight(entity &self, entity &)
{
	gi.sound(self, CHAN_VOICE, sound_sight);
}

REGISTER_STATIC_SAVABLE(flyer_sight);

static void flyer_idle(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, ATTN_IDLE);
}

REGISTER_STATIC_SAVABLE(flyer_idle);

static void flyer_pop_blades(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_sproing);
}

constexpr mframe_t flyer_frames_stand [] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
constexpr mmove_t flyer_move_stand = { FRAME_stand01, FRAME_stand45, flyer_frames_stand };

REGISTER_STATIC_SAVABLE(flyer_move_stand);

constexpr mframe_t flyer_frames_walk [] = {
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 }
};
constexpr mmove_t flyer_move_walk = { FRAME_stand01, FRAME_stand45, flyer_frames_walk };

REGISTER_STATIC_SAVABLE(flyer_move_walk);

constexpr mframe_t flyer_frames_run [] = {
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 }
};
constexpr mmove_t flyer_move_run = { FRAME_stand01, FRAME_stand45, flyer_frames_run };

REGISTER_STATIC_SAVABLE(flyer_move_run);

#ifdef GROUND_ZERO
constexpr mframe_t flyer_frames_kamizake [] =
{
	{ ai_charge, 40 },
	{ ai_charge, 40 },
	{ ai_charge, 40 },
	{ ai_charge, 40 },
	{ ai_charge, 40 }
};
constexpr mmove_t flyer_move_kamikaze = { FRAME_rollr02, FRAME_rollr06, flyer_frames_kamizake };

REGISTER_STATIC_SAVABLE(flyer_move_kamikaze);

static void kamikaze_touch(entity &self, entity &, vector, const surface &)
{
	T_RadiusDamage(self, self, (self.mass / 2), nullptr, self.mass, MOD_EXPLOSIVE, { DAMAGE_NO_RADIUS_HALF | DAMAGE_RADIUS });

	// just in case he's still alive... (he usually is because of the way T_RadiusDamage is calculated)
	if (self.health > 0)
		T_Damage(self, self, self, vec3_origin, self.origin, vec3_origin, self.health, 0, { DAMAGE_NO_PROTECTION }, MOD_EXPLOSIVE);
}

REGISTER_STATIC_SAVABLE(kamikaze_touch);
#endif

static void flyer_run(entity &self)
{
#ifdef GROUND_ZERO
	if (self.mass > 50)
	{
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_kamikaze);
		self.touch = SAVABLE(kamikaze_touch);
	}
	else
#endif
	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_run);
}

REGISTER_STATIC_SAVABLE(flyer_run);

static void flyer_walk(entity &self)
{
#ifdef GROUND_ZERO
	if (self.mass > 50)
		flyer_run (self);
	else
#endif
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_walk);
}

REGISTER_STATIC_SAVABLE(flyer_walk);

static void flyer_stand(entity &self)
{
#ifdef GROUND_ZERO
	if (self.mass > 50)
		flyer_run (self);
	else
#endif
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_stand);
}

REGISTER_STATIC_SAVABLE(flyer_stand);

constexpr mframe_t flyer_frames_pain3 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t flyer_move_pain3 = { FRAME_pain301, FRAME_pain304, flyer_frames_pain3, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_pain3);

constexpr mframe_t flyer_frames_pain2 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t flyer_move_pain2 = { FRAME_pain201, FRAME_pain204, flyer_frames_pain2, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_pain2);

constexpr mframe_t flyer_frames_pain1 [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t flyer_move_pain1 = { FRAME_pain101, FRAME_pain109, flyer_frames_pain1, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_pain1);

inline void flyer_fire(entity &self, monster_muzzleflash flash_number)
{
	vector start;
	vector forward, right;
	vector end;
	vector dir;
	entity_effects effect;

	if (!self.enemy.has_value() || !self.enemy->inuse)
		return;

	if ((self.frame == FRAME_attak204) || (self.frame == FRAME_attak207) || (self.frame == FRAME_attak210))
		effect = EF_HYPERBLASTER;
	else
		effect = EF_NONE;

	AngleVectors(self.angles, &forward, &right, nullptr);
	start = G_ProjectSource(self.origin, monster_flash_offset[flash_number], forward, right);

	end = self.enemy->origin;
	end.z += self.enemy->viewheight;
	dir = end - start;

	monster_fire_blaster(self, start, dir, 1, 1000, flash_number, effect);
}

static void flyer_fireleft(entity &self)
{
	flyer_fire(self, MZ2_FLYER_BLASTER_1);
}

static void flyer_fireright(entity &self)
{
	flyer_fire(self, MZ2_FLYER_BLASTER_2);
}

constexpr mframe_t flyer_frames_attack2 [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, -10, flyer_fireleft },         // left gun
	{ ai_charge, -10, flyer_fireright },        // right gun
	{ ai_charge, -10, flyer_fireleft },         // left gun
	{ ai_charge, -10, flyer_fireright },        // right gun
	{ ai_charge, -10, flyer_fireleft },         // left gun
	{ ai_charge, -10, flyer_fireright },        // right gun
	{ ai_charge, -10, flyer_fireleft },         // left gun
	{ ai_charge, -10, flyer_fireright },        // right gun
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t flyer_move_attack2 = { FRAME_attak201, FRAME_attak217, flyer_frames_attack2, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_attack2);

#ifdef ROGUE_AI
constexpr mframe_t flyer_frames_attack3 [] =
{
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10, flyer_fireleft },		// left gun
		{ ai_charge, 10, flyer_fireright },		// right gun
		{ ai_charge, 10, flyer_fireleft },		// left gun
		{ ai_charge, 10, flyer_fireright },		// right gun
		{ ai_charge, 10, flyer_fireleft },		// left gun
		{ ai_charge, 10, flyer_fireright },		// right gun
		{ ai_charge, 10, flyer_fireleft },		// left gun
		{ ai_charge, 10, flyer_fireright },		// right gun
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10 },
		{ ai_charge, 10 }
};
constexpr mmove_t flyer_move_attack3 = { FRAME_attak201, FRAME_attak217, flyer_frames_attack3, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_attack3);
#endif

static void flyer_slash_left(entity &self)
{
	const vector aim { MELEE_DISTANCE, self.bounds.mins.x, 0 };
	fire_hit(self, aim, 5, 0);
	gi.sound(self, CHAN_WEAPON, sound_slash);
}

static void flyer_slash_right(entity &self)
{
	const vector  aim { MELEE_DISTANCE, self.bounds.maxs.x, 0 };
	fire_hit(self, aim, 5, 0);
	gi.sound(self, CHAN_WEAPON, sound_slash);
}

static void flyer_loop_melee(entity &self);

constexpr mframe_t flyer_frames_start_melee [] = {
	{ ai_charge, 0, flyer_pop_blades },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t flyer_move_start_melee = { FRAME_attak101, FRAME_attak106, flyer_frames_start_melee, flyer_loop_melee };

REGISTER_STATIC_SAVABLE(flyer_move_start_melee);

constexpr mframe_t flyer_frames_end_melee [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t flyer_move_end_melee = { FRAME_attak119, FRAME_attak121, flyer_frames_end_melee, flyer_run };

REGISTER_STATIC_SAVABLE(flyer_move_end_melee);

static void flyer_check_melee(entity &self);

constexpr mframe_t flyer_frames_loop_melee [] = {
	{ ai_charge },     // Loop Start
	{ ai_charge },
	{ ai_charge, 0, flyer_slash_left },     // Left Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, flyer_slash_right },    // Right Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }      // Loop Ends
};
constexpr mmove_t flyer_move_loop_melee = { FRAME_attak107, FRAME_attak118, flyer_frames_loop_melee, flyer_check_melee };

REGISTER_STATIC_SAVABLE(flyer_move_loop_melee);

static void flyer_loop_melee(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(flyer_move_loop_melee);
}

static void flyer_attack(entity &self)
{
#ifdef ROGUE_AI
	// 0% chance of circle in easy
	// 50% chance in normal
	// 75% chance in hard
	// 86.67% chance in nightmare
#ifdef GROUND_ZERO
	if (self.mass > 50)
	{
		flyer_run (self);
		return;
	}
#endif

	float chance = 0;
	
	if (skill)
		chance = 1.0f - (0.5f / skill);

	if (random() > chance)
	{
		self.monsterinfo.attack_state = AS_STRAIGHT;
#endif
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_attack2);
#ifdef ROGUE_AI
	}
	else // circle strafe
	{
		if (random () <= 0.5) // switch directions
			self.monsterinfo.lefty = !self.monsterinfo.lefty;
		self.monsterinfo.attack_state = AS_SLIDING;
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_attack3);
	}
#endif
}

REGISTER_STATIC_SAVABLE(flyer_attack);

static void flyer_melee(entity &self)
{
#ifdef GROUND_ZERO
	if (self.mass > 50)
		flyer_run (self);
	else
#endif
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_start_melee);
}

REGISTER_STATIC_SAVABLE(flyer_melee);

static void flyer_check_melee(entity &self)
{
	if (range(self, self.enemy) == RANGE_MELEE)
		if (random() <= 0.8f)
			self.monsterinfo.currentmove = &SAVABLE(flyer_move_loop_melee);
		else
			self.monsterinfo.currentmove = &SAVABLE(flyer_move_end_melee);
	else
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_end_melee);
}

static void flyer_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t)
{
#ifdef GROUND_ZERO
	// kamikaze's don't feel pain
	if (self.mass != 50)
		return;
#endif

	if (level.framenum < self.pain_debounce_framenum)
		return;

	int32_t n = Q_rand() % 3;
	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_pain1);
	else if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain2);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1);

	self.pain_debounce_framenum = level.framenum + 3 * BASE_FRAMERATE;
	if (skill == 3)
		return;     // no pain anims in nightmare

	n = Q_rand() % 3;
	if (n == 0)
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_pain1);
	else if (n == 1)
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_pain2);
	else
		self.monsterinfo.currentmove = &SAVABLE(flyer_move_pain3);
}

REGISTER_STATIC_SAVABLE(flyer_reacttodamage);

static void flyer_die(entity &self, entity &, entity &, int32_t, vector)
{
	gi.sound(self, CHAN_VOICE, sound_die);
	BecomeExplosion1(self);
}

REGISTER_STATIC_SAVABLE(flyer_die);

#ifdef ROGUE_AI
// PMM - kamikaze code .. blow up if blocked	
static bool flyer_blocked (entity &self, float)
{
#ifdef GROUND_ZERO
	// kamikaze = 100, normal = 50
	if (self.mass == 100)
		return false;
#endif

	// we're a normal flyer
	if (blocked_checkshot(self, 0.25f + (0.05f * skill)))
		return true;

	return false;
}

REGISTER_STATIC_SAVABLE(flyer_blocked);
#endif

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_flyer(entity &self)
{
	if (deathmatch) {
		G_FreeEdict(self);
		return;
	}

	// fix a map bug in jail5.bsp
	if (striequals(level.mapname, "jail5") && (self.origin[2] == -104)) {
		self.targetname = self.target;
		self.target = 0;
	}

	sound_sight = gi.soundindex("flyer/flysght1.wav");
	sound_idle = gi.soundindex("flyer/flysrch1.wav");
	sound_pain1 = gi.soundindex("flyer/flypain1.wav");
	sound_pain2 = gi.soundindex("flyer/flypain2.wav");
	sound_slash = gi.soundindex("flyer/flyatck2.wav");
	sound_sproing = gi.soundindex("flyer/flyatck1.wav");
	sound_die = gi.soundindex("flyer/flydeth1.wav");

	gi.soundindex("flyer/flyatck3.wav");

	self.modelindex = gi.modelindex("models/monsters/flyer/tris.md2");
	self.bounds = {
		.mins = { -16, -16, -24 },
#ifdef ROGUE_AI
		.maxs = { 16, 16, 16 }
#else
		.maxs = { 16, 16, 32 }
#endif
	};
	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;

	self.sound = gi.soundindex("flyer/flyidle1.wav");

	self.health = 50;
	self.mass = 50;

	self.pain = SAVABLE(monster_pain);
	self.die = SAVABLE(flyer_die);

	self.monsterinfo.stand = SAVABLE(flyer_stand);
	self.monsterinfo.walk = SAVABLE(flyer_walk);
	self.monsterinfo.run = SAVABLE(flyer_run);
	self.monsterinfo.attack = SAVABLE(flyer_attack);
	self.monsterinfo.melee = SAVABLE(flyer_melee);
	self.monsterinfo.sight = SAVABLE(flyer_sight);
	self.monsterinfo.idle = SAVABLE(flyer_idle);

#ifdef ROGUE_AI
	self.monsterinfo.blocked = SAVABLE(flyer_blocked);
#endif

	self.monsterinfo.reacttodamage = SAVABLE(flyer_reacttodamage);

	gi.linkentity(self);

	self.monsterinfo.currentmove = &SAVABLE(flyer_move_stand);
	self.monsterinfo.scale = MODEL_SCALE;

	flymonster_start(self);
}

static REGISTER_ENTITY(MONSTER_FLYER, monster_flyer);

#ifdef GROUND_ZERO
static void SP_monster_kamikaze(entity &self)
{
	SP_monster_flyer(self);

	self.effects |= EF_ROCKET;
	self.mass = 100;
}

static REGISTER_ENTITY_INHERIT(MONSTER_FLYER_KAMIKAZE, monster_kamikaze, ET_MONSTER_FLYER);
#endif
#endif