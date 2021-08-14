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

	if (weapon && (dmflags & DF_INFINITE_AMMO))
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
		SetRespawn(ent, 30s);

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
		gi.cprint(ent, PRINT_HIGH, "Can't drop current weapon\n");
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
	if (((dmflags & DF_WEAPONS_STAY) || coop) &&
#else
	if ((dmflags & DF_WEAPONS_STAY) &&
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
				if (dmflags & DF_WEAPONS_STAY)
					ent.flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30s);
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

inline use_weap_result CanUseWeapon(entity &ent, const gitem_t &it)
{
	if (it == ent.client->pers.weapon)
		return USEWEAP_IS_ACTIVE;

	if (!ent.client->pers.inventory[it.id])
		return USEWEAP_OUT_OF_ITEM;

	if (it.ammo && !g_select_empty && !(it.flags & IT_AMMO))
	{
		if (!ent.client->pers.inventory[it.ammo])
			return USEWEAP_NO_AMMO;
		else if (ent.client->pers.inventory[it.ammo] < it.quantity)
			return USEWEAP_NOT_ENOUGH_AMMO;
	}

	return USEWEAP_OK;
}

// check if the specified item belongs in a chain for `it`.
inline bool WeaponIsChained(const gitem_t &it, const gitem_t &chain)
{
	return it.id == chain.id || it.chain == chain.id;
}

constexpr gitem_id NextInChain(gitem_id id)
{
	return (gitem_id) (max(1u, (id + 1) % item_list().size()));
}

void Use_Weapon(entity &ent, const gitem_t &it)
{
	std::initializer_list<gitem_id> chain;

	gitem_id chain_begin = ITEM_NONE;

	if (ent.client->newweapon.has_value() && WeaponIsChained(ent.client->newweapon, it))
		chain_begin = ent.client->newweapon->id;
	else if (ent.client->pers.weapon.has_value() && WeaponIsChained(ent.client->pers.weapon, it))
		chain_begin = ent.client->pers.weapon->id;

	if (chain_begin)
	{
		// we're either holding a weapon that is in this chain or attempting
		// to switch to a weapon that is also in this chain.
		// find the next weapon we can equip that doesn't return false
		for (gitem_id id = NextInChain(chain_begin); id != chain_begin; id = NextInChain(id))
		{
			if (WeaponIsChained(GetItemByIndex(id), it) && // this weapon is in this chain...
				CanUseWeapon(ent, GetItemByIndex(id)) == USEWEAP_OK && // we can use it...
				ent.client->newweapon != id) // we're not already switching to it
			{
				ent.client->newweapon = id;
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
		gi.cprintfmt(ent, PRINT_HIGH, "No {} for {}.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	case USEWEAP_NOT_ENOUGH_AMMO:
		gi.cprintfmt(ent, PRINT_HIGH, "Not enough {} for {}.\n", ammo_item.pickup_name, it.pickup_name);
		return;
	case USEWEAP_OUT_OF_ITEM:
		gi.cprintfmt(ent, PRINT_HIGH, "Out of item: {}.\n", it.pickup_name);
		return;
	default:
		// change to this weapon when down
		ent.client->newweapon = it;
		break;
	}
}

void Drop_Weapon(entity &ent, const gitem_t &it)
{
	if (dmflags & DF_WEAPONS_STAY)
		return;

	// see if we're already using it
	if (((it == ent.client->pers.weapon) || (it == ent.client->newweapon)) && (ent.client->pers.inventory[it.id] == 1))
	{
		gi.cprint(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, it);

	ent.client->pers.inventory[it.id]--;
}