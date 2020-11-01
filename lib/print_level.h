#pragma once

#include "types.h"

// print level
enum print_level : uint8_t
{
	// pickup messages
	PRINT_LOW,
	// death messages
	PRINT_MEDIUM,
	// critical messages
	PRINT_HIGH,
	// chat messages
	PRINT_CHAT
};
