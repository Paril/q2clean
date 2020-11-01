#pragma once

#include "../../lib/types.h"
#include "../../lib/dynarray.h"

using node_id = uint32_t;

constexpr node_id NODE_INVALID = (node_id)-1;

// links types (movetypes required to run node links)
enum ai_link_type : uint16_t
{
	LINK_NONE,
	LINK_MOVE = 1 << 0,
	LINK_STAIRS = 1 << 1,
	LINK_FALL = 1 << 2,
	LINK_CLIMB = 1 << 3,
	LINK_TELEPORT = 1 << 4,
	LINK_PLATFORM = 1 << 5,
	LINK_JUMPPAD = 1 << 6,
	LINK_WATER = 1 << 7,
	LINK_WATERJUMP = 1 << 8,
	LINK_LADDER = 1 << 9,
	LINK_JUMP = 1 << 10,
	LINK_CROUCH = 1 << 11,

	LINK_INVALID = 1 << 12,
	
	DEFAULT_MOVETYPES_MASK = LINK_MOVE | LINK_STAIRS | LINK_FALL | LINK_WATER | LINK_WATERJUMP | LINK_JUMPPAD | LINK_PLATFORM | LINK_TELEPORT
};

MAKE_ENUM_BITWISE(ai_link_type);

enum node_flags : uint16_t
{
	// node flags
	NODEFLAGS_NONE,
	NODEFLAGS_WATER = 1 << 0,
	NODEFLAGS_LADDER = 1 << 1,
	NODEFLAGS_SERVERLINK = 1 << 2,	//plats, doors, teles. Only server can link 2 nodes with this flag
	NODEFLAGS_FLOAT = 1 << 3,	//don't drop node to floor ( air & water )
	NODEFLAGS_ITEM = 1 << 4,	 // dummied/unused
	NODEFLAGS_BOTROAM = 1 << 5,
	NODEFLAGS_JUMPPAD = 1 << 6,	// jalfixme: add NODEFLAGS_REACHATTOUCH
	NODEFLAGS_JUMPPAD_LAND = 1 << 7,
	NODEFLAGS_PLATFORM = 1 << 8,
	NODEFLAGS_TELEPORTER_IN = 1 << 9,	// jalfixme: add NODEFLAGS_REACHATTOUCH
	NODEFLAGS_TELEPORTER_OUT = 1 << 10,
	NODEFLAGS_REACHATTOUCH = 1 << 11,

	// special value which acts as (-1) & ~LADDER
	NODE_ALL = NODEFLAGS_NONE
};

MAKE_ENUM_BITWISE(node_flags);

typedef struct
{
	node_id			node;
	uint32_t		dist;
	ai_link_type	moveType;
} nav_link;

typedef struct nav_node_s
{
	vector				origin;
	node_flags			flags;
	dynarray<nav_link>	links;
} nav_node;

struct astar_path
{
	node_id	originNode;
	node_id	goalNode;

	dynarray<node_id>	nodes;
};

bool AStar_ResolvePath(node_id n1, node_id n2, ai_link_type movetypes);

bool AStar_GetPath(node_id origin, node_id goal, ai_link_type movetypes, astar_path &path);
