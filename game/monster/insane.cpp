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
#include "lib/string/format.h"
#include "insane_model.h"

constexpr spawn_flag CRAWL = (spawn_flag) 4;
constexpr spawn_flag CRUCIFIED = (spawn_flag) 8;
constexpr spawn_flag STAND_GROUND = (spawn_flag) 16;
constexpr spawn_flag ALWAYS_STAND = (spawn_flag) 32;

static sound_index sound_fist;
static sound_index sound_shake;
static sound_index sound_moan;
static sound_index sound_scream[8];

static void insane_fist(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_fist, ATTN_IDLE);
}

static void insane_shake(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_shake, ATTN_IDLE);
}

static void insane_moan(entity &self)
{
	gi.sound(self, CHAN_VOICE, sound_moan, ATTN_IDLE);
}

static void insane_scream(entity &self)
{
	gi.sound(self, CHAN_VOICE, random_of(sound_scream), ATTN_IDLE);
}

static void insane_onground(entity &self);

constexpr mframe_t insane_frames_uptodown [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move,    0,  insane_moan },
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

	{ ai_move,    2.7f },
	{ ai_move,    4.1f },
	{ ai_move,    6 },
	{ ai_move,    7.6f },
	{ ai_move,    3.6f },
	{ ai_move },
	{ ai_move },
	{ ai_move,    0,  insane_fist },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move,    0,  insane_fist },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t insane_move_uptodown = { FRAME_stand1, FRAME_stand40, insane_frames_uptodown, insane_onground };

REGISTER_STATIC_SAVABLE(insane_move_uptodown);

static void insane_stand(entity &self);

constexpr mframe_t insane_frames_downtoup [] = {
	{ ai_move,    -0.7f },           // 41
	{ ai_move,    -1.2f },           // 42
	{ ai_move,    -1.5f },       // 43
	{ ai_move,    -4.5f },       // 44
	{ ai_move,    -3.5f },           // 45
	{ ai_move,    -0.2f },           // 46
	{ ai_move },           // 47
	{ ai_move,    -1.3f },           // 48
	{ ai_move,    -3 },               // 49
	{ ai_move,    -2 },           // 50
	{ ai_move },               // 51
	{ ai_move },               // 52
	{ ai_move },               // 53
	{ ai_move,    -3.3f },           // 54
	{ ai_move,    -1.6f },           // 55
	{ ai_move,    -0.3f },           // 56
	{ ai_move },               // 57
	{ ai_move },               // 58
	{ ai_move }                // 59
};
constexpr mmove_t insane_move_downtoup = { FRAME_stand41, FRAME_stand59, insane_frames_downtoup, insane_stand };

REGISTER_STATIC_SAVABLE(insane_move_downtoup);

constexpr mframe_t insane_frames_jumpdown [] = {
	{ ai_move,    0.2f },
	{ ai_move,    11.5f },
	{ ai_move,    5.1f },
	{ ai_move,    7.1f },
	{ ai_move }
};
constexpr mmove_t insane_move_jumpdown = { FRAME_stand96, FRAME_stand100, insane_frames_jumpdown, insane_onground };

REGISTER_STATIC_SAVABLE(insane_move_jumpdown);

static void insane_checkdown(entity &self)
{
//  if ( (self.frame == FRAME_stand94) || (self.frame == FRAME_stand65) )
	if (self.spawnflags & ALWAYS_STAND)              // Always stand
		return;
	if (random() < 0.3f) {
		if (random() < 0.5f)
			self.monsterinfo.currentmove = &SAVABLE(insane_move_uptodown);
		else
			self.monsterinfo.currentmove = &SAVABLE(insane_move_jumpdown);
	}
}

constexpr mframe_t insane_frames_stand_normal [] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, insane_checkdown }
};
constexpr mmove_t insane_move_stand_normal = { FRAME_stand60, FRAME_stand65, insane_frames_stand_normal, insane_stand };

REGISTER_STATIC_SAVABLE(insane_move_stand_normal);

constexpr mframe_t insane_frames_stand_insane [] = {
	{ ai_stand,   0,  insane_shake },
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
	{ ai_stand,   0,  insane_checkdown }
};
constexpr mmove_t insane_move_stand_insane = { FRAME_stand65, FRAME_stand94, insane_frames_stand_insane, insane_stand };

REGISTER_STATIC_SAVABLE(insane_move_stand_insane);

static void insane_checkup(entity &self)
{
	// If STAND_GROUND and Crawl are set
	if ((self.spawnflags & CRAWL) && (self.spawnflags & STAND_GROUND))
		return;
	if (random() < 0.5f)
		self.monsterinfo.currentmove = &SAVABLE(insane_move_downtoup);
}

constexpr mframe_t insane_frames_down [] = {
	{ ai_move },       // 100
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },       // 110
	{ ai_move,    -1.7f },
	{ ai_move,    -1.6f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move,    0,      insane_fist },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },       // 120
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },       // 130
	{ ai_move },
	{ ai_move },
	{ ai_move,    0,      insane_moan },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },       // 140
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },       // 150
	{ ai_move,    0.5f },
	{ ai_move },
	{ ai_move,    -0.2f,       insane_scream },
	{ ai_move },
	{ ai_move,    0.2f },
	{ ai_move,    0.4f },
	{ ai_move,    0.6f },
	{ ai_move,    0.8f },
	{ ai_move,    0.7f },
	{ ai_move,    0,      insane_checkup }      // 160
};
constexpr mmove_t insane_move_down = { FRAME_stand100, FRAME_stand160, insane_frames_down, insane_onground };

REGISTER_STATIC_SAVABLE(insane_move_down);

static void insane_run(entity &self);
static void insane_walk(entity &self);

constexpr mframe_t insane_frames_walk_normal [] = {
	{ ai_walk,    0,      insane_scream },
	{ ai_walk,    2.5f },
	{ ai_walk,    3.5f },
	{ ai_walk,    1.7f },
	{ ai_walk,    2.3f },
	{ ai_walk,    2.4f },
	{ ai_walk,    2.2f },
	{ ai_walk,    4.2f },
	{ ai_walk,    5.6f },
	{ ai_walk,    3.3f },
	{ ai_walk,    2.4f },
	{ ai_walk,    0.9f },
	{ ai_walk }
};
constexpr mmove_t insane_move_walk_normal = { FRAME_walk27, FRAME_walk39, insane_frames_walk_normal, insane_walk };
constexpr mmove_t insane_move_run_normal = { FRAME_walk27, FRAME_walk39, insane_frames_walk_normal, insane_run };

REGISTER_STATIC_SAVABLE(insane_move_walk_normal);
REGISTER_STATIC_SAVABLE(insane_move_run_normal);

constexpr mframe_t insane_frames_walk_insane [] = {
	{ ai_walk,    0,      insane_scream },      // walk 1
	{ ai_walk,    3.4f },       // walk 2
	{ ai_walk,    3.6f },       // 3
	{ ai_walk,    2.9f },       // 4
	{ ai_walk,    2.2f },       // 5
	{ ai_walk,    2.6f },       // 6
	{ ai_walk },       // 7
	{ ai_walk,    0.7f },       // 8
	{ ai_walk,    4.8f },       // 9
	{ ai_walk,    5.3f },       // 10
	{ ai_walk,    1.1f },       // 11
	{ ai_walk,    2 },       // 12
	{ ai_walk,    0.5f },       // 13
	{ ai_walk },       // 14
	{ ai_walk },       // 15
	{ ai_walk,    4.9f },       // 16
	{ ai_walk,    6.7f },       // 17
	{ ai_walk,    3.8f },       // 18
	{ ai_walk,    2 },       // 19
	{ ai_walk,    0.2f },       // 20
	{ ai_walk },       // 21
	{ ai_walk,    3.4f },       // 22
	{ ai_walk,    6.4f },       // 23
	{ ai_walk,    5 },       // 24
	{ ai_walk,    1.8f },       // 25
	{ ai_walk }        // 26
};
constexpr mmove_t insane_move_walk_insane = { FRAME_walk1, FRAME_walk26, insane_frames_walk_insane, insane_walk };
constexpr mmove_t insane_move_run_insane = { FRAME_walk1, FRAME_walk26, insane_frames_walk_insane, insane_run };

REGISTER_STATIC_SAVABLE(insane_move_walk_insane);
REGISTER_STATIC_SAVABLE(insane_move_run_insane);

constexpr mframe_t insane_frames_stand_pain [] = {
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
constexpr mmove_t insane_move_stand_pain = { FRAME_st_pain2, FRAME_st_pain12, insane_frames_stand_pain, insane_run };

REGISTER_STATIC_SAVABLE(insane_move_stand_pain);

static void insane_dead(entity &self);

constexpr mframe_t insane_frames_stand_death [] = {
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
constexpr mmove_t insane_move_stand_death = { FRAME_st_death2, FRAME_st_death18, insane_frames_stand_death, insane_dead };

REGISTER_STATIC_SAVABLE(insane_move_stand_death);

constexpr mframe_t insane_frames_crawl [] = {
	{ ai_walk,    0,      insane_scream },
	{ ai_walk,    1.5f },
	{ ai_walk,    2.1f },
	{ ai_walk,    3.6f },
	{ ai_walk,    2 },
	{ ai_walk,    0.9f },
	{ ai_walk,    3 },
	{ ai_walk,    3.4f },
	{ ai_walk,    2.4f }
};
constexpr mmove_t insane_move_crawl = { FRAME_crawl1, FRAME_crawl9, insane_frames_crawl };
constexpr mmove_t insane_move_runcrawl = { FRAME_crawl1, FRAME_crawl9, insane_frames_crawl };

REGISTER_STATIC_SAVABLE(insane_move_crawl);
REGISTER_STATIC_SAVABLE(insane_move_runcrawl);

constexpr mframe_t insane_frames_crawl_pain [] = {
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
constexpr mmove_t insane_move_crawl_pain = { FRAME_cr_pain2, FRAME_cr_pain10, insane_frames_crawl_pain, insane_run };

REGISTER_STATIC_SAVABLE(insane_move_crawl_pain);

constexpr mframe_t insane_frames_crawl_death [] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t insane_move_crawl_death = { FRAME_cr_death10, FRAME_cr_death16, insane_frames_crawl_death, insane_dead };

REGISTER_STATIC_SAVABLE(insane_move_crawl_death);

static void insane_cross(entity &self);

constexpr mframe_t insane_frames_cross [] = {
	{ ai_move,    0,      insane_moan },
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
constexpr mmove_t insane_move_cross = { FRAME_cross1, FRAME_cross15, insane_frames_cross, insane_cross };

REGISTER_STATIC_SAVABLE(insane_move_cross);

constexpr mframe_t insane_frames_struggle_cross [] = {
	{ ai_move,    0,      insane_scream },
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
constexpr mmove_t insane_move_struggle_cross = { FRAME_cross16, FRAME_cross30, insane_frames_struggle_cross, insane_cross };

REGISTER_STATIC_SAVABLE(insane_move_struggle_cross);

static void insane_cross(entity &self)
{
	if (random() < 0.8f)
		self.monsterinfo.currentmove = &SAVABLE(insane_move_cross);
	else
		self.monsterinfo.currentmove = &SAVABLE(insane_move_struggle_cross);
}

static void insane_walk(entity &self)
{
	if (self.spawnflags & STAND_GROUND)              // Hold Ground?
		if (self.frame == FRAME_cr_pain10) {
			self.monsterinfo.currentmove = &SAVABLE(insane_move_down);
			return;
		}
	if (self.spawnflags & CRAWL)
		self.monsterinfo.currentmove = &SAVABLE(insane_move_crawl);
	else if (random() <= 0.5f)
		self.monsterinfo.currentmove = &SAVABLE(insane_move_walk_normal);
	else
		self.monsterinfo.currentmove = &SAVABLE(insane_move_walk_insane);
}

REGISTER_STATIC_SAVABLE(insane_walk);

static void insane_run(entity &self)
{
	if (self.spawnflags & STAND_GROUND)              // Hold Ground?
		if (self.frame == FRAME_cr_pain10) {
			self.monsterinfo.currentmove = &SAVABLE(insane_move_down);
			return;
		}
	if (self.spawnflags & CRAWL)               // Crawling?
		self.monsterinfo.currentmove = &SAVABLE(insane_move_runcrawl);
	else if (random() <= 0.5f)              // Else, mix it up
		self.monsterinfo.currentmove = &SAVABLE(insane_move_run_normal);
	else
		self.monsterinfo.currentmove = &SAVABLE(insane_move_run_insane);
}

REGISTER_STATIC_SAVABLE(insane_run);

static void insane_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t)
{
	if (level.framenum < self.pain_debounce_framenum)
		return;

	self.pain_debounce_framenum = level.framenum + 3s;

	int32_t r = 1 + (Q_rand() & 1), l;

	if (self.health < 25)
		l = 25;
	else if (self.health < 50)
		l = 50;
	else if (self.health < 75)
		l = 75;
	else
		l = 100;

	gi.sound(self, CHAN_VOICE, gi.soundindexfmt("player/male/pain{}_{}.wav", l, r), ATTN_IDLE);

	if (skill == 3)
		return;     // no pain anims in nightmare

	// Don't go into pain frames if crucified.
	if (self.spawnflags & CRUCIFIED)
	{
		self.monsterinfo.currentmove = &SAVABLE(insane_move_struggle_cross);
		return;
	}

	if (((self.frame >= FRAME_crawl1) && (self.frame <= FRAME_crawl9)) || ((self.frame >= FRAME_stand99) && (self.frame <= FRAME_stand160)))
		self.monsterinfo.currentmove = &SAVABLE(insane_move_crawl_pain);
	else
		self.monsterinfo.currentmove = &SAVABLE(insane_move_stand_pain);
}

REGISTER_STATIC_SAVABLE(insane_reacttodamage);

static void insane_onground(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(insane_move_down);
}

static void insane_stand(entity &self)
{
	if (self.spawnflags & CRUCIFIED) {         // If crucified
		self.monsterinfo.currentmove = &SAVABLE(insane_move_cross);
		self.monsterinfo.aiflags |= AI_STAND_GROUND;
	}
	// If STAND_GROUND and Crawl are set
	else if ((self.spawnflags & CRAWL) && (self.spawnflags & STAND_GROUND))
		self.monsterinfo.currentmove = &SAVABLE(insane_move_down);
	else if (random() < 0.5f)
		self.monsterinfo.currentmove = &SAVABLE(insane_move_stand_normal);
	else
		self.monsterinfo.currentmove = &SAVABLE(insane_move_stand_insane);
}

REGISTER_STATIC_SAVABLE(insane_stand);

static void insane_dead(entity &self)
{
	if (self.spawnflags & CRUCIFIED) {
		self.flags |= FL_FLY;
	} else {
		self.bounds = {
			.mins = { -16, -16, -24 },
			.maxs = { 16, 16, -8 }
		};
		self.movetype = MOVETYPE_TOSS;
	}
	self.svflags |= SVF_DEADMONSTER;
	self.nextthink = gtime::zero();
	gi.linkentity(self);
}

static void insane_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	int     n;

	if (self.health <= self.gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), ATTN_IDLE);
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

	gi.sound(self, CHAN_VOICE, gi.soundindexfmt("player/male/death{}.wav", (Q_rand() % 4) + 1), ATTN_IDLE);

	self.deadflag = DEAD_DEAD;
	self.takedamage = true;

	if (self.spawnflags & CRUCIFIED) {
		insane_dead(self);
	} else {
		if (((self.frame >= FRAME_crawl1) && (self.frame <= FRAME_crawl9)) || ((self.frame >= FRAME_stand99) && (self.frame <= FRAME_stand160)))
			self.monsterinfo.currentmove = &SAVABLE(insane_move_crawl_death);
		else
			self.monsterinfo.currentmove = &SAVABLE(insane_move_stand_death);
	}
}

REGISTER_STATIC_SAVABLE(insane_die);

/*QUAKED misc_insane (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn CRAWL CRUCIFIED STAND_GROUND ALWAYS_STAND
*/
static void SP_misc_insane(entity &self)
{
//  static int skin = 0;    //@@

	if (deathmatch) {
		G_FreeEdict(self);
		return;
	}

	sound_fist = gi.soundindex("insane/insane11.wav");
	sound_shake = gi.soundindex("insane/insane5.wav");
	sound_moan = gi.soundindex("insane/insane7.wav");
	sound_scream[0] = gi.soundindex("insane/insane1.wav");
	sound_scream[1] = gi.soundindex("insane/insane2.wav");
	sound_scream[2] = gi.soundindex("insane/insane3.wav");
	sound_scream[3] = gi.soundindex("insane/insane4.wav");
	sound_scream[4] = gi.soundindex("insane/insane6.wav");
	sound_scream[5] = gi.soundindex("insane/insane8.wav");
	sound_scream[6] = gi.soundindex("insane/insane9.wav");
	sound_scream[7] = gi.soundindex("insane/insane10.wav");

	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;
	self.modelindex = gi.modelindex("models/monsters/insane/tris.md2");

	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, 32 }
	};

	self.health = 100;
	self.gib_health = -50;
	self.mass = 300;

	self.die = SAVABLE(insane_die);

	self.monsterinfo.stand = SAVABLE(insane_stand);
	self.monsterinfo.walk = SAVABLE(insane_walk);
	self.monsterinfo.run = SAVABLE(insane_run);
	self.monsterinfo.reacttodamage = SAVABLE(insane_reacttodamage);
	self.monsterinfo.aiflags |= AI_GOOD_GUY;

//@@
//  self.skinnum = skin;
//  skin++;
//  if (skin > 12)
//      skin = 0;

	gi.linkentity(self);

	if (self.spawnflags & STAND_GROUND)              // Stand Ground
		self.monsterinfo.aiflags |= AI_STAND_GROUND;

	self.monsterinfo.currentmove = &SAVABLE(insane_move_stand_normal);

	self.monsterinfo.scale = MODEL_SCALE;

	if (self.spawnflags & CRUCIFIED) {                 // Crucified ?
		self.bounds = {
			.mins = { -16, 0, 0 },
			.maxs = { 16, 8, 32 }
		};
		self.flags |= FL_NO_KNOCKBACK;
		flymonster_start(self);
	} else {
		walkmonster_start(self);
		self.skinnum = Q_rand() % 3;
	}
}

static REGISTER_ENTITY(MISC_INSANE, misc_insane);
#endif