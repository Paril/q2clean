#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/items/itemlist.h"
#include "game/cmds.h"
#include "game/game.h"
#include "game/items/entity.h"
#include "game/items/health.h"
#include "items.h"

gtime quad_fire_drop_timeout_hack;

void Use_QuadFire(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	int timeout;

	if (quad_fire_drop_timeout_hack)
	{
		timeout = quad_fire_drop_timeout_hack;
		quad_fire_drop_timeout_hack = 0;
	}
	else
		timeout = 300;

	if (ent.client->quadfire_framenum > level.framenum)
		ent.client->quadfire_framenum += timeout;
	else
		ent.client->quadfire_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire1.wav"), 1, ATTN_NORM, 0);
}

#endif