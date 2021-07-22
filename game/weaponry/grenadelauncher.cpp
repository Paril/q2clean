import "config.h";
import "game/weaponry.h";
import "game/view.h";
import "lib/math/vector.h";
import "lib/math/random.h";
import "game/ballistics/grenade.h";
import "game/entity.h";
import "game/game.h";

static void weapon_grenadelauncher_fire(entity &ent)
{
	vector	offset;
	vector	forward, right;
	vector	start;
#ifdef GROUND_ZERO
	bool	is_prox = ent.client.pers.weapon->id == ITEM_PROX_LAUNCHER;
	int32_t	damage = is_prox ? 90 : 120;
#else
	int32_t	damage = 120;
#endif
	float	radius;

	radius = damage + 40.f;
	if (is_quad)
		damage *= damage_multiplier;

	offset = { 8, 8, ent.viewheight - 8.f };
	AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
	start = P_ProjectSource(ent, ent.s.origin, offset, forward, right);

	ent.client->kick_origin = forward * -2;
	ent.client->kick_angles[0] = -1.f;

#ifdef GROUND_ZERO
	if (is_prox)
		fire_prox(ent, start, forward, damage_multiplier, 600);
	else
#endif
		fire_grenade(ent, start, forward, damage, 600, 2.5f, radius);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort((int16_t) ent.s.number);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent.s.origin, MULTICAST_PVS);

	ent.client->ps.gunframe++;
#ifdef SINGLE_PLAYER

	PlayerNoise(ent, start, PNOISE_WEAPON);
#endif

	if (!((dm_flags) dmflags & DF_INFINITE_AMMO))
		ent.client->pers.inventory[ent.client->ammo_index]--;
}

void Weapon_GrenadeLauncher(entity &ent)
{
	Weapon_Generic(ent, 5, 16, 59, 64, { 34, 51, 59 }, { 6 }, weapon_grenadelauncher_fire);
}
