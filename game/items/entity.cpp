#include "config.h"
#include "game/game.h"
#include "game/savables.h"
#include "lib/protocol.h"
#include "lib/math/random.h"
#include "lib/math/vector.h"
#include "itemlist.h"
#include "game/util.h"
#include "game/entity.h"
#include "entity.h"
#include "lib/string/format.h"
#include "health.h"
#include "armor.h"
#include "powerups.h"
#ifdef GROUND_ZERO
#include "game/rogue/items.h"
#endif
#ifdef THE_RECKONING
#include "game/xatrix/items.h"
#endif

void DoRespawn(entity &item)
{
	entityref ent = item;

	if (ent->team)
	{
		entityref master = ent->teammaster;

#ifdef CTF
		//in ctf, when we are weapons stay, only the master of a team of weapons
		//is spawned
		if (ctf.intVal &&
			(dmflags.intVal & DF_WEAPONS_STAY) &&
			master.item && (master.item->flags & IT_WEAPON))
			ent = master;
		else
		{
#endif	
			uint32_t	count;

			for (count = 0, ent = master; ent.has_value(); ent = ent->chain, count++);

			uint32_t choice = Q_rand_uniform(count);

			for (count = 0, ent = master; count < choice; ent = ent->chain, count++);
#ifdef CTF
		}
#endif
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	// send an effect
	ent->event = EV_ITEM_RESPAWN;
}

REGISTER_STATIC_SAVABLE(DoRespawn);

void SetRespawn(entity &ent, gtimef delay)
{
	ent.flags |= FL_RESPAWN;
	ent.svflags |= SVF_NOCLIENT;
	ent.solid = SOLID_NOT;
	ent.nextthink = duration_cast<gtime>(level.time + delay);
	ent.think = SAVABLE(DoRespawn);
	gi.linkentity(ent);
}

void Touch_Item(entity &ent, entity &other, vector, const surface &)
{
	if (!other.is_client)
		return;
	if (other.health < 1)
		return;     // dead people can't pickup
	if (!ent.item->pickup)
		return;     // not a grabbable item?

	const bool taken = ent.item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other.client.bonus_alpha = 0.25f;

		// show icon and name on status bar
		other.client.ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent.item->icon);
		other.client.ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + (config_string) ent.item->id;
		other.client.pickup_msg_time = level.time + 3s;

		// change selected item
		if (ent.item->use)
			other.client.ps.stats[STAT_SELECTED_ITEM] = other.client.pers.selected_item = ent.item->id;

		if (ent.item->pickup_sound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent.item->pickup_sound));
	}

	if (!(ent.spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets(ent, other);
		ent.spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;
#ifdef SINGLE_PLAYER
	else if (coop && (ent.item->flags & IT_STAY_COOP) && !(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		return;
#endif

	if (ent.flags & FL_RESPAWN)
		ent.flags &= ~FL_RESPAWN;
	else
		G_FreeEdict(ent);
}

REGISTER_SAVABLE(Touch_Item);

static void drop_temp_touch(entity &ent, entity &other, vector plane, const surface &surf)
{
	if (other == ent.owner)
		return;

	Touch_Item(ent, other, plane, surf);
}

REGISTER_STATIC_SAVABLE(drop_temp_touch);

static void drop_make_touchable(entity &ent)
{
	ent.touch = SAVABLE(Touch_Item);

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		ent.nextthink = level.time + 29s;
		ent.think = SAVABLE(G_FreeEdict);
#ifdef SINGLE_PLAYER
	}
#endif
}

REGISTER_STATIC_SAVABLE(drop_make_touchable);

entity &Drop_Item(entity &ent, const gitem_t &it)
{
	entity &dropped = G_Spawn();

	dropped.item = it;
	dropped.spawnflags = DROPPED_ITEM;
	dropped.effects = it.world_model_flags;
	dropped.renderfx = RF_GLOW
#ifdef GROUND_ZERO
		| RF_IR_VISIBLE
#endif
		;
	dropped.bounds = bbox::sized(15.f);
	gi.setmodel(dropped, it.world_model);
	dropped.solid = SOLID_TRIGGER;
	dropped.movetype = MOVETYPE_TOSS;
	dropped.touch = SAVABLE(drop_temp_touch);
	dropped.owner = ent;

	vector forward;

	if (ent.is_client)
	{
		vector right;
		AngleVectors(ent.client.v_angle, &forward, &right, nullptr);
		dropped.origin = G_ProjectSource(ent.origin, { 24, 0, -16 }, forward, right);

		trace tr = gi.trace(ent.origin, dropped.bounds, dropped.origin, ent, CONTENTS_SOLID);
		dropped.origin = tr.endpos;
	}
	else
	{
		AngleVectors(ent.angles, &forward, nullptr, nullptr);
		dropped.origin = ent.origin;
	}

	dropped.velocity = forward * 100;
	dropped.velocity.z = 300.f;

	dropped.think = SAVABLE(drop_make_touchable);
	dropped.nextthink = level.time + 1s;

	gi.linkentity(dropped);
	return dropped;
}

static void Use_Item(entity &ent, entity &, entity &)
{
	ent.svflags &= ~SVF_NOCLIENT;
	ent.use = nullptr;

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = nullptr;
	}
	else
	{
		ent.solid = SOLID_TRIGGER;
		ent.touch = SAVABLE(Touch_Item);
	}

	gi.linkentity(ent);
}

REGISTER_STATIC_SAVABLE(Use_Item);

void droptofloor(entity &ent)
{
	ent.bounds = bbox::sized(15.f);

	if (ent.model)
		gi.setmodel(ent, ent.model);
	else
		gi.setmodel(ent, ent.item->world_model);

	ent.solid = SOLID_TRIGGER;
	ent.movetype = MOVETYPE_TOSS;
	ent.touch = SAVABLE(Touch_Item);

	vector dest = ent.origin;
	dest[2] -= 128;

	trace tr = gi.trace(ent.origin, ent.bounds, dest, ent, MASK_SOLID);

	if (tr.startsolid)
	{
#ifdef THE_RECKONING
		if (ent.item->id == ITEM_FOODCUBE)
		{
			tr.endpos = ent.origin;
			ent.velocity[2] = 0;
		}
		else
		{
#endif
			gi.dprintfmt("{}: {} startsolid\n", ent, __func__);
			G_FreeEdict(ent);
			return;
#ifdef THE_RECKONING
		}
#endif
	}

	ent.origin = tr.endpos;

	if (ent.team)
	{
		ent.flags &= ~FL_TEAMSLAVE;
		ent.chain = ent.teamchain;
		ent.teamchain = nullptr;
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;

		if (ent == ent.teammaster)
		{
			ent.nextthink = level.time + 1_hz;
			ent.think = SAVABLE(DoRespawn);
		}
	}

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = nullptr;
		ent.effects &= ~EF_ROTATE;
		ent.renderfx &= ~RF_GLOW;
	}

	if (ent.spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;
		ent.use = SAVABLE(Use_Item);
	}

	gi.linkentity(ent);
}

REGISTER_SAVABLE(droptofloor);

/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(const gitem_t &it)
{
	if (it.pickup_sound)
		gi.soundindex(it.pickup_sound);
	if (it.world_model)
		gi.modelindex(it.world_model);
	if (it.view_model)
		gi.modelindex(it.view_model);
	if (it.icon)
		gi.imageindex(it.icon);

	// parse everything for its ammo
	if (it.ammo && it.ammo != it.id)
		PrecacheItem(GetItemByIndex(it.ammo));

	// parse the space seperated precache string for other items
	if (!it.precaches)
		return;

	size_t s_pos = 0;

	while (true)
	{
		string v = strtok(it.precaches, s_pos);

		if (!v)
			break;

		size_t v_len = strlen(v);

		if (v_len >= MAX_QPATH || v_len < 5)
		{
			gi.dprintfmt("PrecacheItem: {} has bad precache string \"{}\"", it.classname, it.precaches);
			break;
		}

		string ext = substr(v, v_len - 3);

		// determine type based on extension
		if (ext == "md2" || ext == "sp2")
			gi.modelindex(v);
		else if (ext == "wav")
			gi.soundindex(v);
		else if (ext == "pcx")
			gi.imageindex(v);
		else
			gi.dprintfmt("PrecacheItem: {} has bad precache entry \"{}\"", it.classname, v);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/

#ifdef CTF
void(entity) CTFFlagSetup;
#endif

entity_type ET_ITEM("item");

void SpawnItem(entity &ent, const gitem_t &it)
{
	ent.type = it.type;

	if (ent.spawnflags >= DROPPED_ITEM)
	{
		ent.spawnflags = NO_SPAWNFLAGS;
		gi.dprintfmt("{}: invalid spawnflags set\n", ent);
	}

#ifdef SINGLE_PLAYER
	// some items will be prevented in deathmatch
	if (deathmatch)
	{
#endif
		if ((dmflags & DF_NO_ARMOR) && (it.pickup == Pickup_Armor || it.pickup == Pickup_PowerArmor))
			G_FreeEdict(ent);
		else if ((dmflags & DF_NO_ITEMS) && it.pickup == Pickup_Powerup)
			G_FreeEdict(ent);
		else if ((dmflags & DF_NO_HEALTH) && (it.pickup == Pickup_Health || it.pickup == Pickup_Adrenaline || it.pickup == Pickup_AncientHead))
			G_FreeEdict(ent);
		else if ((dmflags & DF_INFINITE_AMMO) && (it.flags == IT_AMMO || it.id == ITEM_BFG))
			G_FreeEdict(ent);
#ifdef GROUND_ZERO
		else if ((dmflags & DF_NO_MINES) && (it.id == ITEM_PROX || it.id == ITEM_TESLA))
			G_FreeEdict(ent);
		else if ((dmflags & DF_NO_NUKES) && it.id == ITEM_NUKE)
			G_FreeEdict(ent);
#endif
#ifdef SINGLE_PLAYER
	}
#endif
#ifdef GROUND_ZERO
	else if (it.id == ITEM_NUKE)
		G_FreeEdict(ent);
#endif

	// freed by the above
	if (!ent.inuse)
		return;

	PrecacheItem(it);

#ifdef SINGLE_PLAYER
	if (coop && it.id == ITEM_POWER_CUBE)
	{
		if (level.power_cubes == 32) {
			gi.error("Too many power cubes in level!\n");
		} else {
			ent.power_cube_id = level.power_cubes++;
		}
	}

	// don't let them drop items that stay in a coop game
	// FIXME
	if (coop && (it.flags & IT_STAY_COOP))
		const_cast<gitem_t &>(it).drop = nullptr;
#endif

#ifdef CTF
	//Don't spawn the flags unless enabled
	if (!ctf.intVal &&
		(ent->classname == "item_flag_team1" ||
			ent->classname == "item_flag_team2"))
	{
		G_FreeEdict(ent);
		return;
	}
#endif

	ent.item = it;
	ent.nextthink = level.time + 2_hz;    // items start after other solids
	ent.think = SAVABLE(droptofloor);
	ent.effects = it.world_model_flags;
	ent.renderfx = RF_GLOW;

	if (ent.model)
		gi.modelindex(ent.model);

#ifdef GROUND_ZERO
	if (ent.spawnflags & ITEM_TRIGGER_SPAWN)
		SetTriggeredSpawn(ent);
#endif

#ifdef CTF
	//flags are server animated and have special handling
	if (ent.classname == "item_flag_team1" ||
		ent.classname == "item_flag_team2")
		ent.think = CTFFlagSetup;
#endif
}
