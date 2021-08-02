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

static void fire_beams(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, temp_event te_beam, means_of_death mod)
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
		vector water_start = tr.endpos;

		if (start != tr.endpos)
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_HEATBEAM_SPARKS);
			gi.WritePosition (water_start);
			gi.WriteDir (tr.normal);
			gi.multicast (tr.endpos, MULTICAST_PVS);
		}

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
			T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, DAMAGE_ENERGY, mod);
		else if (!water && (strncmp (tr.surface.name, "sky", 3)))
		{
			// This is the truncated steam entry - uses 1+1+2 extra bytes of data
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_HEATBEAM_STEAM);
			gi.WritePosition (tr.endpos);
			gi.WriteDir (tr.normal);
			gi.multicast (tr.endpos, MULTICAST_PVS);
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

		pos = water_start + tr.endpos;
		pos *= 0.5f;

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL2);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}

	vector beam_endpt;

	if (!underwater && !water)
		beam_endpt = tr.endpos;
	else
		beam_endpt = endpoint;
	
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (te_beam);
	gi.WriteShort (self.s.number);
	gi.WritePosition (start);
	gi.WritePosition (beam_endpt);
	gi.multicast (self.s.origin, MULTICAST_ALL);
}

void fire_heatbeam(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick, bool monster)
{
	fire_beams (self, start, aimdir, damage, kick, monster ? TE_MONSTER_HEATBEAM : TE_HEATBEAM, MOD_HEATBEAM);
}
#endif