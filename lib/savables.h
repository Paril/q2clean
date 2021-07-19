#pragma once

import save;

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_FUNCTION(func) \
	registered_save_function<decltype(&func)> func##_savable = { #func, func };

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_FUNCTION(func) \
	extern registered_save_function<decltype(&func)> func##_savable;

// Register a function as savable; this must be done in a source file.
#define REGISTER_SAVABLE_DATA(data) \
	registered_save_data<decltype(data)> data##_savable = { #data, data };

// Forward-declare a function as savable; this is done in headers, generally, to
// expose a _savable to outside of its source file.
#define DECLARE_SAVABLE_DATA(func) \
	extern registered_save_data<decltype(data)> data##_savable;