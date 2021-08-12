#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "melee.h"

void fire_player_melee(entity &self, vector start, vector aim, int32_t reach, int32_t damage, int32_t kick, means_of_death_ref mod)
{
	vector v = vectoangles (aim);
	vector forward, right, up;

	AngleVectors (v, &forward, &right, &up);
	VectorNormalize (forward);

	vector point = start + (reach * forward);

	// see if the hit connects
	trace tr = gi.traceline(start, point, self, MASK_SHOT);

	if (tr.fraction == 1.0)
		return;

	if (tr.ent.takedamage)
	{
		// pull the player forward if you do damage
		self.velocity += (forward * 75) + (up * 75);

		// do the damage
		T_Damage (tr.ent, self, self, vec3_origin, tr.ent.origin, vec3_origin, damage, kick / 2, { .flags = DAMAGE_NO_KNOCKBACK | DAMAGE_DESTROY_ARMOR, .blood = TE_MOREBLOOD }, mod);
	}
	else
		gi.ConstructMessage(svc_temp_entity, TE_GUNSHOT, tr.endpos, vecdir { tr.normal }).multicast(tr.endpos, MULTICAST_PVS);
}
#endif