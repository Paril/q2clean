#include "config.h"
#include "game/entity.h"
#include "game/game.h"
#include "game/weaponry.h"
#include "game/weaponry/blaster.h"
#include "game/view.h"
#include "lib/math/vector.h"
#include "lib/math/random.h"
#include "game/player_frames.h"
#include "hyperblaster.h"

static void Weapon_HyperBlaster_Fire(entity &ent)
{
	ent.client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent.client->buttons & BUTTON_ATTACK))
		ent.client->ps.gunframe++;
	else
	{
		if (!ent.client->pers.inventory[ent.client->ammo_index])
		{
			if (level.framenum >= ent.pain_debounce_framenum)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
			}
			NoAmmoWeaponChange(ent);
		}
		else
		{
			float rotation = (ent.client->ps.gunframe - 5) * (PI / 3);
			vector offset = { -4.f * sinf(rotation), 0, 4.f * cosf(rotation) };
			entity_effects effect;

			if ((ent.client->ps.gunframe == 6) || (ent.client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = EF_NONE;

#ifdef SINGLE_PLAYER
			int32_t damage;

			if (!deathmatch)
				damage = 20;
			else
				damage = 15;

#else
			constexpr int32_t damage = 15;
#endif
			Blaster_Fire(ent, offset, damage, true, effect);

			if (!(dmflags & DF_INFINITE_AMMO))
				ent.client->pers.inventory[ent.client->ammo_index]--;

			ent.client->anim_priority = ANIM_ATTACK;
			if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent.s.frame = FRAME_crattak1 - 1;
				ent.client->anim_end = FRAME_crattak9;
			}
			else
			{
				ent.s.frame = FRAME_attack1 - 1;
				ent.client->anim_end = FRAME_attack8;
			}
		}

		ent.client->ps.gunframe++;

		if (ent.client->ps.gunframe == 12 && ent.client->pers.inventory[ent.client->ammo_index])
			ent.client->ps.gunframe = 6;
	}

	if (ent.client->ps.gunframe == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent.client->weapon_sound = SOUND_NONE;
	}
}

void Weapon_HyperBlaster(entity &ent)
{
	Weapon_Generic(ent, 5, 20, 49, 53, G_IsAnyFrame, G_IsAnyFrame<6, 7, 8, 9, 10, 11>, Weapon_HyperBlaster_Fire);
}