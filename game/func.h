#pragma once

#include "../lib/types.h"
#include "spawn_flag.h"

constexpr spawn_flag PLAT_LOW_TRIGGER	= (spawn_flag)1;

void Move_Calc(entity &ent, vector dest, thinkfunc func);

entity &plat_spawn_inside_trigger(entity &ent);

void func_train_find(entity &self);

void train_use(entity &self, entity &other, entity &cactivator);

void Use_Plat(entity &ent, entity &other [[maybe_unused]], entity &);
