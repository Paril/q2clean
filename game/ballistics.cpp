#include "ballistics.h"
#include "lib/gi.h"
#include "lib/math/random.h"
#include "game/util.h"

#ifdef SINGLE_PLAYER
void check_dodge(entity &self, vector start, vector dir, int32_t speed)
{
	// easy mode only ducks one quarter the time
	if (!skill && random() > 0.25f)
		return;

	vector end = start + (8192 * dir);
	trace tr = gi.traceline(start, end, self, MASK_SHOT);

	if (!tr.ent.is_world() && (tr.ent.svflags & SVF_MONSTER) && (tr.ent.health > 0) && tr.ent.monsterinfo.dodge && infront(tr.ent, self))
	{
		vector v = tr.endpos - start;
		float eta = (VectorLength(v) - tr.ent.bounds.maxs.x) / speed;

		tr.ent.monsterinfo.dodge(tr.ent, self, eta
#ifdef ROGUE_AI
			, tr
#endif
		);
	}
}
#endif