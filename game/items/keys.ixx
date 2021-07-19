module;

#include "../../lib/types.h"
#include "../entity.h"
#include "../game.h"

export module items.keys;

import items.list;

#if defined(SINGLE_PLAYER)
export bool Pickup_Key(entity &ent, entity &other)
{
	if (coop)
	{
		if (ent.item->id == ITEM_POWER_CUBE)
		{
			int cube_flag = (ent.spawnflags & 0x0000ff00) >> 8;

			if (other.client->pers.power_cubes & cube_flag)
				return false;

			other.client->pers.inventory[ent.item->id]++;
			other.client->pers.power_cubes |= cube_flag;
		}
		else
		{
			if (other.client->pers.inventory[ent.item->id])
				return false;

			other.client->pers.inventory[ent.item->id] = 1;
		}

		return true;
	}
	other.client->pers.inventory[ent.item->id]++;
	return true;
}
#endif