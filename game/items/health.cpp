#include "config.h"
#include "itemlist.h"
#include "lib/math.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/spawn.h"
#include "entity.h"
#include "health.h"

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
		SetRespawn(ent, seconds(ent.item->quantity));

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
		SetRespawn(ent, seconds(ent.item->quantity));

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
		self.nextthink = level.time + 1s;
		self.owner->health -= 1;
		return;
	}

#ifdef SINGLE_PLAYER
	if (!(self.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(self.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(self, 20s);
	else
		G_FreeEdict(self);
}

REGISTER_STATIC_SAVABLE(MegaHealth_think);

bool Pickup_Health(entity &ent, entity &other)
{
	if (!(ent.item->flags & IT_HEALTH_IGNORE_MAX))
		if (other.health >= other.max_health)
			return false;

	int32_t quantity = ent.count ? ent.count : ent.item->quantity;

#ifdef CTF
	if (other.health >= 250 && quantity > 25)
		return false;
#endif

	other.health += quantity;

#ifdef CTF
	if (other.health > 250 && quantity > 25)
		other.health = 250;
#endif

	if (!(ent.item->flags & IT_HEALTH_IGNORE_MAX))
		other.health = min(other.health, other.max_health);

	if ((ent.item->flags & IT_HEALTH_TIMED)
#ifdef CTF
		&& !CTFHasRegeneration(other)
#endif
		)
	{
		ent.think = SAVABLE(MegaHealth_think);
		ent.nextthink = level.time + 5s;
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
		SetRespawn(ent, 30s);

	return true;
}