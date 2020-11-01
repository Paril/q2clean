#include "surface.h"

/*static*/ const surface null_surface;

surface::surface() :
	name({ '\0' }),
	flags(SURF_NONE),
	value(0)
{
}