#pragma once

import "game/entity_types.h";
import "itemlist.h";

bool Pickup_Bandolier(entity &ent, entity &other);

bool Pickup_Pack(entity &ent, entity &other);

export void Use_Quad(entity &ent, const gitem_t &it);

void Use_Breather(entity &ent, const gitem_t &it);

void Use_Envirosuit(entity &ent, const gitem_t &it);

void Use_Invulnerability(entity &ent, const gitem_t &it);

void Use_Silencer(entity &ent, const gitem_t &it);

bool Pickup_Powerup(entity &ent, entity &other);