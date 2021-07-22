#pragma once 

#include "config.h"

#ifdef SINGLE_PLAYER
#include "lib/math/vector.h"
#include "game/entity_types.h"

void check_dodge(entity &self, vector start, vector dir, int32_t speed);
#endif