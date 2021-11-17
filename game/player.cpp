#include "config.h"
#include "entity.h"
#include "../lib/info.h"
#include "game.h"
#include "player.h"
#include "chase.h"
#include "cmds.h"
#include "misc.h"
#include "spawn.h"

#include "util.h"
#include "game.h"
#include "hud.h"
#include "lib/protocol.h"
#include "lib/gi.h"
#include "lib/math/random.h"
#include "lib/string/format.h"
#include "view.h"
#ifdef SINGLE_PLAYER
#include "trail.h"

#ifdef GROUND_ZERO
#include "func.h"
#endif
#endif
#include "weaponry.h"
#include "combat.h"
#include "player_frames.h"
#include "game/items/entity.h"

#ifdef SINGLE_PLAYER
//
// Gross, ugly, disgustuing hack section
//

// this function is an ugly as hell hack to fix some map flaws
//
// the coop spawn spots on some maps are SNAFU.  There are coop spots
// with the wrong targetname as well as spots with no name at all
//
// we use carnal knowledge of the maps to fix the coop spot targetnames to match
// that of the nearest named single player spot

static void SP_FixCoopSpots(entity &self)
{
	for (entity &spot : G_IterateEquals<&entity::type>(ET_INFO_PLAYER_START))
	{
		if (!spot.targetname)
			continue;

		vector d = self.origin - spot.origin;

		if (VectorLength(d) < 384)
		{
			if (!striequals(self.targetname, spot.targetname))
				self.targetname = spot.targetname;

			return;
		}
	}
}

REGISTER_STATIC_SAVABLE(SP_FixCoopSpots);

// now if that one wasn't ugly enough for you then try this one on for size
// some maps don't have any coop spots at all, so we need to create them
// where they should have been

static void SP_CreateCoopSpots(entity &)
{
	if (level.mapname == "security")
	{
		{
			entity &spot = G_Spawn();
			spot.type = ET_INFO_PLAYER_COOP;
			spot.origin[0] = 188.f - 64.f;
			spot.origin[1] = -164.f;
			spot.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.angles[1] = 90.f;
		}

		{
			entity &spot = G_Spawn();
			spot.type = ET_INFO_PLAYER_COOP;
			spot.origin[0] = 188.f + 64.f;
			spot.origin[1] = -164.f;
			spot.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.angles[1] = 90.f;
		}

		{
			entity &spot = G_Spawn();
			spot.type = ET_INFO_PLAYER_COOP;
			spot.origin[0] = 188.f + 128.f;
			spot.origin[1] = -164.f;
			spot.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.angles[1] = 90.f;
		}
	}
}

REGISTER_STATIC_SAVABLE(SP_CreateCoopSpots);

#endif
/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
static void SP_info_player_start(entity &self [[maybe_unused]])
{
#ifdef SINGLE_PLAYER
	if (!coop)
		return;

	if (level.mapname == "security")
	{
		// invoke one of our gross, ugly, disgusting hacks
		self.think = SAVABLE(SP_CreateCoopSpots);
		self.nextthink = level.time + 1_hz;
	}
#endif
}

REGISTER_ENTITY(INFO_PLAYER_START, info_player_start);

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
static void SP_info_player_deathmatch(entity &self)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch)
	{
		G_FreeEdict(self);
		return;
	}

#endif
	SP_misc_teleporter_dest(self);
}

REGISTER_ENTITY(INFO_PLAYER_DEATHMATCH, info_player_deathmatch);

#ifdef SINGLE_PLAYER
/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/
static void SP_info_player_coop(entity &self)
{
	if (!coop)
	{
		G_FreeEdict(self);
		return;
	}

	if (level.mapname == "jail2"   ||
		level.mapname == "jail4"   ||
		level.mapname == "mine1"   ||
		level.mapname == "mine2"   ||
		level.mapname == "mine3"   ||
		level.mapname == "mine4"   ||
		level.mapname == "lab"     ||
		level.mapname == "boss1"   ||
		level.mapname == "fact3"   ||
		level.mapname == "biggun"  ||
		level.mapname == "space"   ||
		level.mapname == "command" ||
		level.mapname == "power2"  ||
		level.mapname == "strike")
	{
		// invoke one of our gross, ugly, disgusting hacks
		self.think = SAVABLE(SP_FixCoopSpots);
		self.nextthink = level.time + 1_hz;
	}
}

REGISTER_ENTITY(INFO_PLAYER_COOP, info_player_coop);

#endif
/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
static void SP_info_player_intermission(entity &)
{
}

REGISTER_ENTITY(INFO_PLAYER_INTERMISSION, info_player_intermission);

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
/*QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games on rmine2 where lava level
needs to be checked
*/
static void SP_info_player_coop_lava(entity &self)
{
	if (!coop)
	{
		G_FreeEdict (self);
		return;
	}
}

static REGISTER_ENTITY(INFO_PLAYER_COOP_LAVA, info_player_coop_lava);
#endif

static void TossClientWeapon(entity &self)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch)
		return;

#endif
	itemref it = self.client.pers.weapon;

	if (!self.client.pers.inventory[self.client.ammo_index] || it->id == ITEM_BLASTER
#ifdef CTF
		|| it->id == ITEM_GRAPPLE
#endif
		)
		it = nullptr;

	const bool quad = (dmflags & DF_QUAD_DROP) && (self.client.quad_time > (level.time + 1s));
		
#ifdef THE_RECKONING
	const bool quadfire = (dmflags & DF_QUADFIRE_DROP) && (self.client.quadfire_time > (level.time + 1s));
#endif

	float spread;

	if (it && quad)
		spread = 22.5f;
#ifdef THE_RECKONING
	else if (it && quadfire)
		spread = 12.5;
#endif
	else
		spread = 0.0f;

	if (it)
	{
		self.client.v_angle[YAW] -= spread;
		entity &drop = Drop_Item(self, it);
		self.client.v_angle[YAW] += spread;
		drop.spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self.client.v_angle[YAW] += spread;
		entity &drop = Drop_Item(self, GetItemByIndex(ITEM_QUAD_DAMAGE));
		self.client.v_angle[YAW] -= spread;
		drop.spawnflags |= DROPPED_PLAYER_ITEM;

		drop.touch = SAVABLE(Touch_Item);
		drop.nextthink = self.client.quad_time;
		drop.think = SAVABLE(G_FreeEdict);
	}
	
#ifdef THE_RECKONING
	if (quadfire)
	{
		self.client.v_angle[YAW] += spread;
		entity &drop = Drop_Item (self, GetItemByIndex(ITEM_QUADFIRE));
		self.client.v_angle[YAW] -= spread;
		drop.spawnflags |= DROPPED_PLAYER_ITEM;

		drop.touch = SAVABLE(Touch_Item);
		drop.nextthink = self.client.quadfire_time;
		drop.think = SAVABLE(G_FreeEdict);
	}
#endif
}

/*
==================
LookAtKiller
==================
*/
static void LookAtKiller(entity &self, entity &inflictor, entity &attacker)
{
	if (!attacker.is_world() && attacker != self)
		self.client.killer_yaw = vectoyaw(attacker.origin - self.origin);
	else if (!inflictor.is_world() && inflictor != self)
		self.client.killer_yaw = vectoyaw(inflictor.origin - self.origin);
	else
		self.client.killer_yaw = self.angles[YAW];
}

#ifdef HOOK_CODE
#include "game/ctf/grapple.h"
#endif

#ifdef CTF
void(entity, entity, entity) CTFFragBonuses;
void(entity) CTFDeadDropFlag;
void(entity) CTFDeadDropTech;
#endif

/*
==================
player_die
==================
*/
void player_die(entity &self, entity &inflictor, entity &attacker, int32_t damage, vector)
{
	self.avelocity = vec3_origin;

	self.takedamage = true;
	self.movetype = MOVETYPE_TOSS;

	self.modelindex2 = MODEL_NONE; // remove linked weapon model
#ifdef CTF
	self.modelindex3 = MODEL_NONE; // remove linked flag
#endif

	self.angles[PITCH] = 0;
	self.angles[ROLL] = 0;

	self.sound = SOUND_NONE;
	self.client.weapon_sound = SOUND_NONE;

	self.bounds.maxs.z = -8.f;

	self.svflags |= SVF_DEADMONSTER;

	if (!self.deadflag)
	{
		self.client.respawn_time = level.time + 1s;
		LookAtKiller(self, inflictor, attacker);
		self.client.ps.pmove.pm_type = PM_DEAD;

#ifdef CTF
		// if at start and same team, clear
		if (ctf.intVal && meansOfDeath == MOD_TELEFRAG &&
			self.client.resp.ctf_state < 2 &&
			self.client.resp.ctf_team == attacker.client.resp.ctf_team)
		{
			attacker.client.resp.score--;
			self.client.resp.ctf_state = 0;
		}

		CTFFragBonuses(self, inflictor, attacker);
#endif

		TossClientWeapon(self);

#ifdef HOOK_CODE
		GrapplePlayerReset(self);
#endif

#ifdef CTF
		CTFDeadDropFlag(self);
		CTFDeadDropTech(self);
#endif

#ifdef SINGLE_PLAYER
		if (deathmatch)
#endif
			Cmd_Help_f(self);       // show scores

		// clear inventory
		// this is kind of ugly, but it's how we want to handle keys in coop
		for (auto &it : item_list())
#ifdef SINGLE_PLAYER
		{
			if (coop && (it.flags & IT_KEY))
				self.client.resp.coop_respawn.inventory[it.id] = self.client.pers.inventory[it.id];
#endif
			self.client.pers.inventory[it.id] = 0;
#ifdef SINGLE_PLAYER
		}
#endif
	}

	// remove powerups
	self.client.quad_time = gtime::zero();
	self.client.invincible_time = gtime::zero();
	self.client.breather_time = gtime::zero();
	self.client.enviro_time = gtime::zero();
#ifdef THE_RECKONING
	self.client.quadfire_time = gtime::zero();
#endif
	self.flags &= ~FL_POWER_ARMOR;
	
#ifdef GROUND_ZERO
	self.client.double_time = gtime::zero();
#endif

	if (self.health <= self.gib_health)
	{
		// gib
#ifdef GROUND_ZERO
		if (!(self.flags & FL_NOGIB))
		{
#endif
			gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"));
			for (int n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
#ifdef GROUND_ZERO
		}
#endif

		ThrowClientHead(self, damage);
		self.client.anim_priority = ANIM_DEATH;
		self.client.anim_end = 0;
		self.takedamage = false;
	}
	else if (!self.deadflag) // normal death
	{
		static int i;

		i = (i + 1) % 3;

		// start a death animation
		self.client.anim_priority = ANIM_DEATH;
		if (self.client.ps.pmove.pm_flags & PMF_DUCKED)
		{
			self.frame = FRAME_crdeath1 - 1;
			self.client.anim_end = FRAME_crdeath5;
		}
		else switch (i)
		{
		case 0:
			self.frame = FRAME_death101 - 1;
			self.client.anim_end = FRAME_death106;
			break;
		case 1:
			self.frame = FRAME_death201 - 1;
			self.client.anim_end = FRAME_death206;
			break;
		case 2:
			self.frame = FRAME_death301 - 1;
			self.client.anim_end = FRAME_death308;
			break;
		}

		constexpr stringlit death_sounds[] = {
			"*death1.wav",
			"*death2.wav",
			"*death3.wav",
			"*death4.wav"
		};

		gi.sound(self, CHAN_VOICE, gi.soundindex(random_of(death_sounds)));
	}

	self.deadflag = true;

	gi.linkentity(self);
}

REGISTER_STATIC_SAVABLE(player_die);

//=======================================================================

#ifdef CTF
void(entity) CTFAssignTeam;
#endif

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant(entity &ent)
{
	ent.client.pers = {};

	ent.client.pers.selected_item = ITEM_BLASTER;
	ent.client.pers.inventory[ent.client.pers.selected_item] = 1;
	
	ent.client.pers.weapon = ent.client.pers.lastweapon = GetItemByIndex(ent.client.pers.selected_item);

#ifdef GRAPPLE
	ent.client.pers.inventory[ITEM_GRAPPLE] = 1;
#endif
	
#ifdef SINGLE_PLAYER
	ent.client.pers.health         = 100;
	ent.client.pers.max_health     = 100;
#else
	ent.health         = 100;
	ent.max_health     = 100;
#endif

	auto &max_ammo = ent.client.pers.max_ammo;

	max_ammo[AMMO_BULLETS] = 200;
	max_ammo[AMMO_SHELLS] = 100;
	max_ammo[AMMO_ROCKETS] = 50;
	max_ammo[AMMO_GRENADES] = 50;
	max_ammo[AMMO_CELLS] = 200;
	max_ammo[AMMO_SLUGS] = 50;
	
#ifdef THE_RECKONING
	max_ammo[AMMO_MAGSLUG] = 50;
	max_ammo[AMMO_TRAP] = 5;
#endif

#ifdef GROUND_ZERO
	max_ammo[AMMO_PROX] = 50;
	max_ammo[AMMO_TESLA] = 50;
	max_ammo[AMMO_FLECHETTES] = 200;
	max_ammo[AMMO_DISRUPTOR] = 100;
#endif
	
	ent.client.pers.connected = true;
}

static void InitClientResp(entity &ent)
{
#ifdef CTF
	ctfteam_t ctf_team = ent.client.resp.ctf_team;
	bool id_state = ent.client.resp.id_state;
#endif

	ent.client.resp = {};

#ifdef CTF
	ent.client.resp.ctf_team = ctf_team;
	ent.client.resp.id_state = id_state;
#endif

	ent.client.resp.enterframe = level.time;
#ifdef SINGLE_PLAYER
	ent.client.resp.coop_respawn = ent.client.pers;
#endif

#ifdef CTF
	if (ctf.intVal && ent.client.resp.ctf_team < CTF_TEAM1)
		CTFAssignTeam(ent);
#endif
}

#ifdef SINGLE_PLAYER
void SaveClientData()
{
	for (entity &ent : entity_range(1, game.maxclients))
	{
		if (!ent.inuse)
			continue;

		ent.client.pers.health = ent.health;
		ent.client.pers.max_health = ent.max_health;
		ent.client.pers.savedFlags = (ent.flags & (FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR));

		if (coop)
			ent.client.pers.score = ent.client.resp.score;
	}
}

static void FetchClientEntData(entity &ent)
{
	ent.health = ent.client.pers.health;
	ent.max_health = ent.client.pers.max_health;
	ent.flags |= ent.client.pers.savedFlags;

	if (coop)
		ent.client.resp.score = ent.client.pers.score;
}
#endif

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot(entity &spot)
{
	float bestplayerdistance = FLT_MAX;

	for (entity &player : entity_range(1, game.maxclients))
	{
		if (!player.inuse)
			continue;
		if (player.health <= 0)
			continue;

		bestplayerdistance = min(VectorDistance(spot.origin, player.origin), bestplayerdistance);
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
entityref SelectRandomDeathmatchSpawnPoint()
{
	int	count = 0;
	entityref spot, spot1, spot2;
	float range1 = FLT_MAX, range2 = FLT_MAX;

	while ((spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_DEATHMATCH)).has_value())
	{
		count++;
		float range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return nullptr;

	if (count <= 2)
		spot1 = spot2 = nullptr;
	else
		count -= 2;

	int selection = Q_rand_uniform(count);

	spot = nullptr;
	do
	{
		spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_DEATHMATCH);
		if ((spot1.has_value() && spot == spot1) || (spot2.has_value() && spot == spot2))
			selection++;
	} while (selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint
================
*/
entityref SelectFarthestDeathmatchSpawnPoint()
{
	entityref bestspot;
	float bestdistance = 0;
	
	for (entity &spot : G_IterateEquals<&entity::type>(ET_INFO_PLAYER_DEATHMATCH))
	{
		float bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot.has_value())
		return bestspot;

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	return G_FindEquals<&entity::type>(world, ET_INFO_PLAYER_DEATHMATCH);
}

static entityref SelectDeathmatchSpawnPoint()
{
	if (dmflags & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint();
	else
		return SelectRandomDeathmatchSpawnPoint();
}

#ifdef SINGLE_PLAYER
#ifdef GROUND_ZERO
static entityref SelectLavaCoopSpawnPoint()
{
	float lavatop = -FLT_MAX;
	entityref highestlava;

	// first, find the highest lava
	// remember that some will stop moving when they've filled their
	// areas...
	for (entity &lava : G_IterateEquals<&entity::type>(ET_FUNC_DOOR))
	{
		if ((lava.spawnflags & WATER_SMART) && (gi.pointcontents(lava.absbounds.center()) & MASK_WATER))
		{
			if (lava.absbounds.maxs[2] > lavatop)
			{
				lavatop = lava.absbounds.maxs[2];
				highestlava = lava;
			}
		}
	}

	// if we didn't find ANY lava, then return NULL
	if (!highestlava.has_value())
		return null_entity;

	// find the top of the lava and include a small margin of error (plus bbox size)
	lavatop = highestlava->absbounds.maxs[2] + 64;

	// find all the lava spawn points and store them in spawnPoints[]
	dynarray<entityref> spawnPoints;

	for (entity &spot : G_IterateEquals<&entity::type>(ET_INFO_PLAYER_COOP_LAVA))
		spawnPoints.push_back(spot);

	if (spawnPoints.size() < 1)
		return null_entity;

	// walk up the sorted list and return the lowest, open, non-lava spawn point
	float lowest = FLT_MAX;
	entityref pointWithLeastLava;

	for (size_t index = 0; index < spawnPoints.size(); index++)
	{
		if (spawnPoints[index]->origin[2] < lavatop)
			continue;

		if (PlayersRangeFromSpot(spawnPoints[index]) > 32)
		{
			if (spawnPoints[index]->origin[2] < lowest)
			{
				// save the last point
				pointWithLeastLava = spawnPoints[index];
				lowest = spawnPoints[index]->origin[2];
			}
		}
	}

	// FIXME - better solution????
	// well, we may telefrag someone, but oh well...
	if (pointWithLeastLava.has_value())
		return pointWithLeastLava;

	return null_entity;
}
#endif

static entityref SelectCoopSpawnPoint(entity &ent)
{
#ifdef GROUND_ZERO
	// rogue hack, but not too gross...
	if (!stricmp(level.mapname, "rmine2p") || !stricmp(level.mapname, "rmine2"))
		return SelectLavaCoopSpawnPoint();
#endif

	int index = ent.number - 1;
	
	// player 0 starts in normal player spawn point
	if (!index)
		return null_entity;

	// assume there are four coop spots at each spawnpoint
	for (entity &spot : G_IterateEquals<&entity::type>(ET_INFO_PLAYER_COOP))
	{
		if (striequals(game.spawnpoint, spot.targetname))
		{
			// this is a coop spawn point for one of the clients here
			index--;

			if (!index)
				return spot;        // this is it
		}
	}

	// nope
	return null_entity;
}
#endif

#ifdef CTF
entity(entity) SelectCTFSpawnPoint;
#endif

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
static void SelectSpawnPoint(entity &ent [[maybe_unused]], vector &origin, vector &angles)
{
#ifdef SINGLE_PLAYER
	entityref spot = null_entity;

#ifdef CTF
	if (ctf.intVal)
		spot = SelectCTFSpawnPoint(ent);
	else
#endif
	if (deathmatch)
		spot = SelectDeathmatchSpawnPoint();
	else if (coop)
		spot = SelectCoopSpawnPoint(ent);
#else
	entityref spot = SelectDeathmatchSpawnPoint();
#endif

	// find a single player start spot
	if (!spot.has_value())
		while ((spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_START)).has_value())
		{
			if (!game.spawnpoint && !spot->targetname)
				break;
			else if (striequals(game.spawnpoint, spot->targetname))
				break;
		}

	if (!spot.has_value())
		// there wasn't a spawnpoint found yet
		spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_START);
	
	if (!spot.has_value())
	{
		gi.dprintfmt("Couldn't find spawn point \"{}\"\n", game.spawnpoint);
		return;
	}

	origin = spot->origin;
	origin[2] += 9.f;
	angles = spot->angles;
}

void InitBodyQue()
{
	level.body_que = 0;

	for (size_t i = 0; i < BODY_QUEUE_SIZE; i++)
		G_Spawn();
}

static void body_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	if (self.health < -40)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"));
		for (int n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self.origin[2] -= 48.f;
		ThrowClientHead(self, damage);
		self.takedamage = false;
	}
}

REGISTER_STATIC_SAVABLE(body_die);

static void CopyToBodyQue(entity &ent)
{
	gi.unlinkentity(ent);
	
	// grab a body que and cycle to the next one
	entity &body = itoe(game.maxclients + level.body_que + 1);
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;
	
	// send an effect on the removed body
	if (body.modelindex)
		gi.ConstructMessage(svc_temp_entity, TE_BLOOD, body.origin, vecdir { vec3_origin }).multicast(body.origin, MULTICAST_PVS);
	
	gi.unlinkentity(body);

	body.origin = ent.origin;
	body.angles = ent.angles;
	body.old_origin = ent.angles;
	body.modelindex = ent.modelindex;
	body.frame = ent.frame;
	body.skinnum = ent.skinnum;
	body.renderfx = ent.renderfx;
	body.event = EV_OTHER_TELEPORT;
	
	body.svflags = ent.svflags;
	body.bounds = ent.bounds;
	body.absbounds = ent.absbounds;
	body.size = ent.size;
	body.solid = ent.solid;
	body.clipmask = ent.clipmask;
	body.owner = ent.owner;
	body.velocity = ent.velocity;
	body.avelocity = ent.avelocity;
	body.movetype = ent.movetype;
	body.groundentity = ent.groundentity;
	
	body.die = SAVABLE(body_die);
	body.takedamage = true;
	
	gi.linkentity(body);
}

void respawn(entity &self)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch && !coop)
	{
		// restart the entire server
		gi.AddCommandString("pushmenu loadgame\n");
		return;
	}

#endif
	// spectator's don't leave bodies
	if (self.movetype != MOVETYPE_NOCLIP)
		CopyToBodyQue(self);
	self.svflags &= ~SVF_NOCLIENT;
	PutClientInServer(self);

	// add a teleportation effect
	self.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self.client.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self.client.ps.pmove.pm_time = 14;

	self.client.respawn_time = level.time;
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
static void spectator_respawn(entity &ent)
{
	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators
	if (ent.client.pers.spectator)
	{
		string value = Info_ValueForKey(ent.client.pers.userinfo, "spectator");

		if (spectator_password &&
			spectator_password != "none" &&
			spectator_password != value)
		{
			gi.cprint(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent.client.pers.spectator = false;
			gi.ConstructMessage(svc_stufftext, "spectator 0\n").unicast(ent, true);
			return;
		}

		// count spectators
		uint32_t numspec = 0;

		for (entity &e : entity_range(1, game.maxclients))
			if (e.inuse && e.client.pers.spectator)
				numspec++;

		if (numspec >= maxspectators)
		{
			gi.cprint(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent.client.pers.spectator = false;
			gi.ConstructMessage(svc_stufftext, "spectator 0\n").unicast(ent, true);
			return;
		}
	}
	else
	{
		// he was a spectator and wants to join the game
		// he must have the right password
		string value = Info_ValueForKey(ent.client.pers.userinfo, "password");

		if (password &&
			password != "none" &&
			password != value)
		{
			gi.cprint(ent, PRINT_HIGH, "Password incorrect.\n");
			ent.client.pers.spectator = true;
			gi.ConstructMessage(svc_stufftext, "spectator 1\n").unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent.client.resp.score = 0;
#ifdef SINGLE_PLAYER
	ent.client.pers.score = 0;
#endif

	ent.svflags &= ~SVF_NOCLIENT;
	PutClientInServer(ent);

	// add a teleportation effect
	if (!ent.client.pers.spectator)
	{
		// send effect
		gi.ConstructMessage(svc_muzzleflash, ent, MZ_LOGIN).multicast(ent.origin, MULTICAST_PVS);

		// hold in place briefly
		ent.client.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent.client.ps.pmove.pm_time = 14;
	}

	ent.client.respawn_time = level.time;

	if (ent.client.pers.spectator)
		gi.bprintfmt(PRINT_HIGH, "{} has moved to the sidelines\n", ent.client.pers.netname);
	else
		gi.bprintfmt(PRINT_HIGH, "{} joined the game\n", ent.client.pers.netname);
}

//==============================================================

#ifdef CTF
bool(entity) CTFStartClient;
#endif

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(entity &ent)
{
	constexpr bbox player_bounds = {
		.mins = { -16, -16, -24 },
		.maxs = {  16, 16, 32 }
	};

	// clear playerstate
	ent.client.ps = {};

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	vector spawn_origin, spawn_angles;
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);

	// deathmatch wipes most client data every spawn
	client_respawn	resp;
#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		string userinfo = ent.client.pers.userinfo;
		resp = std::move(ent.client.resp);
		
		InitClientPersistant(ent);
		ClientUserinfoChanged(ent, userinfo);
#ifdef SINGLE_PLAYER
	}
	else if (coop)
	{
		string userinfo = ent.client.pers.userinfo;
		
		resp = std::move(ent.client.resp);

		resp.coop_respawn.game_helpchanged = ent.client.pers.game_helpchanged;
		resp.coop_respawn.helpchanged = ent.client.pers.helpchanged;
		ent.client.pers = resp.coop_respawn;
		ClientUserinfoChanged(ent, userinfo);
		if (resp.score > ent.client.pers.score)
			ent.client.pers.score = resp.score;
	}
	else
		resp = {};
#endif

	ent.client.ps = {};
	
	// clear everything but the persistant data
	client_persistant saved = std::move(ent.client.pers);
	ent.client = {};
	ent.client.pers = std::move(saved);
#ifdef SINGLE_PLAYER
	if (ent.client.pers.health <= 0)
		InitClientPersistant(ent);
#endif
	ent.client.resp = std::move(resp);

#ifdef SINGLE_PLAYER
	// copy some data from the client to the entity
	FetchClientEntData(ent);
#endif

	// clear entity values
	ent.groundentity = null_entity;
	ent.takedamage = true;
	ent.movetype = MOVETYPE_WALK;
	ent.viewheight = 22;
	ent.inuse = true;
	ent.mass = 200;
	ent.solid = SOLID_BBOX;
	ent.deadflag = false;
	ent.air_finished_time = level.time + 12s;
	ent.clipmask = MASK_PLAYERSOLID;
	ent.die = SAVABLE(player_die);
	ent.waterlevel = WATER_NONE;
	ent.watertype = CONTENTS_NONE;
	ent.flags &= ~FL_NO_KNOCKBACK;
	ent.svflags &= ~SVF_DEADMONSTER;
	ent.gib_health = -40;

	ent.bounds = player_bounds;
	ent.velocity = vec3_origin;

	ent.client.ps.pmove.set_origin(spawn_origin);
	ent.client.ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	ent.client.ps.fov = clamp(1.f, (float) atof(Info_ValueForKey(ent.client.pers.userinfo, "fov")), 160.f);

	// clear entity state values
	ent.effects = EF_NONE;
	ent.modelindex = MODEL_PLAYER;        // will use the skin specified model
	ent.modelindex2 = MODEL_PLAYER;       // custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent.skinnum = ent.number - 1;

	ent.frame = 0;
	ent.origin = spawn_origin;
	ent.origin[2] += 1;  // make sure off ground
	ent.old_origin = ent.origin;

	// set the delta angle
	ent.client.ps.pmove.set_delta_angles(spawn_angles - ent.client.resp.cmd_angles);

	ent.angles[PITCH] = 0;
	ent.angles[YAW] = spawn_angles[YAW];
	ent.angles[ROLL] = 0;
	ent.client.ps.viewangles = ent.angles;
	ent.client.v_angle = ent.angles;

	// spawn a spectator
	if (ent.client.pers.spectator)
	{
		ent.client.chase_target = null_entity;
		ent.client.resp.spectator = true;

		ent.movetype = MOVETYPE_NOCLIP;
		ent.solid = SOLID_NOT;
		ent.svflags |= SVF_NOCLIENT;
		ent.client.ps.gunindex = MODEL_NONE;
		gi.linkentity(ent);
		return;
	}
	else
		ent.client.resp.spectator = false;

#ifdef CTF
	if (CTFStartClient(ent))
		return;
#endif

	if (!KillBox(ent)) {
		// could't spawn in?
	}

	gi.linkentity(ent);

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
	// my tribute to cash's level-specific hacks. I hope I live
	// up to his trailblazing cheese.
	if (!deathmatch && level.mapname == "rboss")
	{
		// if you get on to rboss in single player or coop, ensure
		// the player has the nuke key. (not in DM)
		ent.client.pers.selected_item = ITEM_ANTIMATTER_BOMB;
		ent.client.pers.inventory[ITEM_ANTIMATTER_BOMB] = 1;
	}
#endif

	// force the current weapon up
	ent.client.newweapon = ent.client.pers.weapon;
	ChangeWeapon(ent);
}

void ClientBegin(entity &ent)
{
#ifdef SINGLE_PLAYER
	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (!deathmatch && ent.inuse)
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		ent.client.ps.pmove.set_delta_angles(ent.client.ps.viewangles);
	else
	{
#endif
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		G_InitEdict(ent);
		InitClientResp(ent);
		PutClientInServer(ent);
#ifdef SINGLE_PLAYER
	}
#endif

	if (level.intermission_time != gtime::zero())
		MoveClientToIntermission(ent);
	else if (game.maxclients > 1)
	{
		gi.ConstructMessage(svc_muzzleflash, ent, MZ_LOGIN).multicast(ent.origin, MULTICAST_PVS);
		gi.bprintfmt(PRINT_HIGH, "{} entered the game\n", ent.client.pers.netname);
	}

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

#ifdef CTF
void(entity, string) CTFAssignSkin;
#endif

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged(entity &ent, string userinfo)
{
	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
		userinfo = "\\name\\badinfo\\skin\\male/grunt";
	
	// set name
	string str = Info_ValueForKey(userinfo, "name");
	ent.client.pers.netname = str;
	
	// set spectator
	str = Info_ValueForKey(userinfo, "spectator");

	// spectators are only supported in deathmatch
	if (
#ifdef SINGLE_PLAYER
		deathmatch &&
#endif
		!strempty(str) && str != "0")
		ent.client.pers.spectator = true;
	else
		ent.client.pers.spectator = false;
	
	// set skin
	str = Info_ValueForKey(userinfo, "skin");

	// combine name and skin into a configstring
#ifdef CTF
	// set player name field (used in id_state view)
	gi.configstring (CS_GENERAL + ent.number - 1, ent.client.pers.netname);

	if (ctf.intVal)
		CTFAssignSkin(ent, str);
	else
#endif
		gi.configstringfmt((config_string)(CS_PLAYERSKINS + ent.number - 1), "{}\\{}", ent.client.pers.netname, str);
	
	// fov
	ent.client.ps.fov = clamp(1.f, (float)atof(Info_ValueForKey(userinfo, "fov")), 160.f);
	
	// handedness
	str = Info_ValueForKey(userinfo, "hand");
	if (!strempty(str))
		ent.client.pers.hand = clamp(RIGHT_HANDED, (handedness)atoi(str), CENTER_HANDED);
	
	// save off the userinfo in case we want to check something later
	ent.client.pers.userinfo = userinfo;
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool ClientConnect(entity &ent, string &userinfo)
{
	// check to see if they are on the banned IP list
	/*string value = Info_ValueForKey(userinfo, "ip");
	
	if (SV_FilterPacket(value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}*/

	// check for a spectator
	string value = Info_ValueForKey(userinfo, "spectator");

	if (
#ifdef SINGLE_PLAYER
		deathmatch &&
#endif
		value == "1")
	{
		if (spectator_password &&
			spectator_password != "none" &&
			spectator_password != value)
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		uint32_t numspec = 0;
		
		// count spectators
		for (entity &e : entity_range(1, game.maxclients))
			if (e.inuse && e.client.pers.spectator)
				numspec++;

		if (numspec >= maxspectators)
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	}
	else
	{
		// check for a password
		value = Info_ValueForKey(userinfo, "password");

		if (password &&
			password != "none" &&
			password != value)
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}

#ifdef CTF
	if (ctf.intVal)
	{
		// force team join
		ent.client.resp.ctf_team = -1;
		ent.client.resp.id_state = true;
	}
#endif

#ifdef SINGLE_PLAYER
	// they can connect
	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent.inuse == false)
	{
		// clear the respawning variables
		InitClientResp(ent);

		if (!game.autosaved || !ent.client.pers.weapon)
			InitClientPersistant(ent);
	}
#else
	InitClientResp(ent);
	InitClientPersistant(ent);
#endif

	ClientUserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintfmt("{} connected\n", ent.client.pers.netname);

	ent.svflags = SVF_NONE; // make sure we start with known default
	ent.client.pers.connected = true;

	return true;
};

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(entity &ent)
{
	gi.bprintfmt(PRINT_HIGH, "{} disconnected\n", ent.client.pers.netname);

#ifdef CTF
	CTFDeadDropFlag(ent);
	CTFDeadDropTech(ent);
#endif
	
	// send effect
	if (ent.inuse)
		gi.ConstructMessage(svc_muzzleflash, ent, MZ_LOGOUT).multicast(ent.origin, MULTICAST_PVS);
	
	gi.unlinkentity(ent);
	ent.modelindex = MODEL_NONE;
	ent.sound = SOUND_NONE;
	ent.event = EV_NONE;
	ent.effects = EF_NONE;
	ent.solid = SOLID_NOT;
	ent.inuse = false;
	ent.client.pers.connected = false;
}

#ifdef CTF
void(entity) CTFApplyRegeneration;
#endif

void ClientThink(entity &ent, const usercmd &ucmd)
{
	level.current_entity = ent;

	if (level.intermission_time != gtime::zero())
	{
		ent.client.ps.pmove.pm_type = PM_FREEZE;

		if ((level.time > level.intermission_time + 5s) && (ucmd.buttons & BUTTON_ANY))
			level.exitintermission = true;

		return;
	}

	if (ent.client.chase_target.has_value())
		ent.client.resp.cmd_angles = ucmd.get_angles();
	else
	{
		static entityref passent;
		static content_flags mask;

		passent = ent;
		mask = (ent.health > 0) ? MASK_PLAYERSOLID : MASK_DEADSOLID;
		
		// set up for pmove
		if (ent.movetype == MOVETYPE_NOCLIP)
			ent.client.ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent.modelindex != MODEL_PLAYER)
			ent.client.ps.pmove.pm_type = PM_GIB;
		else if (ent.deadflag)
			ent.client.ps.pmove.pm_type = PM_DEAD;
		else
			ent.client.ps.pmove.pm_type = PM_NORMAL;

#ifdef GROUND_ZERO
		ent.client.ps.pmove.gravity = (int16_t)sv_gravity * ent.gravity;
#else
		ent.client.ps.pmove.gravity = (int16_t)sv_gravity;
#endif
		pmove pm = pmove(ent.client.ps.pmove);

		pm.trace = [](auto start, auto mins, auto maxs, auto end) { return gi.trace(start, { mins, maxs }, end, passent, mask); };
		pm.pointcontents = [](auto point) { return gi.pointcontents(point); };

		pm.set_origin(ent.origin);
		pm.set_velocity(ent.velocity);

		pm.snapinitial = !!memcmp(&ent.client.old_pmove, &pm, sizeof(pmove_state));

		pm.cmd = ucmd;

		// perform a pmove
#ifdef QUAKEC_PMOVE
		Pmove(&pm);
#else
		gi.Pmove(pm);
#endif

		// save results of pmove
		ent.client.old_pmove = ent.client.ps.pmove = pmove_state(pm);

		ent.origin = pm.get_origin();
		ent.velocity = pm.get_velocity();

		ent.bounds = pm.bounds;

		ent.client.resp.cmd_angles = ucmd.get_angles();

		if (ent.groundentity.has_value() && !pm.groundentity.has_value() && (pm.cmd.upmove >= 10) && (pm.waterlevel == WATER_NONE))
#ifdef SINGLE_PLAYER
		{
#endif
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"));
#ifdef SINGLE_PLAYER
			PlayerNoise(ent, ent.origin, PNOISE_SELF);
		}
#endif

		ent.viewheight = pm.viewheight;
		ent.waterlevel = pm.waterlevel;
		ent.watertype = pm.watertype;
		ent.groundentity = pm.groundentity;
		if (pm.groundentity.has_value())
			ent.groundentity_linkcount = pm.groundentity->linkcount;

		if (ent.deadflag)
		{
			ent.client.ps.viewangles[ROLL] = 40.f;
			ent.client.ps.viewangles[PITCH] = -15.f;
			ent.client.ps.viewangles[YAW] = ent.client.killer_yaw;
		}
		else
			ent.client.ps.viewangles = ent.client.v_angle = pm.viewangles;

#ifdef HOOK_CODE
		if (ent.client.grapple.has_value())
			GrapplePull(ent.client.grapple);
#endif

		gi.linkentity(ent);

		if (ent.movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers(ent);

#ifdef GROUND_ZERO
		ent.gravity = 1.0;
#endif

		// touch other objects
		for (auto &other : pm.touchents)
		{
			if (!other.has_value())
				break;

			if (other->touch)
				other->touch(other, ent, vec3_origin, null_surface);
		}
	}

	ent.client.oldbuttons = ent.client.buttons;
	ent.client.buttons = ucmd.buttons;
	ent.client.latched_buttons |= ent.client.buttons & ~ent.client.oldbuttons;

	// fire weapon from final position if needed
	if ((ent.client.latched_buttons & BUTTON_ATTACK)
#ifdef CTF
		&& ent.movetype != MOVETYPE_NOCLIP
#endif
		)
	{
		if (ent.client.resp.spectator)
		{
			ent.client.latched_buttons = BUTTON_NONE;

			if (ent.client.chase_target.has_value())
			{
				ent.client.chase_target = nullptr;
				ent.client.ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				GetChaseTarget(ent);
		}
		else if (!ent.client.weapon_thunk)
		{
			ent.client.weapon_thunk = true;
			Think_Weapon(ent);
		}
	}

#ifdef CTF
	CTFApplyRegeneration(ent);
#endif

	if (ent.client.resp.spectator)
	{
		if (ucmd.upmove >= 10)
		{
			if (!(ent.client.ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				ent.client.ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (ent.client.chase_target.has_value())
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else
			ent.client.ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (entity &other : entity_range(1, game.maxclients))
		if (other.inuse && other.client.chase_target == ent)
			UpdateChaseCam(other);
};

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(entity &ent)
{
	if (level.intermission_time != gtime::zero())
		return;

	if (
#ifdef SINGLE_PLAYER
		deathmatch &&
#endif
		ent.client.pers.spectator != ent.client.resp.spectator &&
		(level.time - ent.client.respawn_time) >= 5s)
	{
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!ent.client.weapon_thunk && !ent.client.resp.spectator
#ifdef CTF
		&& ent->movetype != MOVETYPE_NOCLIP
#endif
		)
		Think_Weapon(ent);
	else
		ent.client.weapon_thunk = false;

	if (ent.deadflag)
	{
		// wait for any button just going down
		if (level.time > ent.client.respawn_time)
		{
#ifdef SINGLE_PLAYER
			button_bits buttonMask;

			// in deathmatch, only wait for attack button
			if (deathmatch)
				buttonMask = BUTTON_ATTACK;
			else
				buttonMask = BUTTON_ANY;
#else
			constexpr button_bits buttonMask = BUTTON_ATTACK;

#endif
			if ((ent.client.latched_buttons & buttonMask) || (
#ifdef SINGLE_PLAYER
				deathmatch && 
#endif
				(dmflags & DF_FORCE_RESPAWN)))
			{
				respawn(ent);
				ent.client.latched_buttons = BUTTON_NONE;
			}
		}

		return;
	}
#ifdef SINGLE_PLAYER

	// add player trail so monsters can follow
	if (!deathmatch)
		if (!visible(ent, PlayerTrail_LastSpot()))
			PlayerTrail_Add(ent.old_origin);
#endif

	ent.client.latched_buttons = BUTTON_NONE;
}
