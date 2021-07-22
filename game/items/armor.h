#include "config.h"
#include "itemlist.h"
#include "general.h"
#include "game/entity_types.h"

gitem_id ArmorIndex(entity &ent);

bool Pickup_Armor(entity &ent, entity &other);

gitem_id PowerArmorType(entity &ent);

void Use_PowerArmor(entity &ent, const gitem_t &);

bool Pickup_PowerArmor(entity &ent, entity &other);

void Drop_PowerArmor(entity &ent, const gitem_t &it);