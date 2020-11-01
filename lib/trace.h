#pragma once

#include "types.h"
#include "surface.h"

struct entity;

// a trace is returned when a box is swept through the world
struct trace
{
	trace();
	trace(const trace &tr) :
		allsolid(tr.allsolid),
		startsolid(tr.startsolid),
		fraction(tr.fraction),
		endpos(tr.endpos),
		normal(tr.normal),
		surface(tr.surface),
		contents(tr.contents),
		ent(tr.ent)
	{
	}

	trace &operator=(const trace &tr)
	{
		new(this) trace(tr);
		return *this;
	}

	// if true, plane is not valid
	qboolean		allsolid;
	// if true, the initial point was in a solid area
	qboolean		startsolid;
	// time completed, 1.0 = didn't hit anything
	float			fraction;
	// final position
	vector			endpos;
	// surface normal at impact
	vector			normal;
private:
	int32_t			plane_padding[2] = { 0, 0 };
public:
	// surface hit
	const surface	&surface;
	// contents on other side of surface hit
	content_flags	contents;
	// entity hit
	entity			&ent;
};