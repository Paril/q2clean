#pragma once

#include "../../lib/types.h"
#include "../../lib/map.h"
#include "../../lib/cvar.h"
#include "../itemlist.h"
#include "astar.h"

struct ai_status
{
	bool	jumpadReached;
	bool	TeleportReached;

	array<float, ITEM_TOTAL>	inventoryWeights;
	map<int32_t, float>			playersWeights;
	map<uint32_t, gtime>		broam_timeout_framenums;	//revisit bot roams
};

constexpr int MAX_BOT_SKILL = 5;		//skill levels graduation

using ai_func = void(*)(entity &);

struct ai_pers
{
	int32_t			skillLevel;			// Affects AIM and fire rate
	ai_link_type	moveTypesMask;		// bot can perform these moves, to check against required moves for a given path

	array<float, ITEM_TOTAL>	inventoryWeights;

	//class based functions
	ai_func	UpdateStatus;
	ai_func	RunFrame;
	ai_func	bloquedTimeout;
	ai_func	deadFrame;
};

enum ai_state : uint8_t
{
	// Bot state types
	BOT_STATE_STAND,
	BOT_STATE_MOVE,
	BOT_STATE_WANDER,
	BOT_STATE_ATTACK,
	BOT_STATE_DEFEND
};

struct ai_t
{
	ai_pers		pers;			//persistant definition (class?)
	ai_status	status;			//player (bot, NPC) status for AI frame

	//NPC state
	ai_state	state;			// Bot State (WANDER, MOVE, etc)
	gtime		state_combat_timeout_framenum;

	bool	is_bot;
	bool	is_swim;
	bool	is_step;
	bool	is_ladder;
	bool	was_swim;
	bool	was_step;

	// movement
	vector	move_vector;
	gtime	next_move_framenum;
	gtime	wander_timeout_framenum;
	gtime	bloqued_timeout_framenum;
	gtime	changeweapon_timeout_framenum;

	// nodes
	node_id		current_node;
	node_id		goal_node;
	node_id		next_node;
	uint32_t	node_timeout;

	uint32_t	tries;
	astar_path	path;

	node_id	path_position;
	int32_t	nearest_node_tries;	//for increasing radius of search with each try
};

constexpr content_flags MASK_NODESOLID	= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW);
constexpr content_flags MASK_AISOLID	= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTER|CONTENTS_DEADMONSTER|CONTENTS_MONSTERCLIP);

constexpr int32_t AI_STEPSIZE			= 18;
constexpr int32_t AI_JUMPABLE_HEIGHT	= 34;
constexpr int32_t AI_JUMPABLE_DISTANCE	= 140;

constexpr int32_t NODE_DENSITY		= 128;			// Density setting for nodes
constexpr float AI_GOAL_SR_RADIUS	= 200;

constexpr int32_t NAV_FILE_VERSION		= 11;
constexpr stringlit NAV_FILE_EXTENSION	= "nav";
constexpr stringlit AI_NODES_FOLDER		= "navigation";

struct nav_ents
{
	entityref	ent;
	node_id		node;
};

struct nav_item
{
	gitem_id	item;
	entityref	ent;
	node_id		node;
};

struct nav_broam
{
	node_id	node;
	float	weight;
};

struct nav_data
{
	bool	loaded;
	
	dynarray<nav_item>	items; //keeps track of items related to nodes, type nav_item_t

	dynarray<nav_ents>	ents; //plats, etc, type nav_ents_t

	dynarray<nav_broam>	broams;	//list of nodes wich are botroams, type nav_broam_t

	dynarray<nav_node> nodes;
};

extern nav_data nav;

extern cvarref bot_showpath;
extern cvarref bot_showcombat;
extern cvarref bot_showsrgoal;
extern cvarref bot_showlrgoal;

struct ai_devel
{
	bool		debugMode;

	bool		showPLinks;
	entityref	plinkguy;

	bool		debugChased;
	entityref	chaseguy;
};

extern ai_devel AIDevel;

void BOT_DMclass_InitPersistant(entity &self);
