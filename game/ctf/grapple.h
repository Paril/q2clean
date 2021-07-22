#include "config.h"

#ifdef HOOK_CODE

#include "game/entity_types.h"

void GrapplePlayerReset(entity &ent);

// pull the player toward the grapple
void GrapplePull(entity &self);

#ifdef GRAPPLE
void CTFWeapon_Grapple(entity &ent);
#endif

#ifdef OFFHAND_HOOK
void GrappleCmd(entity &ent);
#endif

#endif