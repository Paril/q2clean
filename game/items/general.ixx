module;

#include "../../lib/types.h"
#include "../entity.h"
#include "../cmds.h"

export module items.general;

export void Drop_General(entity &ent, const gitem_t &it)
{
	Drop_Item(ent, it);
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
}