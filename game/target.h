#pragma once

#include "../lib/types.h"
#include "game/entity_types.h"
#include "savables.h"

constexpr spawn_flag LASER_ON = (spawn_flag)1;
constexpr spawn_flag LASER_RED = (spawn_flag)2;
constexpr spawn_flag LASER_GREEN = (spawn_flag)4;
constexpr spawn_flag LASER_BLUE = (spawn_flag)8;
constexpr spawn_flag LASER_YELLOW = (spawn_flag)16;
constexpr spawn_flag LASER_ORANGE = (spawn_flag)32;
constexpr spawn_flag LASER_FAT = (spawn_flag)64;
constexpr spawn_flag LASER_STOPWINDOW = (spawn_flag)128;
constexpr spawn_flag LASER_BZZT = (spawn_flag)0x80000000;

void target_laser_think(entity &self);

DECLARE_SAVABLE_FUNCTION(target_laser_think);

DECLARE_ENTITY(TARGET_CROSSLEVEL_TARGET);

DECLARE_ENTITY(TARGET_CHANGELEVEL);