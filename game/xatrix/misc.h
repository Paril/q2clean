#pragma once

#include "config.h"

#ifdef THE_RECKONING
#include "game/entity.h"
#include "game/misc.h"

void ThrowGibACID(entity &self, stringlit gibname, int32_t damage, gib_type type);

void ThrowHeadACID(entity &self, stringlit gibname, int32_t damage, gib_type type);
#endif