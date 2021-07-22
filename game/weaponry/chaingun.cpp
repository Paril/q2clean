import "config.h";
import "game/weaponry.h";
import "game/view.h";
import "lib/math/vector.h";
import "lib/math/random.h";
import "game/ballistics/bullet.h";
import "game/player_frames.h";
import "game/entity.h";
import "game/game.h";

static void Chaingun_Fire(entity &ent)
{
	vector	start = vec3_origin;
	vector	forward, right, up;
	float	r, u;
	vector	offset;
	int32_t	kick = 2;
#ifdef SINGLE_PLAYER
	int32_t	damage;

	if (!deathmatch)
		damage = 8;
	else
		damage = 6;
#else
	int32_t	damage = 6;
#endif

	if (ent.client->ps.gunframe == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent.client->ps.gunframe == 14) && !(ent.client->buttons & BUTTON_ATTACK))
	{
		ent.client->ps.gunframe = 32;
		ent.client->weapon_sound = SOUND_NONE;
		return;
	}
	else if ((ent.client->ps.gunframe == 21) && (ent.client->buttons & BUTTON_ATTACK)
		&& ent.client->pers.inventory[ent.client->ammo_index])
		ent.client->ps.gunframe = 15;
	else
		ent.client->ps.gunframe++;

	if (ent.client->ps.gunframe == 22)
	{
		ent.client->weapon_sound = SOUND_NONE;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
		ent.client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	ent.client->anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crattak1 - (ent.client->ps.gunframe & 1);
		ent.client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent.s.frame = FRAME_attack1 - (ent.client->ps.gunframe & 1);
		ent.client->anim_end = FRAME_attack8;
	}

	int32_t shots;

	if (ent.client->ps.gunframe <= 9)
		shots = 1;
	else if (ent.client->ps.gunframe <= 14)
	{
		if (ent.client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent.client->pers.inventory[ent.client->ammo_index] < shots)
		shots = ent.client->pers.inventory[ent.client->ammo_index];

	if (!shots)
	{
		if (level.framenum >= ent.pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent.pain_debounce_framenum = level.framenum + 1 * BASE_FRAMERATE;
		}
		NoAmmoWeaponChange(ent);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent.client->kick_origin = randomv({ -0.35f, -0.35f, -0.35f }, { 0.35f, 0.35f, 0.35f });
	ent.client->kick_angles = randomv({ -0.7f, -0.7f, -0.7f }, { 0.7f, 0.7f, 0.7f });

	for (int32_t i = 0; i < shots; i++)
	{
		// get start / end positions
		AngleVectors(ent.client->v_angle, &forward, &right, &up);
		r = random(3.f, 11.f);
		u = random(-4.f, 4.f);
		offset = { 0, r, u + ent.viewheight - 8 };
		start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

		fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte((uint8_t) ((MZ_CHAINGUN1 + shots - 1) | is_silenced));
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index] -= shots;
}

void Weapon_Chaingun(entity &ent)
{
	Weapon_Generic(ent, 4, 31, 61, 64, { 38, 43, 51, 61 }, { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 }, Chaingun_Fire);
}