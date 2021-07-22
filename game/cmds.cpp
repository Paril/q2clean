import <algorithm>;
import "config.h";
import "entity.h";
import "lib/info.h";
import "game.h";
import "chase.h";
import "player.h";

import "util.h";
import "hud.h";
import "lib/types/dynarray.h";
import "lib/string/format.h";
import "items/weaponry.h";
import "combat.h";
import "player_frames.h";

static string ClientTeam(entity &ent)
{
	if (!ent.is_client())
		return "???";

	string team = Info_ValueForKey(ent.client->pers.userinfo, "skin");
	size_t p = strchr(team, '/');

	if (p == -1)
		return team;

	if ((dm_flags)dmflags & DF_MODELTEAMS)
		return substr(team, 0, p);

	return substr(team, p + 1);
}

bool OnSameTeam(entity &ent1, entity &ent2)
{
	if (ent1 == ent2)
		return false;
	// player-only teams
	else if (ent1.is_client() && ent2.is_client())
	{
#ifdef SINGLE_PLAYER
		// coop players always teamed
		if (coop)
			return true;
		else
#endif
#ifdef CTF
		if (ctf.intVal && ent1.client.resp.ctf_team == ent2.client.resp.ctf_team)
			return true;
		else
#endif
		if ((dm_flags)dmflags & (DF_MODELTEAMS | DF_SKINTEAMS))
			return ClientTeam(ent1) == ClientTeam(ent2);
	}

	return false;
}

static void SelectNextItem(entity &ent, gitem_flags itflags)
{
#ifdef PMENU
	if (ent.client.menu.open)
	{
		PMenu_Next(ent);
		return;
	}
#endif

	if (ent.client->chase_target.has_value())
	{
		ChaseNext(ent);
		return;
	}

	// scan for the next valid one
	for (size_t i = 1; i <= item_list().size(); i++)
	{
		gitem_id index = (gitem_id)((ent.client->pers.selected_item + i) % item_list().size());

		if (!ent.client->pers.inventory[index])
			continue;

		const gitem_t &it = GetItemByIndex(index);

		if (!it.use)
			continue;
		if (!(it.flags & itflags))
			continue;

		ent.client->pers.selected_item = index;
		return;
	}

	ent.client->pers.selected_item = ITEM_NONE;
}

static void SelectPrevItem(entity &ent, gitem_flags itflags)
{
#ifdef PMENU
	if (ent.client.menu.open)
	{
		PMenu_Prev(ent);
		return;
	}
#endif

	if (ent.client->chase_target.has_value())
	{
		ChasePrev(ent);
		return;
	}

	// scan for the next valid one
	for (size_t i = 1; i <= item_list().size(); i++)
	{
		gitem_id index = (gitem_id)((ent.client->pers.selected_item + item_list().size() - i) % item_list().size());

		if (!ent.client->pers.inventory[index])
			continue;

		const gitem_t &it = GetItemByIndex(index);

		if (!it.use)
			continue;
		if (!(it.flags & itflags))
			continue;

		ent.client->pers.selected_item = index;
		return;
	}

	ent.client->pers.selected_item = ITEM_NONE;
}

void ValidateSelectedItem(entity &ent)
{
	if (!ent.client->pers.inventory[ent.client->pers.selected_item])
		SelectNextItem(ent, (gitem_flags)-1);
}

//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(entity &ent)
{
	if (
#ifdef SINGLE_PLAYER
		deathmatch && 
#endif
		!sv_cheats)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	stringlit name = gi.args();
	const bool give_all = striequals(name, "all");

	if (give_all || striequals(gi.argv(1), "health"))
	{
		if (gi.argc() == 3)
			ent.health = atoi(gi.argv(2));
		else
			ent.health = ent.max_health;
		if (!give_all)
			return;
	}

	const bool give_weapons = give_all || striequals(name, "weapons");
	const bool give_ammo = give_all || striequals(name, "ammo");

	if (give_weapons || give_ammo)
	{
		for (auto &it : item_list())
		{
			if (!it.pickup)
				continue;
			if (it.flags & IT_AMMO)
			{
				if (give_ammo)
					Add_Ammo (ent, it, 1000);
			}
			else if (it.flags & IT_WEAPON)
			{
				if (give_weapons)
					ent.client->pers.inventory[it.id] += 1;
			}
			else if (!(it.flags & IT_ARMOR))
			{
				if (give_all)
					ent.client->pers.inventory[it.id] = 1;
			}
		}

		if (!give_all)
			return;
	}

	if (give_all || striequals(name, "armor"))
	{
		ent.client->pers.inventory[ITEM_ARMOR_JACKET] = 0;
		ent.client->pers.inventory[ITEM_ARMOR_COMBAT] = 0;
		ent.client->pers.inventory[ITEM_ARMOR_BODY] = GetItemByIndex(ITEM_ARMOR_BODY).armor.max_count;

		if (!give_all)
			return;
	}

	if (give_all || striequals(name, "Power Shield"))
	{
		const gitem_t &it = GetItemByIndex(ITEM_POWER_SHIELD);

		entity &it_ent = G_Spawn();
		st.classname = it.classname;

		SpawnItem(it_ent, it);
		Touch_Item(it_ent, ent, vec3_origin, null_surface);

		if (it_ent.inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}
	
	if (give_all)
		return;

	itemref it = FindItem(name);

	if (!it)
	{
		it = FindItem(gi.argv(1));

		if (!it)
		{
			gi.cprintf(ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent.client->pers.inventory[it->id] = atoi(gi.argv(2));
		else
			ent.client->pers.inventory[it->id] += it->quantity;
	}
	else
	{
		entity &it_ent = G_Spawn();
		st.classname = it->classname;

		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, vec3_origin, null_surface);

		if (it_ent.inuse)
			G_FreeEdict(it_ent);
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(entity &ent)
{
	if (
#ifdef SINGLE_PLAYER
		deathmatch && 
#endif
		!sv_cheats)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent.flags ^= FL_GODMODE;

	stringlit msg;

	if (!(ent.flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf(ent, PRINT_HIGH, msg);
}

#if defined(SINGLE_PLAYER)
/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(entity &ent)
{
	if (
#ifdef SINGLE_PLAYER
		deathmatch && 
#endif
		!sv_cheats)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent.flags ^= FL_NOTARGET;

	stringlit msg;

	if (!(ent.flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}
#endif

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(entity &ent)
{
	if (
#ifdef SINGLE_PLAYER
		deathmatch && 
#endif
		!sv_cheats)
	{
		gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	stringlit msg;

	if (ent.movetype == MOVETYPE_NOCLIP)
	{
		ent.movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent.movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f(entity &ent)
{
	stringlit s = gi.args();
	itemref it = FindItem(s);

	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}

	if (!ent.client->pers.inventory[it->id])
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
	else if (!it->use)
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
	else
		it->use(ent, it);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
static void Cmd_Drop_f(entity &ent)
{
	string s = strlwr(gi.args());
	
#ifdef CTF
	gitem_t *it;

	if (s == "tech" && (it = CTFWhat_Tech(ent)))
	{
		it->drop(ent, it);
		return;
	}
#else
	itemref
#endif
	it = FindItem(s);

	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s.ptr());
		return;
	}

	if (!ent.client->pers.inventory[it->id])
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s.ptr());
	else if (!it->drop)
		gi.cprintf (ent, PRINT_HIGH, "Item is not droppable.\n");
	else
		it->drop(ent, it);
}

/*
=================
Cmd_Inven_f
=================
*/
static void Cmd_Inven_f(entity& ent)
{
	ent.client->showscores = false;
#ifdef SINGLE_PLAYER
	ent.client->showhelp = false;
#endif
#ifdef PMENU
	if (ent.client.menu.open)
	{
		PMenu_Close(ent);
		ent.client.update_chase = true;
		return;
	}
#endif

	if (ent.client->showinventory)
	{
		ent.client->showinventory = false;
		return;
	}

#ifdef CTF
	if (ctf.intVal && ent.client.resp.ctf_team == CTF_NOTEAM)
	{
		CTFOpenJoinMenu(ent);
		return;
	}
#endif

	ent.client->showinventory = true;

	gi.WriteByte(svc_inventory);
	for (size_t i = 0; i < MAX_ITEMS; i++)
	{
		if (i < item_list().size())
			gi.WriteShort((int16_t)ent.client->pers.inventory[i]);
		else
			gi.WriteShort(0);
	}
	gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
static void Cmd_InvUse_f(entity &ent)
{
#ifdef PMENU
	if (ent.client.menu.open)
	{
		PMenu_Select(ent);
		return;
	}
#endif
	
	ValidateSelectedItem(ent);

	if (!ent.client->pers.selected_item)
	{
		gi.cprintf(ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	const gitem_t &it = GetItemByIndex(ent.client->pers.selected_item);

	if (!it.use)
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
	else
		it.use(ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f(entity &ent)
{
	if (!ent.client->pers.weapon)
		return;

	const gitem_id selected_weapon = ent.client->pers.weapon->id;

	// scan  for the next valid one
	for (size_t i = 1; i <= item_list().size(); i++)
	{
		const gitem_id index = (gitem_id)((selected_weapon + item_list().size() - i) % item_list().size());

		if (!ent.client->pers.inventory[index])
			continue;

		const gitem_t &it = GetItemByIndex(index);

		if (!it.use)
			continue;
		if (!(it.flags & IT_WEAPON))
			continue;

		it.use(ent, it);

		if (ent.client->newweapon == it || ent.client->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(entity &ent)
{
	if (!ent.client->pers.weapon)
		return;

	const gitem_id selected_weapon = ent.client->pers.weapon->id;

	// scan  for the next valid one
	for (size_t i = 1; i <= item_list().size(); i++)
	{
		const gitem_id index = (gitem_id)((selected_weapon + i) % item_list().size());

		if (!ent.client->pers.inventory[index])
			continue;

		const gitem_t &it = GetItemByIndex(index);

		if (!it.use)
			continue;
		if (!(it.flags & IT_WEAPON))
			continue;

		it.use(ent, it);

		if (ent.client->newweapon == it || ent.client->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(entity &ent)
{
	if (!ent.client->pers.weapon || !ent.client->pers.lastweapon)
		return;

	const gitem_id index = ent.client->pers.lastweapon->id;

	if (!ent.client->pers.inventory[index])
		return;

	const gitem_t &it = GetItemByIndex(index);

	if (!it.use)
		return;
	if (!(it.flags & IT_WEAPON))
		return;

	it.use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f(entity &ent)
{
	ValidateSelectedItem(ent);

	if (!ent.client->pers.selected_item)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	const gitem_t &it = GetItemByIndex(ent.client->pers.selected_item);

	if (!it.drop)
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
	else
		it.drop(ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(entity &ent)
{
	if (ent.solid == SOLID_NOT)
		return;
	if ((level.framenum - ent.client->respawn_framenum) < 5 * BASE_FRAMERATE)
		return;

	ent.flags &= ~FL_GODMODE;
	ent.health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(entity &ent)
{
	ent.client->showscores = false;
#ifdef SINGLE_PLAYER
	ent.client->showhelp = false;
#endif
	ent.client->showinventory = false;

#ifdef PMENU
	if (ent.client.menu.open)
		PMenu_Close(ent);
#endif

	ent.client->update_chase = true;
}

static bool PlayerSort(const entityref &a, const entityref &b)
{
	const int32_t anum = a->client->resp.score;
	const int32_t bnum = b->client->resp.score;
	return anum < bnum;
}

/*
=================
Cmd_Players_f
=================
*/
static void Cmd_Players_f(entity &ent)
{
	dynarray<entityref> index;

	for (uint32_t i = 0 ; i < game.maxclients; i++)
	{
		entity &cl = itoe(i + 1);

		if (cl.is_client() && cl.client->pers.connected)
			index.push_back(cl);
	}

	// sort by frags
	std::sort(index.begin(), index.end(), PlayerSort);

	// print information
	string large;

	for (auto &e : index)
	{
		string small = va("%3i %s\n", e->client->resp.score, e->client->pers.netname.ptr());

		if (strlen(small) + strlen(large) > 1280 - 100)
		{
			// can't print all of them in one packet
			large = strconcat(large, "...\n");
			break;
		}

		large = strconcat(large, small);
	}

	gi.cprintf(ent, PRINT_HIGH, "%s\n%u players\n", large.ptr(), index.size());
}

/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f(entity &ent)
{
	// can't wave when ducked
	if (ent.client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent.client->anim_priority > ANIM_WAVE)
		return;

	ent.client->anim_priority = ANIM_WAVE;

	switch (atoi(gi.argv(1)))
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent.s.frame = FRAME_flip01-1;
		ent.client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent.s.frame = FRAME_salute01-1;
		ent.client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent.s.frame = FRAME_taunt01-1;
		ent.client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent.s.frame = FRAME_wave01-1;
		ent.client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent.s.frame = FRAME_point01-1;
		ent.client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(entity &ent, bool team, bool arg0)
{
	if (gi.argc() < 2 && !arg0)
		return;

	team = team && ((dm_flags)dmflags & (DF_MODELTEAMS | DF_SKINTEAMS));

	string text;

	if (team)
		text = va("(%s): ", ent.client->pers.netname.ptr());
	else
		text = va("%s: ", ent.client->pers.netname.ptr());

	if (arg0)
		text = va("%s%s %s", text.ptr(), gi.argv(0), gi.args());
	else
	{
		string p = gi.args();

		if (p[0] == '"')
			p = substr(p, 1, strlen(p) - 2);

		text = va("%s%s", text.ptr(), p.ptr());
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text = va("%.150s", text.ptr());

	text = va("%s\n", text.ptr());

	if (flood_msgs)
	{
		if (level.time < ent.client->flood_locktill)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n", (int32_t)(ent.client->flood_locktill - level.time));
			return;
		}

		int32_t i = (int32_t) (ent.client->flood_whenhead - (int32_t)flood_msgs + 1);

		if (i < 0)
			i = (int32_t) (ent.client->flood_when.size() + i);

		if (ent.client->flood_when[i] && level.time - ent.client->flood_when[i] < (int32_t)flood_persecond)
		{
			ent.client->flood_locktill = level.time + (float)flood_waitdelay;
			gi.cprintf(ent, PRINT_CHAT, "You can't talk for %d more seconds.\n", (int32_t)flood_waitdelay);
			return;
		}

		ent.client->flood_whenhead = (ent.client->flood_whenhead + 1) % ent.client->flood_when.size();
		ent.client->flood_when[ent.client->flood_whenhead] = level.time;
	}

	if (dedicated)
		gi.dprintf("%s", text.ptr());

	for (size_t i = 1; i <= game.maxclients; i++)
	{
		entity &other = itoe(i);
		if (!other.inuse)
			continue;
		if (team && !OnSameTeam(ent, other))
			continue;

		gi.cprintf(other, PRINT_CHAT, "%s", text.ptr());
	}
}

static void Cmd_PlayerList_f(entity &ent)
{
	// connect time, ping, score, name
	string text;

	for (size_t i = 0; i < game.maxclients; i++)
	{
		entity &e2 = itoe(i + 1);

		if (!e2.inuse)
			continue;

		string str = va("%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2.client->resp.enterframe) / 600,
			((level.framenum - e2.client->resp.enterframe) % 600) / 10,
			e2.client->ping,
			e2.client->resp.score,
			e2.client->pers.netname.ptr(),
			e2.client->resp.spectator ? " (spectator)" : "");

		if (strlen(text) + strlen(str) > MESSAGE_LIMIT - 50)
		{
			text = strconcat(text, "And more...\n");
			break;
		}

		text = strconcat(text, str);
	}

	gi.cprintf(ent, PRINT_HIGH, "%s", text.ptr());
}

// spawn.qc
bool ED_CallSpawn(entity &ent);

static void Cmd_Spawn_f(entity &ent)
{
	if (
#ifdef SINGLE_PLAYER
		deathmatch && 
#endif
		!sv_cheats)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	entity &e = G_Spawn();
	st.classname = gi.argv(1);
	
	vector forward;
	AngleVectors(ent.client->v_angle, &forward, nullptr, nullptr);
	
	e.s.origin = ent.s.origin + (forward * 128);
	e.s.angles[YAW] = ent.s.angles[YAW];
	
	ED_CallSpawn(e);
	gi.linkentity(e);
};

#ifdef OFFHAND_HOOK
import ctf.grapple;
#endif

/*
=================
ClientCommand
=================
*/
void ClientCommand(entity &ent)
{
	if (!ent.is_client())
		return;		// not fully in game yet

	string cmd = strlwr(gi.argv(0));

	// allowed any time
	if (cmd == "players")
		return Cmd_Players_f (ent);
	else if (cmd == "say")
		Cmd_Say_f (ent, false, false);
	else if (cmd == "say_team")
		Cmd_Say_f (ent, true, false);
	else if (cmd == "score")
		Cmd_Score_f (ent);
	else if (cmd == "help")
		Cmd_Help_f (ent);
	else if (level.intermission_framenum)
		return;

	// in-game commands only
	else if (cmd == "spawn")
		Cmd_Spawn_f (ent);
	else if (cmd == "use")
		Cmd_Use_f (ent);
	else if (cmd == "drop")
		Cmd_Drop_f (ent);
	else if (cmd == "give")
		Cmd_Give_f (ent);
	else if (cmd == "god")
		Cmd_God_f (ent);
#if defined(SINGLE_PLAYER)
	else if (cmd == "notarget")
		Cmd_Notarget_f (ent);
#endif
	else if (cmd == "noclip")
		Cmd_Noclip_f (ent);
	else if (cmd == "inven")
		Cmd_Inven_f (ent);
	else if (cmd == "invnext")
		SelectNextItem (ent, (gitem_flags)-1);
	else if (cmd == "invprev")
		SelectPrevItem (ent, (gitem_flags)-1);
	else if (cmd == "invnextw")
		SelectNextItem (ent, IT_WEAPON);
	else if (cmd == "invprevw")
		SelectPrevItem (ent, IT_WEAPON);
	else if (cmd == "invnextp")
		SelectNextItem (ent, IT_POWERUP);
	else if (cmd == "invprevp")
		SelectPrevItem (ent, IT_POWERUP);
	else if (cmd == "invuse")
		Cmd_InvUse_f (ent);
	else if (cmd == "invdrop")
		Cmd_InvDrop_f (ent);
	else if (cmd == "weapprev")
		Cmd_WeapPrev_f (ent);
	else if (cmd == "weapnext")
		Cmd_WeapNext_f (ent);
	else if (cmd == "weaplast")
		Cmd_WeapLast_f (ent);
	else if (cmd == "kill")
		Cmd_Kill_f (ent);
	else if (cmd == "putaway")
		Cmd_PutAway_f (ent);
	else if (cmd == "wave")
		Cmd_Wave_f (ent);
	else if (cmd == "playerlist")
		Cmd_PlayerList_f(ent);
#ifdef CTF
	else if (cmd == "team")
		CTFTeam_f (ent);
	else if (cmd == "id")
		CTFID_f (ent);
	else if (cmd == "playerlist")
		CTFPlayerList(ent);
	else if (cmd == "observer")
		CTFObserver(ent);
#endif
#ifdef OFFHAND_HOOK
	else if (cmd == "hook")
		GrappleCmd(ent);
#endif
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}