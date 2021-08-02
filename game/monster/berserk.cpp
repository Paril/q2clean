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
#include "berserk_model.h"

#ifdef ROGUE_AI
#include "game/rogue/ai.h"
#endif

static sound_index sound_pain;
static sound_index sound_die;
static sound_index sound_idle;
static sound_index sound_punch;
static sound_index sound_sight;
static sound_index sound_search;

static void berserk_sight(entity &self, entity &)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static REGISTER_SAVABLE_FUNCTION(berserk_sight);

static void berserk_search(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static REGISTER_SAVABLE_FUNCTION(berserk_search);

static void berserk_stand(entity &self);

constexpr mframe_t berserk_frames_stand_fidget[] = {
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
constexpr mmove_t berserk_move_stand_fidget = { FRAME_standb1, FRAME_standb20, berserk_frames_stand_fidget, berserk_stand };

static REGISTER_SAVABLE_DATA(berserk_move_stand_fidget);

static void berserk_fidget(entity &self)
{
	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() > 0.15f)
		return;

	self.monsterinfo.currentmove = &SAVABLE(berserk_move_stand_fidget);
	gi.sound(self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}

constexpr mframe_t berserk_frames_stand[] = {
	{ ai_stand, 0, berserk_fidget },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
constexpr mmove_t berserk_move_stand = { FRAME_stand1, FRAME_stand5, berserk_frames_stand };

static REGISTER_SAVABLE_DATA(berserk_move_stand);

static void berserk_stand(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(berserk_move_stand);
}

static REGISTER_SAVABLE_FUNCTION(berserk_stand);

constexpr mframe_t berserk_frames_walk[] = {
	{ ai_walk, 9.1f },
	{ ai_walk, 6.3f },
	{ ai_walk, 4.9f },
	{ ai_walk, 6.7f },
	{ ai_walk, 6.0f },
	{ ai_walk, 8.2f },
	{ ai_walk, 7.2f },
	{ ai_walk, 6.1f },
	{ ai_walk, 4.9f },
	{ ai_walk, 4.7f },
	{ ai_walk, 4.7f }
};
constexpr mmove_t berserk_move_walk = { FRAME_walkc1, FRAME_walkc11, berserk_frames_walk };

static REGISTER_SAVABLE_DATA(berserk_move_walk);

static void berserk_walk(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(berserk_move_walk);
}

static REGISTER_SAVABLE_FUNCTION(berserk_walk);

constexpr mframe_t berserk_frames_run1[] = {
	{ ai_run, 21 },
	{ ai_run, 11 },
	{ ai_run, 21 },
	{ ai_run, 25
#ifdef ROGUE_AI
	, monster_done_dodge
#endif
},
	{ ai_run, 18 },
	{ ai_run, 19 }
};
constexpr mmove_t berserk_move_run1 = { FRAME_run1, FRAME_run6, berserk_frames_run1 };

static REGISTER_SAVABLE_DATA(berserk_move_run1);

static void berserk_run(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_run1);
}

static REGISTER_SAVABLE_FUNCTION(berserk_run);

static void berserk_attack_spike(entity &self)
{
	constexpr vector aim { MELEE_DISTANCE, 0, -24 };
	fire_hit(self, aim, (15 + (Q_rand() % 6)), 400);    //  Faster attack -- upwards and backwards
}

static void berserk_swing(entity &self)
{
	gi.sound(self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0);
}

constexpr mframe_t berserk_frames_attack_spike[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, berserk_swing },
	{ ai_charge, 0, berserk_attack_spike },
	{ ai_charge  },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t berserk_move_attack_spike = { FRAME_att_c1, FRAME_att_c8, berserk_frames_attack_spike, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_attack_spike);

static void berserk_attack_club(entity &self)
{
	const vector aim { MELEE_DISTANCE, self.bounds.mins.x, -4.f };
	fire_hit(self, aim, (5 + (Q_rand() % 6)), 400);     // Slower attack
}

constexpr mframe_t berserk_frames_attack_club[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, berserk_swing },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, berserk_attack_club },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
constexpr mmove_t berserk_move_attack_club = { FRAME_att_c9, FRAME_att_c20, berserk_frames_attack_club, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_attack_club);

static void berserk_melee(entity &self)
{
#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if ((Q_rand() % 2) == 0)
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_attack_spike);
	else
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_attack_club);
}

static REGISTER_SAVABLE_FUNCTION(berserk_melee);

constexpr mframe_t berserk_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t berserk_move_pain1 = { FRAME_painc1, FRAME_painc4, berserk_frames_pain1, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_pain1);

constexpr mframe_t berserk_frames_pain2[] = {
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
constexpr mmove_t berserk_move_pain2 = { FRAME_painb1, FRAME_painb20, berserk_frames_pain2, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_pain2);

static void berserk_pain(entity &self, entity &, float, int32_t damage)
{
	if (self.health < (self.max_health / 2))
		self.s.skinnum = 1;

	if (level.framenum < self.pain_debounce_framenum)
		return;

	self.pain_debounce_framenum = level.framenum + 3 * BASE_FRAMERATE;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill == 3)
		return;     // no pain anims in nightmare

#ifdef ROGUE_AI
	monster_done_dodge(self);
#endif

	if ((damage < 20) || (random() < 0.5f))
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_pain1);
	else
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_pain2);
}

static REGISTER_SAVABLE_FUNCTION(berserk_pain);

static void berserk_dead(entity &self)
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

constexpr mframe_t berserk_frames_death1[] = {
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
constexpr mmove_t berserk_move_death1 = { FRAME_death1, FRAME_death13, berserk_frames_death1, berserk_dead };

static REGISTER_SAVABLE_DATA(berserk_move_death1);

constexpr mframe_t berserk_frames_death2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t berserk_move_death2 = { FRAME_deathc1, FRAME_deathc8, berserk_frames_death2, berserk_dead };

static REGISTER_SAVABLE_DATA(berserk_move_death2);

static void berserk_die(entity &self, entity &, entity &, int damage, vector)
{
	int     n;

	if (self.health <= self.gib_health) {
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

	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self.deadflag = DEAD_DEAD;
	self.takedamage = true;

	if (damage >= 50)
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_death1);
	else
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_death2);
}

static REGISTER_SAVABLE_FUNCTION(berserk_die);

#ifdef ROGUE_AI
static void berserk_jump_now(entity &self)
{
	vector	forward, up;

	monster_jump_start(self);

	AngleVectors(self.s.angles, &forward, nullptr, &up);
	self.velocity += 100 * forward;
	self.velocity += 300 * up;
}

static void berserk_jump_wait_land(entity &self)
{
	if (self.groundentity == null_entity)
	{
		self.monsterinfo.nextframe = self.s.frame;

		if (monster_jump_finished(self))
			self.monsterinfo.nextframe = self.s.frame + 1;
	}
	else
		self.monsterinfo.nextframe = self.s.frame + 1;
}

constexpr mframe_t berserk_frames_jump[] =
{
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, berserk_jump_now },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, berserk_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t berserk_move_jump = { FRAME_jump1, FRAME_jump9, berserk_frames_jump, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_jump);

constexpr mframe_t berserk_frames_jump2[] =
{
	{ ai_move, -8 },
	{ ai_move, -4 },
	{ ai_move, -4 },
	{ ai_move, 0, berserk_jump_now },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, berserk_jump_wait_land },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t berserk_move_jump2 = { FRAME_jump1, FRAME_jump9, berserk_frames_jump2, berserk_run };

static REGISTER_SAVABLE_DATA(berserk_move_jump2);

static void berserk_jump(entity &self)
{
	if (!self.enemy.has_value())
		return;

	monster_done_dodge(self);

	if (self.enemy->s.origin[2] > self.s.origin[2])
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_jump2);
	else
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_jump);
}

static bool berserk_blocked(entity &self, float dist)
{
	if (blocked_checkjump(self, dist, 256, 40))
	{
		berserk_jump(self);
		return true;
	}

	if (blocked_checkplat(self, dist))
		return true;

	return false;
}

static REGISTER_SAVABLE_FUNCTION(berserk_blocked);

static void berserk_sidestep(entity &self)
{
	// if we're jumping, don't dodge
	if ((self.monsterinfo.currentmove == &berserk_move_jump) ||
		(self.monsterinfo.currentmove == &berserk_move_jump2))
		return;

	// don't check for attack; the eta should suffice for melee monsters
	if (self.monsterinfo.currentmove != &berserk_move_run1)
		self.monsterinfo.currentmove = &SAVABLE(berserk_move_run1);
}

static REGISTER_SAVABLE_FUNCTION(berserk_sidestep);
#endif

/*QUAKED monster_berserk (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
static void SP_monster_berserk(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

	// pre-caches
	sound_pain = gi.soundindex("berserk/berpain2.wav");
	sound_die = gi.soundindex("berserk/berdeth2.wav");
	sound_idle = gi.soundindex("berserk/beridle1.wav");
	sound_punch = gi.soundindex("berserk/attack.wav");
	sound_search = gi.soundindex("berserk/bersrch1.wav");
	sound_sight = gi.soundindex("berserk/sight.wav");

	self.s.modelindex = gi.modelindex("models/monsters/berserk/tris.md2");
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, 32 }
	};
	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;

	self.health = 240;
	self.gib_health = -60;
	self.mass = 250;

	self.pain = SAVABLE(berserk_pain);
	self.die = SAVABLE(berserk_die);

	self.monsterinfo.stand = SAVABLE(berserk_stand);
	self.monsterinfo.walk = SAVABLE(berserk_walk);
	self.monsterinfo.run = SAVABLE(berserk_run);
#ifdef ROGUE_AI
	self.monsterinfo.dodge = SAVABLE(M_MonsterDodge);
	self.monsterinfo.sidestep = SAVABLE(berserk_sidestep);
	self.monsterinfo.blocked = SAVABLE(berserk_blocked);
#endif
	self.monsterinfo.melee = SAVABLE(berserk_melee);
	self.monsterinfo.sight = SAVABLE(berserk_sight);
	self.monsterinfo.search = SAVABLE(berserk_search);

	self.monsterinfo.currentmove = &SAVABLE(berserk_move_stand);
	self.monsterinfo.scale = MODEL_SCALE;

	gi.linkentity(self);

	walkmonster_start(self);
}

static REGISTER_ENTITY(MONSTER_BERSERKER, monster_berserk);
#endif