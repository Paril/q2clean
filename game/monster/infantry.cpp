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
#include "game/ballistics/hit.h"
#include "infantry_model.h"
#include "infantry.h"

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif

static sound_index	sound_pain1;
static sound_index	sound_pain2;
static sound_index	sound_die1;
static sound_index	sound_die2;

static sound_index	sound_gunshot;
static sound_index	sound_weapon_cock;
static sound_index	sound_punch_swing;
static sound_index	sound_punch_hit;
static sound_index	sound_sight;
static sound_index	sound_search;
static sound_index	sound_idle;

constexpr mframe_t infantry_frames_stand[] = {
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
constexpr mmove_t infantry_move_stand = { FRAME_stand50, FRAME_stand71, infantry_frames_stand };

REGISTER_STATIC_SAVABLE(infantry_move_stand);

static void infantry_stand(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(infantry_move_stand);
}

REGISTER_STATIC_SAVABLE(infantry_stand);

constexpr mframe_t infantry_frames_fidget[] = {
	{ ai_stand, 1 },
	{ ai_stand },
	{ ai_stand, 1 },
	{ ai_stand, 3 },
	{ ai_stand, 6 },
	{ ai_stand, 3 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 1 },
	{ ai_stand },
	{ ai_stand, -1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 1 },
	{ ai_stand },
	{ ai_stand, -2 },
	{ ai_stand, 1 },
	{ ai_stand, 1 },
	{ ai_stand, 1 },
	{ ai_stand, -1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, -1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, -1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 1 },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, -1 },
	{ ai_stand, -1 },
	{ ai_stand },
	{ ai_stand, -3 },
	{ ai_stand, -2 },
	{ ai_stand, -3 },
	{ ai_stand, -3 },
	{ ai_stand, -2 }
};
constexpr mmove_t infantry_move_fidget = { FRAME_stand01, FRAME_stand49, infantry_frames_fidget, infantry_stand };

REGISTER_STATIC_SAVABLE(infantry_move_fidget);

static void infantry_fidget(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(infantry_move_fidget);

	gi.sound(self, CHAN_VOICE, sound_idle, ATTN_IDLE);
}

REGISTER_STATIC_SAVABLE(infantry_fidget);

constexpr mframe_t infantry_frames_walk[] = {
	{ ai_walk, 5 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 5 },
	{ ai_walk, 4 },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 5 }
};
constexpr mmove_t infantry_move_walk = { FRAME_walk03, FRAME_walk14, infantry_frames_walk };

REGISTER_STATIC_SAVABLE(infantry_move_walk);

static void infantry_walk(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(infantry_move_walk);
}

REGISTER_STATIC_SAVABLE(infantry_walk);

constexpr mframe_t infantry_frames_run[] = {
	{ ai_run, 10 },
	{ ai_run, 20 },
	{ ai_run, 5 },
	{ ai_run, 7
#ifdef ROGUE_AI
		, monster_done_dodge
#endif
	},
	{ ai_run, 30 },
	{ ai_run, 35 },
	{ ai_run, 2 },
	{ ai_run, 6 }
};
constexpr mmove_t infantry_move_run = { FRAME_run01, FRAME_run08, infantry_frames_run };

REGISTER_STATIC_SAVABLE(infantry_move_run);

static void infantry_run(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_run);
}

REGISTER_STATIC_SAVABLE(infantry_run);

constexpr mframe_t infantry_frames_pain1[] = {
	{ ai_move, -3 },
	{ ai_move, -2 },
	{ ai_move, -1 },
	{ ai_move, -2 },
	{ ai_move, -1 },
	{ ai_move, 1 },
	{ ai_move, -1 },
	{ ai_move, 1 },
	{ ai_move, 6 },
	{ ai_move, 2 }
};
constexpr mmove_t infantry_move_pain1 = { FRAME_pain101, FRAME_pain110, infantry_frames_pain1, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_pain1);

constexpr mframe_t infantry_frames_pain2[] = {
	{ ai_move, -3 },
	{ ai_move, -3 },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, -2 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move, 5 },
	{ ai_move, 2 }
};
constexpr mmove_t infantry_move_pain2 = { FRAME_pain201, FRAME_pain210, infantry_frames_pain2, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_pain2);

static void infantry_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t)
{
#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if (level.framenum < self.pain_debounce_framenum)
		return;

	self.pain_debounce_framenum = level.framenum + 3 * BASE_FRAMERATE;

	if (skill == 3)
		return;     // no pain anims in nightmare

	int32_t n = Q_rand() % 2;
	if (n == 0)
	{
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_pain1);
		gi.sound(self, CHAN_VOICE, sound_pain1);
	}
	else
	{
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_pain2);
		gi.sound(self, CHAN_VOICE, sound_pain2);
	}

#ifdef ROGUE_AI
	// PMM - clear duck flag
	if (self.monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
#endif
}

REGISTER_STATIC_SAVABLE(infantry_reacttodamage);

static constexpr vector aimangles[] = {
	{ 0, 5, 0 },
	{ 10, 15, 0 },
	{ 20, 25, 0 },
	{ 25, 35, 0 },
	{ 30, 40, 0 },
	{ 30, 45, 0 },
	{ 25, 50, 0 },
	{ 20, 40, 0 },
	{ 15, 35, 0 },
	{ 40, 35, 0 },
	{ 70, 35, 0 },
	{ 90, 35, 0 }
};

static void InfantryMachineGun(entity &self)
{
	vector	start = vec3_origin, target;
	vector	forward, right;
	vector	vec;
	monster_muzzleflash flash_number;

#if defined(THE_RECKONING) || defined(GROUND_ZERO)
	if (self.frame == FRAME_attak103) {
#else
	if (self.frame == FRAME_attak111) {
#endif
		flash_number = MZ2_INFANTRY_MACHINEGUN_1;

		AngleVectors(self.angles, &forward, &right, nullptr);
		start = G_ProjectSource(self.origin, monster_flash_offset[MZ2_INFANTRY_MACHINEGUN_1], forward, right);

		if (self.enemy.has_value()) {
			target = self.enemy->origin + (-0.2f * self.enemy->velocity);
			target.z += self.enemy->viewheight;
			forward = target - start;
			VectorNormalize(forward);
		}
	}
	else {
		flash_number = (monster_muzzleflash) (MZ2_INFANTRY_MACHINEGUN_2 + (self.frame - FRAME_death211));

		AngleVectors(self.angles, &forward, &right, nullptr);
		start = G_ProjectSource(self.origin, monster_flash_offset[flash_number], forward, right);

		vec = self.angles - aimangles[(flash_number - MZ2_INFANTRY_MACHINEGUN_2)];
		AngleVectors(vec, &forward, nullptr, nullptr);
	}

	monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void infantry_sight(entity &self, entity &)
{
	gi.sound(self, CHAN_BODY, sound_sight);
}

REGISTER_STATIC_SAVABLE(infantry_sight);

static void infantry_dead(entity &self)
{
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, -8 }
	};
	self.movetype = MOVETYPE_TOSS;
	self.svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);

	M_FlyCheck(self);
}

constexpr mframe_t infantry_frames_death1[] = {
	{ ai_move, -4 },
	{ ai_move },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, -4 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, 3 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, -2 },
	{ ai_move, 2 },
	{ ai_move, 2 },
	{ ai_move, 9 },
	{ ai_move, 9 },
	{ ai_move, 5 },
	{ ai_move, -3 },
	{ ai_move, -3 }
};
constexpr mmove_t infantry_move_death1 = { FRAME_death101, FRAME_death120, infantry_frames_death1, infantry_dead };

REGISTER_STATIC_SAVABLE(infantry_move_death1);

// Off with his head
constexpr mframe_t infantry_frames_death2[] = {
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move, 5 },
	{ ai_move, -1 },
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, 4 },
	{ ai_move, 3 },
	{ ai_move },
	{ ai_move, -2,  InfantryMachineGun },
	{ ai_move, -2,  InfantryMachineGun },
	{ ai_move, -3,  InfantryMachineGun },
	{ ai_move, -1,  InfantryMachineGun },
	{ ai_move, -2,  InfantryMachineGun },
	{ ai_move, 0,   InfantryMachineGun },
	{ ai_move, 2,   InfantryMachineGun },
	{ ai_move, 2,   InfantryMachineGun },
	{ ai_move, 3,   InfantryMachineGun },
	{ ai_move, -10, InfantryMachineGun },
	{ ai_move, -7,  InfantryMachineGun },
	{ ai_move, -8,  InfantryMachineGun },
	{ ai_move, -6 },
	{ ai_move, 4 },
	{ ai_move }
};
constexpr mmove_t infantry_move_death2 = { FRAME_death201, FRAME_death225, infantry_frames_death2, infantry_dead };

REGISTER_STATIC_SAVABLE(infantry_move_death2);

constexpr mframe_t infantry_frames_death3[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -6 },
	{ ai_move, -11 },
	{ ai_move, -3 },
	{ ai_move, -11 },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t infantry_move_death3 = { FRAME_death301, FRAME_death309, infantry_frames_death3, infantry_dead };

REGISTER_STATIC_SAVABLE(infantry_move_death3);

static void infantry_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	int     n;

	// check for gib
	if (self.health <= self.gib_health) {
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"));
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
	self.deadflag = DEAD_DEAD;
	self.takedamage = true;

	n = Q_rand() % 3;
	if (n == 0) {
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_death1);
		gi.sound(self, CHAN_VOICE, sound_die2);
	}
	else if (n == 1) {
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_death2);
		gi.sound(self, CHAN_VOICE, sound_die1);
	}
	else {
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_death3);
		gi.sound(self, CHAN_VOICE, sound_die2);
	}
}

REGISTER_STATIC_SAVABLE(infantry_die);

#ifdef ROGUE_AI
#define infantry_duck_down monster_duck_down
#define infantry_duck_hold monster_duck_hold
#define infantry_duck_up monster_duck_up
#else
static void infantry_duck_down(entity &self)
{
	if (self.monsterinfo.aiflags & AI_DUCKED)
		return;
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.maxs.z -= 32;
	self.takedamage = true;
	self.monsterinfo.pause_framenum = level.framenum + 1 * BASE_FRAMERATE;
	gi.linkentity(self);
}

static void infantry_duck_hold(entity &self)
{
	if (level.framenum >= self.monsterinfo.pause_framenum)
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self.monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void infantry_duck_up(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_DUCKED;
	self.maxs.z += 32;
	self.takedamage = true;
	gi.linkentity(self);
}
#endif

constexpr mframe_t infantry_frames_duck[] = {
	{ ai_move, -2, infantry_duck_down },
	{ ai_move, -5, infantry_duck_hold },
	{ ai_move, 3 },
	{ ai_move, 4, infantry_duck_up },
	{ ai_move }
};
constexpr mmove_t infantry_move_duck = { FRAME_duck01, FRAME_duck05, infantry_frames_duck, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_duck);

#ifndef ROGUE_AI
static void infantry_dodge(entity &self, entity &attacker, float)
{
	if (random() > 0.25f)
		return;

	if (!self.enemy.has_value())
		self.enemy = attacker;

	self.monsterinfo.currentmove = &SAVABLE(infantry_move_duck);
}

REGISTER_STATIC_SAVABLE(infantry_dodge);
#endif

#if defined(THE_RECKONING) || defined(GROUND_ZERO)
static void infantry_set_firetime(entity &self)
{
	self.monsterinfo.pause_framenum = level.framenum + (Q_rand() & 15) + 5;
}

static void infantry_cock_gun(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_weapon_cock);
}
#else
static void infantry_cock_gun(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_weapon_cock);
	self.monsterinfo.pause_framenum = level.framenum + (Q_rand() & 15) + 3 + 7;
}
#endif

static void infantry_fire(entity &self)
{
	InfantryMachineGun(self);

	if (level.framenum >= self.monsterinfo.pause_framenum)
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self.monsterinfo.aiflags |= AI_HOLD_FRAME;
}

constexpr mframe_t infantry_frames_attack1[] = {
#if defined(THE_RECKONING) || defined(GROUND_ZERO)
	{ ai_charge, 10, infantry_set_firetime },
	{ ai_charge,  6 },
	{ ai_charge,  0, infantry_fire },
	{ ai_charge },
	{ ai_charge,  1 },
	{ ai_charge, -7 },
	{ ai_charge, -6 },
	{ ai_charge, -1 },
	{ ai_charge,  0, infantry_cock_gun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, -1 },
	{ ai_charge, -1 }
#else
	{ ai_charge, 4 },
	{ ai_charge, -1 },
	{ ai_charge, -1 },
	{ ai_charge, 0,  infantry_cock_gun },
	{ ai_charge, -1 },
	{ ai_charge, 1 },
	{ ai_charge, 1 },
	{ ai_charge, 2 },
	{ ai_charge, -2 },
	{ ai_charge, -3 },
	{ ai_charge, 1,  infantry_fire },
	{ ai_charge, 5 },
	{ ai_charge, -1 },
	{ ai_charge, -2 },
	{ ai_charge, -3 }
#endif
};
constexpr mmove_t infantry_move_attack1 = { FRAME_attak101, FRAME_attak115, infantry_frames_attack1, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_attack1);

static void infantry_swing(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_punch_swing);
}

static void infantry_smack(entity &self)
{
	constexpr vector aim { MELEE_DISTANCE, 0, 0 };
	if (fire_hit(self, aim, (5 + (Q_rand() % 5)), 50))
		gi.sound(self, CHAN_WEAPON, sound_punch_hit);
}

constexpr mframe_t infantry_frames_attack2[] = {
	{ ai_charge, 3 },
	{ ai_charge, 6 },
	{ ai_charge, 0, infantry_swing },
	{ ai_charge, 8 },
	{ ai_charge, 5 },
	{ ai_charge, 8, infantry_smack },
	{ ai_charge, 6 },
	{ ai_charge, 3 },
};
constexpr mmove_t infantry_move_attack2 = { FRAME_attak201, FRAME_attak208, infantry_frames_attack2, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_attack2);

static void infantry_attack(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if (range(self, self.enemy) == RANGE_MELEE)
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_attack2);
	else
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_attack1);
}

REGISTER_STATIC_SAVABLE(infantry_attack);

#ifdef ROGUE_AI
static void infantry_jump_now(entity &self)
{
	vector	forward,up;

	monster_jump_start(self);

	AngleVectors(self.angles, &forward, nullptr, &up);
	self.velocity += forward * 100;
	self.velocity += up * 300;
}

static void infantry_jump_wait_land(entity &self)
{
	if (self.groundentity == null_entity)
	{
		self.monsterinfo.nextframe = self.frame;

		if (monster_jump_finished(self))
			self.monsterinfo.nextframe = self.frame + 1;
	}
	else
		self.monsterinfo.nextframe = self.frame + 1;
}

constexpr mframe_t infantry_frames_jump[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, infantry_jump_now },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, infantry_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t infantry_move_jump = { FRAME_jump01, FRAME_jump10, infantry_frames_jump, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_jump);

constexpr mframe_t infantry_frames_jump2[] = {
	{ ai_move, -8 },
	{ ai_move, -4 },
	{ ai_move, -4 },
	{ ai_move, 0, infantry_jump_now },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, infantry_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t infantry_move_jump2 = { FRAME_jump01, FRAME_jump10, infantry_frames_jump2, infantry_run };

REGISTER_STATIC_SAVABLE(infantry_move_jump2);

static void infantry_jump(entity &self)
{
	if (!self.enemy.has_value())
		return;

	monster_done_dodge(self);

	if (self.enemy->origin[2] > self.origin[2])
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_jump2);
	else
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_jump);
}

static bool infantry_blocked(entity &self, float dist)
{
	if (blocked_checkshot(self, 0.25f + (0.05f * skill)))
		return true;

	if (blocked_checkjump(self, dist, 192, 40))
	{
		infantry_jump(self);
		return true;
	}

	if (blocked_checkplat(self, dist))
		return true;

	return false;
}

REGISTER_STATIC_SAVABLE(infantry_blocked);

static void infantry_duck(entity &self, float eta)
{
	// if we're jumping, don't dodge
	if ((self.monsterinfo.currentmove == &infantry_move_jump) ||
		(self.monsterinfo.currentmove == &infantry_move_jump2))
	{
		return;
	}

	if ((self.monsterinfo.currentmove == &infantry_move_attack1) ||
		(self.monsterinfo.currentmove == &infantry_move_attack2))
	{
		// if we're shooting, and not on easy, don't dodge
		if (skill)
		{
			self.monsterinfo.aiflags &= ~AI_DUCKED;
			return;
		}
	}

	if (!skill)
		// PMM - stupid dodge
		self.monsterinfo.duck_wait_framenum = level.framenum + (int) ((eta + 1) * BASE_FRAMERATE);
	else
		self.monsterinfo.duck_wait_framenum = level.framenum + (int) ((eta + (0.1 * (3 - skill))) * BASE_FRAMERATE);

	// has to be done immediately otherwise he can get stuck
	monster_duck_down(self);

	self.monsterinfo.nextframe = FRAME_duck01;
	self.monsterinfo.currentmove = &SAVABLE(infantry_move_duck);
}

REGISTER_STATIC_SAVABLE(infantry_duck);

static void infantry_sidestep(entity &self)
{
	// if we're jumping, don't dodge
	if ((self.monsterinfo.currentmove == &infantry_move_jump) ||
		(self.monsterinfo.currentmove == &infantry_move_jump2))
	{
		return;
	}

	if ((self.monsterinfo.currentmove == &infantry_move_attack1) ||
		(self.monsterinfo.currentmove == &infantry_move_attack2))
	{
		// if we're shooting, and not on easy, don't dodge
		if (skill)
		{
			self.monsterinfo.aiflags &= ~AI_DODGING;
			return;
		}
	}

	if (self.monsterinfo.currentmove != &infantry_move_run)
		self.monsterinfo.currentmove = &SAVABLE(infantry_move_run);
}

REGISTER_STATIC_SAVABLE(infantry_sidestep);
#endif

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_infantry(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	sound_pain1 = gi.soundindex("infantry/infpain1.wav");
	sound_pain2 = gi.soundindex("infantry/infpain2.wav");
	sound_die1 = gi.soundindex("infantry/infdeth1.wav");
	sound_die2 = gi.soundindex("infantry/infdeth2.wav");

	sound_gunshot = gi.soundindex("infantry/infatck1.wav");
	sound_weapon_cock = gi.soundindex("infantry/infatck3.wav");
	sound_punch_swing = gi.soundindex("infantry/infatck2.wav");
	sound_punch_hit = gi.soundindex("infantry/melee2.wav");

	sound_sight = gi.soundindex("infantry/infsght1.wav");
	sound_search = gi.soundindex("infantry/infsrch1.wav");
	sound_idle = gi.soundindex("infantry/infidle1.wav");

	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;
	self.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, 32 }
	};

	self.health = 100;
	self.gib_health = -40;
	self.mass = 200;

	self.pain = SAVABLE(monster_pain);
	self.die = SAVABLE(infantry_die);

	self.monsterinfo.stand = SAVABLE(infantry_stand);
	self.monsterinfo.walk = SAVABLE(infantry_walk);
	self.monsterinfo.run = SAVABLE(infantry_run);
#ifdef ROGUE_AI
	self.monsterinfo.dodge = SAVABLE(M_MonsterDodge);
	self.monsterinfo.duck = SAVABLE(infantry_duck);
	self.monsterinfo.unduck = SAVABLE(monster_duck_up);
	self.monsterinfo.sidestep = SAVABLE(infantry_sidestep);
	self.monsterinfo.blocked = SAVABLE(infantry_blocked);
#else
	self.monsterinfo.dodge = SAVABLE(infantry_dodge);
#endif
	self.monsterinfo.attack = SAVABLE(infantry_attack);
	self.monsterinfo.sight = SAVABLE(infantry_sight);
	self.monsterinfo.idle = SAVABLE(infantry_fidget);
	self.monsterinfo.reacttodamage = SAVABLE(infantry_reacttodamage);

	gi.linkentity(self);

	self.monsterinfo.currentmove = &SAVABLE(infantry_move_stand);
	self.monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}

REGISTER_ENTITY(MONSTER_INFANTRY, monster_infantry);
#endif