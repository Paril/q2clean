#pragma once

#include "items.h"

import gi;
import string;
import items.list;

// handedness values
enum handedness : uint8_t
{
	RIGHT_HANDED,
	LEFT_HANDED,
	CENTER_HANDED
};

// client data that stays across multiple level loads
struct client_persistant
{
	string		userinfo;
	string		netname;
	handedness	hand;
	
	bool	connected = false;	// a loadgame will leave valid entities that
								// just don't have a connection yet
	
	gitem_id	selected_item;
	array<int32_t, ITEM_TOTAL>	inventory;
	
	// ammo capacities
	array<int32_t, AMMO_TOTAL>	max_ammo;
	
	itemref	weapon;
	itemref	lastweapon;

#ifdef SINGLE_PLAYER
	// values saved and restored from edicts when changing levels

	int32_t			health;
	int32_t			max_health;
	int32_t			power_cubes;    // used for tracking the cubes in coop games
	entity_flags	savedFlags;
	int32_t			helpchanged;
	int32_t			game_helpchanged;
	int32_t			score;          // for calculating total unit score in coop games
#endif
	
	bool	spectator;      // client is a spectator
};

#ifdef CTF
typedef enum : int
{
	CTF_NOTEAM,
	CTF_TEAM1,
	CTF_TEAM2
} ctfteam_t;
#endif

// client data that stays across deathmatch respawns
struct client_respawn
{
#ifdef SINGLE_PLAYER
	client_persistant coop_respawn;   // what to set client.pers to on a respawn
#endif

	gtime	enterframe;		// level.framenum the client entered the game
	int32_t	score;			// frags, etc
	vector	cmd_angles;		// angles sent over in the last command

	bool	spectator;		// client is a spectator

#ifdef CTF
	ctfteam_t	ctf_team;			// CTF team
	int		ctf_state;
	int		ctf_lasthurtcarrierframe;
	int		ctf_lastreturnedflagframe;
	int		ctf_flagsinceframe;
	int		ctf_lastfraggedcarrierframe;
	bool	id_state;
	int		lastidframe;
#endif
};

enum weapon_state : uint8_t
{
	WEAPON_READY,
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
};

// client_t.anim_priority
enum anim_priority : uint8_t
{
	ANIM_BASIC,		// stand / run
	ANIM_WAVE,
	ANIM_JUMP,
	ANIM_PAIN,
	ANIM_ATTACK,
	ANIM_DEATH,
	ANIM_REVERSE
};

#ifdef PMENU
typedef enum : int
{
	PMENU_ALIGN_LEFT,
	PMENU_ALIGN_CENTER,
	PMENU_ALIGN_RIGHT
} pmenu_align_t;

typedef void(entity) pmenu_select_func;

struct pmenu_t
{
	string				text;
	pmenu_align_t		align;
	pmenu_select_func	func;
};

struct pmenuhnd_t
{
	bool		open;
	pmenu_t		entries[18];
	int			cur;
	int			num;
	void		*arg;
};
#endif

#ifdef HOOK_CODE
typedef enum : int
{
	GRAPPLE_STATE_FLY,
	GRAPPLE_STATE_PULL,
	GRAPPLE_STATE_HANG
} grapplestate_t;
#endif

// the gclient is game-local client data, stored under every
// client's "client" data.
struct client : public server_client
{
	client_persistant	pers;
	client_respawn		resp;
	pmove_state			old_pmove;  // for detecting out-of-pmove changes
	
	bool	showscores;         // set layout stat
	bool	showinventory;      // set layout stat
#ifdef SINGLE_PLAYER
	bool	showhelp;
	bool	showhelpicon;
#endif

	gitem_id	ammo_index;
	
	button_bits	buttons;
	button_bits	oldbuttons;
	button_bits	latched_buttons;
	
	bool	weapon_thunk;

	itemref	newweapon;
	
	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int32_t	damage_armor;			// damage absorbed by armor
	int32_t	damage_parmor;			// damage absorbed by power armor
	int32_t	damage_blood;			// damage taken out of health
	int32_t	damage_knockback;		// impact damage
	vector	damage_from;	// origin for vector calculation
	
	float	killer_yaw;				// when dead, look at killer
	
	weapon_state	weaponstate;

	vector	kick_angles;    // weapon kicks
	vector	kick_origin;
	float	v_dmg_roll, v_dmg_pitch, v_dmg_time;    // damage kicks
	float	fall_time, fall_value;      // for view drop on fall
	float	bonus_alpha;
	vector	damage_blend;
	float	damage_alpha;
	vector	v_angle;	// aiming direction
	float	bobtime;			// so off-ground doesn't change it
	vector	oldviewangles;
	vector	oldvelocity;
	
	gtime	next_drown_framenum;
	int32_t	old_waterlevel;
	bool	breather_sound;
	
	int32_t	machinegun_shots;   // for weapon raising
	
	// animation vars
	size_t			anim_end;
	anim_priority	anim_priority;
	bool			anim_duck;
	bool			anim_run;
	
	// powerup timers
	gtime	quad_framenum;
	gtime	invincible_framenum;
	gtime	breather_framenum;
	gtime	enviro_framenum;
	
	bool	grenade_blew_up;
	gtime	grenade_framenum;

	int32_t	silencer_shots;
	sound_index	weapon_sound;
	
	gtime	pickup_msg_framenum;
	
	float				flood_locktill;     // locked from talking
	array<float, 10>	flood_when;     // when messages were said
	size_t				flood_whenhead;     // head pointer for when said
	
	gtime	respawn_framenum;   // can respawn when time > this
	
	entityref	chase_target;      // player we are chasing
	bool		update_chase;       // need to update chase info?

#ifdef THE_RECKONING
	gtime	quadfire_framenum;
	gtime	trap_framenum;
	bool	trap_blew_up;
#endif

#ifdef GROUND_ZERO
	gtime		double_framenum;
	gtime		ir_framenum;
	gtime		nuke_framenum;
	gtime		tracker_pain_framenum;
#endif

#ifdef PMENU
	pmenuhnd_t	menu;
#endif

#ifdef HOOK_CODE
	entityref		grapple;	// entity of grapple
	grapplestate_t	grapplestate;
	gtime			grapplereleaseframenum;	// time of grapple release
#endif

#ifdef CTF
	gtime	ctf_regenframenum;		// regen tech
	gtime	ctf_techsndframenum;
	gtime	ctf_lasttechframenum;
#endif
};