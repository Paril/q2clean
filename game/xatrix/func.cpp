#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/misc.h"
#include "game/spawn.h"
#include "game/game.h"
#include "game/util.h"
#include "lib/string/format.h"
#include "func.h"

/*QUAKED rotating_light (0 .5 .8) (-8 -8 -8) (8 8 8) START_OFF ALARM
"health"	if set, the light may be killed.
*/

// RAFAEL 
// note to self
// the lights will take damage from explosions
// this could leave a player in total darkness very bad
 
const spawn_flag START_OFF	= (spawn_flag) 1;

static void rotating_light_alarm(entity &self)
{
	if (self.spawnflags & START_OFF)
	{
		self.think = 0;
		self.nextthink = 0;	
	}
	else
	{
		gi.sound (self, CHAN_NO_PHS_ADD | CHAN_VOICE, self.moveinfo.sound_start, 1, ATTN_STATIC, 0);
		self.nextthink = level.framenum + (1 * BASE_FRAMERATE);
	}
}

static REGISTER_SAVABLE_FUNCTION(rotating_light_alarm);

static void rotating_light_killed(entity &self, entity &, entity &, int32_t, vector)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WELDING_SPARKS);
	gi.WriteByte (30);
	gi.WritePosition (self.s.origin);
	gi.WriteDir (vec3_origin);
	gi.WriteByte (0xe0 + (Q_rand()&7));
	gi.multicast (self.s.origin, MULTICAST_PVS);

	self.s.effects &= ~EF_SPINNINGLIGHTS;
	self.use = 0;

	self.think = SAVABLE(G_FreeEdict);	
	self.nextthink = level.framenum + 1;
}

static REGISTER_SAVABLE_FUNCTION(rotating_light_killed);

static void rotating_light_use(entity &self, entity &, entity &)
{
	if (self.spawnflags & START_OFF)
	{
		self.spawnflags &= ~START_OFF;
		self.s.effects |= EF_SPINNINGLIGHTS;

		if (self.spawnflags & 2)
		{
			self.think = SAVABLE(rotating_light_alarm);
			self.nextthink = level.framenum + 1;
		}
	}
	else
	{
		self.spawnflags |= START_OFF;
		self.s.effects &= ~EF_SPINNINGLIGHTS;
	}
}

static REGISTER_SAVABLE_FUNCTION(rotating_light_use);

static void SP_rotating_light(entity &self)
{
	self.movetype = MOVETYPE_STOP;
	self.solid = SOLID_BBOX;
	
	self.s.modelindex = gi.modelindex ("models/objects/light/tris.md2");
	
	self.s.frame = 0;
		
	self.use = SAVABLE(rotating_light_use);
	
	if (self.spawnflags & START_OFF)
		self.s.effects &= ~EF_SPINNINGLIGHTS;
	else
		self.s.effects |= EF_SPINNINGLIGHTS;

	if (!self.speed)
		self.speed = 32.f;
	// this is a real cheap way
	// to set the radius of the light
	// self.s.frame = self.speed;

	if (!self.health)
	{
		self.health = 10;
		self.max_health = self.health;
		self.die = SAVABLE(rotating_light_killed);
		self.takedamage = true;
	}
	else
	{
		self.max_health = self.health;
		self.die = SAVABLE(rotating_light_killed);
		self.takedamage = true;
	}
	
	if (self.spawnflags & 2)
	{
		self.moveinfo.sound_start = gi.soundindex ("misc/alarm.wav");	
	}
	
	gi.linkentity (self);
}

static REGISTER_ENTITY(ROTATING_LIGHT, rotating_light);

#ifdef SINGLE_PLAYER
/*QUAKED func_object_repair (1 .5 0) (-8 -8 -8) (8 8 8) 
object to be repaired.
The default delay is 1 second
"delay" the delay in seconds for spark to occur
*/
static void object_repair_fx(entity &ent)
{
	ent.nextthink = level.framenum + (gtime)(ent.delay * BASE_FRAMERATE);

	if (ent.health <= 100)
		ent.health++;
 	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_WELDING_SPARKS);
		gi.WriteByte (10);
		gi.WritePosition (ent.s.origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte (0xe0 + (Q_rand()&7));
		gi.multicast (ent.s.origin, MULTICAST_PVS);
	}
}

static REGISTER_SAVABLE_FUNCTION(object_repair_fx);

static void object_repair_dead(entity &ent)
{
	G_UseTargets (ent, ent);
	ent.nextthink = level.framenum + 1;
	ent.think = SAVABLE(object_repair_fx);
}

static REGISTER_SAVABLE_FUNCTION(object_repair_dead);

static void object_repair_sparks(entity &ent)
{
	if (ent.health < 0)
	{
		ent.nextthink = level.framenum + 1;
		ent.think = SAVABLE(object_repair_dead);
		return;
	}

	ent.nextthink = level.framenum + (gtime)(ent.delay * BASE_FRAMERATE);
	
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WELDING_SPARKS);
	gi.WriteByte (10);
	gi.WritePosition (ent.s.origin);
	gi.WriteDir (vec3_origin);
	gi.WriteByte (0xe0 + (Q_rand()&7));
	gi.multicast (ent.s.origin, MULTICAST_PVS);
}

static REGISTER_SAVABLE_FUNCTION(object_repair_sparks);

static void SP_func_object_repair(entity &ent)
{
	ent.movetype = MOVETYPE_NONE;
	ent.solid = SOLID_BBOX;
	ent.bounds = bbox::sized(8.f);
	ent.think = SAVABLE(object_repair_sparks);
	ent.nextthink = level.framenum + (1 * BASE_FRAMERATE);
	ent.health = 100;

	if (!ent.delay)
		ent.delay = 1.0f;
}

REGISTER_ENTITY(OBJECT_REPAIR, func_object_repair);
#endif

#endif