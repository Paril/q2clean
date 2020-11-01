#include "../../lib/types.h"

#ifdef BOTS

#include "../../lib/entity.h"
#include "../../lib/gi.h"
#include "../items.h"

//==========================================
// AI_CanUseArmor
// Check if we can use the armor
//==========================================
bool AI_CanUseArmor(const gitem_t &it, entity &other)
{
	// handle armor shards specially
	if (it.id == ITEM_ARMOR_SHARD)
		return true;

	// get info on new armor
	gitem_id old_armor_index = ArmorIndex(other);
	
	if (!old_armor_index)
		return true;
	
	const gitem_armor &newinfo = it.armor;
	const gitem_armor &oldinfo = GetItemByIndex(old_armor_index).armor;

	if (newinfo.normal_protection <= oldinfo.normal_protection)
	{
		// calc new armor values
		const float salvage = newinfo.normal_protection / oldinfo.normal_protection;
		const int32_t salvagecount = (int32_t)(salvage * newinfo.base_count);
		int32_t newcount = other.client->g.pers.inventory[old_armor_index] + salvagecount;

		if (newcount > oldinfo.max_count)
			newcount = oldinfo.max_count;

		// if we're already maxed out then we don't need the new armor
		if (other.client->g.pers.inventory[old_armor_index] >= newcount)
			return false;
	}

	return true;
}

//==========================================
// AI_CanPick_Ammo
// Check if we can use the Ammo
//==========================================
bool AI_CanPick_Ammo(entity &ent, const gitem_t &it)
{
	return ent.client->g.pers.inventory[it.id] < ent.client->g.pers.max_ammo[it.ammotag];
}


//==========================================
// AI_ItemIsReachable
// Can we get there? Jalfixme: this needs much better checks
//==========================================
bool AI_ItemIsReachable(entity &self, vector goal)
{
	vector v = self.mins;
	v[2] += AI_STEPSIZE;

	trace tr = gi.trace(self.s.origin, v, self.maxs, goal, self, MASK_NODESOLID);

	// Yes we can see it
	return tr.fraction == 1.0;
}


//==========================================
// AI_ItemWeight
// Find out item weight
//==========================================
float AI_ItemWeight(entity &self, entity &it)
{
	if (!it.g.item)
		return 0;

	if (it.g.item->flags & (IT_WEAPON | IT_AMMO | IT_ARMOR
#ifdef CTF
		| IT_FLAG
#endif
		))
		return self.g.ai.status.inventoryWeights[it.g.item->id];

	// IT_HEALTH
	if (it.g.item->flags & IT_HEALTH)
	{
		//CanPickup_Health
		if (!(it.g.style & HEALTH_IGNORE_MAX))
			if (self.g.health >= self.g.max_health)
				return 0;

		if (self.g.health >= 250 && it.g.count > 25)
			return 0;

		// find the weight
		float weight = 0;
		if (self.g.health < 100)
			weight = ((100 - self.g.health) + it.g.count) * 0.01f;
		else if (self.g.health <= 250 && it.g.count == 100) //mega
			weight = 8.0;

		weight += (self.g.health < 25); //we just NEED IT!!!

		if (weight < 0.2f)
			weight = 0.2f;
		
		return weight;
	}

	// IT_POWERUP
	if (it.g.item->flags & IT_POWERUP)
		return 0.7f;
	
#ifdef CTF
	// IT_TECH
	if (it.g.item->flags & IT_TECH)
		return self->ai.status.inventoryWeights[it.item->id];
#endif
	
#ifdef SINGLE_PLAYER
	// IT_STAY_COOP
	if (it.g.item->flags & IT_STAY_COOP)
		return 0;
#endif

	return 0;
}

#endif