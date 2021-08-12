#pragma once

#include "config.h"
#include "entity.h"
#include "lib/math/vector.h"
#include "savables.h"
#include "spawn.h"

constexpr spawn_flag PLAT_LOW_TRIGGER	= (spawn_flag)1;

void Move_Calc(entity &ent, vector dest, savable<ethinkfunc> func);

entity &plat_spawn_inside_trigger(entity &ent);

constexpr spawn_flag TRAIN_START_ON		= (spawn_flag)1;
constexpr spawn_flag TRAIN_TOGGLE		= (spawn_flag)2;
constexpr spawn_flag TRAIN_BLOCK_STOPS	= (spawn_flag)4;

void func_train_find(entity &self);

DECLARE_SAVABLE(func_train_find);

void train_use(entity &self, entity &other, entity &cactivator);

DECLARE_SAVABLE(train_use);

void Use_Plat(entity &ent, entity &other [[maybe_unused]], entity &);

#ifdef GROUND_ZERO
constexpr spawn_flag WATER_SMART = (spawn_flag) 2;
#endif

DECLARE_ENTITY(FUNC_DOOR);

DECLARE_ENTITY(FUNC_DOOR_ROTATING);

DECLARE_ENTITY(FUNC_PLAT);

DECLARE_ENTITY(FUNC_TRAIN);