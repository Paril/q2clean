#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/cmds.h"
#include "powerups.h"
#include "lib/math.h"
#include "entity.h"
#ifdef THE_RECKONING
#include "game/xatrix/items.h"
#endif

inline void AdjustAmmoMax(entity &other, const ammo_id &ammo, const int32_t &new_max)
{
	other.client->pers.max_ammo[ammo] = max(other.client->pers.max_ammo[ammo], new_max);
}

inline void AddAndCapAmmo(entity &other, const gitem_t &ammo)
{
	other.client->pers.inventory[ammo.id] = min(other.client->pers.inventory[ammo.id] + ammo.quantity, other.client->pers.max_ammo[ammo.ammotag]);
}

bool Pickup_Bandolier(entity &ent, entity &other)
{
	AdjustAmmoMax(other, AMMO_BULLETS, 250);
	AdjustAmmoMax(other, AMMO_SHELLS, 150);
	AdjustAmmoMax(other, AMMO_CELLS, 250);
	AdjustAmmoMax(other, AMMO_SLUGS, 75);
#ifdef THE_RECKONING
	AdjustAmmoMax(other, AMMO_MAGSLUG, 75);
#endif
#ifdef GROUND_ZERO
	AdjustAmmoMax(other, AMMO_FLECHETTES, 250);
	AdjustAmmoMax(other, AMMO_DISRUPTOR, 150);
#endif

	AddAndCapAmmo(other, GetItemByIndex(ITEM_BULLETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SHELLS));

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

bool Pickup_Pack(entity &ent, entity &other)
{
	AdjustAmmoMax(other, AMMO_BULLETS, 300);
	AdjustAmmoMax(other, AMMO_SHELLS, 200);
	AdjustAmmoMax(other, AMMO_ROCKETS, 100);
	AdjustAmmoMax(other, AMMO_GRENADES, 100);
	AdjustAmmoMax(other, AMMO_CELLS, 300);
	AdjustAmmoMax(other, AMMO_SLUGS, 100);
#ifdef THE_RECKONING
	AdjustAmmoMax(other, AMMO_MAGSLUG, 100);
#endif
#ifdef GROUND_ZERO
	AdjustAmmoMax(other, AMMO_FLECHETTES, 300);
	AdjustAmmoMax(other, AMMO_DISRUPTOR, 200);
#endif

	AddAndCapAmmo(other, GetItemByIndex(ITEM_BULLETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SHELLS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_CELLS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_GRENADES));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_ROCKETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SLUGS));
#ifdef THE_RECKONING
	AddAndCapAmmo(other, GetItemByIndex(ITEM_MAGSLUG));
#endif
#ifdef GROUND_ZERO
	AddAndCapAmmo(other, FindItem("Flechettes"));
	AddAndCapAmmo(other, FindItem("Rounds"));
#endif

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

static gtime quad_drop_timeout_hack;

void Use_Quad(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	gtime timeout;

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
		timeout = 300;

	if (ent.client->quad_framenum > level.framenum)
		ent.client->quad_framenum += timeout;
	else
		ent.client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

void Use_Breather(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->breather_framenum > level.framenum)
		ent.client->breather_framenum += 300;
	else
		ent.client->breather_framenum = level.framenum + 300;
}

void Use_Envirosuit(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->enviro_framenum > level.framenum)
		ent.client->enviro_framenum += 300;
	else
		ent.client->enviro_framenum = level.framenum + 300;
}

void Use_Invulnerability(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->invincible_framenum > level.framenum)
		ent.client->invincible_framenum += 300;
	else
		ent.client->invincible_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

void Use_Silencer(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
	ent.client->silencer_shots += 30;
}

bool Pickup_Powerup(entity &ent, entity &other)
{
#ifdef SINGLE_PLAYER
	const int32_t &quantity = other.client->pers.inventory[ent.item->id];

	if ((skill == 1 && quantity >= 2) || (skill >= 2 && quantity >= 1))
		return false;

	if (coop && (ent.item->flags & IT_STAY_COOP) && quantity > 0)
		return false;
#endif

	other.client->pers.inventory[ent.item->id]++;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		if (!(ent.spawnflags & DROPPED_ITEM))
			SetRespawn(ent, ent.item->quantity);

		if ((dmflags & DF_INSTANT_ITEMS) || ((ent.item->use == Use_Quad
#ifdef THE_RECKONING
			|| ent.item->use == Use_QuadFire
#endif
			) && (ent.spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if (ent.spawnflags & DROPPED_PLAYER_ITEM)
			{
				if (ent.item->use == Use_Quad)
					quad_drop_timeout_hack = ent.nextthink - level.framenum;
#ifdef THE_RECKONING
				else if (ent.item->use == Use_QuadFire)
					quad_fire_drop_timeout_hack = ent.nextthink - level.framenum;
#endif
			}

			if (ent.item->use)
				ent.item->use(other, ent.item);
		}
#ifdef SINGLE_PLAYER
	}
#endif

	return true;
}