#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/game.h"
#include "game/cmds.h"
#include "game/items/itemlist.h"
#include "game/items/entity.h"
#include "game/rogue/ballistics/nuke.h"
#include "items.h"

bool Pickup_Nuke(entity &ent, entity &other)
{
	const gitem_t &citem = ent.item;
	int32_t quantity = other.client->pers.inventory[citem.id];

	if (quantity >= 1)
		return false;

#ifdef SINGLE_PLAYER
	if (coop && (citem.flags & IT_STAY_COOP) && quantity > 0)
		return false;
#endif

	other.client->pers.inventory[citem.id]++;

#ifdef SINGLE_PLAYER
	if (deathmatch && !(ent.spawnflags & DROPPED_ITEM))
#else
	if !(ent.spawnflags & DROPPED_ITEM)
#endif
		SetRespawn(ent, citem.quantity);

	return true;
}

void Use_IR(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->ir_framenum > level.framenum)
		ent.client->ir_framenum += 600;
	else
		ent.client->ir_framenum = level.framenum + 600;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ir_start.wav"), 1, ATTN_NORM, 0);
}

void Use_Double(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem (ent);

	if (ent.client->double_framenum > level.framenum)
		ent.client->double_framenum += 300;
	else
		ent.client->double_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage1.wav"), 1, ATTN_NORM, 0);
}

void Use_Nuke(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem (ent);

	vector forward;

	AngleVectors (ent.client->v_angle, &forward, nullptr, nullptr);

	fire_nuke (ent, ent.s.origin, forward, 100.f);
}

static void Item_TriggeredSpawn(entity &self, entity &, entity &)
{
	self.svflags &= ~SVF_NOCLIENT;
	self.use = 0;

	if (self.item->id == ITEM_POWER_CUBE)
		self.spawnflags = NO_SPAWNFLAGS;

	droptofloor (self);
}

static REGISTER_SAVABLE_FUNCTION(Item_TriggeredSpawn);

void SetTriggeredSpawn(entity &ent)
{
	// don't do anything on key_power_cubes.
	if (ent.item->id == ITEM_POWER_CUBE)
		return;

	ent.think = 0;
	ent.nextthink = 0;
	ent.use = SAVABLE(Item_TriggeredSpawn);
	ent.svflags |= SVF_NOCLIENT;
	ent.solid = SOLID_NOT;
}

#endif