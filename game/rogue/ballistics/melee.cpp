#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "melee.h"

void fire_player_melee(entity &self, vector start, vector aim, int32_t reach, int32_t damage, int32_t kick, means_of_death mod)
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
		self.velocity += forward * 75;
		self.velocity += up * 75;

		damage_flags dflags = DAMAGE_NO_KNOCKBACK;

		if (mod == MOD_CHAINFIST)
			dflags |= DAMAGE_DESTROY_ARMOR;

		// do the damage
		T_Damage (tr.ent, self, self, vec3_origin, tr.ent.s.origin, vec3_origin, damage, kick / 2, dflags, mod);
	}
	else
	{
		point = tr.normal * 256;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_GUNSHOT);
		gi.WritePosition (tr.endpos);
		gi.WriteDir (point);
		gi.multicast (tr.endpos, MULTICAST_PVS);
	}
}
#endif