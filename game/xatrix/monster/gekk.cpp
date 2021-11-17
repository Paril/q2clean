#include "config.h"

#if defined(THE_RECKONING) && defined(SINGLE_PLAYER)
#include "game/entity.h"
#include "game/game.h"
#include "game/move.h"
#include "game/ai.h"
#include "game/monster.h"
#include "game/misc.h"
#include "game/spawn.h"
#include "lib/gi.h"
#include "game/monster/flash.h"
#include "game/util.h"
#include "gekk_model.h"
#include "game/ballistics/hit.h"
#include "game/combat.h"
#include "game/xatrix/misc.h"
#include "gekk.h"

constexpr means_of_death MOD_GEKK { .self_kill_fmt = "{0}... that's gotta hurt!\n" };

static sound_index	sound_swing;
static sound_index	sound_hit;
static sound_index	sound_hit2;
static sound_index	sound_death;
static sound_index	sound_pain1;
static sound_index	sound_sight;
static sound_index	sound_search;
static sound_index	sound_step1;
static sound_index	sound_step2;
static sound_index	sound_step3;
static sound_index	sound_thud;
static sound_index  sound_chantlow;
static sound_index  sound_chantmid;
static sound_index  sound_chanthigh;

constexpr spawn_flag GEKK_CHANT = (spawn_flag) 8;

//
// CHECKATTACK
//

static bool gekk_check_melee(entity &self)
{
	if (!self.enemy.has_value() || self.enemy->health <= 0)
		return false;

	return VectorDistance(self.origin, self.enemy->origin) < RANGE_MELEE;
}

static bool gekk_check_jump(entity &self)
{
	if (!self.enemy.has_value())
		return false;

	if (self.absbounds.mins[2] > (self.enemy->absbounds.mins[2] + 0.75f * self.enemy->size[2]))
		return false;

	if (self.absbounds.maxs[2] < (self.enemy->absbounds.mins[2] + 0.25f * self.enemy->size[2]))
		return false;

	vector v {	self.origin[0] - self.enemy->origin[0],
				self.origin[1] - self.enemy->origin[1],
				0 };
	float distance = VectorLength(v);

	if (distance < 100)
		return false;
	else if (distance > 100)
		if (random() < 0.9)
			return false;

	return true;
}

static bool gekk_check_jump_close(entity &self)
{
	if (!self.enemy.has_value())
		return false;

	vector v {	self.origin[0] - self.enemy->origin[0],
				self.origin[1] - self.enemy->origin[1],
				0 };
	float distance = VectorLength(v);

	if (distance < 100)
		return self.origin[2] < self.enemy->origin[2];
	
	return true;
}

static bool gekk_checkattack(entity &self)
{
	if (!self.enemy.has_value() || self.enemy->health <= 0)
		return false;

	if (gekk_check_melee(self))
	{
		self.monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (gekk_check_jump(self))
	{
		self.monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	if (gekk_check_jump_close (self) && self.waterlevel <= WATER_LEGS)
	{	
		self.monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return false;
}

REGISTER_STATIC_SAVABLE(gekk_checkattack);

//
// SOUNDS
//

static void gekk_step(entity &self)
{
	int n = Q_rand_uniform(3);

	if (n == 0)
		gi.sound (self, CHAN_VOICE, sound_step1);		
	else if (n == 1)
		gi.sound (self, CHAN_VOICE, sound_step2);
	else
		gi.sound (self, CHAN_VOICE, sound_step3);
}

static void gekk_sight(entity &self, entity &)
{
	gi.sound (self, CHAN_VOICE, sound_sight);
}

REGISTER_STATIC_SAVABLE(gekk_sight);

static void gekk_search(entity &self)
{
	if (self.spawnflags & GEKK_CHANT)
	{
		float r = random();
		if (r < 0.33)
			gi.sound (self, CHAN_VOICE, sound_chantlow);
		else if (r < 0.66)
			gi.sound (self, CHAN_VOICE, sound_chantmid);
		else
			gi.sound (self, CHAN_VOICE, sound_chanthigh);
	}
	else
		gi.sound (self, CHAN_VOICE, sound_search);

	self.health += Q_rand_uniform(10, 20);
	if (self.health > self.max_health)
		self.health = self.max_health;

	if (self.health < (self.max_health / 4))
		self.skinnum = 2;
	else if (self.health < (self.max_health / 2))
		self.skinnum = 1;
	else 
		self.skinnum = 0;
}

REGISTER_STATIC_SAVABLE(gekk_search);


//
// ATTACK
//

static void gekk_jump_touch(entity &self, entity &other, vector, const surface &)
{
	if (self.health <= 0)
	{
		self.touch = nullptr;
		return;
	}

	if (other.takedamage && VectorLength(self.velocity) > 200)
	{
		vector normal = self.velocity;
		VectorNormalize(normal);

		vector point = self.origin + (normal * self.bounds.maxs[0]);
		int damage = Q_rand_uniform(10, 20);

		T_Damage (other, self, self, self.velocity, point, normal, damage, damage, { DAMAGE_NONE }, MOD_GEKK);
	}

	if (!M_CheckBottom (self))
	{
		if (self.groundentity != null_entity)
		{
			self.monsterinfo.nextframe = FRAME_leapatk_11;
			self.touch = nullptr;
		}

		return;
	}

	self.touch = nullptr;
}

REGISTER_STATIC_SAVABLE(gekk_jump_touch);

static void gekk_jump_takeoff(entity &self)
{
	gi.sound (self, CHAN_VOICE, sound_sight);

	vector	forward;
	AngleVectors (self.angles, &forward, nullptr, nullptr);

	self.origin[2] += 1;
	
	// high jump
	if (gekk_check_jump (self))
	{
		self.velocity = forward * 700;
		self.velocity[2] = 250.f;
	}
	else
	{
		self.velocity = forward * 250;
		self.velocity[2] = 400.f;
	}

	self.groundentity = null_entity;
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.monsterinfo.attack_finished = level.time + 3s;
	self.touch = SAVABLE(gekk_jump_touch);
}

static void gekk_jump_takeoff2(entity &self)
{
	gi.sound (self, CHAN_VOICE, sound_sight);

	vector	forward;
	AngleVectors (self.angles, &forward, nullptr, nullptr);

	self.origin[2] = self.enemy->origin[2];
	
	if (gekk_check_jump (self))
	{
		self.velocity = forward * 300;
		self.velocity[2] = 250.f;
	}
	else 
	{
		self.velocity = forward * 150;
		self.velocity[2] = 300.f;
	}

	self.groundentity = null_entity;
	self.monsterinfo.aiflags |= AI_DUCKED;
	self.monsterinfo.attack_finished = level.time + 3s;
	self.touch = SAVABLE(gekk_jump_touch);
}

static void gekk_stop_skid(entity &self)
{
	if (self.groundentity != null_entity)
		self.velocity = vec3_origin;
}

static void gekk_check_landing(entity &self)
{
	if (self.groundentity != null_entity)
	{
		gi.sound (self, CHAN_WEAPON, sound_thud);
		self.monsterinfo.attack_finished = gtime::zero();
		self.monsterinfo.aiflags &= ~AI_DUCKED;
		self.velocity = vec3_origin;
		return;
	}

	if (level.time > self.monsterinfo.attack_finished)
		self.monsterinfo.nextframe = FRAME_leapatk_11;
	else
		self.monsterinfo.nextframe = FRAME_leapatk_12;
}

static void land_to_water(entity &self);

static void gekk_check_underwater(entity &self)
{
	if (self.waterlevel > WATER_LEGS)
		land_to_water (self);
}

static void gekk_run_start(entity &self);

constexpr mframe_t gekk_frames_leapatk2[] = 
{
	{ ai_charge,   0.000f }, // frame 0
	{ ai_charge,  -0.387f }, // frame 1
	{ ai_charge,  -1.113f }, // frame 2
	{ ai_charge,  -0.237f }, // frame 3
	{ ai_charge,   6.720f, gekk_jump_takeoff2 }, // frame 4  last frame on ground
	{ ai_charge,   6.414f }, // frame 5  leaves ground
	{ ai_charge,   0.163f }, // frame 6
	{ ai_charge,  28.316f }, // frame 7
	{ ai_charge,  24.198f }, // frame 8
	{ ai_charge,  31.742f }, // frame 9
	{ ai_charge,  35.977f, gekk_check_landing }, // frame 10  last frame in air
	{ ai_charge,  12.303f, gekk_stop_skid }, // frame 11  feet back on ground
	{ ai_charge,  20.122f, gekk_stop_skid }, // frame 12
	{ ai_charge,  -1.042f, gekk_stop_skid }, // frame 13
	{ ai_charge,   2.556f, gekk_stop_skid }, // frame 14
	{ ai_charge,   0.544f, gekk_stop_skid }, // frame 15
	{ ai_charge,   1.862f, gekk_stop_skid }, // frame 16
	{ ai_charge,   1.224f, gekk_stop_skid }, // frame 17
 
	{ ai_charge,  -0.457f, gekk_check_underwater }, // frame 18
};
constexpr mmove_t gekk_move_leapatk2 = { FRAME_leapatk_01, FRAME_leapatk_19, gekk_frames_leapatk2, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_leapatk2);

static void water_to_land(entity &self)
{
	self.flags &= ~FL_SWIM;
	self.yaw_speed = 20.f;
	self.viewheight = 25;
	
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_leapatk2);
	
	self.bounds = bbox::sized(24.f);
}

static void gekk_run(entity &self);

static void gekk_swim(entity &self)
{
	if (gekk_checkattack(self))	// Knightmare fixed, added argument
	{
		if (self.enemy->waterlevel <= WATER_LEGS && random() > 0.7)
			water_to_land (self);
	}
	else
		gekk_run(self);
}

static void gekk_swim_loop(entity &self);

constexpr mframe_t gekk_frames_swim [] =
{
	{ ai_run, 16 },	
	{ ai_run, 16 },
	{ ai_run, 16 },
	
	{ ai_run, 16, gekk_swim }
};
constexpr mmove_t gekk_move_swim_loop = { FRAME_amb_01, FRAME_amb_04, gekk_frames_swim, gekk_swim_loop };

REGISTER_STATIC_SAVABLE(gekk_move_swim_loop);

static void gekk_swim_loop(entity &self)
{
	self.flags |= FL_SWIM;	
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_swim_loop);
}

static void gekk_hit_left(entity &self)
{
	vector aim { RANGE_MELEE, self.bounds.mins[0], 8 };

	if (fire_hit (self, aim, (15 + (Q_rand() %5)), 100))
		gi.sound (self, CHAN_WEAPON, sound_hit);
	else
		gi.sound (self, CHAN_WEAPON, sound_swing);
}

static void gekk_hit_right(entity &self)
{
	vector aim { RANGE_MELEE, self.bounds.maxs[0], 8 };

	if (fire_hit (self, aim, (15 + (Q_rand() %5)), 100))
		gi.sound (self, CHAN_WEAPON, sound_hit2);
	else
		gi.sound (self, CHAN_WEAPON, sound_swing);
}

static void gekk_bite(entity &self)
{
	constexpr vector aim { RANGE_MELEE, 0, 0 };
	fire_hit(self, aim, 5, 0);
}

constexpr mframe_t gekk_frames_swim_start [] =
{
	{ ai_run, 14 },
	{ ai_run, 14 },
	{ ai_run, 14 },
	{ ai_run, 14 },
	{ ai_run, 16 },
	{ ai_run, 16 },
	{ ai_run, 16 },
	{ ai_run, 18 },
	{ ai_run, 18, gekk_hit_left },
	{ ai_run, 18 },
	
	{ ai_run, 20 },
	{ ai_run, 20 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 24, gekk_hit_right },
	{ ai_run, 24 },
	{ ai_run, 26 },
	{ ai_run, 26 },
	{ ai_run, 24 },
	{ ai_run, 24 },
	
	{ ai_run, 22, gekk_bite },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 22 },
	{ ai_run, 18 },
	{ ai_run, 18 },

	{ ai_run, 18 },
	{ ai_run, 18 }
};
constexpr mmove_t gekk_move_swim_start = { FRAME_swim_01, FRAME_swim_32, gekk_frames_swim_start, gekk_swim_loop };

REGISTER_STATIC_SAVABLE(gekk_move_swim_start);

static void land_to_water(entity &self)
{
	self.flags |= FL_SWIM;
	self.yaw_speed = 10.f;
	self.viewheight = 10;
	
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_swim_start);
	
	self.bounds = {
		.mins = { -24, -24, -24 },
		.maxs = { 24, 24, 16 }
	};
}

constexpr mframe_t gekk_frames_run[] = 
{
	{ ai_run,   3.849f, gekk_check_underwater }, // frame 0
	{ ai_run,  19.606f }, // frame 1
	{ ai_run,  25.583f }, // frame 2
	{ ai_run,  34.625f, gekk_step }, // frame 3
	{ ai_run,  27.365f }, // frame 4
	{ ai_run,  28.480f }, // frame 5
};
constexpr mmove_t gekk_move_run = { FRAME_run_01, FRAME_run_06, gekk_frames_run };

REGISTER_STATIC_SAVABLE(gekk_move_run);

static void gekk_face(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_run);	
}

//
// STAND
//

static void ai_stand2(entity &self, float dist)
{
	if (self.spawnflags & GEKK_CHANT)
	{
		ai_move (self, dist);

		if (!(self.spawnflags & 1) && (self.monsterinfo.idle) && (level.time > self.monsterinfo.idle_time))
		{
			if (self.monsterinfo.idle_time != gtime::zero())
			{
				self.monsterinfo.idle (self);
				self.monsterinfo.idle_time = level.time + random(15s, 30s);
			}
			else
				self.monsterinfo.idle_time = level.time + random(15s);
		}
	}
	else
		ai_stand (self, dist);
}

constexpr mframe_t gekk_frames_stand [] =
{
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },		// 10

	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },		// 20

	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },		// 30

	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	
	{ ai_stand2, 0, gekk_check_underwater },
};
constexpr mmove_t gekk_move_stand = { FRAME_stand_01, FRAME_stand_39, gekk_frames_stand };

REGISTER_STATIC_SAVABLE(gekk_move_stand);

constexpr mframe_t gekk_frames_standunderwater[] =
{
	{ ai_stand2 },	
	{ ai_stand2 },
	{ ai_stand2 },
	
	{ ai_stand2, 0, gekk_check_underwater }
};
constexpr mmove_t gekk_move_standunderwater = { FRAME_amb_01, FRAME_amb_04, gekk_frames_standunderwater };

REGISTER_STATIC_SAVABLE(gekk_move_standunderwater);

static void gekk_stand(entity &self)
{
	if (self.waterlevel > WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_standunderwater);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_stand);
}

REGISTER_STATIC_SAVABLE(gekk_stand);

//
// IDLE
//

static void gekk_idle_loop(entity &self)
{
	if (random() > 0.75 && self.health < self.max_health)
		self.monsterinfo.nextframe = FRAME_idle_01;
}

static void gekk_chant(entity &self);

constexpr mframe_t gekk_frames_idle2 [] =
{
	{ ai_move, 0, gekk_search },		
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

	{ ai_move },
	{ ai_move, 0, gekk_idle_loop }
};
constexpr mmove_t gekk_move_chant = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle2, gekk_chant };

REGISTER_STATIC_SAVABLE(gekk_move_chant);

static void gekk_chant(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_chant);
}

constexpr mframe_t gekk_frames_idle [] =
{
	{ ai_stand2, 0, gekk_search },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	{ ai_stand2 },
	
	{ ai_stand2 },
	{ ai_stand2, 0, gekk_idle_loop }
};
constexpr mmove_t gekk_move_idle = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle, gekk_stand };
constexpr mmove_t gekk_move_idle2 = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle, gekk_face };

REGISTER_STATIC_SAVABLE(gekk_move_idle);
REGISTER_STATIC_SAVABLE(gekk_move_idle2);

static void gekk_idle(entity &self)
{
	if (self.waterlevel <= WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_idle);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_swim_start);
}

REGISTER_STATIC_SAVABLE(gekk_idle);

//
// WALK
//

constexpr mframe_t gekk_frames_walk[] = 
{
	{ ai_walk,  3.849f, gekk_check_underwater }, // frame 0
	{ ai_walk,  19.606f }, // frame 1
	{ ai_walk,  25.583f }, // frame 2
	{ ai_walk,  34.625f, gekk_step }, // frame 3
	{ ai_walk,  27.365f }, // frame 4
	{ ai_walk,  28.480f }, // frame 5
};
constexpr mmove_t gekk_move_walk = { FRAME_run_01, FRAME_run_06, gekk_frames_walk };

REGISTER_STATIC_SAVABLE(gekk_move_walk);

static void gekk_walk(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_walk);
}

REGISTER_STATIC_SAVABLE(gekk_walk);

//
// RUN
//

static void gekk_run(entity &self)
{
	if (self.waterlevel > WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_swim_start);
	else if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_run);
}

REGISTER_STATIC_SAVABLE(gekk_run);

constexpr mframe_t gekk_frames_run_st[] = 
{
	{ ai_run,   0.212f }, // frame 0
	{ ai_run,  19.753f }, // frame 1
};
constexpr mmove_t gekk_move_run_start = { FRAME_stand_01, FRAME_stand_02, gekk_frames_run_st, gekk_run };

REGISTER_STATIC_SAVABLE(gekk_move_run_start);

static void gekk_run_start(entity &self)
{
	if (self.waterlevel > WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_swim_start);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_run_start);
}

REGISTER_STATIC_SAVABLE(gekk_run_start);

//
// MELEE
//

static void loogie_touch(entity &self, entity &other, vector normal, const surface &surf)
{
	if (other == self.owner)
		return;

	if (surf.flags & SURF_SKY)
	{
		G_FreeEdict (self);
		return;
	}

	if (other.takedamage)
		T_Damage (other, self, self.owner, self.velocity, self.origin, normal, self.dmg, 1, { DAMAGE_ENERGY }, MOD_GEKK);
	
	G_FreeEdict (self);
};

REGISTER_STATIC_SAVABLE(loogie_touch);
	
static void fire_loogie(entity &self, vector start, vector dir, int32_t damage, int32_t speed)
{
	VectorNormalize (dir);

	entity &loogie = G_Spawn();
	loogie.origin = start;
	loogie.old_origin = start;
	loogie.angles = vectoangles (dir);
	loogie.velocity = dir * speed;
	loogie.movetype = MOVETYPE_FLYMISSILE;
	loogie.clipmask = MASK_SHOT;
	loogie.solid = SOLID_BBOX;
	loogie.effects |= EF_BLASTER;
	
	loogie.modelindex = gi.modelindex ("models/objects/loogy/tris.md2");
	loogie.owner = self;
	loogie.touch = SAVABLE(loogie_touch);
	loogie.nextthink = level.time + 2s;
	loogie.think = SAVABLE(G_FreeEdict);
	loogie.dmg = damage;
	gi.linkentity (loogie);

	trace tr = gi.traceline(self.origin, loogie.origin, loogie, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		loogie.origin += dir * -10;
		loogie.touch (loogie, tr.ent, vec3_origin, null_surface);
	}
}	

static void loogie(entity &self)
{
	if (!self.enemy.has_value() || self.enemy->health <= 0)
		return;

	vector forward, right, up;
	AngleVectors (self.angles, &forward, &right, &up);

	vector start = G_ProjectSource (self.origin, { -18, -0.8f, 24 }, forward, right);
	start += up * 2;
	
	vector end = self.enemy->origin;
	end[2] += self.enemy->viewheight;

	vector dir = end - start;
	fire_loogie (self, start, dir, 5, 550);
}

static void do_spit(entity &self);

static void reloogie(entity &self)
{
	if (random() > 0.8 && self.health < self.max_health)
	{
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_idle2);
		return;
	}
		
	if (self.enemy->health >= 0)
		if (random() > 0.7 && (VectorDistance(self.origin, self.enemy->origin) < RANGE_NEAR))
			do_spit(self);
}

constexpr mframe_t gekk_frames_spit [] =
{
	{ ai_charge }, 
	{ ai_charge }, 
	{ ai_charge }, 
	{ ai_charge }, 
	{ ai_charge }, 

	{ ai_charge,   0, loogie },
	{ ai_charge,   0, reloogie }
};
constexpr mmove_t gekk_move_spit = { FRAME_spit_01, FRAME_spit_07, gekk_frames_spit, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_spit);

static void do_spit(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_spit);
};

static void gekk_check_refire(entity &self);

constexpr mframe_t gekk_frames_attack1 [] =
{
	{ ai_charge }, 
	{ ai_charge }, 
	{ ai_charge }, 

	{ ai_charge,   0, gekk_hit_left }, 
	{ ai_charge }, 
	{ ai_charge }, 

	{ ai_charge }, 
	{ ai_charge }, 
	{ ai_charge,   0, gekk_check_refire }
};
constexpr mmove_t gekk_move_attack1 = { FRAME_clawatk3_01, FRAME_clawatk3_09, gekk_frames_attack1, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_attack1);

constexpr mframe_t gekk_frames_attack2[] = 
{
	{ ai_charge },
	{ ai_charge },
	{ ai_charge,   0.000, gekk_hit_left }, 

	{ ai_charge },
	{ ai_charge },
	{ ai_charge,   0.000, gekk_hit_right }, 

	{ ai_charge },
	{ ai_charge },
	{ ai_charge,   0.000, gekk_check_refire  }
};
constexpr mmove_t gekk_move_attack2 = { FRAME_clawatk5_01, FRAME_clawatk5_09, gekk_frames_attack2, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_attack2);

static void gekk_check_refire(entity &self)
{
	if (!self.enemy.has_value() || !self.enemy->inuse || self.enemy->health <= 0)
		return;

	if (random() < ((int32_t) skill * 0.1f) && VectorDistance(self.origin, self.enemy->origin) < RANGE_MELEE)
	{
		if (self.frame == FRAME_clawatk3_09)
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack2);
		else if (self.frame == FRAME_clawatk5_09)
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack1);
	}
}

constexpr mframe_t gekk_frames_leapatk[] = 
{
	{ ai_charge }, // frame 0
	{ ai_charge,  -0.387f }, // frame 1
	{ ai_charge,  -1.113f }, // frame 2
	{ ai_charge,  -0.237f }, // frame 3
	{ ai_charge,   6.720f, gekk_jump_takeoff }, // frame 4  last frame on ground
	{ ai_charge,   6.414f }, // frame 5  leaves ground
	{ ai_charge,   0.163f }, // frame 6
	{ ai_charge,  28.316f }, // frame 7
	{ ai_charge,  24.198f }, // frame 8
	{ ai_charge,  31.742f }, // frame 9
	{ ai_charge,  35.977f, gekk_check_landing }, // frame 10  last frame in air
	{ ai_charge,  12.303f, gekk_stop_skid }, // frame 11  feet back on ground
	{ ai_charge,  20.122f, gekk_stop_skid }, // frame 12
	{ ai_charge,  -1.042f, gekk_stop_skid }, // frame 13
	{ ai_charge,   2.556f, gekk_stop_skid }, // frame 14
	{ ai_charge,   0.544f, gekk_stop_skid }, // frame 15
	{ ai_charge,   1.862f, gekk_stop_skid }, // frame 16
	{ ai_charge,   1.224f, gekk_stop_skid }, // frame 17

	{ ai_charge,  -0.457f, gekk_check_underwater }, // frame 18
};
constexpr mmove_t gekk_move_leapatk = { FRAME_leapatk_01, FRAME_leapatk_19, gekk_frames_leapatk, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_leapatk);

constexpr mframe_t gekk_frames_attack [] =
{
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16,	gekk_bite },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16,	gekk_bite },
	
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16,	gekk_hit_left },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16 },
	{ ai_charge, 16,	gekk_hit_right },
	{ ai_charge, 16 },

	{ ai_charge, 16 }
};
constexpr mmove_t gekk_move_attack = { FRAME_attack_01, FRAME_attack_21, gekk_frames_attack, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_attack);

static void gekk_melee(entity &self)
{
	if (self.waterlevel > WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack);
	else if (random() > 0.66)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack1);
	else 
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack2);
}

REGISTER_STATIC_SAVABLE(gekk_melee);

static void gekk_jump(entity &self)
{
	if ((self.flags & FL_SWIM) || self.waterlevel > WATER_LEGS)
	{
		water_to_land(self);
		return;
	}

	if (random() > 0.5 && (VectorDistance (self.origin, self.enemy->origin) > RANGE_MELEE))
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_spit);
	else if (random() > 0.8)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_spit);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_leapatk);
}

REGISTER_STATIC_SAVABLE(gekk_jump);

//
// PAIN
//

constexpr mframe_t gekk_frames_pain[] = 
{
	{ ai_move }, // frame 0
	{ ai_move }, // frame 1
	{ ai_move }, // frame 2
	{ ai_move }, // frame 3
	{ ai_move }, // frame 4
	{ ai_move }, // frame 5
};
constexpr mmove_t gekk_move_pain = { FRAME_pain_01, FRAME_pain_06, gekk_frames_pain, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_pain);

constexpr mframe_t gekk_frames_pain1[] = 
{
	{ ai_move }, // frame 0
	{ ai_move }, // frame 1
	{ ai_move }, // frame 2
	{ ai_move }, // frame 3
	{ ai_move }, // frame 4
	{ ai_move }, // frame 5
	{ ai_move }, // frame 6
	{ ai_move }, // frame 7
	{ ai_move }, // frame 8
	{ ai_move }, // frame 9
	
	{ ai_move,   0.000, gekk_check_underwater }
};
constexpr mmove_t gekk_move_pain1 = { FRAME_pain3_01, FRAME_pain3_11, gekk_frames_pain1, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_pain1);

constexpr mframe_t gekk_frames_pain2[] = 
{
	{ ai_move }, // frame 0
	{ ai_move }, // frame 1
	{ ai_move }, // frame 2
	{ ai_move }, // frame 3
	{ ai_move }, // frame 4
	{ ai_move }, // frame 5
	{ ai_move }, // frame 6
	{ ai_move }, // frame 7
	{ ai_move }, // frame 8
	{ ai_move }, // frame 9
	
	{ ai_move }, // frame 10
	{ ai_move }, // frame 11
	{ ai_move,   0, gekk_check_underwater }, 
};
constexpr mmove_t gekk_move_pain2 = { FRAME_pain4_01, FRAME_pain4_13, gekk_frames_pain2, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_pain2);

static void gekk_pain(entity &self, entity &, float, int32_t)
{
	if (self.health < (self.max_health / 4))
		self.skinnum = 2;
	else if (self.health < (self.max_health / 2))
		self.skinnum = 1;
	else
		self.skinnum = 0;
}

REGISTER_STATIC_SAVABLE(gekk_pain);

static void gekk_reacttodamage(entity &self, entity &, entity &, int32_t, int32_t)
{
	if (self.spawnflags & GEKK_CHANT)
	{
		self.spawnflags &= ~GEKK_CHANT;
		return;
	}

	if (level.time < self.pain_debounce_time)
		return;

	self.pain_debounce_time = level.time + 3s;

	gi.sound (self, CHAN_VOICE, sound_pain1);

	if (self.waterlevel > WATER_LEGS)
	{
		if (!(self.flags & FL_SWIM))
			self.flags |= FL_SWIM;

		self.monsterinfo.currentmove = &SAVABLE(gekk_move_pain);
	}
	else if (random() > 0.5)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_pain1);
	else
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_pain2);
}

REGISTER_STATIC_SAVABLE(gekk_reacttodamage);

//
// DEATH
//

static void gekk_dead(entity &self)
{
	// fix this because of no blocking problem
	if (self.waterlevel > WATER_LEGS)
		return;
		
	self.bounds = {
		.mins = { -16, -16, -24 },
		.maxs = { 16, 16, -8 }
	};
	self.movetype = MOVETYPE_TOSS;
	self.svflags |= SVF_DEADMONSTER;
	self.nextthink = gtime::zero();
	gi.linkentity (self);
}

inline void gekk_gibfest(entity &self)
{
	const int32_t damage = 20;
	
	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"));
	
	ThrowGibACID (self, "models/objects/gekkgib/pelvis/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/arm/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/arm/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/torso/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/claw/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/leg/tris.md2", damage, GIB_ORGANIC);
	ThrowGibACID (self, "models/objects/gekkgib/leg/tris.md2", damage, GIB_ORGANIC);
	
	ThrowHeadACID (self, "models/objects/gekkgib/head/tris.md2", damage, GIB_ORGANIC);

	self.deadflag = true;
}

static void isgibfest(entity &self)
{
	if (random() > 0.9)
		gekk_gibfest (self);
}

constexpr mframe_t gekk_frames_death1[] =
{
	{ ai_move,  -5.151f }, // frame 0
	{ ai_move, -12.223f }, // frame 1
	{ ai_move, -11.484f }, // frame 2
	{ ai_move, -17.952f }, // frame 3
	{ ai_move,  -6.953f }, // frame 4
	{ ai_move,  -7.393f }, // frame 5
	{ ai_move, -10.713f }, // frame 6
	{ ai_move, -17.464f }, // frame 7
	{ ai_move, -11.678f }, // frame 8
	{ ai_move, -11.678f }  // frame 9
};
constexpr mmove_t gekk_move_death1 = { FRAME_death1_01, FRAME_death1_10, gekk_frames_death1, gekk_dead };

REGISTER_STATIC_SAVABLE(gekk_move_death1);

constexpr mframe_t gekk_frames_death3[] =
{
	{ ai_move }, // frame 0
	{ ai_move,   0.022f }, // frame 1
	{ ai_move,   0.169f }, // frame 2
	{ ai_move,  -0.710f }, // frame 3
	{ ai_move, -13.446f }, // frame 4
	{ ai_move,  -7.654f, isgibfest }, // frame 5
	{ ai_move, -31.951f }, // frame 6
};
constexpr mmove_t gekk_move_death3 = { FRAME_death3_01, FRAME_death3_07, gekk_frames_death3, gekk_dead };

REGISTER_STATIC_SAVABLE(gekk_move_death3);

constexpr mframe_t gekk_frames_death4[] = 
{
	{ ai_move,   5.103f }, // frame 0
	{ ai_move,  -4.808f }, // frame 1
	{ ai_move, -10.509f }, // frame 2
	{ ai_move,  -9.899f }, // frame 3
	{ ai_move,   4.033f, isgibfest }, // frame 4
	{ ai_move,  -5.197f }, // frame 5
	{ ai_move,  -0.919f }, // frame 6
	{ ai_move,  -8.821f }, // frame 7
	{ ai_move,  -5.626f }, // frame 8
	{ ai_move,  -8.865f, isgibfest }, // frame 9
	{ ai_move,  -0.845f }, // frame 10
	{ ai_move,   1.986f }, // frame 11
	{ ai_move,   0.170f }, // frame 12
	{ ai_move,   1.339f, isgibfest }, // frame 13
	{ ai_move,  -0.922f }, // frame 14
	{ ai_move,   0.818f }, // frame 15
	{ ai_move,  -1.288f }, // frame 16
	{ ai_move,  -1.408f, isgibfest }, // frame 17
	{ ai_move,  -7.787f }, // frame 18
	{ ai_move,  -3.995f }, // frame 19
	{ ai_move,  -4.604f }, // frame 20
	{ ai_move,  -1.715f, isgibfest }, // frame 21
	{ ai_move,  -0.564f }, // frame 22
	{ ai_move,  -0.597f }, // frame 23
	{ ai_move,   0.074f }, // frame 24
	{ ai_move,  -0.309f, isgibfest }, // frame 25
	{ ai_move,  -0.395f }, // frame 26
	{ ai_move,  -0.501f }, // frame 27
	{ ai_move,  -0.325f }, // frame 28
	{ ai_move,  -0.931f, isgibfest }, // frame 29
	{ ai_move,  -1.433f }, // frame 30
	{ ai_move,  -1.626f }, // frame 31
	{ ai_move,   4.680f }, // frame 32
	{ ai_move,   0.560f }, // frame 33
	{ ai_move,  -0.549f, gekk_gibfest } // frame 34
};
constexpr mmove_t gekk_move_death4 = { FRAME_death4_01, FRAME_death4_35, gekk_frames_death4, gekk_dead };

REGISTER_STATIC_SAVABLE(gekk_move_death4);

constexpr mframe_t gekk_frames_wdeath[] = 
{
	 { ai_move }, // frame 0
	 { ai_move }, // frame 1
	 { ai_move }, // frame 2
	 { ai_move }, // frame 3
	 { ai_move }, // frame 4
	 { ai_move }, // frame 5
	 { ai_move }, // frame 6
	 { ai_move }, // frame 7
	 { ai_move }, // frame 8
	 { ai_move }, // frame 9
	 { ai_move }, // frame 10
	 { ai_move }, // frame 11
	 { ai_move }, // frame 12
	 { ai_move }, // frame 13
	 { ai_move }, // frame 14
	 { ai_move }, // frame 15
	 { ai_move }, // frame 16
	 { ai_move }, // frame 17
	 { ai_move }, // frame 18
	 { ai_move }, // frame 19
	 { ai_move }, // frame 20
	 { ai_move }, // frame 21
	 { ai_move }, // frame 22
	 { ai_move }, // frame 23
	 { ai_move }, // frame 24
	 { ai_move }, // frame 25
	 { ai_move }, // frame 26
	 { ai_move }, // frame 27
	 { ai_move }, // frame 28
	 { ai_move }, // frame 29
	 { ai_move }, // frame 30
	 { ai_move }, // frame 31
	 { ai_move }, // frame 32
	 { ai_move }, // frame 33
	 { ai_move }, // frame 34
	 { ai_move }, // frame 35
	 { ai_move }, // frame 36
	 { ai_move }, // frame 37
	 { ai_move }, // frame 38
	 { ai_move }, // frame 39
	 { ai_move }, // frame 40
	 { ai_move }, // frame 41
	 { ai_move }, // frame 42
	 { ai_move }, // frame 43
	 { ai_move }  // frame 44
};
constexpr mmove_t gekk_move_wdeath = { FRAME_wdeath_01, FRAME_wdeath_45, gekk_frames_wdeath, gekk_dead };

REGISTER_STATIC_SAVABLE(gekk_move_wdeath);

static void gekk_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	if (self.health <= self.gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"));

		ThrowGibACID (self, "models/objects/gekkgib/pelvis/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/arm/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/arm/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/torso/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/claw/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/leg/tris.md2", damage, GIB_ORGANIC);
		ThrowGibACID (self, "models/objects/gekkgib/leg/tris.md2", damage, GIB_ORGANIC);
	
		ThrowHeadACID (self, "models/objects/gekkgib/head/tris.md2", damage, GIB_ORGANIC);

		self.deadflag = true;
		return;
	}

	if (self.deadflag)
		return;

	gi.sound (self, CHAN_VOICE, sound_death);
	self.deadflag = true;
	self.takedamage = true;
	self.skinnum = 2;

	if (self.waterlevel > WATER_LEGS)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_wdeath);
	else
	{
		float r = random();
		if (r > 0.66)
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_death1);
		else if (r > 0.33)
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_death3);
		else
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_death4);
	}
}

REGISTER_STATIC_SAVABLE(gekk_die);

/*
 duck
*/
constexpr mframe_t gekk_frames_lduck[] = 
{
	{ ai_move }, // frame 0
	{ ai_move }, // frame 1
	{ ai_move }, // frame 2
	{ ai_move }, // frame 3
	{ ai_move }, // frame 4
	{ ai_move }, // frame 5
	{ ai_move }, // frame 6
	{ ai_move }, // frame 7
	{ ai_move }, // frame 8
	{ ai_move }, // frame 9
	
	{ ai_move }, // frame 10
	{ ai_move }, // frame 11
	{ ai_move }  // frame 12
};
constexpr mmove_t gekk_move_lduck = { FRAME_lduck_01, FRAME_lduck_13, gekk_frames_lduck, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_lduck);

constexpr mframe_t gekk_frames_rduck[] = 
{
	{ ai_move }, // frame 0
	{ ai_move }, // frame 1
	{ ai_move }, // frame 2
	{ ai_move }, // frame 3
	{ ai_move }, // frame 4
	{ ai_move }, // frame 5
	{ ai_move }, // frame 6
	{ ai_move }, // frame 7
	{ ai_move }, // frame 8
	{ ai_move }, // frame 9
	{ ai_move }, // frame 10
	{ ai_move }, // frame 11
	{ ai_move } // frame 12
};
constexpr mmove_t gekk_move_rduck = { FRAME_rduck_01, FRAME_rduck_13, gekk_frames_rduck, gekk_run_start };

REGISTER_STATIC_SAVABLE(gekk_move_rduck);

#ifdef GROUND_ZERO
static void gekk_dodge(entity &self, entity &attacker, gtimef eta, trace &)
#else
static void gekk_dodge(entity &self, entity &attacker, gtimef eta)
#endif
{
	if (random() > 0.25)
		return;

	if (!self.enemy.has_value())
		self.enemy = attacker;

	if (self.waterlevel > WATER_LEGS)
	{
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack);
		return;
	}
	
	if (skill == 0)
	{
		if (random() > 0.5)
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_lduck);
		else 
			self.monsterinfo.currentmove = &SAVABLE(gekk_move_rduck);
		return;
	}

	self.monsterinfo.pause_time = duration_cast<gtime>(level.time + eta + 0.3s);

	if (skill == 1)
	{
		if (random() > 0.33)
		{
			if (random() > 0.5)
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_lduck);
			else 
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_rduck);
		}
		else
		{
			if (random() > 0.66)
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack1);
			else 
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack2);
			
		}
		return;
	}
	else if (skill == 2)
	{
		if (random() > 0.66)
		{
			if (random() > 0.5)
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_lduck);
			else 
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_rduck);
		}
		else
		{
			if (random() > 0.66)
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack1);
			else 
				self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack2);
		}
		return;
	}

	if (random() > 0.66)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack1);
	else 
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_attack2);
}

REGISTER_STATIC_SAVABLE(gekk_dodge);

//
// SPAWN
//

/*QUAKED monster_gekk (1 .5 0) (-24 -24 -24) (24 24 24) Ambush Trigger_Spawn Sight Chant
*/
static void SP_monster_gekk(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict (self);
		return;
	}

	sound_swing = gi.soundindex ("gek/gk_atck1.wav");
	sound_hit = gi.soundindex ("gek/gk_atck2.wav");
	sound_hit2 = gi.soundindex ("gek/gk_atck3.wav");
	sound_death = gi.soundindex ("gek/gk_deth1.wav");
	sound_pain1 = gi.soundindex ("gek/gk_pain1.wav");
	sound_sight = gi.soundindex ("gek/gk_sght1.wav");
	sound_search = gi.soundindex ("gek/gk_idle1.wav");
	sound_step1 = gi.soundindex ("gek/gk_step1.wav");
	sound_step2 = gi.soundindex ("gek/gk_step2.wav");
	sound_step3 = gi.soundindex ("gek/gk_step3.wav");
	sound_thud = gi.soundindex ("mutant/thud1.wav");
	
	sound_chantlow = gi.soundindex ("gek/gek_low.wav");
	sound_chantmid = gi.soundindex ("gek/gek_mid.wav");
	sound_chanthigh = gi.soundindex ("gek/gek_high.wav");
	
	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;
	self.modelindex = gi.modelindex ("models/monsters/gekk/tris.md2");
	self.bounds = bbox::sized(24.f);

	gi.modelindex ("models/objects/gekkgib/pelvis/tris.md2");
	gi.modelindex ("models/objects/gekkgib/arm/tris.md2");
	gi.modelindex ("models/objects/gekkgib/torso/tris.md2");
	gi.modelindex ("models/objects/gekkgib/claw/tris.md2");
	gi.modelindex ("models/objects/gekkgib/leg/tris.md2");
	gi.modelindex ("models/objects/gekkgib/head/tris.md2");
		
	self.health = 125;
	self.gib_health = -30;
	self.mass = 300;
	self.bleed_style = BLEED_GREEN;

	self.pain = SAVABLE(gekk_pain);
	self.die = SAVABLE(gekk_die);

	self.monsterinfo.stand = SAVABLE(gekk_stand);

	self.monsterinfo.walk = SAVABLE(gekk_walk);
	self.monsterinfo.run = SAVABLE(gekk_run_start);
	self.monsterinfo.dodge = SAVABLE(gekk_dodge);
	self.monsterinfo.attack = SAVABLE(gekk_jump);
	self.monsterinfo.melee = SAVABLE(gekk_melee);
	self.monsterinfo.sight = SAVABLE(gekk_sight);
	
	self.monsterinfo.search = SAVABLE(gekk_search);
	self.monsterinfo.idle = SAVABLE(gekk_idle);
	self.monsterinfo.checkattack = SAVABLE(gekk_checkattack);
	self.monsterinfo.reacttodamage = SAVABLE(gekk_reacttodamage);

	gi.linkentity (self);
	
	self.monsterinfo.currentmove = &SAVABLE(gekk_move_stand);

	self.monsterinfo.scale = MODEL_SCALE;
	walkmonster_start (self);

	if (self.spawnflags & GEKK_CHANT)
		self.monsterinfo.currentmove = &SAVABLE(gekk_move_chant);
}

REGISTER_ENTITY(MONSTER_GEKK, monster_gekk);
#endif