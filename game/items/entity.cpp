import "config.h";
import "game/game.h";
import "lib/savables.h";
import "lib/protocol.h";
import "lib/math/random.h";
import "lib/math/vector.h";
import "itemlist.h";
import "health.h";
import "game/util.h";
import "game/entity.h";
import "entity.h";

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
	ent->s.event = EV_ITEM_RESPAWN;
}

REGISTER_SAVABLE_FUNCTION(DoRespawn);

void SetRespawn(entity &ent, float delay)
{
	ent.flags |= FL_RESPAWN;
	ent.svflags |= SVF_NOCLIENT;
	ent.solid = SOLID_NOT;
	ent.nextthink = level.framenum + (gtime) (delay * BASE_FRAMERATE);
	ent.think = DoRespawn_savable;
	gi.linkentity(ent);
}

void Touch_Item(entity &ent, entity &other, vector plane, const surface &surf)
{
	if (!other.is_client())
		return;
	if (other.health < 1)
		return;     // dead people can't pickup
	if (!ent.item->pickup)
		return;     // not a grabbable item?

	const bool taken = ent.item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other.client->bonus_alpha = 0.25f;

		// show icon and name on status bar
		other.client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent.item->icon);
		other.client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + (config_string) ent.item->id;
		other.client->pickup_msg_framenum = level.framenum + (int) (3.0f * BASE_FRAMERATE);

		// change selected item
		if (ent.item->use)
			other.client->ps.stats[STAT_SELECTED_ITEM] = other.client->pers.selected_item = ent.item->id;

		if (ent.item->pickup == Pickup_Health)
		{
			if (ent.count == 2)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent.count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent.count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent.item->pickup_sound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent.item->pickup_sound), 1, ATTN_NORM, 0);
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

REGISTER_SAVABLE_FUNCTION(Touch_Item);

static void drop_temp_touch(entity &ent, entity &other, vector plane, const surface &surf)
{
	if (other == ent.owner)
		return;

	Touch_Item(ent, other, plane, surf);
}

REGISTER_SAVABLE_FUNCTION(drop_temp_touch);

static void drop_make_touchable(entity &ent)
{
	ent.touch = Touch_Item_savable;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		ent.nextthink = level.framenum + 29 * BASE_FRAMERATE;
		ent.think = G_FreeEdict_savable;
#ifdef SINGLE_PLAYER
	}
#endif
}

REGISTER_SAVABLE_FUNCTION(drop_make_touchable);

entity &Drop_Item(entity &ent, const gitem_t &it)
{
	entity &dropped = G_Spawn();

	dropped.type = ET_ITEM;
	dropped.item = it;
	dropped.spawnflags = DROPPED_ITEM;
	dropped.s.effects = it.world_model_flags;
	dropped.s.renderfx = RF_GLOW
#ifdef GROUND_ZERO
		| RF_IR_VISIBLE
#endif
		;
	dropped.mins = { -15, -15, -15 };
	dropped.maxs = { 15, 15, 15 };
	gi.setmodel(dropped, it.world_model);
	dropped.solid = SOLID_TRIGGER;
	dropped.movetype = MOVETYPE_TOSS;
	dropped.touch = drop_temp_touch_savable;
	dropped.owner = ent;

	vector forward;

	if (ent.is_client())
	{
		vector right;
		AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
		dropped.s.origin = G_ProjectSource(ent.s.origin, { 24, 0, -16 }, forward, right);

		trace tr = gi.trace(ent.s.origin, dropped.mins, dropped.maxs, dropped.s.origin, ent, CONTENTS_SOLID);
		dropped.s.origin = tr.endpos;
	}
	else
	{
		AngleVectors(ent.s.angles, &forward, nullptr, nullptr);
		dropped.s.origin = ent.s.origin;
	}

	dropped.velocity = forward * 100;
	dropped.velocity.z = 300.f;

	dropped.think = drop_make_touchable_savable;
	dropped.nextthink = level.framenum + 1 * BASE_FRAMERATE;

	gi.linkentity(dropped);
	return dropped;
}

static void Use_Item(entity &ent, entity &, entity &)
{
	ent.svflags &= ~SVF_NOCLIENT;
	ent.use = 0;

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = 0;
	}
	else
	{
		ent.solid = SOLID_TRIGGER;
		ent.touch = Touch_Item_savable;
	}

	gi.linkentity(ent);
}

REGISTER_SAVABLE_FUNCTION(Use_Item);

void droptofloor(entity &ent)
{
	ent.mins = { -15, -15, -15 };
	ent.maxs = { 15, 15, 15 };

	if (ent.model)
		gi.setmodel(ent, ent.model);
	else
		gi.setmodel(ent, ent.item->world_model);

	ent.solid = SOLID_TRIGGER;
	ent.movetype = MOVETYPE_TOSS;
	ent.touch = Touch_Item_savable;

	vector dest = ent.s.origin;
	dest[2] -= 128;

	trace tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, dest, ent, MASK_SOLID);

	if (tr.startsolid)
	{
#ifdef THE_RECKONING
		if (ent.classname == "foodcube")
		{
			tr.endpos = ent.s.origin;
			ent.velocity[2] = 0;
		}
		else
		{
#endif
			gi.dprintf("droptofloor: %i startsolid at %s\n", ent.type, vtos(ent.s.origin).ptr());
			G_FreeEdict(ent);
			return;
#ifdef THE_RECKONING
		}
#endif
	}

	ent.s.origin = tr.endpos;

	if (ent.team)
	{
		ent.flags &= ~FL_TEAMSLAVE;
		ent.chain = ent.teamchain;
		ent.teamchain = nullptr;
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;

		if (ent == ent.teammaster)
		{
			ent.nextthink = level.framenum + 1;
			ent.think = DoRespawn_savable;
		}
	}

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = 0;
		ent.s.effects &= ~EF_ROTATE;
		ent.s.renderfx &= ~RF_GLOW;
	}

	if (ent.spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;
		ent.use = Use_Item_savable;
	}

	gi.linkentity(ent);
}

REGISTER_SAVABLE_FUNCTION(droptofloor);

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
			gi.dprintf("PrecacheItem: %s has bad precache string \"%s\"", it.classname, it.precaches);
			break;
		}

		string ext = substr(v, v_len - 3);

		// determine type based on extension
		if (ext == "md2")
			gi.modelindex(v);
		else if (ext == "sp2")
			gi.modelindex(v);
		else if (ext == "wav")
			gi.soundindex(v);
		else if (ext == "pcx")
			gi.imageindex(v);
		else
			gi.dprintf("PrecacheItem: %s has bad precache entry \"%s\"", it.classname, v.ptr());
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

#ifdef GROUND_ZERO
void(entity ent) SetTriggeredSpawn;
#endif

#ifdef CTF
void(entity) CTFFlagSetup;
#endif

void SpawnItem(entity &ent, const gitem_t &it)
{
	if (ent.spawnflags
#ifdef GROUND_ZERO
		> ITEM_TRIGGER_SPAWN
#endif
#ifdef SINGLE_PLAYER
		&& it.id != ITEM_POWER_CUBE
#endif
		)
	{
		ent.spawnflags = NO_SPAWNFLAGS;
		gi.dprintf("%s at %s has invalid spawnflags set\n", it.classname, vtos(ent.s.origin).ptr());
	}

#ifdef SINGLE_PLAYER
	// some items will be prevented in deathmatch
	if (deathmatch)
	{
#endif
		if (((dm_flags) dmflags & DF_NO_ARMOR) && (it.pickup == Pickup_Armor || it.pickup == Pickup_PowerArmor))
			G_FreeEdict(ent);
		else if (((dm_flags) dmflags & DF_NO_ITEMS) && it.pickup == Pickup_Powerup)
			G_FreeEdict(ent);
		else if (((dm_flags) dmflags & DF_NO_HEALTH) && (it.pickup == Pickup_Health || it.pickup == Pickup_Adrenaline || it.pickup == Pickup_AncientHead))
			G_FreeEdict(ent);
		else if (((dm_flags) dmflags & DF_INFINITE_AMMO) && (it.flags == IT_AMMO || it.id == ITEM_BFG))
			G_FreeEdict(ent);
#ifdef GROUND_ZERO
		else if ((dmflags.intVal & DF_NO_MINES) && (ent.classname == "ammo_prox" || ent.classname == "ammo_tesla"))
			G_FreeEdict(ent);
		else if ((dmflags.intVal & DF_NO_NUKES) && ent.classname == "ammo_nuke")
			G_FreeEdict(ent);
#endif
#ifdef SINGLE_PLAYER
	}
#endif
#ifdef GROUND_ZERO
	else if (ent.classname == "ammo_nuke")
		G_FreeEdict(ent);
#endif

	// freed by the above
	if (!ent.inuse)
		return;

	PrecacheItem(it);

#ifdef SINGLE_PLAYER
	if (coop && it.id == ITEM_POWER_CUBE)
	{
		ent.spawnflags |= (spawn_flag) (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if (coop && (it.flags & IT_STAY_COOP))
		// FIXME
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
	ent.nextthink = level.framenum + 2;    // items start after other solids
	ent.think = droptofloor_savable;
	ent.s.effects = it.world_model_flags;
	ent.s.renderfx = RF_GLOW;
	ent.type = ET_ITEM;

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
