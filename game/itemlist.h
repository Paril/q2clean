#pragma once

#include "../lib/entity_effects.h"
#include "items.h"

using pickup_func = bool(entity&, entity&);
using use_func = void(entity&, const gitem_t&);
using drop_func = use_func;
using weaponthink_func = void(entity&);

struct gitem_armor
{
	int32_t	base_count;
	int32_t	max_count;

	float	normal_protection;
	float	energy_protection;
};

struct gitem_t
{
	stringlit			classname; // spawning name
	pickup_func			*pickup;
	use_func			*use;
	drop_func			*drop;
	weaponthink_func	*weaponthink;
	stringlit			pickup_sound;
	stringlit			world_model;
	entity_effects		world_model_flags;
	stringlit			view_model;
	stringlit			vwep_model;
	
	// client side info
	stringlit	icon;
	stringlit	pickup_name;	// for printing on pickup
	
	uint16_t	quantity;	// for ammo how much, for weapons how much is used per shot
	gitem_id	ammo;		// for weapons
	gitem_flags	flags;		// IT_* flags

	gitem_armor	armor;
	ammo_id		ammotag;
	
	stringlit	precaches;	// string of all models, sounds, and images this item will use

	// do NOT set these via the itemlist; they are overwritten later!

	// the item's ID
	gitem_id	id;
	// weapon model index (for weapons)
	uint8_t		vwep_id;
};

// fetch the immutable item list
const array<gitem_t, ITEM_TOTAL> &item_list();

// fetch specific item by id, with bounds check
inline const gitem_t &GetItemByIndex(const gitem_id &id)
{
	return item_list()[id];
}

#include "itemref.h"

/*
===============
FindItemByClassname
===============
*/
inline itemref FindItemByClassname(const stringref &classname)
{
	for (const gitem_t &item : item_list())
		if (striequals(item.classname, classname))
			return item;

	return nullptr;
}

/*
===============
FindItem
===============
*/
inline itemref FindItem(const stringref &pickup_name)
{
	for (const gitem_t &item : item_list())
		if (striequals(item.pickup_name, pickup_name))
			return item;

	return nullptr;
}