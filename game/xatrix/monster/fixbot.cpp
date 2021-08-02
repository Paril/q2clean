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
#include "game/spawn.h"
#include "game/ballistics/hit.h"
#include "game/xatrix/monster.h"
#include "game/combat.h"
#include "game/xatrix/func.h"
#include "fixbot_model.h"
#include "fixbot.h"

constexpr monster_muzzleflash MZ2_fixbot_BLASTER_1 = MZ2_HOVER_BLASTER_1;

static sound_index	sound_pain1;
static sound_index	sound_die;
static sound_index	sound_weld1;
static sound_index	sound_weld2;
static sound_index	sound_weld3;

static entityref fixbot_FindDeadMonster(entity &self)
{
	entityref ent, best;

	while ((ent = findradius(ent, self.s.origin, 1024)).has_value())
	{
		if (ent == self)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		if (ent->owner.has_value())
			continue;
		if (ent->health > 0)
			continue;
		if (ent->nextthink)
			continue;
		if (!visible(self, ent))
			continue;
		if (!best.has_value())
			best = ent;
		else if (ent->max_health > best->max_health)
			best = ent;
	}

	return best;
}

static bool fixbot_search(entity &self)
{
	if (self.goalentity.has_value())
		return false;

	entityref ent = fixbot_FindDeadMonster(self);
	
	if (!ent.has_value())
		return false;

	self.oldenemy = self.enemy;
	self.enemy = ent;
	self.enemy->owner = self;
	self.monsterinfo.aiflags |= AI_MEDIC;
	FoundTarget (self);

	return true;
}

static void ai_move2(entity &self, float dist)
{
	if (dist)
		M_walkmove (self, self.s.angles[YAW], dist);

	vector v = self.goalentity->s.origin - self.s.origin;
	self.ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
};

static void fixbot_fire_welder(entity &self);
static void weldstate(entity &self);
static void change_to_roam(entity &self);

constexpr mframe_t fixbot_frames_weld [] =
{
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, weldstate }
};
constexpr mmove_t fixbot_move_weld = { FRAME_weldmiddle_01, FRAME_weldmiddle_07, fixbot_frames_weld };

static REGISTER_SAVABLE_DATA(fixbot_move_weld);

constexpr mframe_t fixbot_frames_weld_end [] =
{
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2, weldstate }
};
constexpr mmove_t fixbot_move_weld_end = { FRAME_weldend_01, FRAME_weldend_07, fixbot_frames_weld_end };

static REGISTER_SAVABLE_DATA(fixbot_move_weld_end);

constexpr mframe_t fixbot_frames_stand [] =
{
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
	{ ai_move, 0, change_to_roam }
};
constexpr mmove_t fixbot_move_stand = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand };

static REGISTER_SAVABLE_DATA(fixbot_move_stand);

static void weldstate(entity &self)
{
	if (self.s.frame == FRAME_weldstart_10)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_weld);
	else if (self.s.frame == FRAME_weldmiddle_07)
	{
		if (self.goalentity->health < 0) 
		{
			self.enemy->owner = 0;
			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_weld_end);
		}
		else
			self.goalentity->health -= 10;
	}
	else
	{
		self.goalentity = self.enemy = 0;
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
	}
}

constexpr mframe_t fixbot_frames_weld_start [] = 
{
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2 },
	{ ai_move2, 0, weldstate }
};
constexpr mmove_t fixbot_move_weld_start = { FRAME_weldstart_01, FRAME_weldstart_10, fixbot_frames_weld_start };

static REGISTER_SAVABLE_DATA(fixbot_move_weld_start);

static entity_type ET_BOT_GOAL("bot_goal");

static void use_scanner(entity &self)
{
	entityref	ent;
	float   radius = 1024;

	while ((ent = findradius(ent, self.s.origin, radius)).has_value())
	{
		if (ent->health >= 100)
		{
			if (ent->type == ET_OBJECT_REPAIR)
			{
				if (visible(self, ent))
				{	
					// remove the old one
					if (self.goalentity->type == ET_BOT_GOAL)
					{
						self.goalentity->nextthink = level.framenum + 1;
						self.goalentity->think = SAVABLE(G_FreeEdict);
					}	
					
					self.goalentity = self.enemy = ent;
					
					vector vec = self.s.origin - self.goalentity->s.origin;
					float len = VectorNormalize (vec);

					if (len < 32)
					{
						self.monsterinfo.currentmove = &SAVABLE(fixbot_move_weld_start);
						return;
					}
					return;
				}
			}
		}
	}

	vector vec = self.s.origin - self.goalentity->s.origin;
	float len = VectorLength (vec);

	if (len < 32)
	{
		if (self.goalentity->type == ET_OBJECT_REPAIR)
			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_weld_start);
		else 
		{
			self.goalentity->nextthink = level.framenum + 1;
			self.goalentity->think = SAVABLE(G_FreeEdict);
			self.goalentity = self.enemy = 0;
			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
		}
		return;
	}

	vec = self.s.origin - self.s.old_origin;
	len = VectorLength (vec);

	if (len)
		return;

	// bot is stuck get new goalentity
	if (self.goalentity->type == ET_OBJECT_REPAIR)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
	else 
	{
		self.goalentity->nextthink = level.framenum + 1;
		self.goalentity->think = SAVABLE(G_FreeEdict);
		self.goalentity = self.enemy = 0;
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
	}
}

static void ai_movetogoal(entity &self, float dist)
{
	M_MoveToGoal (self, dist);
}

constexpr mframe_t fixbot_frames_forward [] =
{
	{ ai_movetogoal,	5,	use_scanner }
};
constexpr mmove_t fixbot_move_forward = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_forward };

static REGISTER_SAVABLE_DATA(fixbot_move_forward);

static void ai_facing(entity &self, float)
{
	if (infront (self, self.goalentity))
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_forward);
	else
	{
		vector v = self.goalentity->s.origin - self.s.origin;
		self.ideal_yaw = vectoyaw(v);	
		M_ChangeYaw (self);
	}
}

constexpr mframe_t fixbot_frames_turn [] =
{
	{ ai_facing }
};
constexpr mmove_t fixbot_move_turn = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_turn };

static REGISTER_SAVABLE_DATA(fixbot_move_turn);

static void roam_goal(entity &self)
{
	entity &ent = G_Spawn ();	
	ent.type = ET_BOT_GOAL;
	ent.solid = SOLID_BBOX;
	ent.owner = self;
	gi.linkentity (ent);

	float oldlen = 0;
	vector	whichvec = vec3_origin;

	for (int i = 0; i < 12; i++) 
	{
		vector dang = self.s.angles;

		if (i < 6)
			dang[YAW] += 30 * i;
		else 
			dang[YAW] -= 30 * (i-6);

		vector forward;
		AngleVectors (dang, &forward, nullptr, nullptr);
		vector end = self.s.origin + (forward * 8192);

		trace tr = gi.traceline (self.s.origin, end, self, MASK_SHOT);

		vector vec = self.s.origin - tr.endpos;
		float len = VectorNormalize (vec);
		
		if (len > oldlen)
		{
			oldlen = len;
			whichvec = tr.endpos;
		}
	}
	
	ent.s.origin = whichvec;
	self.goalentity = self.enemy = ent;
	
	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_turn);
}

/*
	generic frame to move bot
*/
constexpr mframe_t fixbot_frames_roamgoal [] =
{
	{ ai_move, 0, roam_goal }
};
constexpr mmove_t fixbot_move_roamgoal = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_roamgoal };

static REGISTER_SAVABLE_DATA(fixbot_move_roamgoal);

constexpr mframe_t fixbot_frames_stand2 [] =
{
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
constexpr mmove_t fixbot_move_stand2 = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand2 };

static REGISTER_SAVABLE_DATA(fixbot_move_stand2);

static void fly_vertical2(entity &self);

constexpr mframe_t fixbot_frames_landing [] =
{
	{ ai_move },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 }
};
constexpr mmove_t fixbot_move_landing = { FRAME_landing_01, FRAME_landing_58, fixbot_frames_landing };

static REGISTER_SAVABLE_DATA(fixbot_move_landing);

inline void landing_goal(entity &self) 
{
	entity &ent = G_Spawn ();	
	ent.type = ET_BOT_GOAL;
	ent.solid = SOLID_BBOX;
	ent.owner = self;
	gi.linkentity (ent);
	
	ent.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, 24 }
	};
	
	vector forward, up;
	AngleVectors (self.s.angles, &forward, nullptr, &up);
	vector end = self.s.origin + (forward * 32) + (up * -8192); // Paril: this wasn't right in vanilla

	trace tr = gi.trace (self.s.origin, ent.bounds, end, self, MASK_MONSTERSOLID);

	ent.s.origin = tr.endpos;
	
	self.goalentity = self.enemy = ent;
	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_landing);
}

static void fly_vertical(entity &self);

constexpr mframe_t fixbot_frames_takeoff [] =
{
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },

	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical },
	{ ai_move,	0.01f,	fly_vertical }
};
constexpr mmove_t fixbot_move_takeoff = { FRAME_takeoff_01, FRAME_takeoff_16, fixbot_frames_takeoff };

static REGISTER_SAVABLE_DATA(fixbot_move_takeoff);

inline void takeoff_goal(entity &self)
{
	entity &ent = G_Spawn ();	
	ent.type = ET_BOT_GOAL;
	ent.solid = SOLID_BBOX;
	ent.owner = self;
	gi.linkentity (ent);

	ent.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, 24 }
	};

	vector forward, up;
	AngleVectors (self.s.angles, &forward, nullptr, &up);
	vector end = self.s.origin + (forward * 32) + (up * 128); // Paril: this wasn't right in vanilla

	trace tr = gi.trace (self.s.origin, ent.bounds, end, self, MASK_MONSTERSOLID);

	ent.s.origin = tr.endpos;
	
	self.goalentity = self.enemy = ent;
	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_takeoff);
}

static void change_to_roam(entity &self)
{
	if (fixbot_search(self))
		return;

	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_roamgoal);
	
	if (self.spawnflags & 16)
	{
		landing_goal (self);
		self.spawnflags &= (spawn_flag) ~16;
		self.spawnflags = (spawn_flag) 32;
	}
	if (self.spawnflags & 8) 
	{
		takeoff_goal (self);
		self.spawnflags &= (spawn_flag) ~8;
		self.spawnflags = (spawn_flag) 32;
	}
	if (self.spawnflags & 4)
	{
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_roamgoal);
		self.spawnflags &= (spawn_flag) ~4;
		self.spawnflags = (spawn_flag) 32;
	}

	if (!self.spawnflags)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand2);
}

/*
	generic ambient stand
*/

static void fly_vertical2(entity &self)
{
	vector v = self.goalentity->s.origin - self.s.origin;
	float len = VectorLength (v);
	self.ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
	
	if (len < 32)
	{
		self.goalentity->nextthink = level.framenum + 1;
		self.goalentity->think = SAVABLE(G_FreeEdict);
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
		self.goalentity = self.enemy = 0;
	}
}

/*
	when the bot has found a landing pad
	it will proceed to its goalentity
	just above the landing pad and
	decend translated along the z the current
	frames are at 10fps
*/ 
static void blastoff(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_impact, int32_t hspread, int32_t vspread)
{
	hspread += (self.s.frame - FRAME_takeoff_01);
	vspread += (self.s.frame - FRAME_takeoff_01);

	trace	tr = gi.traceline (self.s.origin, start, self, MASK_SHOT);

	vector	water_start = vec3_origin;
	bool	water = false;

	if (!(tr.fraction < 1.0))
	{
		vector dir = vectoangles (aimdir);
		vector forward, right, up;
		AngleVectors (dir, &forward, &right, &up);

		float r = crandom()*hspread;
		float u = crandom()*vspread;
		vector end = start + (forward * 8192);
		end += right * r;
		end += up * u;

		content_flags content_mask = MASK_SHOT | MASK_WATER;

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			water_start = start;
			content_mask &= ~MASK_WATER;
		}

		tr = gi.traceline(start, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int		color;

			water = true;
			water_start = tr.endpos;

			if (start != tr.endpos)
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (tr.surface.name == "*brwater")
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				dir = end - start;
				dir = vectoangles (dir);
				AngleVectors (dir, &forward, &right, &up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				end = water_start + (forward * 8192);
				end += right * u;
				end += up * u;
			}

			// re-trace ignoring water this time
			tr = gi.traceline(water_start, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (tr.fraction < 1.0 && !(tr.surface.flags & SURF_SKY))
	{
		if (tr.ent.takedamage)
			T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, DAMAGE_BULLET, MOD_BLASTOFF);
		else if (strncmp (tr.surface.name, "sky", 3) != 0)
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (te_impact);
			gi.WritePosition (tr.endpos);
			gi.WriteDir (tr.normal);
			gi.multicast (tr.endpos, MULTICAST_PVS);
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vector dir = tr.endpos - water_start;
		VectorNormalize (dir);
		vector pos = tr.endpos + (dir * -2);
		if (gi.pointcontents (pos) & MASK_WATER)
			tr.endpos = pos;
		else
			tr = gi.traceline (pos, water_start, tr.ent, MASK_WATER);

		pos = water_start + tr.endpos;
		pos *= 0.5f;

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}

static void fly_vertical(entity &self)
{
	vector v = self.goalentity->s.origin - self.s.origin;
	self.ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
	
	if (self.s.frame == FRAME_landing_58 || self.s.frame == FRAME_takeoff_16)
	{
		self.goalentity->nextthink = level.framenum + 1;
		self.goalentity->think = SAVABLE(G_FreeEdict);
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
		self.goalentity = self.enemy = null_entity;
	}

	// kick up some particles
	vector tempvec = self.s.angles;
	tempvec[PITCH] += 90;

	vector forward;
	AngleVectors (tempvec, &forward, nullptr, nullptr);
	vector start = self.s.origin;
	
	for (int32_t i = 0; i < 10; i++)
		blastoff (self, start, forward, 2, 1, TE_SHOTGUN, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD);
}

/*
	takeoff
*/
static void fixbot_fire_welder(entity &self)
{
	if (!self.enemy.has_value())
		return;

	vector forward, right;
	AngleVectors (self.s.angles, &forward, &right, nullptr);
	vector start = G_ProjectSource (self.s.origin, { 24.f, -0.8f, -10.0f }, forward, right);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WELDING_SPARKS);
	gi.WriteByte (10);
	gi.WritePosition (start);
	gi.WriteDir (vec3_origin);
	gi.WriteByte (0xe0 + (Q_rand() & 7));
	gi.multicast (self.s.origin, MULTICAST_PVS);

	if (random() > 0.8)
	{
		float r = random();

		if (r < 0.33)
			gi.sound (self, CHAN_VOICE, sound_weld1, 1, ATTN_IDLE, 0);
		else if (r < 0.66)
			gi.sound (self, CHAN_VOICE, sound_weld2, 1, ATTN_IDLE, 0);
		else
			gi.sound (self, CHAN_VOICE, sound_weld3, 1, ATTN_IDLE, 0);
	}
}

static void fixbot_stand(entity &self)
{
	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_stand);

/*
	
*/
constexpr mframe_t fixbot_frames_run [] =
{
	{ ai_run,	10 }
};
constexpr mmove_t fixbot_move_run = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_run };

static REGISTER_SAVABLE_DATA(fixbot_move_run);

static void fixbot_run(entity &self)
{
	if (self.monsterinfo.aiflags & AI_STAND_GROUND)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
	else
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_run);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_run);

/* findout what this is */
constexpr mframe_t fixbot_frames_paina [] =
{
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t fixbot_move_paina = { FRAME_paina_01, FRAME_paina_06, fixbot_frames_paina, fixbot_run };

static REGISTER_SAVABLE_DATA(fixbot_move_paina);

/* findout what this is */
constexpr mframe_t fixbot_frames_painb [] =
{
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
constexpr mmove_t fixbot_move_painb = { FRAME_painb_01, FRAME_painb_08, fixbot_frames_painb, fixbot_run };

static REGISTER_SAVABLE_DATA(fixbot_move_painb);

/*
	backup from pain
	call a generic painsound
	some spark effects
*/
constexpr mframe_t fixbot_frames_pain3 [] =
{
	{ ai_move, -1 }
};
constexpr mmove_t fixbot_move_pain3 = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_pain3, fixbot_run };

static REGISTER_SAVABLE_DATA(fixbot_move_pain3);

/*
	
*/
constexpr mframe_t fixbot_frames_walk [] =
{
	{ ai_walk,	5 }
};
constexpr mmove_t fixbot_move_walk = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_walk };

static REGISTER_SAVABLE_DATA(fixbot_move_walk);

static bool check_telefrag(entity &self)
{
	vector up;
	AngleVectors (self.enemy->s.angles, nullptr, nullptr, &up);

	vector start = self.enemy->s.origin + (up * 48); // Paril: this was broken in vanilla
	trace tr = gi.trace (self.enemy->s.origin, self.enemy->bounds, start, self, MASK_MONSTERSOLID);

	if (tr.ent.takedamage)
	{
		tr.ent.health = -1000;
		return false;
	}

	return true;
}

static void fixbot_fire_laser(entity &self)
{
	// critter dun got blown up while bein' fixed
	if (self.enemy->health <= self.enemy->gib_health)
	{
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
		self.monsterinfo.aiflags &= ~AI_MEDIC;
		return;
	}

	gi.sound(self, CHAN_AUTO, gi.soundindex("misc/lasfly.wav"), 1, ATTN_STATIC, 0);

	vector start = self.s.origin;
	vector end = self.enemy->s.origin;
	vector dir = end - start;
	vector angles = vectoangles (dir);
	
	entity &ent = G_Spawn ();
	ent.s.origin = self.s.origin;

	vector forward;
	AngleVectors (angles, &forward, nullptr, nullptr);
	ent.s.angles = angles;
	start = ent.s.origin + (forward * 16);
	
	ent.s.origin = start;
	ent.enemy = self.enemy;
	ent.owner = self;
	ent.dmg = -1;
	monster_dabeam (ent);

	if (self.enemy->health > (self.enemy->mass / 10))
	{
		// sorry guys but had to fix the problem this way
		// if it doesn't do this then two creatures can share the same space
		// and its real bad.
		if (check_telefrag (self)) 
		{
			self.enemy->spawnflags = NO_SPAWNFLAGS;
			self.enemy->monsterinfo.aiflags = AI_NONE;
			self.enemy->target = 0;
			self.enemy->targetname = 0;
			self.enemy->combattarget = 0;
			self.enemy->deathtarget = 0;
			self.enemy->owner = self;
			ED_CallSpawn (self.enemy);
			self.enemy->owner = 0;
			self.s.origin[2] += 1;
		
			self.enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;

			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);
			self.monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else
		self.enemy->monsterinfo.aiflags |= AI_RESURRECTING;
}

constexpr mframe_t fixbot_frames_laserattack [] =
{
	{ ai_charge,	0,	fixbot_fire_laser },
	{ ai_charge,	0,	fixbot_fire_laser },
	{ ai_charge,	0,	fixbot_fire_laser },
	{ ai_charge,	0,	fixbot_fire_laser },
	{ ai_charge,	0,	fixbot_fire_laser },
	{ ai_charge,	0,	fixbot_fire_laser }
};
constexpr mmove_t fixbot_move_laserattack = { FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_laserattack };

static REGISTER_SAVABLE_DATA(fixbot_move_laserattack);

static void fixbot_fire_blaster(entity &self)
{
	if (!visible(self, self.enemy))
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_run);	

	vector forward, right;
	AngleVectors (self.s.angles, &forward, &right, nullptr);

	vector start = G_ProjectSource (self.s.origin, monster_flash_offset[MZ2_fixbot_BLASTER_1], forward, right);
	vector end = self.enemy->s.origin;
	end[2] += self.enemy->viewheight;
	vector dir = end - start;

	monster_fire_blaster (self, start, dir, 15, 1000, MZ2_fixbot_BLASTER_1, EF_BLASTER);
}

/*
	need to get forward translation data
	for the charge attack
*/
constexpr mframe_t fixbot_frames_attack2 [] =
{
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },

	{ ai_charge,	-10 },
	{ ai_charge,  -10 },
	{ ai_charge,	-10 },
	{ ai_charge,  -10 },
	{ ai_charge,	-10 },
	{ ai_charge,  -10 },
	{ ai_charge,	-10 },
	{ ai_charge,  -10 },
	{ ai_charge,	-10 },
	{ ai_charge,  -10 },

	{ ai_charge,	0,	fixbot_fire_blaster },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },

	{ ai_charge }
};
constexpr mmove_t fixbot_move_attack2 = { FRAME_charging_01, FRAME_charging_31, fixbot_frames_attack2, fixbot_run };

static REGISTER_SAVABLE_DATA(fixbot_move_attack2);

static void fixbot_attack(entity &self)
{
	if (self.monsterinfo.aiflags & AI_MEDIC)
	{
		if (!visible (self, self.goalentity))
			return;
		vector vec = self.s.origin - self.enemy->s.origin;
		float len = VectorLength(vec);

		if (len <= 128)
			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_laserattack);
	}
	else
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_attack2);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_attack);

//
constexpr mframe_t fixbot_frames_start_attack [] =
{
	{ ai_charge }
};
constexpr mmove_t fixbot_move_start_attack = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_start_attack, fixbot_attack };

static REGISTER_SAVABLE_DATA(fixbot_move_start_attack);

static void fixbot_walk(entity &self)
{
	if (self.goalentity->type == ET_OBJECT_REPAIR)
	{
		vector vec = self.s.origin - self.goalentity->s.origin;
		float len = VectorLength (vec);

		if (len < 32)
		{
			self.monsterinfo.currentmove = &SAVABLE(fixbot_move_weld_start);
			return;
		}
	}

	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_walk);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_walk);

static void fixbot_pain(entity &self, entity &, float, int32_t damage)
{
	if (level.framenum < self.pain_debounce_framenum)
		return;

	self.pain_debounce_framenum = level.framenum + (int)(3 * BASE_FRAMERATE);
	gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (damage <= 10)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_pain3);
	else if (damage <= 25)
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_painb);
	else
		self.monsterinfo.currentmove = &SAVABLE(fixbot_move_paina);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_pain);

static void fixbot_die(entity &self, entity &, entity &, int32_t, vector)
{
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);
}

static REGISTER_SAVABLE_FUNCTION(fixbot_die);

/*QUAKED monster_fixbot (1 .5 0) (-32 -32 -24) (32 32 24) Ambush Trigger_Spawn Fixit Takeoff Landing
*/
static void SP_monster_fixbot(entity &self)
{
	if (deathmatch)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("flyer/flypain1.wav");
	sound_die = gi.soundindex ("flyer/flydeth1.wav");

	sound_weld1 = gi.soundindex ("misc/welder1.wav");
	sound_weld2 = gi.soundindex ("misc/welder2.wav");
	sound_weld3 = gi.soundindex ("misc/welder3.wav");

	self.s.modelindex = gi.modelindex ("models/monsters/fixbot/tris.md2");
	
	self.bounds = {
		.mins = { -32, -32, -24 },
		.maxs = { 32, 32, 24 }
	};
	
	self.movetype = MOVETYPE_STEP;
	self.solid = SOLID_BBOX;

	self.health = 150;
	self.mass = 150;

	self.pain = SAVABLE(fixbot_pain);
	self.die = SAVABLE(fixbot_die);

	self.monsterinfo.stand = SAVABLE(fixbot_stand);
	self.monsterinfo.walk = SAVABLE(fixbot_walk);
	self.monsterinfo.run = SAVABLE(fixbot_run);
	self.monsterinfo.attack = SAVABLE(fixbot_attack);
	
	gi.linkentity (self);

	self.monsterinfo.currentmove = &SAVABLE(fixbot_move_stand);	
	self.monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}

static REGISTER_ENTITY(MONSTER_FIXBOT, monster_fixbot);

#endif
