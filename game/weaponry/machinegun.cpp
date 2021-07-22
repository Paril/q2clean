import "config.h";
import "game/entity.h";
import "game/game.h";
import "game/weaponry.h";
import "game/view.h";
import "lib/math/vector.h";
import "lib/math/random.h";
import "game/player_frames.h";
import "game/ballistics/bullet.h";

static void Machinegun_Fire(entity &ent)
{
	vector	start;
	vector	forward, right;
	vector	angles;
	int32_t	damage = 8;
	int32_t	kick = 2;
	vector	offset;

	if (!(ent.client->buttons & BUTTON_ATTACK)) {
		ent.client->machinegun_shots = 0;
		ent.client->ps.gunframe++;
		return;
	}

	if (ent.client->ps.gunframe == 5)
		ent.client->ps.gunframe = 4;
	else
		ent.client->ps.gunframe = 5;

	if (ent.client->pers.inventory[ent.client->ammo_index] < 1)
	{
		ent.client->ps.gunframe = 6;
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

	ent.client->kick_origin[1] = random(-0.35f, 0.35f);
	ent.client->kick_angles[1] = random(-0.7f, 0.7f);
	ent.client->kick_origin[2] = random(-0.35f, 0.35f);
	ent.client->kick_angles[2] = random(-0.7f, 0.7f);

	ent.client->kick_origin[0] = random(-0.35f, 0.35f);
	ent.client->kick_angles[0] = ent.client->machinegun_shots * -1.5f;
#ifdef SINGLE_PLAYER

	// raise the gun as it is firing
	if (!deathmatch)
	{
		ent.client->machinegun_shots++;
		if (ent.client->machinegun_shots > 9)
			ent.client->machinegun_shots = 9;
	}
#endif

	// get start / end positions
	angles = ent.client->v_angle + ent.client->kick_angles;
	AngleVectors(angles, &forward, &right, nullptr);
	offset = { 0, 8, ent.viewheight - 8.f };
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;

	ent.client->anim_priority = ANIM_ATTACK;
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent.s.frame = FRAME_crattak1 - (int) random(0.25f, 1.25f);
		ent.client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent.s.frame = FRAME_attack1 - (int) random(0.25f, 1.25f);
		ent.client->anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun(entity &ent)
{
	Weapon_Generic(ent, 3, 5, 45, 49, { 23, 45 }, { 4, 5 }, Machinegun_Fire);
}