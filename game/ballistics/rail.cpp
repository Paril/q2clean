#include "config.h"
#include "lib/gi.h"
#include "game/util.h"
#include "game/weaponry.h"
#include "game/ballistics.h"
#include "lib/math/random.h"
#include "game/combat.h"
#include "game/entity.h"
#include "game/game.h"
#include "rail.h"

constexpr means_of_death MOD_RAILGUN { .other_kill_fmt = "{0} was railed by {3}.\n" };

/*
=================
fire_rail
=================
*/
void fire_rail(entity &self, vector start, vector aimdir, int32_t damage, int32_t kick)
{
	if (gi.pointcontents(start) & MASK_SOLID)
		return;

	vector	from = start;
	vector	end = start + (8192 * aimdir);
	entityref	ignore = self;
	content_flags	mask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;
	bool	water = false;
	trace	tr;

	while (ignore.has_value())
	{
		tr = gi.traceline(from, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME | CONTENTS_LAVA);
			water = true;
		}
		else
		{
			//ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
			if ((tr.ent.svflags & SVF_MONSTER) || tr.ent.is_client ||
				(tr.ent.solid == SOLID_BBOX))
				ignore = tr.ent;
			else
				ignore = nullptr;

			if ((tr.ent != self) && (tr.ent.takedamage))
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.normal, damage, kick, { DAMAGE_NONE }, MOD_RAILGUN);
		}

		from = tr.endpos;
	}

	// send gun puff / flash
	gi.ConstructMessage(svc_temp_entity, TE_RAILTRAIL, start, tr.endpos).multicast(self.origin, MULTICAST_PHS);

	if (water)
		gi.ConstructMessage(svc_temp_entity, TE_RAILTRAIL, start, tr.endpos).multicast(tr.endpos, MULTICAST_PHS);
#ifdef SINGLE_PLAYER

	if (self.is_client)
		PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
}