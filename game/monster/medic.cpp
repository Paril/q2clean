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
#include "medic_model.h"
#include "medic.h"

#if defined(ROGUE_AI)
#include "game/combat.h"
#endif

#if defined(ROGUE_AI) || defined(GROUND_ZERO)
#include "game/rogue/ai.h"
#endif

#ifdef GROUND_ZERO
#endif

#ifdef ROGUE_AI
constexpr int32_t MEDIC_MIN_DISTANCE	= 32;
constexpr float MEDIC_MAX_HEAL_DISTANCE	= 400.f;
constexpr gtime MEDIC_TRY_TIME			= 10s;
#endif

static sound_index sound_idle1;
static sound_index sound_pain1;
static sound_index sound_pain2;
static sound_index sound_die;
static sound_index sound_sight;
static sound_index sound_search;
static sound_index sound_hook_launch;
static sound_index sound_hook_hit;
static sound_index sound_hook_heal;
static sound_index sound_hook_retract;

#ifdef GROUND_ZERO
#include "game/rogue/ballistics/tesla.h"
#include "game/rogue/monster.h"
#include "game/monster/soldier.h"
#include "game/monster/infantry.h"
#include "game/monster/gunner.h"
#include "game/monster/gladiator.h"

// PMM - commander sounds
static sound_index commander_sound_idle1;
static sound_index commander_sound_pain1;
static sound_index commander_sound_pain2;
static sound_index commander_sound_die;
static sound_index commander_sound_sight;
static sound_index commander_sound_search;
static sound_index commander_sound_hook_launch;
static sound_index commander_sound_hook_hit;
static sound_index commander_sound_hook_heal;
static sound_index commander_sound_hook_retract;
static sound_index commander_sound_spawn;

constexpr const entity_type_ref reinforcements[] = {
	ET_MONSTER_SOLDIER_LIGHT,
	ET_MONSTER_SOLDIER,
	ET_MONSTER_SOLDIER_SS,
	ET_MONSTER_INFANTRY,
	ET_MONSTER_GUNNER,
	ET_MONSTER_MEDIC,
	ET_MONSTER_GLADIATOR
};

constexpr vector reinforcement_mins[] = {
	{ -16, -16, -24 },
	{ -16, -16, -24 },
	{ -16, -16, -24 },
	{ -16, -16, -24 },
	{ -16, -16, -24 },
	{ -16, -16, -24 },
	{ -32, -32, -24 }
};

constexpr vector reinforcement_maxs[] = {
	{ 16, 16, 32 },
	{ 16, 16, 32 },
	{ 16, 16, 32 },
	{ 16, 16, 32 },
	{ 16, 16, 32 },
	{ 16, 16, 32 },
	{ 32, 32, 64 }
};

constexpr vector reinforcement_position[] = {
	{ 80, 0, 0 },
	{ 40, 60, 0 },
	{ 40, -60, 0 },
	{ 0, 80, 0 },
	{ 0, -80, 0 }
};
#endif

#ifdef ROGUE_AI
static void cleanupHeal(entity &self, bool change_frame)
{
	// clean up target, if we have one and it's legit
	if (self.enemy.has_value() && self.enemy->inuse)
	{
		self.enemy->monsterinfo.healer = 0;
		self.enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
		self.enemy->takedamage = true;
		M_SetEffects (self.enemy);
	}

	if (change_frame)
		self.monsterinfo.nextframe = FRAME_attack52;
}

static void abortHeal(entity &self, bool change_frame, bool gib, bool mark)
{
	int hurt;
	constexpr vector pain_normal { 0, 0, 1 };

	// clean up target
	cleanupHeal (self, change_frame);

	// gib em!
	if (mark && self.enemy.has_value() && self.enemy->inuse)
	{
		// if the first badMedic slot is filled by a medic, skip it and use the second one
		if ((self.enemy->monsterinfo.badMedic1.has_value()) && (self.enemy->monsterinfo.badMedic1->inuse)
			&& (self.enemy->monsterinfo.badMedic1->type == ET_MONSTER_MEDIC))
			self.enemy->monsterinfo.badMedic2 = self;
		else
			self.enemy->monsterinfo.badMedic1 = self;
	}

	if (gib && self.enemy.has_value() && self.enemy->inuse)
	{
		if (self.enemy->gib_health)
			hurt = -self.enemy->gib_health;
		else
			hurt = 500;

		T_Damage (self.enemy, self, self, vec3_origin, self.enemy->origin,
			pain_normal, hurt, 0, { DAMAGE_NONE });
	}
	// clean up self

	self.monsterinfo.aiflags &= ~AI_MEDIC;
	if (self.oldenemy.has_value() && self.oldenemy->inuse)
		self.enemy = self.oldenemy;
	else
		self.enemy = 0;

	self.monsterinfo.medicTries = 0;
}
#endif

static entityref medic_FindDeadMonster(entity &self)
{
	entityref best;
	float radius = 1024;
#ifdef ROGUE_AI

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		radius = MEDIC_MAX_HEAL_DISTANCE;
#endif

	for (entity &ent : G_IterateRadius(self.origin, radius))
	{
		if (ent == self)
			continue;
		if (!(ent.svflags & SVF_MONSTER))
			continue;
		if (ent.monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
#ifdef ROGUE_AI
		// check to make sure we haven't bailed on this guy already
		if ((ent.monsterinfo.badMedic1 == self) || (ent.monsterinfo.badMedic2 == self))
			continue;
		if (ent.monsterinfo.healer.has_value())
			// FIXME - this is correcting a bug that is somewhere else
			// if the healer is a monster, and it's in medic mode .. continue .. otherwise
			//   we will override the healer, if it passes all the other tests
			if ((ent.monsterinfo.healer->inuse) && (ent.monsterinfo.healer->health > 0) &&
				(ent.monsterinfo.healer->svflags & SVF_MONSTER) && (ent.monsterinfo.healer->monsterinfo.aiflags & AI_MEDIC))
				continue;
#else
		if (ent.owner.has_value())
			continue;
#endif
		if (ent.health > 0)
			continue;
		if (ent.nextthink != gtime::zero() && ent.think != M_FliesOn)
			continue;
		if (!visible(self, ent))
			continue;
#ifdef ROGUE_AI
		if (VectorDistance(self.origin, ent.origin) <= MEDIC_MIN_DISTANCE)
			continue;
#endif
		if (!best.has_value() || ent.max_health > best->max_health)
			best = ent;
	}
#ifdef ROGUE_AI

	if (best.has_value())
		self.timestamp = level.time + MEDIC_TRY_TIME;
#endif

	return best;
}

static void medic_idle(entity &self)
{
	gi.sound (self, CHAN_VOICE,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_idle1 :
#endif
		sound_idle1, ATTN_IDLE);

#ifdef ROGUE_AI
	if (!self.oldenemy.has_value())
	{
#endif
		entityref ent = medic_FindDeadMonster(self);

		if (ent.has_value())
		{
#ifdef ROGUE_AI
			self.oldenemy = self.enemy;
#endif
			self.enemy = ent;
#ifdef ROGUE_AI
			self.enemy->monsterinfo.healer = self;
#else
			self.enemy->owner = self;
#endif
			self.monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
		}
#ifdef ROGUE_AI
	}
#endif
}

REGISTER_STATIC_SAVABLE(medic_idle);

static void medic_search(entity &self)
{
	gi.sound(self, CHAN_VOICE,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_search :
#endif
		sound_search, ATTN_IDLE);

	if (!self.oldenemy.has_value())
	{
		entityref ent = medic_FindDeadMonster(self);

		if (ent.has_value())
		{
			self.oldenemy = self.enemy;
			self.enemy = ent;
#ifdef ROGUE_AI
			self.enemy->monsterinfo.healer = self;
#else
			self.enemy->owner = self;
#endif
			self.monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
		}
	}
}

REGISTER_STATIC_SAVABLE(medic_search);

static void medic_sight(entity &self, entity &)
{
	gi.sound (self, CHAN_VOICE,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_sight :
#endif
		sound_sight);
}

REGISTER_STATIC_SAVABLE(medic_sight);

constexpr mframe_t medic_frames_stand[] = {
	{ ai_stand, 0, medic_idle },
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
constexpr mmove_t medic_move_stand = { FRAME_wait1, FRAME_wait90, medic_frames_stand };

REGISTER_STATIC_SAVABLE(medic_move_stand);

static void medic_stand(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(medic_move_stand);
}

REGISTER_STATIC_SAVABLE(medic_stand);

constexpr mframe_t medic_frames_walk[] = {
	{ ai_walk, 6.2f },
	{ ai_walk, 18.1f },
	{ ai_walk, 1 },
	{ ai_walk, 9 },
	{ ai_walk, 10 },
	{ ai_walk, 9 },
	{ ai_walk, 11 },
	{ ai_walk, 11.6f },
	{ ai_walk, 2 },
	{ ai_walk, 9.9f },
	{ ai_walk, 14.f },
	{ ai_walk, 9.3f }
};
constexpr mmove_t medic_move_walk = { FRAME_walk1, FRAME_walk12, medic_frames_walk };

REGISTER_STATIC_SAVABLE(medic_move_walk);

static void medic_walk(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(medic_move_walk);
}

REGISTER_STATIC_SAVABLE(medic_walk);

constexpr mframe_t medic_frames_run [] = {
	{ ai_run, 18 },
	{ ai_run, 22.5f },
	{ ai_run, 25.4f
#ifdef ROGUE_AI
		, monster_done_dodge
#endif
	},
	{ ai_run, 23.4f },
	{ ai_run, 24 },
	{ ai_run, 35.6f }

};
constexpr mmove_t medic_move_run = { FRAME_run1, FRAME_run6, medic_frames_run };

REGISTER_STATIC_SAVABLE(medic_move_run);

static void medic_run(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);
#endif

	if (!(self.monsterinfo.aiflags & AI_MEDIC))
	{
		entityref ent = medic_FindDeadMonster(self);

		if (ent.has_value())
		{
			self.oldenemy = self.enemy;
			self.enemy = ent;
#ifdef ROGUE_AI
			self.enemy->monsterinfo.healer = self;
#else
			self.enemy->owner = self;
#endif
			self.monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
			return;
		}
	}

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(medic_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(medic_move_run);
}

REGISTER_STATIC_SAVABLE(medic_run);

constexpr mframe_t medic_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t medic_move_pain1 = { FRAME_paina1, FRAME_paina8, medic_frames_pain1, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_pain1);

constexpr mframe_t medic_frames_pain2[] = {
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
constexpr mmove_t medic_move_pain2 = { FRAME_painb1, FRAME_painb15, medic_frames_pain2, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_pain2);

static void medic_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t damage [[maybe_unused]])
{
#ifdef ROGUE_AI
	monster_done_dodge (self);
#endif

	if (level.time < self.pain_debounce_time)
		return;

	self.pain_debounce_time = level.time + 3s;

	if (skill == 3)
		return;     // no pain anims in nightmare

#ifdef GROUND_ZERO
	// if we're healing someone, we ignore pain
	if (self.monsterinfo.aiflags & AI_MEDIC)
		return;

	if (self.mass > 400)
	{
		if (damage < 35)
		{
			gi.sound (self, CHAN_VOICE, commander_sound_pain1);
			return;
		}

		self.monsterinfo.aiflags &= ~(AI_MANUAL_STEERING | AI_HOLD_FRAME);

		gi.sound (self, CHAN_VOICE, commander_sound_pain2);

		if (random() < min(damage * 0.005f, 0.5f))		// no more than 50% chance of big pain
			self.monsterinfo.currentmove = &SAVABLE(medic_move_pain2);
		else
			self.monsterinfo.currentmove = &SAVABLE(medic_move_pain1);
	}
	else
#endif
	if (random() < 0.5f)
	{
		self.monsterinfo.currentmove = &SAVABLE(medic_move_pain1);
		gi.sound(self, CHAN_VOICE, sound_pain1);
	}
	else
	{
		self.monsterinfo.currentmove = &SAVABLE(medic_move_pain2);
		gi.sound(self, CHAN_VOICE, sound_pain2);
	}

#ifdef ROGUE_AI
	if (self.monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
#endif
}

REGISTER_STATIC_SAVABLE(medic_reacttodamage);

static void medic_fire_blaster(entity &self)
{
	vector start;
	vector forward, right;
	vector end;
	vector dir;
	entity_effects effect = EF_NONE;
	int32_t damage = 2;

	if (!self.enemy.has_value() || !self.enemy->inuse)
		return;

	if ((self.frame == FRAME_attack9) || (self.frame == FRAME_attack12))
		effect = EF_BLASTER;
	else if ((self.frame == FRAME_attack19) || (self.frame == FRAME_attack22) || (self.frame == FRAME_attack25) || (self.frame == FRAME_attack28))
		effect = EF_HYPERBLASTER;

	AngleVectors(self.angles, &forward, &right, nullptr);
	start = G_ProjectSource(self.origin, monster_flash_offset[MZ2_MEDIC_BLASTER_1], forward, right);

	end = self.enemy->origin;
	end[2] += self.enemy->viewheight;
	dir = end - start;

#ifdef GROUND_ZERO
	if (self.enemy->type == ET_TESLA)
		damage = 3;

	if (self.mass > 400)
		monster_fire_blaster2(self, start, dir, damage, 1000, MZ2_MEDIC_BLASTER_2, effect & ~EF_HYPERBLASTER);
	else
#endif
		monster_fire_blaster(self, start, dir, damage, 1000, MZ2_MEDIC_BLASTER_1, effect);
}


static void medic_dead(entity &self)
{
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, -8 }
	};
	self.movetype = MOVETYPE_TOSS;
	self.svflags |= SVF_DEADMONSTER;
	self.nextthink = gtime::zero();
	gi.linkentity(self);
}

constexpr mframe_t medic_frames_death[] = {
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
constexpr mmove_t medic_move_death = { FRAME_death1, FRAME_death30, medic_frames_death, medic_dead };

REGISTER_STATIC_SAVABLE(medic_move_death);

static void medic_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	int     n;

#ifndef ROGUE_AI
	// if we had a pending patient, free him up for another medic
	if (self.enemy.has_value() && self.enemy->owner == self)
		self.enemy->owner = 0;
#endif

// check for gib
	if (self.health <= self.gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"));
		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self.deadflag = true;
		return;
	}

	if (self.deadflag)
		return;

// regular death
	gi.sound(self, CHAN_VOICE,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_die :
#endif
		sound_die);
	self.deadflag = true;
	self.takedamage = true;

	self.monsterinfo.currentmove = &SAVABLE(medic_move_death);
}

REGISTER_STATIC_SAVABLE(medic_die);

#ifdef ROGUE_AI
#define medic_duck_down monster_duck_down
#define medic_duck_hold monster_duck_hold
#define medic_duck_up monster_duck_up
#else
static void medic_duck_down(entity &self)
{
	if (self.monsterinfo.aiflags & AI_DUCKED)
		return;
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.bounds.maxs[2] -= 32;
	self.takedamage = true;
	self.monsterinfo.pause_time = level.time + 1s;
	gi.linkentity(self);
}

static void medic_duck_hold(entity &self)
{
	if (level.time >= self.monsterinfo.pause_time)
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self.monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void medic_duck_up(entity &self)
{
	self.monsterinfo.aiflags &= ~AI_DUCKED;
	self.bounds.maxs[2] += 32;
	self.takedamage = true;
	gi.linkentity(self);
}
#endif

constexpr mframe_t medic_frames_duck[] = {
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1,    medic_duck_down },
	{ ai_move, -1,    medic_duck_hold },
	{ ai_move, -1 },
	{ ai_move, -1 },
#ifndef ROGUE_AI
	{ ai_move, -1,    medic_duck_up },
#endif
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, -1 },
#ifdef ROGUE_AI
	{ ai_move, -1,    medic_duck_up },
#endif
	{ ai_move, -1 },
	{ ai_move, -1 }
};
constexpr mmove_t medic_move_duck = { FRAME_duck1, FRAME_duck16, medic_frames_duck, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_duck);

#ifndef ROGUE_AI
static void medic_dodge(entity &self, entity &attacker, gtimef)
{
	if (random() > 0.25f)
		return;

	if (!self.enemy.has_value())
		self.enemy = attacker;

	self.monsterinfo.currentmove = &SAVABLE(medic_move_duck);
}

REGISTER_STATIC_SAVABLE(medic_dodge);
#endif

constexpr mframe_t medic_frames_attackHyperBlaster[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge, 0,   medic_fire_blaster }
};
constexpr mmove_t medic_move_attackHyperBlaster = { FRAME_attack15, FRAME_attack30, medic_frames_attackHyperBlaster, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_attackHyperBlaster);

static void medic_continue(entity &self)
{
	if (visible(self, self.enemy))
		if (random() <= 0.95f)
			self.monsterinfo.currentmove = &SAVABLE(medic_move_attackHyperBlaster);
}

constexpr mframe_t medic_frames_attackBlaster [] = {
	{ ai_charge },
	{ ai_charge, 5 },
	{ ai_charge, 5 },
	{ ai_charge, 3 },
	{ ai_charge, 2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0,   medic_fire_blaster },
	{ ai_charge },
	{ ai_charge, 0,   medic_continue }  // Change to medic_continue... Else, go to frame 32
};
constexpr mmove_t medic_move_attackBlaster = { FRAME_attack1, FRAME_attack14, medic_frames_attackBlaster, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_attackBlaster);

static void medic_hook_launch(entity &self)
{
	gi.sound(self, CHAN_WEAPON,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_hook_launch :
#endif
		sound_hook_launch);
}

constexpr vector medic_cable_offsets[] = {
	{ 45.0f,  -9.2f, 15.5f },
	{ 48.4f,  -9.7f, 15.2f },
	{ 47.8f,  -9.8f, 15.8f },
	{ 47.3f,  -9.3f, 14.3f },
	{ 45.4f, -10.1f, 13.1f },
	{ 41.9f, -12.7f, 12.0f },
	{ 37.8f, -15.8f, 11.2f },
	{ 34.3f, -18.4f, 10.7f },
	{ 32.7f, -19.7f, 10.4f },
	{ 32.7f, -19.7f, 10.4f }
};

static void medic_cable_attack(entity &self)
{
	vector  offset, start, end, f, r;
	vector  dir;
	float   distance;

#ifdef ROGUE_AI
	if (!self.enemy.has_value() || !self.enemy->inuse || (self.enemy->effects & EF_GIB) || self.enemy->is_client || self.enemy->health > 0)
	{
		abortHeal (self, true, false, false);
		return;
	}
#else
	if (!self.enemy.has_value() || !self.enemy->inuse)
		return;
#endif

	AngleVectors(self.angles, &f, &r, nullptr);
	offset = medic_cable_offsets[self.frame - FRAME_attack42];
	start = G_ProjectSource(self.origin, offset, f, r);

	// check for max distance
	dir = start - self.enemy->origin;
	distance = VectorLength(dir);

#ifdef ROGUE_AI
	if (distance < MEDIC_MIN_DISTANCE)
	{
		abortHeal (self, true, true, false);
		return;
	}
#else
	if (distance > 256)
		return;
#endif

	// check for min/max pitch
#ifndef ROGUE_AI
	vector angles = vectoangles(dir);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 45)
		return;
#endif

	trace tr = gi.traceline(start, self.enemy->origin, self, MASK_SOLID);

	if (tr.fraction != 1.0f && tr.ent != self.enemy)
#ifdef ROGUE_AI
	{
		if (tr.ent.is_world())
		{
			// give up on second try
			if (self.monsterinfo.medicTries > 1)
			{
				abortHeal (self, true, false, true);
				return;
			}
			self.monsterinfo.medicTries++;
			cleanupHeal (self, 1);
			return;
		}

		abortHeal (self, true, false, false);
#endif
		return;
#ifdef ROGUE_AI
	}
#endif

	if (self.frame == FRAME_attack43)
	{
		self.enemy->takedamage = false;
		M_SetEffects (self.enemy);

		gi.sound(self.enemy, CHAN_AUTO,
#ifdef GROUND_ZERO
			(self.mass > 400) ? commander_sound_hook_hit :
#endif
			sound_hook_hit);
		self.enemy->monsterinfo.aiflags |= AI_RESURRECTING;
	}
	else if (self.frame == FRAME_attack50)
	{
		self.enemy->spawnflags = NO_SPAWNFLAGS;
		self.enemy->monsterinfo.aiflags = AI_NONE;
		self.enemy->target = 0;
		self.enemy->targetname = 0;
		self.enemy->combattarget = 0;
		self.enemy->deathtarget = 0;
#ifdef ROGUE_AI
		self.enemy->monsterinfo.healer = self;
#else
		self.enemy->owner = self;
#endif

#ifdef ROGUE_AI
		vector cmaxs = self.enemy->bounds.maxs;
		cmaxs[2] += 48;   // compensate for change when they die

		tr = gi.trace (self.enemy->origin, { .mins = self.enemy->absbounds.mins, .maxs = cmaxs }, self.enemy->origin, self.enemy, MASK_MONSTERSOLID);

		if (tr.startsolid || tr.allsolid)
		{
			abortHeal (self, true, true, false);
			return;
		} 
		else if (!tr.ent.is_world())
		{
			abortHeal (self, true, true, false);
			return;
		}
		else
		{
			self.enemy->monsterinfo.aiflags |= AI_DO_NOT_COUNT;
#endif

			st.classname = FindClassnameFromEntityType(self.enemy->type);

			ED_CallSpawn(self.enemy);

#ifndef ROGUE_AI
			self.enemy->owner = 0;
#endif
			if (self.enemy->think)
			{
				self.enemy->nextthink = level.time;
				self.enemy->think(self.enemy);
			}
#if defined(ROGUE_AI) || defined(GROUND_ZERO)
			self.enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
#endif
#ifdef ROGUE_AI
			self.enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
			self.enemy->monsterinfo.aiflags |= AI_DO_NOT_COUNT;
			// turn off flies
			self.enemy->effects &= ~EF_FLIES;
			self.enemy->monsterinfo.healer = 0;

			if (self.oldenemy.has_value() && self.oldenemy->inuse && self.oldenemy->health > 0)
			{
#else
			self.enemy->monsterinfo.aiflags |= AI_RESURRECTING;

			if (self.oldenemy.has_value() && self.oldenemy->is_client)
			{
#endif
				self.enemy->enemy = self.oldenemy;
				FoundTarget(self.enemy);
			}
#ifdef ROGUE_AI
			else
			{
				self.enemy->enemy = 0;

				if (!FindTarget (self.enemy))
				{
					// no valid enemy, so stop acting
					self.enemy->monsterinfo.pause_time = gtime::max();
					self.enemy->monsterinfo.stand (self.enemy);
				}

				self.enemy = self.oldenemy = 0;

				if (!FindTarget (self))
				{
					// no valid enemy, so stop acting
					self.monsterinfo.pause_time = gtime::max();
					self.monsterinfo.stand (self);
					return;
				}
			}
		}
#endif
	}
	else if (self.frame == FRAME_attack44)
		gi.sound(self, CHAN_WEAPON,
#ifdef GROUND_ZERO
			(self.mass > 400) ? commander_sound_hook_heal :
#endif
			sound_hook_heal);

	// adjust start for beam origin being in middle of a segment
	start = start + (8 * f);

	// adjust end z for end spot since the monster is currently dead
	end = self.enemy->origin;
	end[2] = self.enemy->absbounds.mins[2] + self.enemy->size[2] / 2;

	gi.ConstructMessage(svc_temp_entity, TE_MEDIC_CABLE_ATTACK, self, start, end).multicast(self.origin, MULTICAST_PVS);
}

static void medic_hook_retract(entity &self)
{
	gi.sound(self, CHAN_WEAPON,
#ifdef GROUND_ZERO
		(self.mass > 400) ? commander_sound_hook_retract :
#endif
		sound_hook_retract);

#ifdef ROGUE_AI
	self.monsterinfo.aiflags &= ~AI_MEDIC;

	if (self.oldenemy.has_value() && self.oldenemy->inuse)
		self.enemy = self.oldenemy;
	else
	{
		self.enemy = self.oldenemy = 0;
		if (!FindTarget (self))
		{
			// no valid enemy, so stop acting
			self.monsterinfo.pause_time = gtime::max();
			self.monsterinfo.stand (self);
			return;
		}
	}
#else
	self.enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
#endif
}

constexpr mframe_t medic_frames_attackCable[] = {
#ifdef ROGUE_AI
	{ ai_charge, 2 },					//33
	{ ai_charge, 3 },
	{ ai_charge, 5 },
	{ ai_charge, -4.4f },					//36
	{ ai_charge, -4.7f },					//37
	{ ai_charge, -5 },
	{ ai_charge, -6 },
	{ ai_charge, -4 },					//40
#else
	{ ai_move, 2 },
	{ ai_move, 3 },
	{ ai_move, 5 },
	{ ai_move, 4.4f },
	{ ai_charge, 4.7f },
	{ ai_charge, 5 },
	{ ai_charge, 6 },
	{ ai_charge, 4 },
#endif
	{ ai_charge },
	{ ai_move, 0,     medic_hook_launch },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, 0,     medic_cable_attack },
	{ ai_move, -15,   medic_hook_retract },
	{ ai_move, -1.5f },
	{ ai_move, -1.2f },
	{ ai_move, -3 },
	{ ai_move, -2 },
	{ ai_move, 0.3f },
	{ ai_move, 0.7f },
	{ ai_move, 1.2f },
	{ ai_move, 1.3f }
};
constexpr mmove_t medic_move_attackCable = { FRAME_attack33, FRAME_attack60, medic_frames_attackCable, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_attackCable);

#ifdef GROUND_ZERO
static void medic_start_spawn(entity &self)
{
	gi.sound (self, CHAN_WEAPON, commander_sound_spawn);
	self.monsterinfo.nextframe = FRAME_attack48;
}

static void medic_determine_spawn(entity &self)
{
	vector	f, r, offset, startpoint, spawnpoint;
	int		count;
	int		inc;
	int		num_summoned; // should be 1, 3, or 5
	int		num_success = 0;

	float lucky = random();
	int32_t summonStr = (int32_t) skill;

	// bell curve - 0 = 67%, 1 = 93%, 2 = 99% -- too steep
	// this ends up with
	// -3 = 5%
	// -2 = 10%
	// -1 = 15%
	// 0  = 40%
	// +1 = 15%
	// +2 = 10%
	// +3 = 5%
	if (lucky < 0.05)
		summonStr -= 3;
	else if (lucky < 0.15)
		summonStr -= 2;
	else if (lucky < 0.3)
		summonStr -= 1;
	else if (lucky > 0.95)
		summonStr += 3;
	else if (lucky > 0.85)
		summonStr += 2;
	else if (lucky > 0.7)
		summonStr += 1;

	if (summonStr < 0)
		summonStr = 0;

	self.monsterinfo.summon_type = summonStr;
	AngleVectors (self.angles, &f, &r, nullptr);

// this yields either 1, 3, or 5
	if (summonStr)
		num_summoned = (summonStr - 1) + (summonStr % 2);
	else
		num_summoned = 1;

	for (count = 0; count < num_summoned; count++)
	{
		inc = count + (count%2); // 0, 2, 2, 4, 4
		offset = reinforcement_position[count];

		startpoint = G_ProjectSource (self.origin, offset, f, r);
		// a little off the ground
		startpoint[2] += 10;

		if (FindSpawnPoint (startpoint, reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], spawnpoint, 32))
		{
			if (CheckGroundSpawnPoint(spawnpoint, 
				reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], 
				256, -1))
			{
				num_success++;
				// we found a spot, we're done here
				count = num_summoned;
			}
		}
	}

	if (num_success == 0)
	{
		for (count = 0; count < num_summoned; count++)
		{
			inc = count + (count%2); // 0, 2, 2, 4, 4
			offset = reinforcement_position[count];
			
			// check behind
			offset[0] *= -1;
			offset[1] *= -1;

			startpoint = G_ProjectSource (self.origin, offset, f, r);
			// a little off the ground
			startpoint[2] += 10;

			if (FindSpawnPoint (startpoint, reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], spawnpoint, 32))
			{
				if (CheckGroundSpawnPoint(spawnpoint, 
					reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], 
					256, -1))
				{
					num_success++;
					// we found a spot, we're done here
					count = num_summoned;
				}
			}
		}

		if (num_success)
		{
			self.monsterinfo.aiflags |= AI_MANUAL_STEERING;
			self.ideal_yaw = anglemod(self.angles[YAW]) + 180;
			if (self.ideal_yaw > 360.0f)
				self.ideal_yaw -= 360.0f;
		}
	}

	if (num_success == 0)
		self.monsterinfo.nextframe = FRAME_attack53;
}

static void medic_spawngrows(entity &self)
{
	vector	f, r, offset, startpoint, spawnpoint;
	int		count;
	int		inc;
	int		num_summoned; // should be 1, 3, or 5
	int		num_success = 0;
	float	current_yaw;

	// if we've been directed to turn around
	if (self.monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		current_yaw = anglemod(self.angles[YAW]);
		if (fabs(current_yaw - self.ideal_yaw) > 0.1)
		{
			self.monsterinfo.aiflags |= AI_HOLD_FRAME;
			return;
		}

		// done turning around
		self.monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self.monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
	}

	int32_t summonStr = self.monsterinfo.summon_type;

	AngleVectors (self.angles, &f, &r, nullptr);

	if (summonStr)
		num_summoned = (summonStr - 1) + (summonStr % 2);
	else
		num_summoned = 1;

	for (count = 0; count < num_summoned; count++)
	{
		inc = count + (count%2); // 0, 2, 2, 4, 4
		offset = reinforcement_position[count];

		startpoint = G_ProjectSource (self.origin, offset, f, r);
		// a little off the ground
		startpoint[2] += 10;

		if (FindSpawnPoint (startpoint, reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], spawnpoint, 32))
		{
			if (CheckGroundSpawnPoint(spawnpoint, 
				reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], 
				256, -1))
			{
				num_success++;
				if ((summonStr-inc) > 3)
					SpawnGrow_Spawn (spawnpoint, 1);		// big monster
				else
					SpawnGrow_Spawn (spawnpoint, 0);		// normal size
			}
		}
	}

	if (num_success == 0)
		self.monsterinfo.nextframe = FRAME_attack53;
}

static void medic_finish_spawn(entity &self)
{
	vector	f, r, offset, startpoint, spawnpoint;
	int		count;
	int		inc;
	int		num_summoned; // should be 1, 3, or 5
	bool		behind = false;

	if (self.monsterinfo.summon_type < 0)
	{
		behind = true;
		self.monsterinfo.summon_type *= -1;
	}
	int32_t summonStr = self.monsterinfo.summon_type;

	AngleVectors (self.angles, &f, &r, nullptr);

	if (summonStr)
		num_summoned = (summonStr - 1) + (summonStr % 2);
	else
		num_summoned = 1;

	for (count = 0; count < num_summoned; count++)
	{
		inc = count + (count%2); // 0, 2, 2, 4, 4
		offset = reinforcement_position[count];

		startpoint = G_ProjectSource (self.origin, offset, f, r);

		// a little off the ground
		startpoint[2] += 10;

		entityref ent = 0;
		if (FindSpawnPoint (startpoint, reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc], spawnpoint, 32))
		{
			if (CheckSpawnPoint (spawnpoint, reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc]))
				ent = CreateGroundMonster (spawnpoint, self.angles,
					reinforcement_mins[summonStr-inc], reinforcement_maxs[summonStr-inc],
					reinforcements[summonStr-inc], 256);
		}

		if (!ent.has_value())
			continue;

		if (ent->think)
		{
			ent->nextthink = level.time;
			ent->think(ent);
		}

		ent->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT | AI_SPAWNED_MEDIC_C;
		ent->monsterinfo.commander = self;
		self.monsterinfo.monster_slots--;

		entityref designated_enemy;

		if (self.monsterinfo.aiflags & AI_MEDIC)
			designated_enemy = self.oldenemy;
		else
			designated_enemy = self.enemy;

		if (coop)
		{
			designated_enemy = PickCoopTarget(ent);

			if (designated_enemy.has_value())
			{
				// try to avoid using my enemy
				if (designated_enemy == self.enemy)
				{
					designated_enemy = PickCoopTarget(ent);

					if (!designated_enemy.has_value())
						designated_enemy = self.enemy;
				}
			}
			else
				designated_enemy = self.enemy;
		}

		if (designated_enemy.has_value() && designated_enemy->inuse && designated_enemy->health > 0)
		{
			ent->enemy = designated_enemy;
			FoundTarget(ent);
		}
		else
		{
			ent->enemy = 0;
			ent->monsterinfo.stand(ent);
		}
	}
}

constexpr mframe_t medic_frames_callReinforcements [] =
{
	// ROGUE - 33-36 now ai_charge
	{ ai_charge, 2 },					//33
	{ ai_charge, 3 },
	{ ai_charge, 5 },
	{ ai_charge, 4.4f },					//36
	{ ai_charge, 4.7f },
	{ ai_charge, 5 },
	{ ai_charge, 6 },
	{ ai_charge, 4 },					//40
	{ ai_charge },
	{ ai_move, 0,		medic_start_spawn },		//42
	{ ai_move },		//43 -- 43 through 47 are skipped
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0,		medic_determine_spawn },		//48
	{ ai_charge, 0,		medic_spawngrows },			//49
	{ ai_move },		//50
	{ ai_move },		//51
	{ ai_move, -15,	medic_finish_spawn },		//52
	{ ai_move, -1.5f },
	{ ai_move, -1.2f },
	{ ai_move, -3 },
	{ ai_move, -2 },
	{ ai_move, 0.3f },
	{ ai_move, 0.7f },
	{ ai_move, 1.2f },
	{ ai_move, 1.3f }					//60
};
constexpr mmove_t medic_move_callReinforcements = { FRAME_attack33, FRAME_attack60, medic_frames_callReinforcements, medic_run };

REGISTER_STATIC_SAVABLE(medic_move_callReinforcements);

#endif
static void medic_attack(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge (self);

#endif
#ifdef GROUND_ZERO
	// signal from checkattack to spawn
	if (self.monsterinfo.aiflags & AI_BLOCKED)
	{
		self.monsterinfo.currentmove = &SAVABLE(medic_move_callReinforcements);
		self.monsterinfo.aiflags &= ~AI_BLOCKED;
	}
#endif
#ifdef GROUND_ZERO
	float r = random();
#endif

	if (self.monsterinfo.aiflags & AI_MEDIC)
	{
#ifdef GROUND_ZERO
		if ((self.mass > 400) && (r > 0.8) && (self.monsterinfo.monster_slots > 2))
			self.monsterinfo.currentmove = &SAVABLE(medic_move_callReinforcements);
		else
#endif
			self.monsterinfo.currentmove = &SAVABLE(medic_move_attackCable);
	}
	else
	{
#ifdef GROUND_ZERO
		if (self.monsterinfo.attack_state == AS_BLIND)
		{
			self.monsterinfo.currentmove = &SAVABLE(medic_move_callReinforcements);
			return;
		}

		if ((self.mass > 400) && (r > 0.2) && (VectorDistance(self.origin, self.enemy->origin) >= RANGE_MELEE) && (self.monsterinfo.monster_slots > 2))
			self.monsterinfo.currentmove = &SAVABLE(medic_move_callReinforcements);
		else
#endif
			self.monsterinfo.currentmove = &SAVABLE(medic_move_attackBlaster);
	}
}

REGISTER_STATIC_SAVABLE(medic_attack);

static bool medic_checkattack(entity &self)
{
	if (self.monsterinfo.aiflags & AI_MEDIC)
	{
#ifdef ROGUE_AI
		// if our target went away
		if (!self.enemy.has_value() || !self.enemy->inuse)
		{
			abortHeal(self, true, false, false);
			return false;
		}

		// if we ran out of time, give up
		if (self.timestamp < level.time)
		{
			abortHeal(self, true, false, true);
			self.timestamp = gtime::zero();
			return false;
		}
	
		if (VectorDistance(self.origin, self.enemy->origin) < MEDIC_MAX_HEAL_DISTANCE + 10)
		{
#endif
			medic_attack(self);
			return true;
#ifdef ROGUE_AI
		}
		else
		{
			self.monsterinfo.attack_state = AS_STRAIGHT;
			return false;
		}
#endif
	}

#ifdef GROUND_ZERO
	if (self.enemy->is_client && !visible(self, self.enemy) && (self.monsterinfo.monster_slots > 2))
	{
		self.monsterinfo.attack_state = AS_BLIND;
		return true;
	}

	// give a LARGE bias to spawning things when we have room
	// use AI_BLOCKED as a signal to attack to spawn
	if ((random() < 0.8) && (self.monsterinfo.monster_slots > 5) && (VectorDistance(self.origin, self.enemy->origin) > 150))
	{
		self.monsterinfo.aiflags |= AI_BLOCKED;
		self.monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	// since his idle animation looks kinda bad in combat, if we're not in easy mode, always attack
	// when he's on a combat point
	if (skill > 0)
		if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		{
			self.monsterinfo.attack_state = AS_MISSILE;
			return true;
		}
#endif

	return M_CheckAttack(self);
}

REGISTER_STATIC_SAVABLE(medic_checkattack);

#ifdef GROUND_ZERO
static void MedicCommanderCache()
{
	//FIXME - better way to do this?  this is quick and dirty
	for (int32_t i = 0; i < 7; i++)
	{
		entity &newEnt = G_Spawn();

		newEnt.type = reinforcements[i];

		newEnt.monsterinfo.aiflags |= AI_DO_NOT_COUNT;

		ED_CallSpawn(newEnt);

		if (newEnt.inuse)
			G_FreeEdict (newEnt);
	}

	gi.modelindex("models/items/spawngro/tris.md2");
	gi.modelindex("models/items/spawngro2/tris.md2");
}
#endif

#ifdef ROGUE_AI
static void medic_duck(entity &self, gtimef eta)
{
	//	don't dodge if you're healing
	if (self.monsterinfo.aiflags & AI_MEDIC)
		return;

	if ((self.monsterinfo.currentmove == &medic_move_attackHyperBlaster) ||
		(self.monsterinfo.currentmove == &medic_move_attackCable) ||
		(self.monsterinfo.currentmove == &medic_move_attackBlaster)
#ifdef GROUND_ZERO
		|| (self.monsterinfo.currentmove == &medic_move_callReinforcements)
#endif
		)
	{
		// he ignores skill
		self.monsterinfo.aiflags &= ~AI_DUCKED;
		return;
	}

	if (!skill)
		// PMM - stupid dodge
		self.monsterinfo.duck_wait_time = duration_cast<gtime>(level.time + eta + 1s);
	else
		self.monsterinfo.duck_wait_time = duration_cast<gtime>(level.time + eta + (100ms * (3 - skill)));

	// has to be done immediately otherwise he can get stuck
	monster_duck_down(self);

	self.monsterinfo.nextframe = FRAME_duck1;
	self.monsterinfo.currentmove = &SAVABLE(medic_move_duck);
}

REGISTER_STATIC_SAVABLE(medic_duck);

static void medic_sidestep(entity &self)
{
	if ((self.monsterinfo.currentmove == &medic_move_attackHyperBlaster) ||
		(self.monsterinfo.currentmove == &medic_move_attackCable) ||
		(self.monsterinfo.currentmove == &medic_move_attackBlaster)
#ifdef GROUND_ZERO
		|| (self.monsterinfo.currentmove == &medic_move_callReinforcements)
#endif
		)
	{
		// if we're shooting, and not on easy, don't dodge
		if (skill)
		{
			self.monsterinfo.aiflags &= ~AI_DODGING;
			return;
		}
	}

	if (self.monsterinfo.currentmove != &medic_move_run)
		self.monsterinfo.currentmove = &SAVABLE(medic_move_run);
}

REGISTER_STATIC_SAVABLE(medic_sidestep);

static bool medic_blocked(entity &self, float dist)
{
	if (blocked_checkshot(self, 0.25f + (0.05f * skill)))
		return true;

	if (blocked_checkplat(self, dist))
		return true;

	return false;
}

REGISTER_STATIC_SAVABLE(medic_blocked);
#endif

/*QUAKED monster_medic (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_medic(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;
	self.modelindex = gi.modelindex("models/monsters/medic/tris.md2");
	self.bounds = {
		.mins = { -24, -24, -24 },
		.maxs = { 24, 24, 32 }
	};

#if defined(GROUND_ZERO) || defined(ROGUE_AI)
	self.monsterinfo.aiflags |= AI_IGNORE_SHOTS;
#endif

#ifdef ROGUE_AI
	self.monsterinfo.blocked = SAVABLE(medic_blocked);
	self.monsterinfo.dodge = SAVABLE(M_MonsterDodge);
	self.monsterinfo.duck = SAVABLE(medic_duck);
	self.monsterinfo.unduck = SAVABLE(monster_duck_up);
	self.monsterinfo.sidestep = SAVABLE(medic_sidestep);
#else
	self.monsterinfo.dodge = SAVABLE(medic_dodge);
#endif

#ifdef GROUND_ZERO
	if (self.type == ET_MONSTER_MEDIC_COMMANDER)
	{
		// commander sounds
		commander_sound_idle1 = gi.soundindex ("medic_commander/medidle.wav");
		commander_sound_pain1 = gi.soundindex ("medic_commander/medpain1.wav");
		commander_sound_pain2 = gi.soundindex ("medic_commander/medpain2.wav");
		commander_sound_die = gi.soundindex ("medic_commander/meddeth.wav");
		commander_sound_sight = gi.soundindex ("medic_commander/medsght.wav");
		commander_sound_search = gi.soundindex ("medic_commander/medsrch.wav");
		commander_sound_hook_launch = gi.soundindex ("medic_commander/medatck2c.wav");
		commander_sound_hook_hit = gi.soundindex ("medic_commander/medatck3a.wav");
		commander_sound_hook_heal = gi.soundindex ("medic_commander/medatck4a.wav");
		commander_sound_hook_retract = gi.soundindex ("medic_commander/medatck5a.wav");
		commander_sound_spawn = gi.soundindex ("medic_commander/monsterspawn1.wav");
		gi.soundindex ("tank/tnkatck3.wav");

		MedicCommanderCache();

		self.health = 600;
		self.gib_health = -130;
		self.mass = 600;
		self.yaw_speed = 40.f;

		if (skill == 0)
			self.monsterinfo.monster_slots = 3;
		else if (skill == 1)
			self.monsterinfo.monster_slots = 4;
		else
			self.monsterinfo.monster_slots = 6;
	}
	else
	{
#endif
		sound_idle1 = gi.soundindex("medic/idle.wav");
		sound_pain1 = gi.soundindex("medic/medpain1.wav");
		sound_pain2 = gi.soundindex("medic/medpain2.wav");
		sound_die = gi.soundindex("medic/meddeth1.wav");
		sound_sight = gi.soundindex("medic/medsght1.wav");
		sound_search = gi.soundindex("medic/medsrch1.wav");
		sound_hook_launch = gi.soundindex("medic/medatck2.wav");
		sound_hook_hit = gi.soundindex("medic/medatck3.wav");
		sound_hook_heal = gi.soundindex("medic/medatck4.wav");
		sound_hook_retract = gi.soundindex("medic/medatck5.wav");

		gi.soundindex("medic/medatck1.wav");
		
		self.health = 300;
		self.gib_health = -130;
		self.mass = 400;
#ifdef GROUND_ZERO
	}
#endif

	self.pain = SAVABLE(monster_pain);
	self.die = SAVABLE(medic_die);

	self.monsterinfo.stand = SAVABLE(medic_stand);
	self.monsterinfo.walk = SAVABLE(medic_walk);
	self.monsterinfo.run = SAVABLE(medic_run);
	self.monsterinfo.attack = SAVABLE(medic_attack);
	self.monsterinfo.sight = SAVABLE(medic_sight);
	self.monsterinfo.idle = SAVABLE(medic_idle);
	self.monsterinfo.search = SAVABLE(medic_search);
	self.monsterinfo.checkattack = SAVABLE(medic_checkattack);
	self.monsterinfo.reacttodamage = SAVABLE(medic_reacttodamage);

	gi.linkentity(self);

	self.monsterinfo.currentmove = &SAVABLE(medic_move_stand);
	self.monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}

REGISTER_ENTITY(MONSTER_MEDIC, monster_medic);

#ifdef GROUND_ZERO
static void SP_monster_medic_commander(entity &self)
{
	SP_monster_medic(self);
	self.skinnum = 2;
}

REGISTER_ENTITY_INHERIT(MONSTER_MEDIC_COMMANDER, monster_medic_commander, ET_MONSTER_MEDIC);
#endif
#endif