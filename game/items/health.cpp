#include "config.h"
#include "health.h"
#include "itemlist.h"
#include "lib/math.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/spawn.h"
#include "entity.h"

bool Pickup_Adrenaline(entity &ent, entity &other)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch)
		other.max_health += 1;
#endif

	other.health = max(other.health, other.max_health);

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

bool Pickup_AncientHead(entity &ent, entity &other)
{
	other.max_health += 2;

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

//======================================================================

#ifdef CTF
bool(entity) CTFHasRegeneration;
#endif

void MegaHealth_think(entity &self)
{
	if (self.owner->health > self.owner->max_health
#ifdef CTF
		&& !CTFHasRegeneration(self.owner)
#endif
		)
	{
		self.nextthink = level.framenum + 1 * BASE_FRAMERATE;
		self.owner->health -= 1;
		return;
	}

#ifdef SINGLE_PLAYER
	if (!(self.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(self.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(self, 20);
	else
		G_FreeEdict(self);
}

REGISTER_SAVABLE_FUNCTION(MegaHealth_think);

bool Pickup_Health(entity &ent, entity &other)
{
	if (!(ent.style & HEALTH_IGNORE_MAX))
		if (other.health >= other.max_health)
			return false;

#ifdef CTF
	if (other.health >= 250 && ent.count > 25)
		return false;
#endif

	other.health += ent.count;

#ifdef CTF
	if (other.health > 250 && ent.count > 25)
		other.health = 250;
#endif

	if (!(ent.style & HEALTH_IGNORE_MAX))
		other.health = min(other.health, other.max_health);

	if (ent.style & HEALTH_TIMED
#ifdef CTF
		&& !CTFHasRegeneration(other)
#endif
		)
	{
		ent.think = MegaHealth_think_savable;
		ent.nextthink = level.framenum + 5 * BASE_FRAMERATE;
		ent.owner = other;
		ent.flags |= FL_RESPAWN;
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;
	}
#ifdef SINGLE_PLAYER
	else if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	else if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, 30);

	return true;
}

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags) dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags) dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/medium/tris.md2";
	self.count = 10;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/n_health.wav");
}

REGISTER_ENTITY(item_health, ET_ITEM);

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_small(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags) dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags) dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/stimpack/tris.md2";
	self.count = 2;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	self.style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
}

REGISTER_ENTITY(item_health_small, ET_ITEM);

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_large(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags) dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags) dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/large/tris.md2";
	self.count = 25;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/l_health.wav");
}

REGISTER_ENTITY(item_health_large, ET_ITEM);

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_mega(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags) dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags) dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/mega_h/tris.md2";
	self.count = 100;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/m_health.wav");
	self.style = HEALTH_IGNORE_MAX | HEALTH_TIMED;
}

REGISTER_ENTITY(item_health_mega, ET_ITEM);