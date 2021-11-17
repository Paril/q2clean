#include "config.h"

#if defined(SINGLE_PLAYER)
#include "keys.h"
#include "itemlist.h"
#include "game/entity.h"
#include "game/game.h"

#include <bitset>

bool Pickup_Key(entity &ent, entity &other)
{
	if (coop)
	{
		if (ent.item->id == ITEM_POWER_CUBE)
		{
			if (other.client.pers.power_cubes.test(ent.power_cube_id))
				return false;

			other.client.pers.inventory[ent.item->id]++;
			other.client.pers.power_cubes.set(ent.power_cube_id);
		}
		else
		{
			if (other.client.pers.inventory[ent.item->id])
				return false;

			other.client.pers.inventory[ent.item->id] = 1;
		}

		return true;
	}

	other.client.pers.inventory[ent.item->id]++;
	return true;
}
#endif