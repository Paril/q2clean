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
#include "gladiator_model.h"
#include "game/ballistics/hit.h"
#include "gladiator.h"

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif

#ifdef THE_RECKONING
#include "game/xatrix/ballistics/plasma.h"
#endif

static sound_index sound_pain1;
static sound_index sound_pain2;
static sound_index sound_die;
static sound_index sound_gun;
static sound_index sound_cleaver_swing;
static sound_index sound_cleaver_hit;
static sound_index sound_cleaver_miss;
static sound_index sound_idle;
static sound_index sound_search;
static sound_index sound_sight;

#ifdef THE_RECKONING
static sound_index sound_plasgun;
#endif

static void gladiator_idle(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_idle);

static void gladiator_sight(entity &self, entity &)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_sight);

static void gladiator_search(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_search);

static void gladiator_cleaver_swing(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

constexpr mframe_t gladiator_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
constexpr mmove_t gladiator_move_stand = { FRAME_stand1, FRAME_stand7, gladiator_frames_stand };

static REGISTER_SAVABLE_DATA(gladiator_move_stand);

static void gladiator_stand(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_stand);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_stand);

constexpr mframe_t gladiator_frames_walk [] = {
	{ ai_walk, 15 },
	{ ai_walk, 7 },
	{ ai_walk, 6 },
	{ ai_walk, 5 },
	{ ai_walk, 2 },
	{ ai_walk },
	{ ai_walk, 2 },
	{ ai_walk, 8 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 1 },
	{ ai_walk, 8 }
};
constexpr mmove_t gladiator_move_walk = { FRAME_walk1, FRAME_walk16, gladiator_frames_walk };

static REGISTER_SAVABLE_DATA(gladiator_move_walk);

static void gladiator_walk(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_walk);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_walk);

constexpr mframe_t gladiator_frames_run [] = {
	{ ai_run, 23 },
	{ ai_run, 14 },
	{ ai_run, 14 },
	{ ai_run, 21 },
	{ ai_run, 12 },
	{ ai_run, 13 }
};
constexpr mmove_t gladiator_move_run = { FRAME_run1, FRAME_run6, gladiator_frames_run };

static REGISTER_SAVABLE_DATA(gladiator_move_run);

static void gladiator_run(entity &self)
{
	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(gladiator_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(gladiator_move_run);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_run);

static void GaldiatorMelee(entity &self)
{
	const vector aim { MELEE_DISTANCE, self.bounds.mins.x, -4 };

	if (fire_hit(self, aim, (20 + (Q_rand() % 5)), 300))
		gi.sound(self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
}

constexpr mframe_t gladiator_frames_attack_melee [] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladiator_cleaver_swing },
	{ ai_charge },
	{ ai_charge, 0, GaldiatorMelee },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladiator_cleaver_swing },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, GaldiatorMelee },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t gladiator_move_attack_melee = { FRAME_melee1, FRAME_melee17, gladiator_frames_attack_melee, gladiator_run };

static REGISTER_SAVABLE_DATA(gladiator_move_attack_melee);

static void gladiator_melee(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_attack_melee);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_melee);

static void GladiatorGun(entity &self)
{
#ifdef THE_RECKONING
	if (self.mass == 350)
		return;

#endif
	vector  start;
	vector  dir;
	vector  forward, right;

	AngleVectors(self.s.angles, &forward, &right, nullptr);
	start = G_ProjectSource(self.s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right);

	// calc direction to where we targted
	dir = self.pos1 - start;
	VectorNormalize(dir);

	monster_fire_railgun(self, start, dir, 50, 100, MZ2_GLADIATOR_RAILGUN_1);
}

#ifdef THE_RECKONING
static void gladbGun(entity &self)
{
	if (self.mass != 350)
		return;

	vector	start;
	vector	dir;
	vector	forward, right;

	AngleVectors (self.s.angles, &forward, &right, nullptr);
	start = G_ProjectSource (self.s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right);

	// calc direction to where we targted
	dir = self.pos1 - start;
	VectorNormalize(dir);

	fire_plasma (self, start, dir, 100, 725, 60, 60);
}

static void gladbGun_check(entity &self)
{
	if (skill == 3)
		gladbGun (self);
}
#endif

constexpr mframe_t gladiator_frames_attack_gun[] = {
	{ ai_charge },
	{ ai_charge },
#ifdef THE_RECKONING
	{ ai_charge, 0, gladbGun },
#else
	{ ai_charge },
#endif
	{ ai_charge, 0, GladiatorGun },
	{ ai_charge },

#ifdef THE_RECKONING
	{ ai_charge, 0, gladbGun },
#else
	{ ai_charge },
#endif
	{ ai_charge },
	{ ai_charge },

#ifdef THE_RECKONING
	{ ai_charge, 0, gladbGun_check },
#else
	{ ai_charge }
#endif
};
constexpr mmove_t gladiator_move_attack_gun = { FRAME_attack1, FRAME_attack9, gladiator_frames_attack_gun, gladiator_run };

static REGISTER_SAVABLE_DATA(gladiator_move_attack_gun);

static void gladiator_attack(entity &self)
{
	float   range;
	vector  v;

	// a small safe zone
	v = self.s.origin - self.enemy->s.origin;
	range = VectorLength(v);
	if (range <= (MELEE_DISTANCE + 32))
		return;

	// charge up the railgun
#ifdef THE_RECKONING
	if (self.mass == 350)
		gi.sound(self, CHAN_WEAPON, sound_plasgun, 1, ATTN_NORM, 0);
	else
#endif
		gi.sound(self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
	self.pos1 = self.enemy->s.origin;  //save for aiming the shot
	self.pos1[2] += self.enemy->viewheight;
	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_attack_gun);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_attack);

constexpr mframe_t gladiator_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t gladiator_move_pain = { FRAME_pain1, FRAME_pain6, gladiator_frames_pain, gladiator_run };

static REGISTER_SAVABLE_DATA(gladiator_move_pain);

constexpr mframe_t gladiator_frames_pain_air [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t gladiator_move_pain_air = { FRAME_painup1, FRAME_painup7, gladiator_frames_pain_air, gladiator_run };

static REGISTER_SAVABLE_DATA(gladiator_move_pain_air);

static void gladiator_pain(entity &self, entity &, float, int32_t)
{
	if (self.health < (self.max_health / 2))
		self.s.skinnum = 1;

	if (level.framenum < self.pain_debounce_framenum)
	{
		if ((self.velocity[2] > 100) && (self.monsterinfo.currentmove == &gladiator_move_pain))
			self.monsterinfo.currentmove = &SAVABLE(gladiator_move_pain_air);
		return;
	}

	self.pain_debounce_framenum = level.framenum + 3 * BASE_FRAMERATE;

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill == 3)
		return;     // no pain anims in nightmare

	if (self.velocity[2] > 100)
		self.monsterinfo.currentmove = &SAVABLE(gladiator_move_pain_air);
	else
		self.monsterinfo.currentmove = &SAVABLE(gladiator_move_pain);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_pain);

static void gladiator_dead(entity &self)
{
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, -8 }
	};
	self.movetype = MOVETYPE_TOSS;
	self.svflags |= SVF_DEADMONSTER;
	self.nextthink = 0;
	gi.linkentity(self);
}

constexpr mframe_t gladiator_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
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
constexpr mmove_t gladiator_move_death = { FRAME_death1, FRAME_death22, gladiator_frames_death, gladiator_dead };

static REGISTER_SAVABLE_DATA(gladiator_move_death);

static void gladiator_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	int     n;

// check for gib
	if (self.health <= self.gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self.deadflag = DEAD_DEAD;
		return;
	}

	if (self.deadflag == DEAD_DEAD)
		return;

// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self.deadflag = DEAD_DEAD;
	self.takedamage = true;

	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_death);
}

static REGISTER_SAVABLE_FUNCTION(gladiator_die);

#ifdef GROUND_ZERO
static bool gladiator_blocked(entity &self, float dist)
{
	if(blocked_checkshot (self, 0.25f + (0.05f * skill)))
		return true;

	if(blocked_checkplat (self, dist))
		return true;

	return false;
}

static REGISTER_SAVABLE_FUNCTION(gladiator_blocked);
#endif

/*QUAKED monster_gladiator (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
static void SP_monster_gladiator(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	sound_pain1 = gi.soundindex("gladiator/pain.wav");
	sound_pain2 = gi.soundindex("gladiator/gldpain2.wav");
	sound_die = gi.soundindex("gladiator/glddeth2.wav");
	sound_cleaver_swing = gi.soundindex("gladiator/melee1.wav");
	sound_cleaver_hit = gi.soundindex("gladiator/melee2.wav");
	sound_cleaver_miss = gi.soundindex("gladiator/melee3.wav");
	sound_idle = gi.soundindex("gladiator/gldidle1.wav");
	sound_search = gi.soundindex("gladiator/gldsrch1.wav");
	sound_sight = gi.soundindex("gladiator/sight.wav");
	
#ifdef THE_RECKONING
	if (self.type == ET_MONSTER_GLADIATOR_BETA)
	{
		sound_plasgun = gi.soundindex ("weapons/plasshot.wav");
		self.s.modelindex = gi.modelindex ("models/monsters/gladb/tris.md2");

		self.health = 800;
		self.gib_health = -175;
		self.mass = 350;

		self.monsterinfo.power_armor_type = ITEM_POWER_SHIELD;
		self.monsterinfo.power_armor_power = 400;
	}
	else
	{
#endif
		sound_gun = gi.soundindex("gladiator/railgun.wav");
		self.s.modelindex = gi.modelindex("models/monsters/gladiatr/tris.md2");

		self.health = 400;
		self.gib_health = -175;
		self.mass = 400;
#ifdef THE_RECKONING
	}
#endif

	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;
	self.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, 64 }
	};

	self.pain = SAVABLE(gladiator_pain);
	self.die = SAVABLE(gladiator_die);

	self.monsterinfo.stand = SAVABLE(gladiator_stand);
	self.monsterinfo.walk = SAVABLE(gladiator_walk);
	self.monsterinfo.run = SAVABLE(gladiator_run);
	self.monsterinfo.attack = SAVABLE(gladiator_attack);
	self.monsterinfo.melee = SAVABLE(gladiator_melee);
	self.monsterinfo.sight = SAVABLE(gladiator_sight);
	self.monsterinfo.idle = SAVABLE(gladiator_idle);
	self.monsterinfo.search = SAVABLE(gladiator_search);
#ifdef GROUND_ZERO
	self.monsterinfo.blocked = SAVABLE(gladiator_blocked);
#endif

	gi.linkentity(self);
	self.monsterinfo.currentmove = &SAVABLE(gladiator_move_stand);
	self.monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}

REGISTER_ENTITY(MONSTER_GLADIATOR, monster_gladiator);

#ifdef THE_RECKONING
/*QUAKED monster_gladb (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
static void SP_monster_gladb(entity &self)
{
	SP_monster_gladiator(self);
}

REGISTER_ENTITY_INHERIT(MONSTER_GLADIATOR_BETA, monster_gladb, ET_MONSTER_GLADIATOR);
#endif
#endif