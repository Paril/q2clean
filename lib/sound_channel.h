#pragma once

#include "types.h"

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
enum sound_channel : uint8_t
{
	CHAN_AUTO,
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	// 3 unused IDs

	// bitflags; can be added/OR'd with other channels

	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
	CHAN_NO_PHS_ADD	= 1 << 3,
	// send by reliable message, not datagram
	CHAN_RELIABLE	= 1 << 4
};

MAKE_ENUM_BITWISE(sound_channel);