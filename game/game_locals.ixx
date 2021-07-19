module;

#include "lib/types.h"

export module game_locals;

import string;

// game.serverflags values
export enum cross_server_flags : uint8_t
{
	SFL_CROSS_TRIGGER_1 = 1 << 0,
	SFL_CROSS_TRIGGER_2 = 1 << 1,
	SFL_CROSS_TRIGGER_3 = 1 << 2,
	SFL_CROSS_TRIGGER_4 = 1 << 3,
	SFL_CROSS_TRIGGER_5 = 1 << 4,
	SFL_CROSS_TRIGGER_6 = 1 << 5,
	SFL_CROSS_TRIGGER_7 = 1 << 6,
	SFL_CROSS_TRIGGER_8 = 1 << 7,

	SFL_CROSS_TRIGGER_MASK = 0xff
};

MAKE_ENUM_BITWISE(cross_server_flags, export);

export struct game_locals
{
#ifdef SINGLE_PLAYER
	string	helpmessage1;
	string	helpmessage2;
	// flash F1 icon if non 0, play sound; increment only if 1, 2, or 3
	uint8_t	helpchanged;

	// cross level triggers
	cross_server_flags	serverflags;

	bool	autosaved;
#endif

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore.
	// needed for coop respawns
	string		spawnpoint;

	// store latched cvars here that we want to get at often
	uint32_t	maxclients;
};

export game_locals game;