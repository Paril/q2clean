#include "config.h"
#include "lib/math.h"
#include "weaponry.h"
#include "entity.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/cmds.h"

bool Add_Ammo(entity &ent, const gitem_t &it, int32_t count)
{
	if (!ent.is_client())
		return false;

	int32_t max = ent.client->pers.max_ammo[it.ammotag];

	if (ent.client->pers.inventory[it.id] == max)
		return false;

	ent.client->pers.inventory[it.id] = min(max, ent.client->pers.inventory[it.id] + count);
	return true;
}

bool Pickup_Ammo(entity &ent, entity &other)
{
	const bool weapon = (ent.item->flags & IT_WEAPON);
	int32_t ammocount;

	if (weapon && ((dm_flags) dmflags & DF_INFINITE_AMMO))
		ammocount = 1000;
	else if (ent.count)
		ammocount = ent.count;
	else
		ammocount = ent.item->quantity;

	const int32_t oldcount = other.client->pers.inventory[ent.item->id];

	if (!Add_Ammo(other, ent.item, ammocount))
		return false;

	if (weapon && !oldcount && other.client->pers.weapon != ent.item &&
#ifdef SINGLE_PLAYER
	(!deathmatch ||
#endif
		(other.client->pers.weapon && other.client->pers.weapon->id == ITEM_BLASTER))
#ifdef SINGLE_PLAYER
		)
#endif
		other.client->newweapon = ent.item;

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && deathmatch)
#else
	if (!(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
#endif
		SetRespawn(ent, 30);

	return true;
}

void Drop_Ammo(entity &ent, const gitem_t &it)
{
	entity &dropped = Drop_Item(ent, it);

	if (ent.client->pers.inventory[it.id] >= it.quantity)
		dropped.count = it.quantity;
	else
		dropped.count = ent.client->pers.inventory[it.id];

	if (ent.client->pers.weapon &&
		ent.client->pers.weapon->ammotag == AMMO_GRENADES &&
		it.ammotag == AMMO_GRENADES &&
		(ent.client->pers.inventory[it.id] - dropped.count) <= 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent.client->pers.inventory[it.id] -= dropped.count;
	ValidateSelectedItem(ent);
}

bool Pickup_Weapon(entity &ent, entity &other)
{
	const gitem_id index = ent.item->id;

#ifdef SINGLE_PLAYER
	if ((((dm_flags) dmflags & DF_WEAPONS_STAY) || coop) &&
#else
	if (((dm_flags) dmflags & DF_WEAPONS_STAY) &&
#endif
		other.client->pers.inventory[index])
	{
		if (!(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;   // leave the weapon for others to pickup
	}

	other.client->pers.inventory[index]++;

		if (!(ent.spawnflags & DROPPED_ITEM))
		{
			// give them some ammo with it
			if (ent.item->ammo)
			{
				const gitem_t &ammo = GetItemByIndex(ent.item->ammo);
				Add_Ammo(other, ammo, ammo.quantity);
			}

			if (!(ent.spawnflags & DROPPED_PLAYER_ITEM))
			{
#ifdef SINGLE_PLAYER
				if (deathmatch)
				{
#endif
					if ((dm_flags) dmflags & DF_WEAPONS_STAY)
						ent.flags |= FL_RESPAWN;
					else
						SetRespawn(ent, 30);
#ifdef SINGLE_PLAYER
				}
				if (coop)
					ent.flags |= FL_RESPAWN;
#endif
			}
		}

	if (other.client->pers.weapon != ent.item && (other.client->pers.inventory[index] == 1) && (
#ifdef SINGLE_PLAYER
		!deathmatch ||
#endif
		other.client->pers.weapon->id == ITEM_BLASTER))
		other.client->newweapon = ent.item;

	return true;
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
enum use_weap_result : uint8_t
{
	USEWEAP_OK,
	USEWEAP_NO_AMMO,
	USEWEAP_NOT_ENOUGH_AMMO,
	USEWEAP_IS_ACTIVE,
	USEWEAP_OUT_OF_ITEM
};

use_weap_result CanUseWeapon(entity &ent, const gitem_t &it)
{
	if (it == ent.client->pers.weapon)
		return USEWEAP_IS_ACTIVE;

	if (!ent.client->pers.inventory[it.id])
		return USEWEAP_OUT_OF_ITEM;

	if (it.ammo && !(int32_t) g_select_empty && !(it.flags & IT_AMMO))
	{
		if (!ent.client->pers.inventory[it.ammo])
			return USEWEAP_NO_AMMO;
		else if (ent.client->pers.inventory[it.ammo] < it.quantity)
			return USEWEAP_NOT_ENOUGH_AMMO;
	}

	return USEWEAP_OK;
};

#ifdef THE_RECKONING
const std::initializer_list<gitem_id> rail_chain = { ITEM_RAILGUN, ITEM_PHALANX };
#endif

#ifdef GROUND_ZERO
const std::initializer_list<gitem_id> grenadelauncher_chain = { ITEM_GRENADE_LAUNCHER, ITEM_PROX_LAUNCHER };
const std::initializer_list<gitem_id> machinegun_chain = { ITEM_MACHINEGUN, ITEM_ETF_RIFLE };
const std::initializer_list<gitem_id> bfg_chain = { ITEM_BFG, ITEM_DISRUPTOR };
#endif

#if defined(GROUND_ZERO) && defined(GRAPPLE)
const std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_CHAINFIST, ITEM_GRAPPLE };
#elif defined(GRAPPLE)
const std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_GRAPPLE };
#elif defined(GROUND_ZERO)
const std::initializer_list<gitem_id> blaster_chain = { ITEM_BLASTER, ITEM_CHAINFIST };
#endif

#if defined(GROUND_ZERO) && defined(THE_RECKONING)
const std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TRAP, ITEM_TESLA };
const std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_BOOMER, ITEM_PLASMA_BEAM };
#elif defined(GROUND_ZERO)
const std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TESLA };
const std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_PLASMA_BEAM };
#elif defined(THE_RECKONING)
const std::initializer_list<gitem_id> grenade_chain = { ITEM_GRENADES, ITEM_TRAP };
const std::initializer_list<gitem_id> hyperblaster_chain = { ITEM_HYPERBLASTER, ITEM_BOOMER };
#endif

constexpr bool GetChain(entity &ent [[maybe_unused]], const gitem_t &it [[maybe_unused]], std::initializer_list<gitem_id> &chain [[maybe_unused]] )
{
#if defined(GROUND_ZERO) || defined(GRAPPLE)
	if (it.id == ITEM_BLASTER)
		chain = blaster_chain;
#endif
#if defined(GROUND_ZERO) || defined(THE_RECKONING)
	else if (it.id == ITEM_HYPERBLASTER)
		chain = hyperblaster_chain;
	else if (it.id == ITEM_GRENADES)
		chain = grenade_chain;
#endif
#if defined(GROUND_ZERO)
	else if (it.id == ITEM_GRENADE_LAUNCHER)
		chain = grenadelauncher_chain;
	else if (it.id == ITEM_MACHINEGUN)
		chain = machinegun_chain;
	else if (it.id == ITEM_BFG)
		chain = bfg_chain;
#endif
#if defined(THE_RECKONING)
	else if (it.id == ITEM_RAILGUN)
		chain = rail_chain;
#endif
#if defined(GROUND_ZERO) || defined(THE_RECKONING) || defined(GRAPPLE)
	else
#endif
		return false;
#if defined(GROUND_ZERO) || defined(THE_RECKONING) || defined(GRAPPLE)

	return true;
#endif
}

void Use_Weapon(entity &ent, const gitem_t &it)
{
	std::initializer_list<gitem_id> chain;

	// we have a weapon chain; find our current position in this chain
	if (GetChain(ent, it, chain))
	{
		size_t chain_start = 0;

		for (const gitem_id *c = chain.begin(); c != chain.end(); c++)
		{
			const gitem_t &cit = GetItemByIndex(*c);

			if (ent.client->pers.weapon == cit || ent.client->newweapon == cit)
			{
				chain_start = (c - chain.begin()) + 1;
				break;
			}
		}

		// find the next weapon we can equip that doesn't return false
		for (size_t cid = 0; cid < chain.size(); cid++)
		{
			const gitem_t &cit = GetItemByIndex(*(chain.begin() + ((chain_start + cid) % chain.size())));

			// got it!
			if (CanUseWeapon(ent, cit) == USEWEAP_OK)
			{
				ent.client->newweapon = cit;
				return;
			}
		}

		// fall back to default warning message
	}

	const use_weap_result result = CanUseWeapon(ent, it);
	const gitem_t &ammo_item = GetItemByIndex(it.ammo);

	switch (result)
	{
	case USEWEAP_IS_ACTIVE:
		return;
	case USEWEAP_NO_AMMO:
		gi.cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	case USEWEAP_NOT_ENOUGH_AMMO:
		gi.cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	}

	// change to this weapon when down
	ent.client->newweapon = it;
}

void Drop_Weapon(entity &ent, const gitem_t &it)
{
	if ((dm_flags) dmflags & DF_WEAPONS_STAY)
		return;

	// see if we're already using it
	if (((it == ent.client->pers.weapon) || (it == ent.client->newweapon)) && (ent.client->pers.inventory[it.id] == 1))
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, it);

	ent.client->pers.inventory[it.id]--;
}