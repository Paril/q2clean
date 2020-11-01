#include "string.h"
#include "gi.h"

string va(stringlit fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	string s = va(fmt, argptr);
	va_end(argptr);
	return s;
}

static char *format_buffer = nullptr;
static size_t format_size = 0;

string va(stringlit fmt, va_list argptr)
{
	if (!format_buffer)
	{
		format_size = 2048;
		format_buffer = gi.TagMalloc<char>(format_size + 1, TAG_GAME);
	}

	const size_t needed_to_write = vsnprintf(format_buffer, format_size, fmt, argptr) + 1;

	if (needed_to_write > format_size)
	{
		gi.TagFree(format_buffer);

		if (format_size * 2 < needed_to_write)
			format_size = needed_to_write;
		else
			format_size *= 2;
		format_buffer = gi.TagMalloc<char>(format_size + 1, TAG_GAME);

		vsnprintf(format_buffer, format_size, fmt, argptr);
	}

	return format_buffer;
}