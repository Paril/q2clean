#include "ballistics.h"
#include "lib/gi.h"
#include "lib/math/random.h"
#include "game/util.h"

#ifdef SINGLE_PLAYER
void check_dodge(entity &self, vector start, vector dir, int32_t speed)
{
	vector	end;
	vector	v;
	trace	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if ((int32_t) skill == 0) {
		if (random() > 0.25f)
			return;
	}
	end = start + (8192 * dir);
	tr = gi.traceline(start, end, self, MASK_SHOT);
	if (!tr.ent.is_world() && (tr.ent.svflags & SVF_MONSTER) && (tr.ent.health > 0) && tr.ent.monsterinfo.dodge && infront(tr.ent, self)) {
		v = tr.endpos - start;
		eta = (VectorLength(v) - tr.ent.maxs.x) / speed;
		tr.ent.monsterinfo.dodge(tr.ent, self, eta
#ifdef GROUND_ZERO
			, &tr
#endif
		);
	}
}
#endif