#pragma once 

import "config.h";

#ifdef SINGLE_PLAYER
import "game/entity_types.h";

export void check_dodge(entity &self, vector start, vector dir, int32_t speed);
#endif