#include "config.h"

#ifdef SINGLE_PLAYER
#include "trail.h"
#include "lib/gi.h"
#include "util.h"

static constexpr int TRAIL_LENGTH = 8;

static array<entityref, TRAIL_LENGTH>	trail;
static int								trail_head;
static bool								trail_active = false;

#define NEXT(n)     (((n) + 1) & (TRAIL_LENGTH - 1))
#define PREV(n)     (((n) - 1) & (TRAIL_LENGTH - 1))

void PlayerTrail_Init()
{
	int     n;

	if (deathmatch)
		return;

	for (n = 0; n < TRAIL_LENGTH; n++)
		trail[n] = G_Spawn();

	trail_head = 0;
	trail_active = true;
}


void PlayerTrail_Add(vector spot)
{
	vector	temp;

	if (!trail_active)
		return;

	trail[trail_head]->origin = spot;
	trail[trail_head]->timestamp = level.time;

	temp = spot - trail[PREV(trail_head)]->origin;
	trail[trail_head]->angles[YAW] = vectoyaw(temp);

	trail_head = NEXT(trail_head);
}


void PlayerTrail_New(vector spot)
{
	if (!trail_active)
		return;

	PlayerTrail_Init();
	PlayerTrail_Add(spot);
}


entityref PlayerTrail_PickFirst(entity &self)
{
	int	marker;
	int	n;

	if (!trail_active)
		return null_entity;

	for (marker = trail_head, n = TRAIL_LENGTH; n; n--)
	{
		if (trail[marker]->timestamp <= self.monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	if (visible(self, trail[marker]))
		return trail[marker];

	if (visible(self, trail[PREV(marker)]))
		return trail[PREV(marker)];

	return trail[marker];
}

entityref PlayerTrail_PickNext(entity &self)
{
	int	marker;
	int	n;

	if (!trail_active)
		return null_entity;

	for (marker = trail_head, n = TRAIL_LENGTH; n; n--) {
		if (trail[marker]->timestamp <= self.monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	return trail[marker];
}

entityref PlayerTrail_LastSpot()
{
	return trail[PREV(trail_head)];
}

#endif