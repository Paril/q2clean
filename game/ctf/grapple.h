import "config.h";

#ifdef HOOK_CODE

import "game/entity_types.h";

export void GrapplePlayerReset(entity &ent);

// pull the player toward the grapple
export void GrapplePull(entity &self);

#ifdef GRAPPLE
void CTFWeapon_Grapple(entity &ent);
#endif

#ifdef OFFHAND_HOOK
void GrappleCmd(entity &ent);
#endif

#endif