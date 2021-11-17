#include "general.h"
#include "entity.h"
#include "game/entity.h"
#include "game/cmds.h"

void Drop_General(entity &ent, const gitem_t &it)
{
	Drop_Item(ent, it);
	ent.client.pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
}