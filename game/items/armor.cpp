#include "armor.h"
#include "game/entity.h"
#include "game/game.h"
#include "entity.h"

gitem_id ArmorIndex(entity &ent)
{
	if (!ent.is_client())
		return ITEM_NONE;

	if (ent.client->pers.inventory[ITEM_ARMOR_JACKET] > 0)
		return ITEM_ARMOR_JACKET;
	else if (ent.client->pers.inventory[ITEM_ARMOR_COMBAT] > 0)
		return ITEM_ARMOR_COMBAT;
	else if (ent.client->pers.inventory[ITEM_ARMOR_BODY] > 0)
		return ITEM_ARMOR_BODY;

	return ITEM_NONE;
}

bool Pickup_Armor(entity &ent, entity &other)
{
	gitem_id old_armor_index = ArmorIndex(other);

	// handle armor shards specially
	if (ent.item->id == ITEM_ARMOR_SHARD)
		other.client->pers.inventory[old_armor_index ? old_armor_index : ITEM_ARMOR_JACKET] += 2;
	else
	{
		// get info on new armor
		const gitem_armor &newinfo = ent.item->armor;

		// if player has no armor, just use it
		if (!old_armor_index)
			other.client->pers.inventory[ent.item->id] = newinfo.base_count;
		// use the better armor
		else
		{
			// get info on old armor
			const gitem_armor &oldinfo = GetItemByIndex(old_armor_index).armor;

			if (newinfo.normal_protection > oldinfo.normal_protection)
			{
				// calc new armor values
				const float salvage = oldinfo.normal_protection / newinfo.normal_protection;
				const int32_t salvagecount = (int32_t) (salvage * other.client->pers.inventory[old_armor_index]);
				const int32_t newcount = min(newinfo.base_count + salvagecount, newinfo.max_count);

				// zero count of old armor so it goes away
				other.client->pers.inventory[old_armor_index] = 0;

				// change armor to new item with computed value
				other.client->pers.inventory[ent.item->id] = newcount;
			}
			else
			{
				// calc new armor values
				const float salvage = newinfo.normal_protection / oldinfo.normal_protection;
				const int32_t salvagecount = (int32_t) (salvage * newinfo.base_count);
				const int32_t newcount = min(other.client->pers.inventory[old_armor_index] + salvagecount, oldinfo.max_count);

				// if we're already maxed out then we don't need the new armor
				if (other.client->pers.inventory[old_armor_index] >= newcount)
					return false;

				// update current armor value
				other.client->pers.inventory[old_armor_index] = newcount;
			}
		}
	}

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, 20s);

	return true;
}

gitem_id PowerArmorType(entity &ent)
{
	if (ent.is_client() && (ent.flags & FL_POWER_ARMOR))
	{
		if (ent.client->pers.inventory[ITEM_POWER_SHIELD] > 0)
			return ITEM_POWER_SHIELD;
		else if (ent.client->pers.inventory[ITEM_POWER_SCREEN] > 0)
			return ITEM_POWER_SCREEN;
	}

	return ITEM_NONE;
}

void Use_PowerArmor(entity &ent, const gitem_t &)
{
	if (ent.flags & FL_POWER_ARMOR)
		gi.sound(ent, gi.soundindex("misc/power2.wav"));
	else
	{
		if (!ent.client->pers.inventory[ITEM_CELLS])
		{
			gi.cprint(ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		gi.sound(ent, gi.soundindex("misc/power1.wav"));
	}

	ent.flags ^= FL_POWER_ARMOR;
}

bool Pickup_PowerArmor(entity &ent, entity &other)
{
	const int32_t quantity = other.client->pers.inventory[ent.item->id];

	other.client->pers.inventory[ent.item->id]++;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		if (!(ent.spawnflags & DROPPED_ITEM))
			SetRespawn(ent, seconds(ent.item->quantity));

		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent.item->use(other, ent.item);
#ifdef SINGLE_PLAYER
	}
#endif

	return true;
}

void Drop_PowerArmor(entity &ent, const gitem_t &it)
{
	if ((ent.flags & FL_POWER_ARMOR) && (ent.client->pers.inventory[it.id] == 1))
		Use_PowerArmor(ent, it);

	Drop_General(ent, it);
}
