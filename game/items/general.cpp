import "general.h";
import "entity.h";
import "game/entity.h";
import "game/cmds.h";

void Drop_General(entity &ent, const gitem_t &it)
{
	Drop_Item(ent, it);
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
}