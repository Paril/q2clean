#pragma once

#include "config.h"
#include "entity.h"
#include "lib/math/vector.h"
#include "lib/savables.h"

constexpr spawn_flag PLAT_LOW_TRIGGER	= (spawn_flag)1;

void Move_Calc(entity &ent, vector dest, savable_function<thinkfunc> func);

entity &plat_spawn_inside_trigger(entity &ent);

void func_train_find(entity &self);

DECLARE_SAVABLE_FUNCTION(func_train_find);

void train_use(entity &self, entity &other, entity &cactivator);

DECLARE_SAVABLE_FUNCTION(train_use);

void Use_Plat(entity &ent, entity &other [[maybe_unused]], entity &);
