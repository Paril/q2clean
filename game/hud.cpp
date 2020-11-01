#include "../lib/types.h"
#include "../lib/entity.h"
#include "../lib/dynarray.h"
#include "../lib/gi.h"
#include "game.h"
#include "player.h"
#include "util.h"
#include "itemlist.h"
#include <algorithm>

void DeathmatchScoreboardMessage(entity &ent, entityref killer)
{
#ifdef CTF
	if (ctf.intVal)
	{
		CTFScoreboardMessage(ent, killer);
		return;
	}
#endif

	string entry;
	string str;
	size_t stringlength;
	dynarray<entityref> sorted;
	int	x, y;
	uint32_t j;
	stringlit tag;

	// sort the clients by score
	for (uint32_t i = 0 ; i < game.maxclients; i++)
	{
		entity &cl_ent = itoe(1 + i);

		if (!cl_ent.inuse || cl_ent.client->g.resp.spectator)
			continue;

		sorted.push_back(cl_ent);
	}

	// print level name and exit rules
	stringlength = 0;

	std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b)
	{
		return b->client->g.resp.score < a->client->g.resp.score;
	});

	// add the clients in sorted order
	if (sorted.size() > 12)
		sorted.resize(12);

	size_t i = 0;

	for (auto &cl_ent : sorted)
	{
		if (i >= 6)
			x = 160;
		else
			x = 0;

		y = 32 + 32 * (i % 6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = nullptr;

		if (tag)
		{
			entry = va("xv %i yv %i picn %s ", x + 32, y, tag);
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			str = strconcat(str, entry);
			stringlength += j;
		}

		// send the layout
		entry = va("client %i %i %i %i %i %i ", x, y, sorted[i]->s.number - 1, cl_ent->client->g.resp.score, cl_ent->client->ping, ((level.framenum - cl_ent->client->g.resp.enterframe) / 600));
		j = strlen(entry);

		if (stringlength + j > 1024)
			break;

		str = strconcat(str, entry);
		stringlength += j;

		i++;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(str);
}

/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission(entity &ent)
{
#ifdef SINGLE_PLAYER
	if (deathmatch.intVal || coop.intVal)
#endif
		ent.client->g.showscores = true;
	ent.s.origin = level.intermission_origin;
	ent.client->ps.pmove.set_origin(level.intermission_origin);
	ent.client->ps.viewangles = level.intermission_angle;
	ent.client->ps.pmove.pm_type = PM_FREEZE;
	ent.client->ps.gunindex = MODEL_NONE;
	ent.client->ps.blend[3] = 0;
	ent.client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent.client->g.quad_framenum = 0;
	ent.client->g.invincible_framenum = 0;
	ent.client->g.breather_framenum = 0;
	ent.client->g.enviro_framenum = 0;
	ent.client->g.grenade_blew_up = false;
	ent.client->g.grenade_framenum = 0;

#ifdef THE_RECKONING
	ent.client.quadfire_framenum = 0;
	ent.client.trap_blew_up = false;
	ent.client.trap_framenum = 0;
#endif

#ifdef GROUND_ZERO
	ent.client.ps.rdflags &= ~RDF_IRGOGGLES;		// PGM
	ent.client.ir_framenum = 0;					// PGM
	ent.client.nuke_framenum = 0;					// PMM
	ent.client.double_framenum = 0;				// PMM
#endif

	ent.g.viewheight = 0;
	ent.s.modelindex = MODEL_NONE;
	ent.s.modelindex2 = MODEL_NONE;
	ent.s.modelindex3 = MODEL_NONE;
	ent.s.modelindex4 = MODEL_NONE;
	ent.s.effects = EF_NONE;
	ent.s.sound = SOUND_NONE;
	ent.solid = SOLID_NOT;

	gi.linkentity(ent);

	// add the layout
#ifdef SINGLE_PLAYER
	if (deathmatch.intVal || coop.intVal)
	{
#endif
		DeathmatchScoreboardMessage(ent, world);
		gi.unicast(ent, true);
#ifdef SINGLE_PLAYER
	}
#endif
}

void BeginIntermission(entity &targ)
{
	if (level.intermission_framenum)
		return;     // already activated

#ifdef CTF
	if (ctf.intVal)
		CTFCalcScores();
#endif

#ifdef SINGLE_PLAYER
	game.autosaved = false;
#endif

	// respawn any dead clients
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		entity &cl = itoe(1 + i);
		if (!cl.inuse)
			continue;
		if (cl.g.health <= 0)
			respawn(cl);
	}

	level.intermission_framenum = level.framenum;
	level.changemap = targ.g.map;

#ifdef SINGLE_PLAYER
	if (strstr(level.changemap, "*") != -1)
	{
		if (coop.intVal)
		{
			for (int i = 0 ; i < game.maxclients ; i++)
			{
				cl = itoe(1 + i);
				if (!cl.inuse)
					continue;
					
				gitem_t *it = itemlist;
				// strip players of all keys between units
				for (int n = 0; n < itemlist.length; n++, it++)
					if (it->flags & IT_KEY)
						cl.client.pers.inventory[n] = 0;
			}
		}
	}
	else
	{
		if (!deathmatch.intVal)
		{
			level.exitintermission = 1;     // go immediately to the next level
			return;
		}
	}
#endif

	level.exitintermission = 0;

	// find an intermission spot
	entityref ent = G_FindEquals(world, g.type, ET_INFO_PLAYER_INTERMISSION);

	if (!ent.has_value())
	{
		// the map creator forgot to put in an intermission point...
		ent = G_FindEquals(world, g.type, ET_INFO_PLAYER_START);

		if (!ent.has_value())
			ent = G_FindEquals(world, g.type, ET_INFO_PLAYER_DEATHMATCH);
	}
	else
	{
		// choose one of four spots
		int32_t i = Q_rand() & 3;
		while (i--)
		{
			ent = G_FindEquals(ent, g.type, ET_INFO_PLAYER_INTERMISSION);
			if (!ent.has_value())   // wrap around the list
				ent = G_FindEquals(ent, g.type, ET_INFO_PLAYER_INTERMISSION);
		}
	}

	level.intermission_origin = ent->s.origin;
	level.intermission_angle = ent->s.angles;

	// move all clients to the intermission point
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		entity &cl = itoe(1 + i);
		if (cl.inuse)
			MoveClientToIntermission(cl);
	}
}

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the MESSAGE_LIMIT!
==================
*/
static void DeathmatchScoreboard(entity &ent)
{
	DeathmatchScoreboardMessage(ent, ent.g.enemy);
	gi.unicast(ent, true);
}

void Cmd_Score_f(entity &ent)
{
	ent.client->g.showinventory = false;

#ifdef PMENU
	if (ent.client.menu.open)
		PMenu_Close(ent);
#endif

#ifdef SINGLE_PLAYER
	ent.client.showhelp = false;

	if (!deathmatch.intVal && !coop.intVal)
		return;
#endif

	if (ent.client->g.showscores)
	{
		ent.client->g.showscores = false;
		return;
	}

	ent.client->g.showscores = true;
	DeathmatchScoreboard(ent);
}

#ifdef SINGLE_PLAYER
/*
==================
HelpComputer

Draw help computer.
==================
*/
static void(entity ent) HelpComputer =
{
	string	str;
	string	sk;

	if (skill.intVal == 0)
		sk = "easy";
	else if (skill.intVal == 1)
		sk = "medium";
	else if (skill.intVal == 2)
		sk = "hard";
	else
		sk = "hard+";

	// send the layout
	str = va(	"xv 32 yv 8 picn help "             // background
			"xv 202 yv 12 string2 \"%s\" "      // skill
			"xv 0 yv 24 cstring2 \"%s\" "       // level name
			"xv 0 yv 54 cstring2 \"%s\" "       // help 1
			"xv 0 yv 110 cstring2 \"%s\" ",     // help 2
			sk,
			level.level_name,
			game.helpmessage1,
			game.helpmessage2);
	
	str = va(	"%sxv 50 yv 164 string2 \" kills     goals    secrets\" "
			"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ",
			str,
			level.killed_monsters, level.total_monsters,
			level.found_goals, level.total_goals,
			level.found_secrets, level.total_secrets);

	gi.WriteByte(svc_layout);
	gi.WriteString(str);
	gi.unicast(ent, true);
}
#endif

void Cmd_Help_f(entity &ent)
{
#ifdef SINGLE_PLAYER
	// this is for backwards compatability
	if (deathmatch.intVal)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent.client.showinventory = false;
	ent.client.showscores = false;

	if (ent.client.showhelp && (ent.client.pers.game_helpchanged == game.helpchanged)) {
		ent.client.showhelp = false;
		return;
	}

	ent.client.showhelp = true;
	ent.client.pers.helpchanged = 0;
	HelpComputer(ent);
#else
	Cmd_Score_f(ent);
#endif
}

//=======================================================================

void G_SetStats(entity &ent)
{
	//
	// health
	//
	ent.client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent.client->ps.stats[STAT_HEALTH] = ent.g.health;

	//
	// ammo
	//
	if (!ent.client->g.ammo_index)
	{
		ent.client->ps.stats[STAT_AMMO_ICON] = 0;
		ent.client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		const gitem_t &it = GetItemByIndex(ent.client->g.ammo_index);
		ent.client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(it.icon);
		ent.client->ps.stats[STAT_AMMO] = ent.client->g.pers.inventory[it.id];
	}

	//
	// armor
	//
	gitem_id power_armor_type = PowerArmorType(ent);
	int32_t cells = 0;

	if (power_armor_type)
	{
		cells = ent.client->g.pers.inventory[ITEM_CELLS];
		
		if (!cells)
		{
			// ran out of cells for power armor
			ent.g.flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = ITEM_NONE;
		}
	}

	gitem_id index = ArmorIndex(ent);
	if (power_armor_type && (!index || (level.framenum & 8)))
	{
		// flash between power armor and other armor icon
		// Knightmare- use correct icon for power screen
		if (power_armor_type == ITEM_POWER_SHIELD)
			ent.client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex("i_powershield");
		else	// POWER_ARMOR_SCREEN
			ent.client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex("i_powerscreen");
		ent.client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		const gitem_t &it = GetItemByIndex(index);
		ent.client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(it.icon);
		ent.client->ps.stats[STAT_ARMOR] = ent.client->g.pers.inventory[index];
	}
	else
	{
		ent.client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent.client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.framenum > ent.client->g.pickup_msg_framenum)
	{
		ent.client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent.client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent.client->g.quad_framenum > level.framenum)
	{
		ent.client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_quad");
		ent.client->ps.stats[STAT_TIMER] = (ent.client->g.quad_framenum - level.framenum) / 10;
	}
#ifdef THE_RECKONING
	// RAFAEL
	else if (ent.client.quadfire_framenum > level.framenum)
	{
		ent.client.ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quadfire");
		ent.client.ps.stats[STAT_TIMER] = (ent.client.quadfire_framenum - level.framenum)/10;
	}
#endif
#ifdef GROUND_ZERO
	else if (ent.client.double_framenum > level.framenum)
	{
		ent.client.ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_double");
		ent.client.ps.stats[STAT_TIMER] = (ent.client.double_framenum - level.framenum)/10;
	}
#endif
	else if (ent.client->g.invincible_framenum > level.framenum)
	{
		ent.client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_invulnerability");
		ent.client->ps.stats[STAT_TIMER] = (ent.client->g.invincible_framenum - level.framenum) / 10;
	}
	else if (ent.client->g.enviro_framenum > level.framenum)
	{
		ent.client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_envirosuit");
		ent.client->ps.stats[STAT_TIMER] = (ent.client->g.enviro_framenum - level.framenum) / 10;
	}
	else if (ent.client->g.breather_framenum > level.framenum)
	{
		ent.client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_rebreather");
		ent.client->ps.stats[STAT_TIMER] = (ent.client->g.breather_framenum - level.framenum) / 10;
	}
#ifdef GROUND_ZERO
	else if (ent.client.ir_framenum > level.framenum)
	{
		ent.client.ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_ir");
		ent.client.ps.stats[STAT_TIMER] = (ent.client.ir_framenum - level.framenum)/10;
	}
#endif
	else
	{
		ent.client->ps.stats[STAT_TIMER_ICON] = 0;
		ent.client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (!ent.client->g.pers.selected_item)
		ent.client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent.client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(GetItemByIndex(ent.client->g.pers.selected_item).icon);

	ent.client->ps.stats[STAT_SELECTED_ITEM] = ent.client->g.pers.selected_item;

	//
	// layouts
	//
	ent.client->ps.stats[STAT_LAYOUTS] = 0;
#ifdef SINGLE_PLAYER

	if (deathmatch.intVal)
	{
#endif
		if (ent.g.health <= 0 || level.intermission_framenum || ent.client->g.showscores)
			ent.client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent.client->g.showinventory && ent.g.health > 0)
			ent.client->ps.stats[STAT_LAYOUTS] |= 2;
#ifdef SINGLE_PLAYER
	}
	else
	{
		if (ent.client.showscores || ent.client.showhelp)
			ent.client.ps.stats[STAT_LAYOUTS] |= 1;
		if (ent.client.showinventory && ent.client.pers.health > 0)
			ent.client.ps.stats[STAT_LAYOUTS] |= 2;
	}
#endif

	//
	// frags
	//
	ent.client->ps.stats[STAT_FRAGS] = ent.client->g.resp.score;

	//
	// help icon / current weapon if not shown
	//
#ifdef SINGLE_PLAYER
	if (ent.client.pers.helpchanged && (level.framenum & 8))
		ent.client.ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
	else
#endif
	if ((ent.client->g.pers.hand == CENTER_HANDED || ent.client->ps.fov > 91.f) && ent.client->g.pers.weapon)
		ent.client->ps.stats[STAT_HELPICON] = gi.imageindex(ent.client->g.pers.weapon->icon);
	else
		ent.client->ps.stats[STAT_HELPICON] = 0;

	ent.client->ps.stats[STAT_SPECTATOR] = 0;

#ifdef CTF
	SetCTFStats(ent);
#endif
}

void G_SetSpectatorStats(entity &ent)
{
	if (!ent.client->g.chase_target.has_value())
		G_SetStats(ent);

	ent.client->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	ent.client->ps.stats[STAT_LAYOUTS] = 0;
	if (ent.g.health <= 0 || level.intermission_framenum || ent.client->g.showscores)
		ent.client->ps.stats[STAT_LAYOUTS] |= 1;
	if (ent.client->g.showinventory && ent.g.health > 0)
		ent.client->ps.stats[STAT_LAYOUTS] |= 2;

	if (ent.client->g.chase_target.has_value() && ent.client->g.chase_target->inuse)
		ent.client->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + (ent.client->g.chase_target->s.number - 1);
	else
		ent.client->ps.stats[STAT_CHASE] = 0;
}

void G_CheckChaseStats(entity &ent)
{
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		entity &cl = itoe(i);
		if (!cl.inuse || cl.client->g.chase_target != ent)
			continue;
		cl.client->ps.stats = ent.client->ps.stats;
		G_SetSpectatorStats(cl);
	}
}
