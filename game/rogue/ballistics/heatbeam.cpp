#include "config.h"

#ifdef GROUND_ZERO
#include "game/entity.h"
#include "game/util.h"
#include "game/combat.h"
#ifdef SINGLE_PLAYER
#include "game/weaponry.h"
#include "game/ballistics.h"
#endif
#include "heatbeam.h"

constexpr means_of_death MOD_HEATBEAM { .other_kill_fmt = "{0} was scorched by {3}'s plasma beam.\n" };

static void fire_beams(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_beam, means_of_death_ref mod)
{
	vector dir = vectoangles(aimdir), water_start = vec3_origin;

	vector forward;
	AngleVectors (dir, &forward, nullptr, nullptr);

	vector end = start + (forward * 8192);
	bool water = false, underwater = false;

	if (gi.pointcontents (start) & MASK_WATER)
	{
		underwater = true;
		water_start = start;
	}

	trace tr = gi.traceline (start, end, self, MASK_SHOT | MASK_WATER);

	// see if we hit water
	if (tr.contents & MASK_WATER)
	{
		water = true;
		water_start = tr.endpos;

		if (start != tr.endpos)
			gi.ConstructMessage(svc_temp_entity, TE_HEATBEAM_SPARKS, water_start, vecdir { tr.normal }).multicast(tr.endpos, MULTICAST_PVS);

		// re-trace ignoring water this time
		tr = gi.traceline (water_start, end, self, MASK_SHOT);
	}

	vector endpoint = tr.endpos;

	// halve the damage if target underwater
	if (water)
		damage /= 2;

	// send gun puff / flash
	if (tr.fraction < 1.0 && !(tr.surface.flags & SURF_SKY))
	{
		if (tr.ent.takedamage)
			T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, { DAMAGE_ENERGY }, mod);
		else if (!water && (strncmp (tr.surface.name, "sky", 3)))
		{
			// This is the truncated steam entry - uses 1+1+2 extra bytes of data
			gi.ConstructMessage(svc_temp_entity, TE_HEATBEAM_STEAM, tr.endpos, vecdir { tr.normal }).multicast(tr.endpos, MULTICAST_PVS);
#if defined(SINGLE_PLAYER)

			if (self.is_client())
				PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if ((water) || (underwater))
	{
		dir = tr.endpos - water_start;
		VectorNormalize (dir);
		vector pos = tr.endpos + (dir * -2);

		if (gi.pointcontents (pos) & MASK_WATER)
			tr.endpos = pos;
		else
			tr = gi.traceline(pos, water_start, tr.ent, MASK_WATER);

		pos = (water_start + tr.endpos) * 0.5f;

		gi.ConstructMessage(svc_temp_entity, TE_BUBBLETRAIL2, water_start, tr.endpos).multicast(pos, MULTICAST_PVS);
	}

	vector beam_endpt;

	if (!underwater && !water)
		beam_endpt = tr.endpos;
	else
		beam_endpt = endpoint;

	gi.ConstructMessage(svc_temp_entity, te_beam, self, start, beam_endpt).multicast(self.origin, MULTICAST_ALL);
}

void fire_heatbeam(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, bool monster)
{
	fire_beams (self, start, aimdir, damage, kick, monster ? TE_MONSTER_HEATBEAM : TE_HEATBEAM, MOD_HEATBEAM);
}
#endif