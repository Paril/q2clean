#pragma once

#include "types.h"

// destination class for gi.multicast()
enum multicast_destination : uint8_t
{
	// send to all
	MULTICAST_ALL,
	// send only if inside phs
	MULTICAST_PHS,
	// send only if inside pvs
	MULTICAST_PVS,

	// reliable version of all
	MULTICAST_ALL_R,
	// reliable version of phs
	MULTICAST_PHS_R,
	// reliable version of pvs
	MULTICAST_PVS_R
};