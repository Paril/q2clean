import "config.h";

#ifdef SINGLE_PLAYER
import "game/entity_types.h";
import "lib/math/vector.h";

/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
bool fire_hit(entity &self, vector aim, int32_t damage, int32_t kick);
#endif