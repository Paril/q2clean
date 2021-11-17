#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/func.h"
#include "game/game.h"
#include "game/spawn.h"

#include "lib/gi.h"
#include "game/util.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "game/combat.h"

#ifdef SINGLE_PLAYER
#include "game/ai.h"
#endif

//==========================================================

/*QUAKED target_steam (1 0 0) (-8 -8 -8) (8 8 8)
Creates a steam effect (particles w/ velocity in a line).

  speed = velocity of particles (default 50)
  count = number of particles (default 32)
  sounds = color of particles (default 8 for steam)
     the color range is from this color to this color + 6
  wait = seconds to run before stopping (overrides default
     value derived from func_timer)

  best way to use this is to tie it to a func_timer that "pokes"
  it every second (or however long you set the wait time, above)

  note that the width of the base is proportional to the speed
  good colors to use:
  6-9 - varying whites (darker to brighter)
  224 - sparks
  176 - blue water
  80  - brown water
  208 - slime
  232 - blood
*/
static void use_target_steam(entity &self, entity &other, entity &)
{
	static short nextid;

	if (nextid > 20000)
		nextid = nextid % 20000;

	nextid++;
	
	// automagically set wait from func_timer unless they set it already
	if (self.wait == gtimef::zero() && other.wait != gtimef::zero())
		self.wait = other.wait;

	if (self.enemy.has_value())
	{
		vector point = self.enemy->absbounds.center();
		self.movedir = point - self.origin;
		VectorNormalize (self.movedir);
	}

	vector point = self.origin + (self.movedir * self.speed * 0.5);
	if (self.wait > 100ms)
		gi.ConstructMessage(svc_temp_entity, TE_STEAM, nextid, (uint8_t) self.count, self.origin, vecdir { self.movedir },
							(uint8_t) self.sounds, (int16_t) self.speed, (int32_t) (self.wait.count() * 1000)).multicast(self.origin, MULTICAST_PVS);
	else
		gi.ConstructMessage(svc_temp_entity, TE_STEAM, (int16_t) -1, (uint8_t) self.count, self.origin, vecdir { self.movedir },
							(uint8_t) self.sounds, (int16_t) self.speed).multicast(self.origin, MULTICAST_PVS);
}

static REGISTER_SAVABLE(use_target_steam);

static void target_steam_start(entity &self)
{
	self.use = SAVABLE(use_target_steam);

	if (self.target)
	{
		entityref ent = G_FindEquals<&entity::targetname>(world, self.target);
		if (!ent.has_value())
			gi.dprintfmt("{}: {} is a bad target\n", self, self.target);
		self.enemy = ent;
	}
	else
		G_SetMovedir (self.angles, self.movedir);

	if (!self.count)
		self.count = 32;
	if (!self.speed)
		self.speed = 75;
	if (!self.sounds)
		self.sounds = 8;

	self.svflags = SVF_NOCLIENT;

	gi.linkentity (self);
}

static REGISTER_SAVABLE(target_steam_start);

static void SP_target_steam(entity &self)
{
	if (self.target)
	{
		self.think = SAVABLE(target_steam_start);
		self.nextthink = level.time + 100ms;
	}
	else
		target_steam_start (self);
}

static REGISTER_ENTITY(TARGET_STEAM, target_steam);

#ifdef SINGLE_PLAYER
//==========================================================
// target_anger
//==========================================================

static void target_anger_use(entity &self, entity &, entity &)
{
	entityref ctarget = G_FindEquals<&entity::targetname>(world, self.killtarget);

	if (!ctarget.has_value())
		return;

	// Make whatever a "good guy" so the monster will try to kill it!
	ctarget->monsterinfo.aiflags |= AI_GOOD_GUY;
	ctarget->svflags |= SVF_MONSTER;
	ctarget->health = 300;

	for (entity &t : G_IterateEquals<&entity::targetname>(self.target))
	{
		if (t == self)
			gi.dprint("WARNING: entity used itself.\n");
		else if (t.use)
		{
			if (t.health < 0)
				return;

			t.enemy = ctarget;
			t.monsterinfo.aiflags |= AI_TARGET_ANGER;
			FoundTarget(t);
		}

		if (!self.inuse)
		{
			gi.dprint("entity was removed while using targets\n");
			return;
		}
	}
}

static REGISTER_SAVABLE(target_anger_use);

/*QUAKED target_anger (1 0 0) (-8 -8 -8) (8 8 8)
This trigger will cause an entity to be angry at another entity when a player touches it. Target the
entity you want to anger, and killtarget the entity you want it to be angry at.

target - entity to piss off
killtarget - entity to be pissed off at
*/
static void SP_target_anger(entity &self)
{	
	if (!self.target)
	{
		gi.dprintfmt("{} without target!\n", self);
		G_FreeEdict (self);
		return;
	}
	if (!self.killtarget)
	{
		gi.dprintfmt("{} without killtarget!\n", self);
		G_FreeEdict (self);
		return;
	}

	self.use = SAVABLE(target_anger_use);
	self.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(TARGET_ANGER, target_anger);
#endif

// ***********************************
// target_killplayers
// ***********************************
static void target_killplayers_use(entity &self, entity &, entity &)
{
	// kill the players
	for (entity &player : entity_range(1, game.maxclients))
	{
		if (!player.inuse)
			continue;

		// nail it
		T_Damage (player, self, self, vec3_origin, self.origin, vec3_origin, 100000, 0, { DAMAGE_NO_PROTECTION }, MOD_DEFAULT);
	}

	// kill any visible monsters
	for (entity &ent : entity_range(game.maxclients + 1, num_entities - 1))
	{
		if (!ent.inuse)
			continue;
		if (ent.health < 1)
			continue;
		if (!ent.takedamage)
			continue;
		
		for (entity &player : entity_range(1, game.maxclients))
		{
			if (player.inuse && visible(player, ent))
			{
				T_Damage (ent, self, self, vec3_origin, ent.origin, vec3_origin, ent.health, 0, { DAMAGE_NO_PROTECTION }, MOD_DEFAULT);
				break;
			}
		}
	}
}

static REGISTER_SAVABLE(target_killplayers_use);

/*QUAKED target_killplayers (1 0 0) (-8 -8 -8) (8 8 8)
When triggered, this will kill all the players on the map.
*/
static void SP_target_killplayers(entity &self)
{
	self.use = SAVABLE(target_killplayers_use);
	self.svflags = SVF_NOCLIENT;
}

static REGISTER_ENTITY(TARGET_KILLPLAYERS, target_killplayers);
#ifdef SINGLE_PLAYER

/*QUAKED target_blacklight (1 0 1) (-16 -16 -24) (16 16 24) 
Pulsing black light with sphere in the center
*/
static void blacklight_think(entity &self)
{
	self.angles = randomv({ 360.f, 360.f, 360.f });
	self.nextthink = level.time + 1_hz;
}

static REGISTER_SAVABLE(blacklight_think);

static void SP_target_blacklight(entity &ent)
{
	if (deathmatch)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent.effects |= (EF_TRACKERTRAIL|EF_TRACKER);
	ent.think = SAVABLE(blacklight_think);
	ent.modelindex = gi.modelindex ("models/items/spawngro2/tris.md2");
	ent.frame = 1;
	ent.nextthink = level.time + 1_hz;
	gi.linkentity (ent);
}

static REGISTER_ENTITY(TARGET_BLACKLIGHT, target_blacklight);

/*QUAKED target_orb (1 0 1) (-16 -16 -24) (16 16 24) 
Translucent pulsing orb with speckles
*/

static void SP_target_orb(entity &ent)
{
	if (deathmatch)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent.think = SAVABLE(blacklight_think);
	ent.nextthink = level.time + 1_hz;
	ent.modelindex = gi.modelindex ("models/items/spawngro2/tris.md2");
	ent.frame = 2;
	ent.effects |= EF_SPHERETRANS;
	gi.linkentity (ent);
}

static REGISTER_ENTITY(TARGET_ORB, target_orb);
#endif
#endif