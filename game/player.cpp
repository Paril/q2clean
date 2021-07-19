#include "../lib/types.h"
#include "entity.h"
#include "../lib/info.h"
#include "combat.h"
#include "game.h"
#include "player.h"
#include "chase.h"
#include "cmds.h"
#include "misc.h"
#include "spawn.h"
#include "m_player.h"

import util;
import game_locals;
import hud;
import protocol;
import gi;
import math.random;
import string.format;
import player.view;
#ifdef SINGLE_PLAYER
import player.trail;
#endif
import weaponry;

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
	entityref spot = world;

	while ((spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_START)).has_value())
	{
		if (!spot->targetname)
			continue;
		vector d = self.s.origin - spot->s.origin;
		if (VectorLength(d) < 384)
		{
			if (!striequals(self.targetname, spot->targetname))
				self.targetname = spot->targetname;
			return;
		}
	}
}

REGISTER_SAVABLE_FUNCTION(SP_FixCoopSpots);

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
			spot.s.origin[0] = 188.f - 64.f;
			spot.s.origin[1] = -164.f;
			spot.s.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.s.angles[1] = 90.f;
		}

		{
			entity &spot = G_Spawn();
			spot.type = ET_INFO_PLAYER_COOP;
			spot.s.origin[0] = 188.f + 64.f;
			spot.s.origin[1] = -164.f;
			spot.s.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.s.angles[1] = 90.f;
		}

		{
			entity &spot = G_Spawn();
			spot.type = ET_INFO_PLAYER_COOP;
			spot.s.origin[0] = 188.f + 128.f;
			spot.s.origin[1] = -164.f;
			spot.s.origin[2] = 80.f;
			spot.targetname = "jail3";
			spot.s.angles[1] = 90.f;
		}
	}
}

REGISTER_SAVABLE_FUNCTION(SP_CreateCoopSpots);

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
		self.think = SP_CreateCoopSpots_savable;
		self.nextthink = level.framenum + 1;
	}
#endif
}

REGISTER_ENTITY(info_player_start, ET_INFO_PLAYER_START);

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

REGISTER_ENTITY(info_player_deathmatch, ET_INFO_PLAYER_DEATHMATCH);

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
		self.think = SP_FixCoopSpots_savable;
		self.nextthink = level.framenum + 1;
	}
}

REGISTER_ENTITY(info_player_coop, ET_INFO_PLAYER_COOP);

#endif
/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
static void SP_info_player_intermission(entity &)
{
}

REGISTER_ENTITY(info_player_intermission, ET_INFO_PLAYER_INTERMISSION);

#if defined(GROUND_ZERO) && defined(SINGLE_PLAYER)
/*QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games on rmine2 where lava level
needs to be checked
*/
API_FUNC static void(entity self) SP_info_player_coop_lava =
{
	if (!coop.intVal)
	{
		G_FreeEdict (self);
		return;
	}
}

#endif
//=======================================================================

static void player_pain(entity &, entity &, float, int)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}

REGISTER_SAVABLE_FUNCTION(player_pain);

enum gender_id : uint8_t
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTRAL
};

static gender_id GetGender(entity &ent)
{
	if (!ent.is_client())
		return GENDER_MALE;
	
	string info = Info_ValueForKey(ent.client->pers.userinfo, "gender");

	if (info[0] == 'f' || info[0] == 'F')
		return GENDER_FEMALE;
	else if (info[0] != 'm' && info[0] != 'm')
		return GENDER_NEUTRAL;

	return GENDER_MALE;
}

static void ClientObituary(entity &self, entity &attacker) 
{
	if (OnSameTeam(self, attacker))
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	int ff = meansOfDeath & MOD_FRIENDLY_FIRE;
	int mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
	stringref message;
	stringref message2;
	gender_id gender = GetGender(self);

	static stringlit their[] = {
		"his",
		"her",
		"their"
	};

	static stringlit themself[] = {
		"himself",
		"herself",
		"themself"
	};

	switch (mod)
	{
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_EXPLOSIVE:
	case MOD_BARREL:
		message = "blew up";
		break;
	case MOD_EXIT:
		message = "found a way out";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TARGET_BLASTER:
		message = "got blasted";
		break;
	case MOD_BOMB:
	case MOD_SPLASH:
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
#ifdef THE_RECKONING
	case MOD_GEKK:
	case MOD_BRAINTENTACLE:
		message = "... that's gotta hurt!";
		break;
#endif
	}

	if (attacker == self)
	{
		switch (mod)
		{
		case MOD_HELD_GRENADE:
			message = "tried to put the pin back in";
			break;
		case MOD_HG_SPLASH:
		case MOD_G_SPLASH:
			message = va("tripped on %s own grenade", their[gender]);
			break;
		case MOD_R_SPLASH:
			message = va("blew %s up", themself[gender]);
			break;
		case MOD_BFG_BLAST:
			message = "should have used a smaller gun";
			break;
#ifdef THE_RECKONING
		case MOD_TRAP:
			message = va("was sucked into %s own trap", their[gender]);
			break;
#endif
		default:
			message = va("killed %s", themself[gender]);
			break;
		}
	}

	if (message)
	{
		gi.bprintf(PRINT_MEDIUM, "%s %s.\n", self.client->pers.netname.ptr(), message.ptr());
#ifdef SINGLE_PLAYER
		if (deathmatch)
#endif
			self.client->resp.score--;
		self.enemy = nullptr;
		return;
	}

	self.enemy = attacker;
	if (attacker.is_client())
	{
		switch (mod) {
		case MOD_BLASTER:
			message = "was blasted by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_SSHOTGUN:
			message = "was blown away by";
			message2 = "'s super shotgun";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_CHAINGUN:
			message = "was cut in half by";
			message2 = "'s chaingun";
			break;
		case MOD_GRENADE:
			message = "was popped by";
			message2 = "'s grenade";
			break;
		case MOD_G_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_R_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_HYPERBLASTER:
			message = "was melted by";
			message2 = "'s hyperblaster";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_BFG_LASER:
			message = "saw the pretty lights from";
			message2 = "'s BFG";
			break;
		case MOD_BFG_BLAST:
			message = "was disintegrated by";
			message2 = "'s BFG blast";
			break;
		case MOD_BFG_EFFECT:
			message = "couldn't hide from";
			message2 = "'s BFG";
			break;
		case MOD_HANDGRENADE:
			message = "caught";
			message2 = "'s handgrenade";
			break;
		case MOD_HG_SPLASH:
			message = "didn't see";
			message2 = "'s handgrenade";
			break;
		case MOD_HELD_GRENADE:
			message = "feels";
			message2 = "'s pain";
			break;
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
#ifdef THE_RECKONING
		case MOD_RIPPER:
			message = "was ripped to shreds by";
			message2 = "'s ripper gun";
			break;
		case MOD_PHALANX:
			message = "was evaporated by";
			break;
		case MOD_TRAP:
			message = "was caught in";
			message2 = "'s trap";
			break;
#endif
#ifdef GROUND_ZERO
		case MOD_CHAINFIST:
			message = "was shredded by";
			message2 = "'s ripsaw";
			break;
		case MOD_DISINTEGRATOR:
			message = "lost his grip courtesy of";
			message2 = "'s disintegrator";
			break;
		case MOD_ETF_RIFLE:
			message = "was perforated by";
			break;
		case MOD_HEATBEAM:
			message = "was scorched by";
			message2 = "'s plasma beam";
			break;
		case MOD_TESLA:
			message = "was enlightened by";
			message2 = "'s tesla mine";
			break;
		case MOD_PROX:
			message = "got too close to";
			message2 = "'s proximity mine";
			break;
		case MOD_NUKE:
			message = "was nuked by";
			message2 = "'s antimatter bomb";
			break;
		case MOD_TRACKER:
			message = "was annihilated by";
			message2 = "'s disruptor";
			break;
#endif
#ifdef HOOK_CODE
		case MOD_GRAPPLE:
			message = "was caught by";
			message2 = "'s grapple";
			break;
#endif
		}

		if (message)
		{
			gi.bprintf(PRINT_MEDIUM, "%s %s %s%s\n", self.client->pers.netname.ptr(), message.ptr(), attacker.client->pers.netname.ptr(), message2.ptr());
#ifdef SINGLE_PLAYER

			if (deathmatch)
			{
#endif
				if (ff)
					attacker.client->resp.score--;
				else
					attacker.client->resp.score++;
#ifdef SINGLE_PLAYER
			}
#endif
			return;
		}
	}
}

static void TossClientWeapon(entity &self)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch)
		return;

#endif
	itemref it = self.client->pers.weapon;

	if (!self.client->pers.inventory[self.client->ammo_index] || it->id == ITEM_BLASTER
#ifdef CTF
		|| it->id == ITEM_GRAPPLE
#endif
		)
		it = nullptr;

	const bool quad = ((dm_flags)dmflags & DF_QUAD_DROP) && (self.client->quad_framenum > (level.framenum + 10));
		
#ifdef THE_RECKONING
	const bool quadfire = (dmflags.intVal & DF_QUADFIRE_DROP) && (self.client.quadfire_framenum > (level.framenum + 10));
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
		self.client->v_angle[YAW] -= spread;
		entity &drop = Drop_Item(self, it);
		self.client->v_angle[YAW] += spread;
		drop.spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self.client->v_angle[YAW] += spread;
		entity &drop = Drop_Item(self, GetItemByIndex(ITEM_QUAD_DAMAGE));
		self.client->v_angle[YAW] -= spread;
		drop.spawnflags |= DROPPED_PLAYER_ITEM;

		drop.touch = Touch_Item_savable;
		drop.nextthink = self.client->quad_framenum;
		drop.think = G_FreeEdict_savable;
	}
	
#ifdef THE_RECKONING
	if (quadfire)
	{
		self.client.v_angle[YAW] += spread;
		entity drop = Drop_Item (self, FindItemByClassname ("item_quadfire"));
		self.client.v_angle[YAW] -= spread;
		drop.spawnflags |= DROPPED_PLAYER_ITEM;

		drop.touch = Touch_Item;
		drop.nextthink = self.client.quadfire_framenum;
		drop.think = G_FreeEdict;
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
		self.client->killer_yaw = vectoyaw(attacker.s.origin - self.s.origin);
	else if (!inflictor.is_world() && inflictor != self)
		self.client->killer_yaw = vectoyaw(inflictor.s.origin - self.s.origin);
	else
		self.client->killer_yaw = self.s.angles[YAW];
}

#ifdef HOOK_CODE
import ctf.grapple;
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

	self.s.modelindex2 = MODEL_NONE; // remove linked weapon model
#ifdef CTF
	self.s.modelindex3 = MODEL_NONE; // remove linked flag
#endif

	self.s.angles[PITCH] = 0;
	self.s.angles[ROLL] = 0;

	self.s.sound = SOUND_NONE;
	self.client->weapon_sound = SOUND_NONE;

	self.maxs.z = -8.f;

	self.svflags |= SVF_DEADMONSTER;

	if (!self.deadflag)
	{
		self.client->respawn_framenum = (gtime)(level.framenum + 1.0f * BASE_FRAMERATE);
		LookAtKiller(self, inflictor, attacker);
		self.client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, attacker);

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
				self.client->resp.coop_respawn.inventory[it.id] = self.client->pers.inventory[it.id];
#endif
			self.client->pers.inventory[it.id] = 0;
#ifdef SINGLE_PLAYER
		}
#endif
	}

	// remove powerups
	self.client->quad_framenum = 0;
	self.client->invincible_framenum = 0;
	self.client->breather_framenum = 0;
	self.client->enviro_framenum = 0;
#ifdef THE_RECKONING
	self.client.quadfire_framenum = 0;
#endif
	self.flags &= ~FL_POWER_ARMOR;
	
#ifdef GROUND_ZERO
	self.client.double_framenum = 0;

	// if we've been killed by the tracker, GIB!
	if ((meansOfDeath & ~MOD_FRIENDLY_FIRE) == MOD_TRACKER)
	{
		self.health = -100;
		damage = 400;
	}
	
	// if we got obliterated by the nuke, don't gib
	if ((self.health < -80) && (meansOfDeath == MOD_NUKE))
		self.flags |= FL_NOGIB;
#endif

	if (self.health < -40)
	{
		// gib
#ifdef GROUND_ZERO
		if (!(self.flags & FL_NOGIB))
		{
#endif
			gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
			for (int n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
#ifdef GROUND_ZERO
		}
		self.flags &= ~FL_NOGIB;
#endif

		ThrowClientHead(self, damage);
		self.client->anim_priority = ANIM_DEATH;
		self.client->anim_end = 0;
		self.takedamage = false;
	}
	else if (!self.deadflag) // normal death
	{
		static int i;

		i = (i + 1) % 3;

		// start a death animation
		self.client->anim_priority = ANIM_DEATH;
		if (self.client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			self.s.frame = FRAME_crdeath1 - 1;
			self.client->anim_end = FRAME_crdeath5;
		}
		else switch (i)
		{
		case 0:
			self.s.frame = FRAME_death101 - 1;
			self.client->anim_end = FRAME_death106;
			break;
		case 1:
			self.s.frame = FRAME_death201 - 1;
			self.client->anim_end = FRAME_death206;
			break;
		case 2:
			self.s.frame = FRAME_death301 - 1;
			self.client->anim_end = FRAME_death308;
			break;
		}

		gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (Q_rand() % 4) + 1)), 1, ATTN_NORM, 0);
	}

	self.deadflag = DEAD_DEAD;

	gi.linkentity(self);
}

REGISTER_SAVABLE_FUNCTION(player_die);

//=======================================================================

static inline void SetAmmoMax(const entity &other, ammo_id ammo, int32_t new_max)
{
	other.client->pers.max_ammo[ammo] = new_max;
}

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
	ent.client->pers = {};

	ent.client->pers.selected_item = ITEM_BLASTER;
	ent.client->pers.inventory[ent.client->pers.selected_item] = 1;
	
	ent.client->pers.weapon = ent.client->pers.lastweapon = GetItemByIndex(ent.client->pers.selected_item);

#ifdef GRAPPLE
	ent.client->pers.inventory[ITEM_GRAPPLE] = 1;
#endif
	
#ifdef SINGLE_PLAYER
	ent.client->pers.health         = 100;
	ent.client->pers.max_health     = 100;
#else
	ent.health         = 100;
	ent.max_health     = 100;
#endif

	SetAmmoMax(ent, AMMO_BULLETS, 200);
	SetAmmoMax(ent, AMMO_SHELLS, 100);
	SetAmmoMax(ent, AMMO_ROCKETS, 50);
	SetAmmoMax(ent, AMMO_GRENADES, 50);
	SetAmmoMax(ent, AMMO_CELLS, 200);
	SetAmmoMax(ent, AMMO_SLUGS, 50);
	
#ifdef THE_RECKONING
	SetAmmoMax(ent, AMMO_MAGSLUG, 50);
	SetAmmoMax(ent, AMMO_TRAP, 5);
#endif

#ifdef GROUND_ZERO
	SetAmmoMax(ent, AMMO_PROX, 50);
	SetAmmoMax(ent, AMMO_TESLA, 50);
	SetAmmoMax(ent, AMMO_FLECHETTES, 200);
	SetAmmoMax(ent, AMMO_DISRUPTOR, 100);
#endif
	
	ent.client->pers.connected = true;
}

static void InitClientResp(entity &ent)
{
#ifdef CTF
	ctfteam_t ctf_team = ent.client.resp.ctf_team;
	bool id_state = ent.client.resp.id_state;
#endif

	ent.client->resp = {};

#ifdef CTF
	ent.client.resp.ctf_team = ctf_team;
	ent.client.resp.id_state = id_state;
#endif

	ent.client->resp.enterframe = level.framenum;
#ifdef SINGLE_PLAYER
	ent.client->resp.coop_respawn = ent.client->pers;
#endif

#ifdef CTF
	if (ctf.intVal && ent.client.resp.ctf_team < CTF_TEAM1)
		CTFAssignTeam(ent);
#endif
}

#ifdef SINGLE_PLAYER
void SaveClientData()
{
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		entity &ent = itoe(1 + i);

		if (!ent.inuse)
			continue;

		ent.client->pers.health = ent.health;
		ent.client->pers.max_health = ent.max_health;
		ent.client->pers.savedFlags = (ent.flags & (FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR));
		if (coop)
			ent.client->pers.score = ent.client->resp.score;
	}
}

static void FetchClientEntData(entity &ent)
{
	ent.health = ent.client->pers.health;
	ent.max_health = ent.client->pers.max_health;
	ent.flags |= ent.client->pers.savedFlags;
	if (coop)
		ent.client->resp.score = ent.client->pers.score;
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

	for (uint32_t n = 1; n <= game.maxclients; n++)
	{
		entity &player = itoe(n);

		if (!player.inuse)
			continue;

		if (player.health <= 0)
			continue;

		bestplayerdistance = min(VectorDistance(spot.s.origin, player.s.origin), bestplayerdistance);
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
	entityref spot;
	entityref bestspot;
	float bestdistance = 0;
	
	while ((spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_DEATHMATCH)).has_value())
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
	if ((dm_flags)dmflags & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint();
	else
		return SelectRandomDeathmatchSpawnPoint();
}

#ifdef SINGLE_PLAYER
#ifdef GROUND_ZERO
static entity(entity ent) SelectLavaCoopSpawnPoint =
{
	float lavatop = -FLT_MAX;
	entity highestlava = world;

	// first, find the highest lava
	// remember that some will stop moving when they've filled their
	// areas...
	entity lava = world;
	while ((lava = G_Find (lava, classname, "func_door")))
	{
		vector center = (lava.absmax + lava.absmin) * 0.5f;

		if (lava.spawnflags & 2 && (gi.pointcontents(center) & MASK_WATER))
		{
			if (lava.absmax[2] > lavatop)
			{
				lavatop = lava.absmax[2];
				highestlava = lava;
			}
		}
	}

	// if we didn't find ANY lava, then return NULL
	if !(highestlava)
		return world;

	// find the top of the lava and include a small margin of error (plus bbox size)
	lavatop = highestlava.absmax[2] + 64;

	// find all the lava spawn points and store them in spawnPoints[]
	entity spot = world;
	int numPoints = 0;
	entity spawnPoints[64] = { 0 };

	while ((spot = G_Find (spot, classname, "info_player_coop_lava")))
	{
		if (numPoints == 64)
			break;

		spawnPoints[numPoints++] = spot;
	}

	if (numPoints < 1)
		return world;

	// walk up the sorted list and return the lowest, open, non-lava spawn point
	spot = world;
	float lowest = FLT_MAX;
	entity pointWithLeastLava = world;

	for (int index = 0; index < numPoints; index++)
	{
		if (spawnPoints[index].s.origin[2] < lavatop)
			continue;

		if (PlayersRangeFromSpot(spawnPoints[index]) > 32)
		{
			if (spawnPoints[index].s.origin[2] < lowest)
			{
				// save the last point
				pointWithLeastLava = spawnPoints[index];
				lowest = spawnPoints[index].s.origin[2];
			}
		}
	}

	// FIXME - better solution????
	// well, we may telefrag someone, but oh well...
	if (pointWithLeastLava)
		return pointWithLeastLava;

	return world;
}
#endif

static entityref SelectCoopSpawnPoint(entity &ent)
{
#ifdef GROUND_ZERO
	// rogue hack, but not too gross...
	if (!stricmp(level.mapname, "rmine2p") || !stricmp(level.mapname, "rmine2"))
		return SelectLavaCoopSpawnPoint (ent);
#endif

	int index = ent.s.number - 1;
	
	// player 0 starts in normal player spawn point
	if (!index)
		return null_entity;

	entityref spot = world;

	// assume there are four coop spots at each spawnpoint
	while ((spot = G_FindEquals<&entity::type>(spot, ET_INFO_PLAYER_COOP)).has_value())
	{
		if (stricmp(game.spawnpoint, spot->targetname) == 0)
		{
			// this is a coop spawn point for one of the clients here
			index--;
			if (!index)
				return spot;        // this is it
		}
	}


	return spot;
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
		gi.dprintf("Couldn't find spawn point \"%s\"\n", game.spawnpoint.ptr());
		return;
	}

	origin = spot->s.origin;
	origin[2] += 9.f;
	angles = spot->s.angles;
}

void InitBodyQue()
{
	level.body_que = 0;

	for (int32_t i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		entity &ent = G_Spawn();
		ent.type = ET_BODYQUEUE;
	}
}

static void body_die(entity &self, entity &, entity &, int32_t damage, vector)
{
	if (self.health < -40)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (int n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self.s.origin[2] -= 48.f;
		ThrowClientHead(self, damage);
		self.takedamage = false;
	}
}

REGISTER_SAVABLE_FUNCTION(body_die);

static void CopyToBodyQue(entity &ent)
{
	gi.unlinkentity(ent);
	
	// grab a body que and cycle to the next one
	entity &body = itoe(game.maxclients + level.body_que + 1);
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;
	
	// send an effect on the removed body
	if (body.s.modelindex)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BLOOD);
		gi.WritePosition(body.s.origin);
		gi.WriteDir(vec3_origin);
		gi.multicast(body.s.origin, MULTICAST_PVS);
	}
	
	gi.unlinkentity(body);
	uint32_t num = body.s.number;
	memcpy(&body.s, &ent.s, sizeof(ent.s));
	body.s.number = num;
	body.s.event = EV_OTHER_TELEPORT;
	
	body.svflags = ent.svflags;
	body.mins = ent.mins;
	body.maxs = ent.maxs;
	body.absmin = ent.absmin;
	body.absmax = ent.absmax;
	body.size = ent.size;
	body.solid = ent.solid;
	body.clipmask = ent.clipmask;
	body.owner = ent.owner;
	body.velocity = ent.velocity;
	body.avelocity = ent.avelocity;
	body.movetype = ent.movetype;
	body.groundentity = ent.groundentity;
	
	body.die = body_die_savable;
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
	self.s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self.client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self.client->ps.pmove.pm_time = 14;

	self.client->respawn_framenum = level.framenum;
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
static void spectator_respawn(entity &ent)
{
	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators
	if (ent.client->pers.spectator)
	{
		string value = Info_ValueForKey(ent.client->pers.userinfo, "spectator");

		if (spectator_password &&
			spectator_password != "none" &&
			spectator_password != value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent.client->pers.spectator = false;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		uint32_t i, numspec;

		for (i = 1, numspec = 0; i <= game.maxclients; i++)
			if (itoe(i).inuse && itoe(i).client->pers.spectator)
				numspec++;

		if (numspec >= (uint32_t)maxspectators)
		{
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent.client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	}
	else
	{
		// he was a spectator and wants to join the game
		// he must have the right password
		string value = Info_ValueForKey(ent.client->pers.userinfo, "password");

		if (password &&
			password != "none" &&
			password != value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent.client->pers.spectator = true;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent.client->resp.score = 0;
#ifdef SINGLE_PLAYER
	ent.client->pers.score = 0;
#endif

	ent.svflags &= ~SVF_NOCLIENT;
	PutClientInServer(ent);

	// add a teleportation effect
	if (!ent.client->pers.spectator)
	{
		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort((int16_t)ent.s.number);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent.s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent.client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent.client->ps.pmove.pm_time = 14;
	}

	ent.client->respawn_framenum = level.framenum;

	if (ent.client->pers.spectator)
		gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n", ent.client->pers.netname.ptr());
	else
		gi.bprintf(PRINT_HIGH, "%s joined the game\n", ent.client->pers.netname.ptr());
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
	constexpr vector player_mins = { -16, -16, -24 };
	constexpr vector player_maxs = {  16, 16, 32 };

	// clear playerstate
	ent.client->ps = {};

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
		string userinfo = ent.client->pers.userinfo;
		resp = std::move(ent.client->resp);
		
		InitClientPersistant(ent);
		ClientUserinfoChanged(ent, userinfo);
#ifdef SINGLE_PLAYER
	}
	else if (coop)
	{
		string userinfo = ent.client->pers.userinfo;
		
		resp = std::move(ent.client->resp);

		resp.coop_respawn.game_helpchanged = ent.client->pers.game_helpchanged;
		resp.coop_respawn.helpchanged = ent.client->pers.helpchanged;
		ent.client->pers = resp.coop_respawn;
		ClientUserinfoChanged(ent, userinfo);
		if (resp.score > ent.client->pers.score)
			ent.client->pers.score = resp.score;
	}
	else
		resp = {};
#endif

	ent.client->ps = {};
	
	// clear everything but the persistant data
	client_persistant saved = std::move(ent.client->pers);
	*ent.client = {};
	ent.client->pers = std::move(saved);
#ifdef SINGLE_PLAYER
	if (ent.client->pers.health <= 0)
		InitClientPersistant(ent);
#endif
	ent.client->resp = std::move(resp);

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
	ent.type = ET_PLAYER;
	ent.mass = 200;
	ent.solid = SOLID_BBOX;
	ent.deadflag = DEAD_NO;
	ent.air_finished_framenum = level.framenum + 12 * BASE_FRAMERATE;
	ent.clipmask = MASK_PLAYERSOLID;
	ent.pain = player_pain_savable;
	ent.die = player_die_savable;
	ent.waterlevel = 0;
	ent.watertype = CONTENTS_NONE;
	ent.flags &= ~FL_NO_KNOCKBACK;
	ent.svflags &= ~SVF_DEADMONSTER;

	ent.mins = player_mins;
	ent.maxs = player_maxs;
	ent.velocity = vec3_origin;

	ent.client->ps.pmove.set_origin(spawn_origin);
	ent.client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	ent.client->ps.fov = clamp(1.f, (float)atof(Info_ValueForKey(ent.client->pers.userinfo, "fov")), 160.f);

	// clear entity state values
	ent.s.effects = EF_NONE;
	ent.s.modelindex = MODEL_PLAYER;        // will use the skin specified model
	ent.s.modelindex2 = MODEL_PLAYER;       // custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent.s.skinnum = ent.s.number - 1;

	ent.s.frame = 0;
	ent.s.origin = spawn_origin;
	ent.s.origin[2] += 1;  // make sure off ground
	ent.s.old_origin = ent.s.origin;

	// set the delta angle
	ent.client->ps.pmove.set_delta_angles(spawn_angles - ent.client->resp.cmd_angles);

	ent.s.angles[PITCH] = 0;
	ent.s.angles[YAW] = spawn_angles[YAW];
	ent.s.angles[ROLL] = 0;
	ent.client->ps.viewangles = ent.s.angles;
	ent.client->v_angle = ent.s.angles;

	// spawn a spectator
	if (ent.client->pers.spectator)
	{
		ent.client->chase_target = null_entity;
		ent.client->resp.spectator = true;

		ent.movetype = MOVETYPE_NOCLIP;
		ent.solid = SOLID_NOT;
		ent.svflags |= SVF_NOCLIENT;
		ent.client->ps.gunindex = MODEL_NONE;
		gi.linkentity(ent);
		return;
	}
	else
		ent.client->resp.spectator = false;

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
	if (!deathmatch.intVal && level.mapname == "rboss")
	{
		// if you get on to rboss in single player or coop, ensure
		// the player has the nuke key. (not in DM)
		gitem_t *it = FindItem("Antimatter Bomb");
		ent.client.pers.selected_item = it->id;
		ent.client.pers.inventory[ent.client.pers.selected_item] = 1;
	}
#endif

	// force the current weapon up
	ent.client->newweapon = ent.client->pers.weapon;
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
		ent.client->ps.pmove.set_delta_angles(ent.client->ps.viewangles);
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

	if (level.intermission_framenum)
		MoveClientToIntermission(ent);
	else if (game.maxclients > 1)
	{
		// send effect if in a multiplayer game
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort((int16_t)ent.s.number);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent.s.origin, MULTICAST_PVS);

		gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent.client->pers.netname.ptr());
	}

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
};

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
	ent.client->pers.netname = str;
	
	// set spectator
	str = Info_ValueForKey(userinfo, "spectator");

	// spectators are only supported in deathmatch
	if (
#ifdef SINGLE_PLAYER
		deathmatch &&
#endif
		!strempty(str) && str != "0")
		ent.client->pers.spectator = true;
	else
		ent.client->pers.spectator = false;
	
	// set skin
	str = Info_ValueForKey(userinfo, "skin");

	// combine name and skin into a configstring
#ifdef CTF
	// set player name field (used in id_state view)
	gi.configstring (CS_GENERAL + ent.s.number - 1, ent.client.pers.netname);

	if (ctf.intVal)
		CTFAssignSkin(ent, str);
	else
#endif
		gi.configstring((config_string)(CS_PLAYERSKINS + ent.s.number - 1), va("%s\\%s", ent.client->pers.netname.ptr(), str.ptr()));
	
	// fov
	ent.client->ps.fov = clamp(1.f, (float)atof(Info_ValueForKey(userinfo, "fov")), 160.f);
	
	// handedness
	str = Info_ValueForKey(userinfo, "hand");
	if (!strempty(str))
		ent.client->pers.hand = clamp(RIGHT_HANDED, (handedness)atoi(str), CENTER_HANDED);
	
	// save off the userinfo in case we want to check something later
	ent.client->pers.userinfo = userinfo;
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

		uint32_t i, numspec;
		
		// count spectators
		for (i = numspec = 0; i < game.maxclients; i++)
		{
			entity &e = itoe(i + 1);
			
			if (e.inuse && e.client->pers.spectator)
				numspec++;
		}

		if (numspec >= (uint32_t)maxspectators)
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

		if (!game.autosaved || !ent.client->pers.weapon)
			InitClientPersistant(ent);
	}
#else
	InitClientResp(ent);
	InitClientPersistant(ent);
#endif

	ClientUserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintf("%s connected\n", ent.client->pers.netname.ptr());

	ent.svflags = SVF_NONE; // make sure we start with known default
	ent.client->pers.connected = true;

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
	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent.client->pers.netname.ptr());

#ifdef CTF
	CTFDeadDropFlag(ent);
	CTFDeadDropTech(ent);
#endif
	
	// send effect
	if (ent.inuse)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort((int16_t)ent.s.number);
		gi.WriteByte(MZ_LOGOUT);
		gi.multicast(ent.s.origin, MULTICAST_PVS);
	}
	
	gi.unlinkentity(ent);
	ent.s.modelindex = MODEL_NONE;
	ent.s.sound = SOUND_NONE;
	ent.s.event = EV_NONE;
	ent.s.effects = EF_NONE;
	ent.solid = SOLID_NOT;
	ent.inuse = false;
	ent.type = ET_DISCONNECTED_PLAYER;
	ent.client->pers.connected = false;
}

#ifdef CTF
void(entity) CTFApplyRegeneration;
#endif

void ClientThink(entity &ent, const usercmd &ucmd)
{
	level.current_entity = ent;

	if (level.intermission_framenum)
	{
		ent.client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if (level.framenum > level.intermission_framenum + 5.0f * BASE_FRAMERATE
			&& (ucmd.buttons & BUTTON_ANY))
			level.exitintermission = true;
		return;
	}

	if (ent.client->chase_target.has_value())
		ent.client->resp.cmd_angles = ucmd.get_angles();
	else
	{
		pmove_t	pm = {};
		
		static entityref passent;
		static content_flags mask;

		passent = ent;
		mask = (ent.health > 0) ? MASK_PLAYERSOLID : MASK_DEADSOLID;
		
		pm.trace = [](auto start, auto mins, auto maxs, auto end) { return gi.trace(start, mins, maxs, end, passent, mask); };
		pm.pointcontents = [](auto point) { return gi.pointcontents(point); };

		// set up for pmove
		if (ent.movetype == MOVETYPE_NOCLIP)
			ent.client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent.s.modelindex != MODEL_PLAYER)
			ent.client->ps.pmove.pm_type = PM_GIB;
		else if (ent.deadflag)
			ent.client->ps.pmove.pm_type = PM_DEAD;
		else
			ent.client->ps.pmove.pm_type = PM_NORMAL;

#ifdef GROUND_ZERO
		ent.client.ps.pmove.gravity = (int16_t)sv_gravity * ent.gravity;
#else
		ent.client->ps.pmove.gravity = (int16_t)sv_gravity;
#endif
		pm.s = ent.client->ps.pmove;

		pm.s.set_origin(ent.s.origin);
		pm.s.set_velocity(ent.velocity);

		pm.snapinitial = !!memcmp(&ent.client->old_pmove, &pm.s, sizeof(pm.s));

		pm.cmd = ucmd;

		// perform a pmove
#ifdef QUAKEC_PMOVE
		Pmove(&pm);
#else
		gi.Pmove(pm);
#endif

		// save results of pmove
		ent.client->old_pmove = ent.client->ps.pmove = pm.s;

		ent.s.origin = pm.s.get_origin();
		ent.velocity = pm.s.get_velocity();

		ent.mins = pm.mins;
		ent.maxs = pm.maxs;

		ent.client->resp.cmd_angles = ucmd.get_angles();

		if (ent.groundentity.has_value() && !pm.groundentity.has_value() && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
#ifdef SINGLE_PLAYER
		{
#endif
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
#ifdef SINGLE_PLAYER
			PlayerNoise(ent, ent.s.origin, PNOISE_SELF);
		}
#endif

		ent.viewheight = (int)pm.viewheight;
		ent.waterlevel = pm.waterlevel;
		ent.watertype = pm.watertype;
		ent.groundentity = pm.groundentity;
		if (pm.groundentity.has_value())
			ent.groundentity_linkcount = pm.groundentity->linkcount;

		if (ent.deadflag)
		{
			ent.client->ps.viewangles[ROLL] = 40.f;
			ent.client->ps.viewangles[PITCH] = -15.f;
			ent.client->ps.viewangles[YAW] = ent.client->killer_yaw;
		}
		else
			ent.client->ps.viewangles = ent.client->v_angle = pm.viewangles;

#ifdef HOOK_CODE
		if (ent.client->grapple.has_value())
			GrapplePull(ent.client->grapple);
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

	ent.client->oldbuttons = ent.client->buttons;
	ent.client->buttons = ucmd.buttons;
	ent.client->latched_buttons |= ent.client->buttons & ~ent.client->oldbuttons;

	// fire weapon from final position if needed
	if ((ent.client->latched_buttons & BUTTON_ATTACK)
#ifdef CTF
		&& ent.movetype != MOVETYPE_NOCLIP
#endif
		)
	{
		if (ent.client->resp.spectator)
		{
			ent.client->latched_buttons = BUTTON_NONE;

			if (ent.client->chase_target.has_value())
			{
				ent.client->chase_target = nullptr;
				ent.client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				GetChaseTarget(ent);
		}
		else if (!ent.client->weapon_thunk)
		{
			ent.client->weapon_thunk = true;
			Think_Weapon(ent);
		}
	}

#ifdef CTF
	CTFApplyRegeneration(ent);
#endif

	if (ent.client->resp.spectator)
	{
		if (ucmd.upmove >= 10)
		{
			if (!(ent.client->ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				ent.client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (ent.client->chase_target.has_value())
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else
			ent.client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		entity &other = itoe(i);
		if (other.inuse && other.client->chase_target == ent)
			UpdateChaseCam(other);
	}
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
	if (level.intermission_framenum)
		return;

	if (
#ifdef SINGLE_PLAYER
		deathmatch &&
#endif
		ent.client->pers.spectator != ent.client->resp.spectator &&
		(level.framenum - ent.client->respawn_framenum) >= 5 * BASE_FRAMERATE)
	{
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!ent.client->weapon_thunk && !ent.client->resp.spectator
#ifdef CTF
		&& ent->movetype != MOVETYPE_NOCLIP
#endif
		)
		Think_Weapon(ent);
	else
		ent.client->weapon_thunk = false;

	if (ent.deadflag)
	{
		// wait for any button just going down
		if (level.framenum > ent.client->respawn_framenum)
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
			if ((ent.client->latched_buttons & buttonMask) || (
#ifdef SINGLE_PLAYER
				deathmatch && 
#endif
				((dm_flags)dmflags & DF_FORCE_RESPAWN)))
			{
				respawn(ent);
				ent.client->latched_buttons = BUTTON_NONE;
			}
		}

		return;
	}
#ifdef SINGLE_PLAYER

	// add player trail so monsters can follow
	if (!deathmatch)
		if (!visible(ent, PlayerTrail_LastSpot()))
			PlayerTrail_Add(ent.s.old_origin);
#endif

	ent.client->latched_buttons = BUTTON_NONE;
}
