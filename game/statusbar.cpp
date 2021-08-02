#include "config.h"
#include "lib/string.h"
#include "lib/string/format.h"
#include "lib/gi.h"
#include "game/game.h"
#include "statusbar.h"

enum class anchor
{
	none,

	left = 1,
	top = 1,
	right = 2,
	bottom = 2,
	virt = 3
};

struct axis
{
	anchor	anchor = anchor::none;
	int32_t	offset = 0;

	inline bool operator==(const axis &a) const { return anchor == a.anchor && offset == a.offset; }
	inline bool operator!=(const axis &a) const { return !(*this == a); }
};

// simple struct to handle dynamic construction of a statusbar
struct statusbar
{
private:
	string	strval;

	bool		in_conditional = false;

	inline void append(const stringref &ref)
	{
		strval = strconcat(strval, " ", ref);
	}

	axis	last_vertical = {};
	axis	last_horizontal = {};

	inline void write_axis(axis &current)
	{
		if (&current == &horizontal && current == last_horizontal)
			return;
		else if (&current == &vertical && current == last_vertical)
			return;

		if (current.anchor == anchor::none)
			gi.error("Invalid statusbar format: invalid position");

		constexpr stringlit x[] = { "xl", "xr", "xv" }, y[] = { "yt", "yb", "yv" };

		append(va("%s %i", ((&current == &horizontal) ? x : y)[((int32_t) current.anchor) - 1], current.offset));

		if (&current == &horizontal)
			last_horizontal = current;
		else
			last_vertical = current;
	}

	inline void write_position()
	{
		write_axis(horizontal);
		write_axis(vertical);
	}

	axis	backup_vertical, backup_last_vertical, backup_horizontal, backup_last_horizontal;

public:
	axis	vertical;
	axis	horizontal;

	inline stringref str()
	{
		if (in_conditional)
			gi.error("Invalid statusbar format: conditional not finished");

		return strval;
	}

	inline void if_stat(const stat_index &stat)
	{
		if (in_conditional)
			gi.error("Invalid statusbar format: nested conditionals not supported");

		in_conditional = true;
		append(va("if %i", stat));

		backup_vertical = vertical;
		backup_horizontal = horizontal;
		backup_last_vertical = last_vertical;
		backup_last_horizontal = last_horizontal;
	}

	inline void endif_stat()
	{
		if (!in_conditional)
			gi.error("Invalid statusbar format: not in a conditional");

		in_conditional = false;
		append("endif");

		vertical = backup_vertical;
		horizontal = backup_horizontal;
		last_vertical = backup_last_vertical;
		last_horizontal = backup_last_horizontal;
	}

	inline void hnum()
	{
		write_position();
		append("hnum");
	}

	inline void anum()
	{
		write_position();
		append("anum");
	}

	inline void rnum()
	{
		write_position();
		append("rnum");
	}

	inline void pic(const stat_index &stat)
	{
		write_position();
		append(va("pic %i", stat));
	}

	inline void num(const stat_index &stat, const int32_t &width)
	{
		write_position();
		append(va("num %i %i", width, stat));
	}

	inline void picn(const stringref &str)
	{
		write_position();
		append(va("picn %s", str.ptr()));
	}

	inline void stat_string(const stat_index &stat)
	{
		write_position();
		append(va("stat_string %i", stat));
	}

	inline void string(const stringref &str)
	{
		write_position();
		append(va("string \"%s\"", str.ptr()));
	}

	inline void string2(const stringref &str)
	{
		write_position();
		append(va("string2 \"%s\"", str.ptr()));
	}

	inline void cstring(const stringref &str)
	{
		write_position();
		append(va("cstring \"%s\"", str.ptr()));
	}

	inline void cstring2(const stringref &str)
	{
		write_position();
		append(va("cstring2 \"%s\"", str.ptr()));
	}
};

inline statusbar compile_statusbar()
{
	statusbar sb;

	sb.vertical = { anchor::bottom, -24 };

	// health
	sb.horizontal.anchor = { anchor::virt };
	sb.hnum();
	sb.horizontal.offset = 50;
	sb.pic(STAT_HEALTH_ICON);

	// ammo
	sb.if_stat(STAT_AMMO_ICON);
	sb.horizontal.offset = 100;
	sb.anum();
	sb.horizontal.offset = 150;
	sb.pic(STAT_AMMO_ICON);
	sb.endif_stat();

	// armor
	sb.if_stat(STAT_ARMOR_ICON);
	sb.horizontal.offset = 200;
	sb.rnum();
	sb.horizontal.offset = 250;
	sb.pic(STAT_ARMOR_ICON);
	sb.endif_stat();

	// selected item
	sb.if_stat(STAT_SELECTED_ICON);
	sb.horizontal.offset = 296;
	sb.pic(STAT_SELECTED_ICON);
	sb.endif_stat();

	sb.vertical.offset = -50;

	// picked up item
	sb.if_stat(STAT_PICKUP_ICON);
	sb.horizontal.offset = 0;
	sb.pic(STAT_PICKUP_ICON);
	sb.vertical.offset = -42;
	sb.horizontal.offset = 26;
	sb.stat_string(STAT_PICKUP_STRING);
	sb.endif_stat();

	// timer
	sb.if_stat(STAT_TIMER_ICON);
	sb.horizontal.offset = 246;
	sb.num(STAT_TIMER, 2);
	sb.horizontal.offset = 296;
	sb.pic(STAT_TIMER_ICON);
	sb.endif_stat();

	// help/weapon icon
	sb.if_stat(STAT_HELPICON);
	sb.horizontal.offset = 150;
	sb.pic(STAT_HELPICON);
	sb.endif_stat();

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		// frags
		sb.horizontal = { anchor::right, -50 };
		sb.vertical = { anchor::top, 2 };
		sb.num(STAT_FRAGS, 3);

		// spectator
		sb.if_stat(STAT_SPECTATOR);
		sb.horizontal = { anchor::virt };
		sb.vertical = { anchor::bottom, -58 };
		sb.string2("SPECTATOR MODE");
		sb.endif_stat();

		// chase camera
		sb.if_stat(STAT_CHASE);
		sb.horizontal = { anchor::virt };
		sb.vertical = { anchor::bottom, -68 };
		sb.string("Chasing");
		sb.horizontal = { anchor::virt, 64 };
		sb.stat_string(STAT_CHASE);
		sb.endif_stat();
#ifdef SINGLE_PLAYER
	}
#endif

	return sb;
}

stringref G_GetStatusBar()
{
	return compile_statusbar().str();
}