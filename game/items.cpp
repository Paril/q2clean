#include "entity.h"
#include "../lib/gi.h"
#include "itemlist.h"
#include "game.h"
#include "util.h"
#include "pweapon.h"
#include "spawn.h"
#ifdef GRAPPLE
#include "grapple.h"
#endif

// forward declarations from this file

static pickup_func Pickup_Armor;
static use_func Use_Quad;
static pickup_func Pickup_PowerArmor;
static use_func Use_PowerArmor;
static drop_func Drop_PowerArmor;
static pickup_func Pickup_Ammo;
static drop_func Drop_Ammo;
static pickup_func Pickup_Powerup;
static drop_func Drop_General;
static use_func Use_Invulnerability;
static use_func Use_Silencer;
static use_func Use_Breather;
static use_func Use_Envirosuit;
static pickup_func Pickup_AncientHead;
static pickup_func Pickup_Adrenaline;
static pickup_func Pickup_Bandolier;
static pickup_func Pickup_Pack;
static pickup_func Pickup_Health;

#if defined(SINGLE_PLAYER)
static pickup_func Pickup_Key;
#endif

// note that this order must match the order of gitem_id!
static array<gitem_t, ITEM_TOTAL> itemlist
{{
	{ },

	//
	// ARMOR
	//

	/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_armor_body",
		.pickup = Pickup_Armor,
		.pickup_sound = "misc/ar1_pkup.wav",
		.world_model = "models/items/armor/body/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_bodyarmor",
		.pickup_name = "Body Armor",
		.flags = IT_ARMOR,
		.armor = {
			.base_count = 100,
			.max_count = 200,
			.normal_protection = .8f,
			.energy_protection = .6f
		}
	},

	/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_armor_combat",
		.pickup = Pickup_Armor,
		.pickup_sound = "misc/ar1_pkup.wav",
		.world_model = "models/items/armor/combat/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_combatarmor",
		.pickup_name = "Combat Armor",
		.flags = IT_ARMOR,
		.armor = {
			.base_count = 50,
			.max_count = 100,
			.normal_protection = .6f,
			.energy_protection = .3f
		}
	},

	/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_armor_jacket",
		.pickup = Pickup_Armor,
		.pickup_sound = "misc/ar1_pkup.wav",
		.world_model = "models/items/armor/jacket/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_jacketarmor",
		.pickup_name = "Jacket Armor",
		.flags = IT_ARMOR,
		.armor = {
			.base_count = 25,
			.max_count = 50,
			.normal_protection = .3f,
			.energy_protection = .0f
		}
	},

	/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_armor_shard",
		.pickup = Pickup_Armor,
		.pickup_sound = "misc/ar2_pkup.wav",
		.world_model = "models/items/armor/shard/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_jacketarmor",
		.pickup_name = "Armor Shard",
		.flags = IT_ARMOR,
		.armor = {
			.base_count = 2
		}
	},

	/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_power_screen",
		.pickup = Pickup_PowerArmor,
		.use = Use_PowerArmor,
		.drop = Drop_PowerArmor,
		.pickup_sound = "misc/ar3_pkup.wav",
		.world_model = "models/items/armor/screen/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_powerscreen",
		.pickup_name = "Power Screen",
		.quantity = 60,
		.flags = IT_ARMOR,
		.precaches = "misc/power2.wav misc/power1.wav"
	},

	/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_power_shield",
		.pickup = Pickup_PowerArmor,
		.use = Use_PowerArmor,
		.drop = Drop_PowerArmor,
		.pickup_sound = "misc/ar3_pkup.wav",
		.world_model = "models/items/armor/shield/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_powershield",
		.pickup_name = "Power Shield",
		.quantity = 60,
		.flags = IT_ARMOR,
		.precaches = "misc/power2.wav misc/power1.wav"
	},

	//
	// WEAPONS
	//
#ifdef GRAPPLE
/* weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		.classname = "weapon_grapple", 
		.use = Use_Weapon,
		.weaponthink = CTFWeapon_Grapple,
		.pickup_sound = "misc/w_pkup.wav",
		.view_model = "models/weapons/grapple/tris.md2",
		.vwep_model = "#w_grapple.md2",
		.icon = "w_grapple",
		.pickup_name = "Grapple",
		.flags = IT_WEAPON_MASK,
#ifdef HOOK_STANDARD_ASSETS
		.precaches = "models/objects/debris2/tris.md2 medic/Medatck2.wav flyer/Flyatck1.wav world/turbine1.wav weapons/Sshotr1b.wav"
#else
		.precaches = "models/weapons/grapple/hook/tris.md2 weapons/grapple/grfire.wav weapons/grapple/grpull.wav weapons/grapple/grhang.wav weapons/grapple/grreset.wav weapons/grapple/grhit.wav"
#endif
	},
#endif

	/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
	always owned, never in the world
	*/
	{
		.classname = "weapon_blaster",
		.use = Use_Weapon,
		.weaponthink = Weapon_Blaster,
		.pickup_sound = "misc/w_pkup.wav",
		.view_model = "models/weapons/v_blast/tris.md2",
		.vwep_model = "#w_blaster.md2",
		.icon = "w_blaster",
		.pickup_name = "Blaster",
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/blastf1a.wav misc/lasfly.wav models/objects/laser/tris.md2"
	},

#ifdef GROUND_ZERO
	/*QUAKED weapon_chainfist (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "weapon_chainfist",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_ChainFist,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_chainf/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_chainf/tris.md2",
		.icon = "w_chainfist",
		.pickup_name = "Chainfist",
		.flags = IT_WEAPON_MASK,
		.weapmodel = WEAP_CHAINFIST,
		.precaches = "weapons/sawidle.wav weapons/sawhit.wav"
	},
#endif

	/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_shotgun",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Shotgun,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_shotg/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_shotg/tris.md2",
		.vwep_model = "#w_shotgun.md2",
		.icon = "w_shotgun",
		.pickup_name = "Shotgun",
		.quantity = 1,
		.ammo = ITEM_SHELLS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

	/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_supershotgun",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_SuperShotgun,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_shotg2/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_shotg2/tris.md2",
		.vwep_model = "#w_sshotgun.md2",
		.icon = "w_sshotgun",
		.pickup_name = "Super Shotgun",
		.quantity = 2,
		.ammo = ITEM_SHELLS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/sshotf1b.wav"
	},

	/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_machinegun",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Machinegun,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_machn/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_machn/tris.md2",
		.vwep_model = "#w_machinegun.md2",
		.icon = "w_machinegun",
		.pickup_name = "Machinegun",
		.quantity = 1,
		.ammo = ITEM_BULLETS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

#ifdef GROUND_ZERO
	/*QUAKED weapon_etf_rifle (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "weapon_etf_rifle",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_ETF_Rifle,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_etf_rifle/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_etf_rifle/tris.md2",
		.icon = "w_etf_rifle",
		.pickup_name = "ETF Rifle",
		.quantity = 1,
		.ammo = ITEM_FLECHETTES,
		.flags = IT_WEAPON_MASK,
		.weapmodel = WEAP_ETFRIFLE,
		.precaches = "weapons/nail1.wav models/proj/flechette/tris.md2"
	},
#endif
	
	/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_chaingun",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Chaingun,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_chain/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_chain/tris.md2",
		.vwep_model = "#w_chaingun.md2",
		.icon = "w_chaingun",
		.pickup_name = "Chaingun",
		.quantity = 1,
		.ammo = ITEM_BULLETS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav weapons/chngnd1a.wav"
	},

	/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_grenades",
		.pickup = Pickup_Ammo,
		.use = Use_Weapon,
		.drop = Drop_Ammo,
		.weaponthink = Weapon_Grenade,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/grenades/medium/tris.md2",
		.view_model = "models/weapons/v_handgr/tris.md2",
		.vwep_model = "#a_grenades.md2",
		.icon = "a_grenades",
		.pickup_name = "Grenades",
		.quantity = 5,
		.ammo = ITEM_GRENADES,
		.flags = IT_AMMO | IT_WEAPON,
		.ammotag = AMMO_GRENADES,
		.precaches = "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},

#ifdef THE_RECKONING
	/*QUAKED ammo_trap (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_trap",
		.pickup = Pickup_Ammo,
		.use = Use_Weapon,
		.drop = Drop_Ammo,
		.weaponthink = Weapon_Trap,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/weapons/g_trap/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_trap/tris.md2",
		.icon = "a_trap",
		.pickup_name = "Trap",
		.quantity = 1,
		.ammo = ITEM_TRAP,
		.flags = IT_AMMO | IT_WEAPON,
		.ammotag = AMMO_TRAP,
		.precaches = "weapons/trapcock.wav weapons/traploop.wav weapons/trapsuck.wav weapons/trapdown.wav"
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED ammo_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "ammo_tesla",
		.pickup = Pickup_Ammo,
		.use = Use_Weapon,
		.drop = Drop_Ammo,
		.weaponthink = Weapon_Tesla,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/ammo/am_tesl/tris.md2",
		.view_model = "models/weapons/v_tesla/tris.md2",
		.icon = "a_tesla",
		.pickup_name = "Tesla",
		.quantity = 5,
		.ammo = ITEM_TESLA,
		.flags = IT_AMMO | IT_WEAPON,
		.ammotag = AMMO_TESLA,
		.precaches = "models/weapons/v_tesla2/tris.md2 weapons/teslaopen.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav models/weapons/g_tesla/tris.md2"
	},
#endif

	/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_grenadelauncher",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_GrenadeLauncher,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_launch/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_launch/tris.md2",
		.vwep_model = "#w_glauncher.md2",
		.icon = "w_glauncher",
		.pickup_name = "Grenade Launcher",
		.quantity = 1,
		.ammo = ITEM_GRENADES,
		.flags = IT_WEAPON_MASK,
		.precaches = "models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

#ifdef GROUND_ZERO
	/*QUAKED weapon_proxlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "weapon_proxlauncher",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_GrenadeLauncher,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_plaunch/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_plaunch/tris.md2",
		.icon = "w_proxlaunch",
		.pickup_name = "Prox Launcher",
		.quantity = 1,
		.ammo = ITEM_PROX,
		.flags = IT_WEAPON_MASK,
		.weapmodel = WEAP_PROXLAUNCH,
		.precaches = "weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav weapons/proxwarn.wav weapons/proxopen.wav"
	},
#endif

	/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_rocketlauncher",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_RocketLauncher,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_rocket/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_rocket/tris.md2",
		.vwep_model = "#w_rlauncher.md2",
		.icon = "w_rlauncher",
		.pickup_name = "Rocket Launcher",
		.quantity = 1,
		.ammo = ITEM_ROCKETS,
		.flags = IT_WEAPON_MASK,
		.precaches = "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

	/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_hyperblaster",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_HyperBlaster,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_hyperb/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_hyperb/tris.md2",
		.vwep_model = "#w_hyperblaster.md2",
		.icon = "w_hyperblaster",
		.pickup_name = "HyperBlaster",
		.quantity = 1,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

#ifdef THE_RECKONING
	/*QUAKED weapon_boomer (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_boomer",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Ionripper,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_boom/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_boomer/tris.md2",
		.icon = "w_ripper",
		.pickup_name = "Ionripper",
		.quantity = 2,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
		.weapmodel = WEAP_BOOMER,
		.precaches = "weapons/rg_hum.wav weapons/rippfire.wav"
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED weapon_plasmabeam (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/ 
	{
		.classname = "weapon_plasmabeam",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Heatbeam,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_beamer/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_beamer/tris.md2",
		.icon = "w_heatbeam",
		.pickup_name = "Plasma Beam",
		.quantity = 2,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
		.weapmodel = WEAP_PLASMA,
		.precaches = "models/weapons/v_beamer2/tris.md2 weapons/bfg__l1a.wav"
	},
#endif
	
	/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_railgun",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Railgun,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_rail/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_rail/tris.md2",
		.vwep_model = "#w_railgun.md2",
		.icon = "w_railgun",
		.pickup_name = "Railgun",
		.quantity = 1,
		.ammo = ITEM_SLUGS,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/rg_hum.wav"
	},
	
#ifdef THE_RECKONING
	/*QUAKED weapon_phalanx (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_phalanx",
		"Pickup_Weapon",
		"Use_Weapon",
		"Drop_Weapon",
		"Weapon_Phalanx",
		"misc/w_pkup.wav",
		"models/weapons/g_shotx/tris.md2", EF_ROTATE,
		"models/weapons/v_shotx/tris.md2",
/* icon */	"w_phallanx",
/* pickup */ "Phalanx",
		0,
		1,
		"Mag Slug",
		IT_WEAPON_MASK,
		WEAP_PHALANX,
		0,
		0,
/* precache */ "weapons/plasshot.wav"
	},
#endif

	/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "weapon_bfg",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_BFG,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_bfg/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_bfg/tris.md2",
		.vwep_model = "#w_bfg.md2",
		.icon = "w_bfg",
		.pickup_name = "BFG10K",
		.quantity = 50,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
		.precaches = "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg_.x1b.wav weapons/bfg_hum.wav"
	},

#ifdef GROUND_ZERO
	/*QUAKED weapon_disintegrator (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"weapon_disintegrator",								// classname
		"Pickup_Weapon",										// pickup function
		"Use_Weapon",											// use function
		"Drop_Weapon",										// drop function
		"Weapon_Disintegrator",								// weapon think function
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_dist/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_dist/tris.md2",					// view model
		"w_disintegrator",									// icon
		"Disruptor",										// name printed when picked up 
		0,													// number of digits for statusbar
		1,													// amount used / contained
		"Rounds",
		IT_WEAPON_MASK,											// inventory flags
		WEAP_DISRUPTOR,										// visible weapon
		0,												// info (void *)
		1,													// tag
		"models/items/spawngro/tris.md2 models/proj/disintegrator/tris.md2 weapons/disrupt.wav weapons/disint2.wav weapons/disrupthit.wav",	// precaches
	},
#endif

	//
	// AMMO ITEMS
	//

	/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_shells",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/shells/medium/tris.md2",
		.icon = "a_shells",
		.pickup_name = "Shells",
		.quantity = 10,
		.flags = IT_AMMO,
		.ammotag = AMMO_SHELLS
	},

	/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_bullets",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/bullets/medium/tris.md2",
		.icon = "a_bullets",
		.pickup_name = "Bullets",
		.quantity = 50,
		.flags = IT_AMMO,
		.ammotag = AMMO_BULLETS
	},

	/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_cells",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/cells/medium/tris.md2",
		.icon = "a_cells",
		.pickup_name = "Cells",
		.quantity = 50,
		.flags = IT_AMMO,
		.ammotag = AMMO_CELLS
	},

	/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_rockets",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/rockets/medium/tris.md2",
		.icon = "a_rockets",
		.pickup_name = "Rockets",
		.quantity = 5,
		.flags = IT_AMMO,
		.ammotag = AMMO_ROCKETS
	},

	/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "ammo_slugs",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/items/ammo/slugs/medium/tris.md2",
		.icon = "a_slugs",
		.pickup_name = "Slugs",
		.quantity = 10,
		.flags = IT_AMMO,
		.ammotag = AMMO_SLUGS
	},

#ifdef THE_RECKONING
/*QUAKED ammo_magslug (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_magslug",
		"Pickup_Ammo",
		0,
		"Drop_Ammo",
		0,
		"misc/am_pkup.wav",
		"models/objects/ammo/tris.md2", 0,
		0,
/* icon */		"a_mslugs",
/* pickup */	"Mag Slug",
/* width */		3,
		10,
		0,
		IT_AMMO,
		0,
		0,
		AMMO_MAGSLUG,
/* precache */ ""
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED ammo_flechettes (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"ammo_flechettes",
		"Pickup_Ammo",
		0,
		"Drop_Ammo",
		0,
		"misc/am_pkup.wav",
		"models/ammo/am_flechette/tris.md2", 0,
		0,
		"a_flechettes",
		"Flechettes",
		3,
		50,
		0,
		IT_AMMO,
		0,
		0,
		AMMO_FLECHETTES
	},

	/*QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"ammo_prox",										// Classname
		"Pickup_Ammo",										// pickup function
		0,												// use function
		"Drop_Ammo",											// drop function
		0,												// weapon think function
		"misc/am_pkup.wav",									// pickup sound
		"models/ammo/am_prox/tris.md2", 0,					// world model, world model flags
		0,												// view model
		"a_prox",											// icon
		"Prox",												// Name printed when picked up
		3,													// number of digits for status bar
		5,													// amount contained
		0,												// ammo type used
		IT_AMMO,											// inventory flags
		0,													// vwep index
		0,												// info (void *)
		AMMO_PROX,											// tag
		"models/weapons/g_prox/tris.md2 weapons/proxwarn.wav"	// precaches
	},

	/*QUAKED ammo_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"ammo_nuke",
		"Pickup_Nuke",
		"Use_Nuke",						// PMM
		"Drop_Ammo",
		0,							// PMM
		"misc/am_pkup.wav",
		"models/weapons/g_nuke/tris.md2", EF_ROTATE,
		0,
/* icon */		"p_nuke",
/* pickup */	"A-M Bomb",
/* width */		3,
		300, /* quantity (used for respawn time) */
		"A-M Bomb",
		IT_POWERUP,	
		0,
		0,
		0,
		"weapons/nukewarn2.wav world/rumble.wav"
	},

	/*QUAKED ammo_disruptor (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"ammo_disruptor",
		"Pickup_Ammo",
		0,
		"Drop_Ammo",
		0,
		"misc/am_pkup.wav",
		"models/ammo/am_disr/tris.md2", 0,
		0,
		"a_disruptor",
		"Rounds",		// FIXME 
		3,
		15,
		0,
		IT_AMMO,											// inventory flags
		0,
		0,
		AMMO_DISRUPTOR,
	},
#endif

	//
	// POWERUP ITEMS
	//
	/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_quad",
		.pickup = Pickup_Powerup,
		.use = Use_Quad,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/quaddama/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_quad",
		.pickup_name = "Quad Damage",
		.quantity = 60,
		.flags = IT_POWERUP,
		.precaches = "items/damage.wav items/damage2.wav items/damage3.wav"
	},

	/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_invulnerability",
		.pickup = Pickup_Powerup,
		.use = Use_Invulnerability,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/invulner/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_invulnerability",
		.pickup_name = "Invulnerability",
		.quantity = 300,
		.flags = IT_POWERUP,
		.precaches = "items/protect.wav items/protect2.wav items/protect4.wav"
	},

	/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_silencer",
		.pickup = Pickup_Powerup,
		.use = Use_Silencer,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/silencer/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_silencer",
		.pickup_name = "Silencer",
		.quantity = 60,
		.flags = IT_POWERUP
	},

	/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_breather",
		.pickup = Pickup_Powerup,
		.use = Use_Breather,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/breather/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_rebreather",
		.pickup_name = "Rebreather",
		.quantity = 60,
#ifdef SINGLE_PLAYER
		.flags = IT_STAY_COOP | IT_POWERUP,
#else
		.flags = IT_POWERUP,
#endif
		.precaches = "items/airout.wav player/u_breath1.wav player/u_breath2.wav"
	},

	/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_enviro",
		.pickup = Pickup_Powerup,
		.use = Use_Envirosuit,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/enviro/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_envirosuit",
		.pickup_name = "Environment Suit",
		.quantity = 60,
#ifdef SINGLE_PLAYER
		.flags = IT_STAY_COOP | IT_POWERUP,
#else
		.flags = IT_POWERUP,
#endif
		.precaches = "items/airout.wav"
	},

#ifdef THE_RECKONING
	/*QUAKED item_quadfire (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_quadfire", 
		"Pickup_Powerup",
		"Use_QuadFire",
		"Drop_General",
		0,
		"items/pkup.wav",
		"models/items/quadfire/tris.md2", EF_ROTATE,
		0,
/* icon */		"p_quadfire",

/* pickup */	"DualFire Damage",
/* width */		2,
		60,
		0,
		IT_POWERUP,
		0,
		0,
		0,
/* precache */ "items/quadfire1.wav items/quadfire2.wav items/quadfire3.wav"
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED item_ir_goggles (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	gives +1 to maximum health
	*/
	{
		"item_ir_goggles",
		"Pickup_Powerup",
		"Use_IR",
		"Drop_General",
		0,
		"items/pkup.wav",
		"models/items/goggles/tris.md2", EF_ROTATE,
		0,
/* icon */		"p_ir",
/* pickup */	"IR Goggles",
/* width */		2,
		60,
		0,
		IT_POWERUP,
		0,
		0,
		0,
/* precache */ "misc/ir_start.wav"
	},

	/*QUAKED item_double (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"item_double", 
		"Pickup_Powerup",
		"Use_Double",
		"Drop_General",
		0,
		"items/pkup.wav",
		"models/items/ddamage/tris.md2", EF_ROTATE,
		0,
/* icon */		"p_double",
/* pickup */	"Double Damage",
/* width */		2,
		60,
		0,
		IT_POWERUP,
		0,
		0,
		0,
/* precache */ "misc/ddamage1.wav misc/ddamage2.wav misc/ddamage3.wav"
	},
#endif

	/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
	Special item that gives +2 to maximum health
	*/
	{
		.classname = "item_ancient_head",
		.pickup = Pickup_AncientHead,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/c_head/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_fixme",
		.pickup_name = "Ancient Head",
		.quantity = 60,
		.flags = IT_HEALTH
	},

	/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
	gives +1 to maximum health
	*/
	{
		.classname = "item_adrenaline",
		.pickup = Pickup_Adrenaline,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/adrenal/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_adrenaline",
		.pickup_name = "Adrenaline",
		.quantity = 60,
		.flags = IT_HEALTH
	},

	/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_bandolier",
		.pickup = Pickup_Bandolier,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/band/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_bandolier",
		.pickup_name = "Bandolier",
		.quantity = 60
	},

	/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_pack",
		.pickup = Pickup_Pack,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/pack/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_pack",
		.pickup_name = "Ammo Pack",
		.quantity = 180
	},

#ifdef SINGLE_PLAYER
	//
	// KEYS
	//
	
	/*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for computer centers
	*/
	{
		.classname = "key_data_cd",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/data_cd/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_datacd",
		.pickup_name = "Data CD",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
	warehouse circuits
	*/
	{
		.classname = "key_power_cube",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/power/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_powercube",
		.pickup_name = "Power Cube",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for the entrance of jail3
	*/
	{
		.classname = "key_pyramid",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/pyramid/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_pyramid",
		.pickup_name = "Pyramid Key",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for the city computer
	*/
	{
		.classname = "key_data_spinner",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/spinner/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_dataspin",
		.pickup_name = "Data Spinner",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
	security pass for the security level
	*/
	{
		.classname = "key_pass",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/pass/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_security",
		.pickup_name = "Security Pass",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	normal door key - blue
	*/
	{
		.classname = "key_blue_key",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/key/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_bluekey",
		.pickup_name = "Blue Key",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	normal door key - red
	*/
	{
		.classname = "key_red_key",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/red_key/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_redkey",
		.pickup_name = "Red Key",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
	tank commander's head
	*/
	{
		.classname = "key_commander_head",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/monsters/commandr/head/tris.md2",
		.world_model_flags = EF_GIB,
		.icon = "k_comhead",
		.pickup_name = "Commander's Head",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16)
	tank commander's head
	*/
	{
		.classname = "key_airstrike_target",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/target/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_airstrike",
		.pickup_name = "Airstrike Marker",
		.flags = IT_STAY_COOP | IT_KEY
	},

#ifdef THE_RECKONING
	/*QUAKED key_green_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	normal door key - blue
	*/
	{
		"key_green_key",
		"Pickup_Key",
		0,
		"Drop_General",
		0,
		"items/pkup.wav",
		"models/items/keys/green_key/tris.md2", EF_ROTATE,
		0,
		"k_green",
		"Green Key",
		2,
		0,
		0,
		IT_STAY_COOP|IT_KEY,
		0,
		0,
		0,
/* precache */ ""
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED key_nuke_container (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"key_nuke_container",								// classname
		"Pickup_Key",											// pickup function
		0,												// use function
		"Drop_General",										// drop function
		0,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/weapons/g_nuke/tris.md2",					// world model
		EF_ROTATE,											// world model flags
		0,												// view model
		"i_contain",										// icon
		"Antimatter Pod",									// name printed when picked up 
		2,													// number of digits for statusbar
		0,													// respawn time
		0,												// ammo type used 
		IT_STAY_COOP|IT_KEY,								// inventory flags
		0,
		0,												// info (void *)
		0,													// tag
		0,												// precaches
	},

	/*QUAKED key_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		"key_nuke",											// classname
		"Pickup_Key",											// pickup function
		0,												// use function
		"Drop_General",										// drop function
		0,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/weapons/g_nuke/tris.md2",					// world model
		EF_ROTATE,											// world model flags
		0,												// view model
		"i_nuke",											// icon
		"Antimatter Bomb",									// name printed when picked up 
		2,													// number of digits for statusbar
		0,													// respawn time
		0,												// ammo type used 
		IT_STAY_COOP|IT_KEY,								// inventory flags
		0,
		0,												// info (void *)
		0,													// tag
		0,												// precaches
	},
#endif
#endif

#ifdef CTF
/*QUAKED item_flag_team1 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_flag_team1",
		"CTFPickup_Flag",
		0,
		"CTFDrop_Flag", //Should this be null if we don't want players to drop it manually?
		0,
		"ctf/flagtk.wav",
		"players/male/flag1.md2", EF_FLAG1,
		0,
/* icon */		"i_ctf1",
/* pickup */	"Red Flag",
/* width */		2,
		0,
		0,
		IT_FLAG,
		0,
		0,
		0,
/* precache */ "ctf/flagcap.wav"
	},

/*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_flag_team2",
		"CTFPickup_Flag",
		0,
		"CTFDrop_Flag", //Should this be null if we don't want players to drop it manually?
		0,
		"ctf/flagtk.wav",
		"players/male/flag2.md2", EF_FLAG2,
		0,
/* icon */		"i_ctf2",
/* pickup */	"Blue Flag",
/* width */		2,
		0,
		0,
		IT_FLAG,
		0,
		0,
		0,
/* precache */ "ctf/flagcap.wav"
	},

/* Resistance Tech */
	{
		"item_tech1",
		"CTFPickup_Tech",
		0,
		"CTFDrop_Tech", //Should this be null if we don't want players to drop it manually?
		0,
		"items/pkup.wav",
		"models/ctf/resistance/tris.md2", EF_ROTATE,
		0,
/* icon */		"tech1",
/* pickup */	"Disruptor Shield",
/* width */		2,
		0,
		0,
		IT_TECH,
		0,
		0,
		0,
/* precache */ "ctf/tech1.wav"
	},

/* Strength Tech */
	{
		"item_tech2",
		"CTFPickup_Tech",
		0,
		"CTFDrop_Tech", //Should this be null if we don't want players to drop it manually?
		0,
		"items/pkup.wav",
		"models/ctf/strength/tris.md2", EF_ROTATE,
		0,
/* icon */		"tech2",
/* pickup */	"Power Amplifier",
/* width */		2,
		0,
		0,
		IT_TECH,
		0,
		0,
		0,
/* precache */ "ctf/tech2.wav ctf/tech2x.wav"
	},

/* Haste Tech */
	{
		"item_tech3",
		"CTFPickup_Tech",
		0,
		"CTFDrop_Tech", //Should this be null if we don't want players to drop it manually?
		0,
		"items/pkup.wav",
		"models/ctf/haste/tris.md2", EF_ROTATE,
		0,
/* icon */		"tech3",
/* pickup */	"Time Accel",
/* width */		2,
		0,
		0,
		IT_TECH,
		0,
		0,
		0,
/* precache */ "ctf/tech3.wav"
	},

/* Regeneration Tech */
	{
		"item_tech4",
		"CTFPickup_Tech",
		0,
		"CTFDrop_Tech", //Should this be null if we don't want players to drop it manually?
		0,
		"items/pkup.wav",
		"models/ctf/regeneration/tris.md2", EF_ROTATE,
		0,
/* icon */		"tech4",
/* pickup */	"AutoDoc",
/* width */		2,
		0,
		0,
		IT_TECH,
		0,
		0,
		0,
/* precache */ "ctf/tech4.wav"
	},
#endif

	{
		.pickup = Pickup_Health,
		.icon = "i_health",
		.pickup_name = "Health",
		.flags = IT_HEALTH,
		.precaches = "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	}
}};

const array<gitem_t, ITEM_TOTAL> &item_list()
{
	return itemlist;
}

void InitItems()
{
	uint8_t weapon_id = 1;

	for (auto &it : itemlist)
	{
		it.id = (gitem_id)(&it - itemlist.data());

		if (it.vwep_model)
			it.vwep_id = weapon_id++;
	}
}

static gtime quad_drop_timeout_hack;

#ifdef THE_RECKONING
gtime quad_fire_drop_timeout_hack;
#endif

//======================================================================

void DoRespawn(entity &item)
{
	entityref ent = item;

	if (ent->team)
	{
		entityref master = ent->teammaster;

#ifdef CTF
//in ctf, when we are weapons stay, only the master of a team of weapons
//is spawned
		if (ctf.intVal &&
			(dmflags.intVal & DF_WEAPONS_STAY) &&
			master.item && (master.item->flags & IT_WEAPON))
			ent = master;
		else
		{
#endif	
			uint32_t	count;

			for (count = 0, ent = master; ent.has_value(); ent = ent->chain, count++) ;

			uint32_t choice = Q_rand_uniform(count);

			for (count = 0, ent = master; count < choice; ent = ent->chain, count++) ;
#ifdef CTF
		}
#endif
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

REGISTER_SAVABLE_FUNCTION(DoRespawn);

void SetRespawn(entity &ent, float delay)
{
	ent.flags |= FL_RESPAWN;
	ent.svflags |= SVF_NOCLIENT;
	ent.solid = SOLID_NOT;
	ent.nextthink = level.framenum + (gtime)(delay * BASE_FRAMERATE);
	ent.think = DoRespawn_savable;
	gi.linkentity(ent);
}

#ifdef THE_RECKONING
use_func Use_QuadFire;
#endif

static bool Pickup_Powerup(entity &ent, entity &other)
{
#ifdef SINGLE_PLAYER
	const int32_t &quantity = other.client->pers.inventory[ent.item->id];

	if (((int32_t)skill == 1 && quantity >= 2) || ((int32_t)skill >= 2 && quantity >= 1))
		return false;

	if (coop && (ent.item->flags & IT_STAY_COOP) && quantity > 0)
		return false;
#endif

	other.client->pers.inventory[ent.item->id]++;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		if (!(ent.spawnflags & DROPPED_ITEM))
			SetRespawn(ent, ent.item->quantity);

		if (((dm_flags)dmflags & DF_INSTANT_ITEMS) || ((ent.item->use == Use_Quad
#ifdef THE_RECKONING
			|| ent.item->use == Use_QuadFire
#endif
			) && (ent.spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if (ent.spawnflags & DROPPED_PLAYER_ITEM)
			{
				if (ent.item->use == Use_Quad)
					quad_drop_timeout_hack = ent.nextthink - level.framenum;
#ifdef THE_RECKONING
				else if (ent.item->use == Use_QuadFire)
					quad_drop_timeout_hack = ent.nextthink - level.framenum;
#endif
			}

			if (ent.item->use)
				ent.item->use(other, ent.item);
		}
#ifdef SINGLE_PLAYER
	}
#endif

	return true;
}

// from cmds.qc
void ValidateSelectedItem(entity &);

static void Drop_General(entity &ent, const gitem_t &it)
{
	Drop_Item(ent, it);
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
}

//======================================================================

static bool Pickup_Adrenaline(entity &ent, entity &other)
{
#ifdef SINGLE_PLAYER
	if (!deathmatch)
		other.max_health += 1;
#endif

	other.health = max(other.health, other.max_health);

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

static bool Pickup_AncientHead(entity &ent, entity &other)
{
	other.max_health += 2;

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

static inline void AdjustAmmoMax(entity &other, const ammo_id &ammo, const int32_t &new_max)
{
	other.client->pers.max_ammo[ammo] = max(other.client->pers.max_ammo[ammo], new_max);
}

static inline void AddAndCapAmmo(entity &other, const gitem_t &ammo)
{
	other.client->pers.inventory[ammo.id] = min(other.client->pers.inventory[ammo.id] + ammo.quantity, other.client->pers.max_ammo[ammo.ammotag]);
}

static bool Pickup_Bandolier(entity &ent, entity &other)
{
	AdjustAmmoMax(other, AMMO_BULLETS, 250);
	AdjustAmmoMax(other, AMMO_SHELLS, 150);
	AdjustAmmoMax(other, AMMO_CELLS, 250);
	AdjustAmmoMax(other, AMMO_SLUGS, 75);
#ifdef THE_RECKONING
	AdjustAmmoMax(other, AMMO_MAGSLUG, 75);
#endif
#ifdef GROUND_ZERO
	AdjustAmmoMax(other, AMMO_FLECHETTES, 250);
	AdjustAmmoMax(other, AMMO_DISRUPTOR, 150);
#endif

	AddAndCapAmmo(other, GetItemByIndex(ITEM_BULLETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SHELLS));

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

static bool Pickup_Pack(entity &ent, entity &other)
{
	AdjustAmmoMax(other, AMMO_BULLETS, 300);
	AdjustAmmoMax(other, AMMO_SHELLS, 200);
	AdjustAmmoMax(other, AMMO_ROCKETS, 100);
	AdjustAmmoMax(other, AMMO_GRENADES, 100);
	AdjustAmmoMax(other, AMMO_CELLS, 300);
	AdjustAmmoMax(other, AMMO_SLUGS, 100);
#ifdef THE_RECKONING
	AdjustAmmoMax(other, AMMO_MAGSLUG, 100);
#endif
#ifdef GROUND_ZERO
	AdjustAmmoMax(other, AMMO_FLECHETTES, 300);
	AdjustAmmoMax(other, AMMO_DISRUPTOR, 200);
#endif

	AddAndCapAmmo(other, GetItemByIndex(ITEM_BULLETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SHELLS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_CELLS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_GRENADES));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_ROCKETS));
	AddAndCapAmmo(other, GetItemByIndex(ITEM_SLUGS));
#ifdef THE_RECKONING
	AddAndCapAmmo(other, FindItem("Mag Slug"));
#endif
#ifdef GROUND_ZERO
	AddAndCapAmmo(other, FindItem("Flechettes"));
	AddAndCapAmmo(other, FindItem("Rounds"));
#endif

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, ent.item->quantity);

	return true;
}

//======================================================================

static void Use_Quad(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	gtime timeout;

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
		timeout = 300;

	if (ent.client->quad_framenum > level.framenum)
		ent.client->quad_framenum += timeout;
	else
		ent.client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Breather(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->breather_framenum > level.framenum)
		ent.client->breather_framenum += 300;
	else
		ent.client->breather_framenum = level.framenum + 300;
}

//======================================================================

static void Use_Envirosuit(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->enviro_framenum > level.framenum)
		ent.client->enviro_framenum += 300;
	else
		ent.client->enviro_framenum = level.framenum + 300;
}

//======================================================================

static void Use_Invulnerability(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);

	if (ent.client->invincible_framenum > level.framenum)
		ent.client->invincible_framenum += 300;
	else
		ent.client->invincible_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Silencer(entity &ent, const gitem_t &it)
{
	ent.client->pers.inventory[it.id]--;
	ValidateSelectedItem(ent);
	ent.client->silencer_shots += 30;
}

//======================================================================

#ifdef SINGLE_PLAYER
static bool Pickup_Key(entity &ent, entity &other)
{
	if (coop)
	{
		if (ent.item->id == ITEM_POWER_CUBE)
		{
			int cube_flag = (ent.spawnflags & 0x0000ff00) >> 8;

			if (other.client->pers.power_cubes & cube_flag)
				return false;

			other.client->pers.inventory[ent.item->id]++;
			other.client->pers.power_cubes |= cube_flag;
		}
		else
		{
			if (other.client->pers.inventory[ent.item->id])
				return false;

			other.client->pers.inventory[ent.item->id] = 1;
		}

		return true;
	}
	other.client->pers.inventory[ent.item->id]++;
	return true;
}
#endif

//======================================================================

bool Add_Ammo(entity &ent, const gitem_t &it, int32_t count)
{
	if (!ent.is_client())
		return false;

	int32_t max = ent.client->pers.max_ammo[it.ammotag];

	if (ent.client->pers.inventory[it.id] == max)
		return false;

	ent.client->pers.inventory[it.id] = min(max, ent.client->pers.inventory[it.id] + count);
	return true;
}

static bool Pickup_Ammo(entity &ent, entity &other)
{
	const bool weapon = (ent.item->flags & IT_WEAPON);
	int32_t ammocount;

	if (weapon && ((dm_flags)dmflags & DF_INFINITE_AMMO))
		ammocount = 1000;
	else if (ent.count)
		ammocount = ent.count;
	else
		ammocount = ent.item->quantity;

	const int32_t oldcount = other.client->pers.inventory[ent.item->id];

	if (!Add_Ammo(other, ent.item, ammocount))
		return false;

	if (weapon && !oldcount && other.client->pers.weapon != ent.item &&
#ifdef SINGLE_PLAYER
		(!deathmatch ||
#endif
		(other.client->pers.weapon && other.client->pers.weapon->id == ITEM_BLASTER))
#ifdef SINGLE_PLAYER
		)
#endif
			other.client->newweapon = ent.item;

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && deathmatch)
#else
	if (!(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
#endif
		SetRespawn(ent, 30);

	return true;
}

static void Drop_Ammo(entity &ent, const gitem_t &it)
{
	entity &dropped = Drop_Item(ent, it);

	if (ent.client->pers.inventory[it.id] >= it.quantity)
		dropped.count = it.quantity;
	else
		dropped.count = ent.client->pers.inventory[it.id];

	if (ent.client->pers.weapon &&
		ent.client->pers.weapon->ammotag == AMMO_GRENADES &&
		it.ammotag == AMMO_GRENADES &&
		(ent.client->pers.inventory[it.id] - dropped.count) <= 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent.client->pers.inventory[it.id] -= dropped.count;
	ValidateSelectedItem(ent);
}

//======================================================================

#ifdef CTF
bool(entity) CTFHasRegeneration;
#endif

static void MegaHealth_think(entity &self)
{
	if (self.owner->health > self.owner->max_health
#ifdef CTF
		&& !CTFHasRegeneration(self.owner)
#endif
		)
	{
		self.nextthink = level.framenum + 1 * BASE_FRAMERATE;
		self.owner->health -= 1;
		return;
	}

#ifdef SINGLE_PLAYER
	if (!(self.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(self.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(self, 20);
	else
		G_FreeEdict(self);
}

REGISTER_SAVABLE_FUNCTION(MegaHealth_think);

static bool Pickup_Health(entity &ent, entity &other)
{
	if (!(ent.style & HEALTH_IGNORE_MAX))
		if (other.health >= other.max_health)
			return false;

#ifdef CTF
	if (other.health >= 250 && ent.count > 25)
		return false;
#endif

	other.health += ent.count;

#ifdef CTF
	if (other.health > 250 && ent.count > 25)
		other.health = 250;
#endif

	if (!(ent.style & HEALTH_IGNORE_MAX))
		other.health = min(other.health, other.max_health);

	if (ent.style & HEALTH_TIMED
#ifdef CTF
		&& !CTFHasRegeneration(other)
#endif
		)
	{
		ent.think = MegaHealth_think_savable;
		ent.nextthink = level.framenum + 5 * BASE_FRAMERATE;
		ent.owner = other;
		ent.flags |= FL_RESPAWN;
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;
	}
#ifdef SINGLE_PLAYER
	else if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	else if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, 30);

	return true;
}

//======================================================================

gitem_id ArmorIndex(entity &ent)
{
	if (!ent.is_client())
		return ITEM_NONE;

	if (ent.client->pers.inventory[ITEM_ARMOR_JACKET] > 0)
		return ITEM_ARMOR_JACKET;
	else if (ent.client->pers.inventory[ITEM_ARMOR_COMBAT] > 0)
		return ITEM_ARMOR_COMBAT;
	else if (ent.client->pers.inventory[ITEM_ARMOR_BODY] > 0)
		return ITEM_ARMOR_BODY;

	return ITEM_NONE;
}

static bool Pickup_Armor(entity &ent, entity &other)
{
	gitem_id old_armor_index = ArmorIndex(other);

	// handle armor shards specially
	if (ent.item->id == ITEM_ARMOR_SHARD)
		other.client->pers.inventory[old_armor_index ? old_armor_index : ITEM_ARMOR_JACKET] += 2;
	else
	{
		// get info on new armor
		const gitem_armor &newinfo = ent.item->armor;

		// if player has no armor, just use it
		if (!old_armor_index)
			other.client->pers.inventory[ent.item->id] = newinfo.base_count;
		// use the better armor
		else
		{
			// get info on old armor
			const gitem_armor &oldinfo = GetItemByIndex(old_armor_index).armor;

			if (newinfo.normal_protection > oldinfo.normal_protection)
			{
				// calc new armor values
				const float salvage = oldinfo.normal_protection / newinfo.normal_protection;
				const int32_t salvagecount = (int32_t)(salvage * other.client->pers.inventory[old_armor_index]);
				const int32_t newcount = min(newinfo.base_count + salvagecount, newinfo.max_count);

				// zero count of old armor so it goes away
				other.client->pers.inventory[old_armor_index] = 0;

				// change armor to new item with computed value
				other.client->pers.inventory[ent.item->id] = newcount;
			}
			else
			{
				// calc new armor values
				const float salvage = newinfo.normal_protection / oldinfo.normal_protection;
				const int32_t salvagecount = (int32_t)(salvage * newinfo.base_count);
				const int32_t newcount = min(other.client->pers.inventory[old_armor_index] + salvagecount, oldinfo.max_count);

				// if we're already maxed out then we don't need the new armor
				if (other.client->pers.inventory[old_armor_index] >= newcount)
					return false;

				// update current armor value
				other.client->pers.inventory[old_armor_index] = newcount;
			}
		}
	}

#ifdef SINGLE_PLAYER
	if (!(ent.spawnflags & DROPPED_ITEM) && deathmatch)
#else
	if (!(ent.spawnflags & DROPPED_ITEM))
#endif
		SetRespawn(ent, 20);

	return true;
}

//======================================================================

gitem_id PowerArmorType(entity &ent)
{
	if (ent.is_client() && (ent.flags & FL_POWER_ARMOR))
	{
		if (ent.client->pers.inventory[ITEM_POWER_SHIELD] > 0)
			return ITEM_POWER_SHIELD;
		else if (ent.client->pers.inventory[ITEM_POWER_SCREEN] > 0)
			return ITEM_POWER_SCREEN;
	}

	return ITEM_NONE;
}

static void Use_PowerArmor(entity &ent, const gitem_t &)
{
	if (ent.flags & FL_POWER_ARMOR)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	else
	{
		if (!ent.client->pers.inventory[ITEM_CELLS])
		{
			gi.cprintf(ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
	}

	ent.flags ^= FL_POWER_ARMOR;
}

static bool Pickup_PowerArmor(entity &ent, entity &other)
{
	const int32_t quantity = other.client->pers.inventory[ent.item->id];

	other.client->pers.inventory[ent.item->id]++;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		if (!(ent.spawnflags & DROPPED_ITEM))
			SetRespawn(ent, ent.item->quantity);

		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent.item->use(other, ent.item);
#ifdef SINGLE_PLAYER
	}
#endif

	return true;
}

static void Drop_PowerArmor(entity &ent, const gitem_t &it)
{
	if ((ent.flags & FL_POWER_ARMOR) && (ent.client->pers.inventory[it.id] == 1))
		Use_PowerArmor(ent, it);

	Drop_General(ent, it);
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item(entity &ent, entity &other, vector , const surface &)
{
	if (!other.is_client())
		return;
	if (other.health < 1)
		return;     // dead people can't pickup
	if (!ent.item->pickup)
		return;     // not a grabbable item?

	const bool taken = ent.item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other.client->bonus_alpha = 0.25f;

		// show icon and name on status bar
		other.client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent.item->icon);
		other.client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + (config_string)ent.item->id;
		other.client->pickup_msg_framenum = level.framenum + (int)(3.0f * BASE_FRAMERATE);

		// change selected item
		if (ent.item->use)
			other.client->ps.stats[STAT_SELECTED_ITEM] = other.client->pers.selected_item = ent.item->id;

		if (ent.item->pickup == Pickup_Health)
		{
			if (ent.count == 2)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent.count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent.count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent.item->pickup_sound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent.item->pickup_sound), 1, ATTN_NORM, 0);
	}

	if (!(ent.spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets(ent, other);
		ent.spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;
#ifdef SINGLE_PLAYER
	else if (coop && (ent.item->flags & IT_STAY_COOP) && !(ent.spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		return;
#endif

	if (ent.flags & FL_RESPAWN)
		ent.flags &= ~FL_RESPAWN;
	else
		G_FreeEdict(ent);
}

REGISTER_SAVABLE_FUNCTION(Touch_Item);

//======================================================================

static void drop_temp_touch(entity &ent, entity &other, vector plane, const surface &surf)
{
	if (other == ent.owner)
		return;

	Touch_Item(ent, other, plane, surf);
}

REGISTER_SAVABLE_FUNCTION(drop_temp_touch);

static void drop_make_touchable(entity &ent)
{
	ent.touch = Touch_Item_savable;

#ifdef SINGLE_PLAYER
	if (deathmatch)
	{
#endif
		ent.nextthink = level.framenum + 29 * BASE_FRAMERATE;
		ent.think = G_FreeEdict_savable;
#ifdef SINGLE_PLAYER
	}
#endif
}

REGISTER_SAVABLE_FUNCTION(drop_make_touchable);

entity &Drop_Item(entity &ent, const gitem_t &it)
{
	entity &dropped = G_Spawn();

	dropped.type = ET_ITEM;
	dropped.item = it;
	dropped.spawnflags = DROPPED_ITEM;
	dropped.s.effects = it.world_model_flags;
	dropped.s.renderfx = RF_GLOW
#ifdef GROUND_ZERO
		| RF_IR_VISIBLE
#endif
		;
	dropped.mins = { -15, -15, -15 };
	dropped.maxs = { 15, 15, 15 };
	gi.setmodel(dropped, it.world_model);
	dropped.solid = SOLID_TRIGGER;
	dropped.movetype = MOVETYPE_TOSS;
	dropped.touch = drop_temp_touch_savable;
	dropped.owner = ent;

	vector forward;

	if (ent.is_client())
	{
		vector right;
		AngleVectors(ent.client->v_angle, &forward, &right, nullptr);
		dropped.s.origin = G_ProjectSource(ent.s.origin, { 24, 0, -16 }, forward, right);

		trace tr = gi.trace(ent.s.origin, dropped.mins, dropped.maxs, dropped.s.origin, ent, CONTENTS_SOLID);
		dropped.s.origin = tr.endpos;
	}
	else
	{
		AngleVectors(ent.s.angles, &forward, nullptr, nullptr);
		dropped.s.origin = ent.s.origin;
	}

	dropped.velocity = forward * 100;
	dropped.velocity.z = 300.f;

	dropped.think = drop_make_touchable_savable;
	dropped.nextthink = level.framenum + 1 * BASE_FRAMERATE;

	gi.linkentity(dropped);
	return dropped;
}

static void Use_Item(entity &ent, entity &, entity &)
{
	ent.svflags &= ~SVF_NOCLIENT;
	ent.use = 0;

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = 0;
	}
	else
	{
		ent.solid = SOLID_TRIGGER;
		ent.touch = Touch_Item_savable;
	}

	gi.linkentity(ent);
}

REGISTER_SAVABLE_FUNCTION(Use_Item);

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor(entity &ent)
{
	ent.mins = { -15, -15, -15 };
	ent.maxs = { 15, 15, 15 };

	if (ent.model)
		gi.setmodel(ent, ent.model);
	else
		gi.setmodel(ent, ent.item->world_model);

	ent.solid = SOLID_TRIGGER;
	ent.movetype = MOVETYPE_TOSS;
	ent.touch = Touch_Item_savable;

	vector dest = ent.s.origin;
	dest[2] -= 128;

	trace tr = gi.trace(ent.s.origin, ent.mins, ent.maxs, dest, ent, MASK_SOLID);

	if (tr.startsolid)
	{
#ifdef THE_RECKONING
		if (ent.classname == "foodcube")
		{
			tr.endpos = ent.s.origin;
			ent.velocity[2] = 0;
		}
		else
		{
#endif
			gi.dprintf("droptofloor: %i startsolid at %s\n", ent.type, vtos(ent.s.origin).ptr());
			G_FreeEdict(ent);
			return;
#ifdef THE_RECKONING
		}
#endif
	}

	ent.s.origin = tr.endpos;

	if (ent.team)
	{
		ent.flags &= ~FL_TEAMSLAVE;
		ent.chain = ent.teamchain;
		ent.teamchain = nullptr;
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;

		if (ent == ent.teammaster)
		{
			ent.nextthink = level.framenum + 1;
			ent.think = DoRespawn_savable;
		}
	}

	if (ent.spawnflags & ITEM_NO_TOUCH)
	{
		ent.solid = SOLID_BBOX;
		ent.touch = 0;
		ent.s.effects &= ~EF_ROTATE;
		ent.s.renderfx &= ~RF_GLOW;
	}

	if (ent.spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent.svflags |= SVF_NOCLIENT;
		ent.solid = SOLID_NOT;
		ent.use = Use_Item_savable;
	}

	gi.linkentity(ent);
}

REGISTER_SAVABLE_FUNCTION(droptofloor);

/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(const gitem_t &it)
{
	if (it.pickup_sound)
		gi.soundindex(it.pickup_sound);
	if (it.world_model)
		gi.modelindex(it.world_model);
	if (it.view_model)
		gi.modelindex(it.view_model);
	if (it.icon)
		gi.imageindex(it.icon);

	// parse everything for its ammo
	if (it.ammo && it.ammo != it.id)
		PrecacheItem(GetItemByIndex(it.ammo));

	// parse the space seperated precache string for other items
	if (!it.precaches)
		return;

	size_t s_pos = 0;

	while (true)
	{
		string v = strtok(it.precaches, s_pos);
		
		if (!v)
			break;

		size_t v_len = strlen(v);

		if (v_len >= MAX_QPATH || v_len < 5)
		{
			gi.dprintf("PrecacheItem: %s has bad precache string \"%s\"", it.classname, it.precaches);
			break;
		}

		string ext = substr(v, v_len - 3);

		// determine type based on extension
		if (ext == "md2")
			gi.modelindex(v);
		else if (ext == "sp2")
			gi.modelindex(v);
		else if (ext == "wav")
			gi.soundindex(v);
		else if (ext == "pcx")
			gi.imageindex(v);
		else
			gi.dprintf("PrecacheItem: %s has bad precache entry \"%s\"", it.classname, v.ptr());
	}
}

#ifdef GROUND_ZERO
void(entity ent) SetTriggeredSpawn;
#endif

#ifdef CTF
void(entity) CTFFlagSetup;
#endif

void SpawnItem(entity &ent, const gitem_t &it)
{
	if (ent.spawnflags
#ifdef GROUND_ZERO
		> ITEM_TRIGGER_SPAWN
#endif
#ifdef SINGLE_PLAYER
		&& it.id != ITEM_POWER_CUBE
#endif
		)
	{
		ent.spawnflags = NO_SPAWNFLAGS;
		gi.dprintf("%s at %s has invalid spawnflags set\n", it.classname, vtos(ent.s.origin).ptr());
	}

#ifdef SINGLE_PLAYER
	// some items will be prevented in deathmatch
	if (deathmatch)
	{
#endif
		if (((dm_flags)dmflags & DF_NO_ARMOR) && (it.pickup == Pickup_Armor || it.pickup == Pickup_PowerArmor))
			G_FreeEdict(ent);
		else if (((dm_flags)dmflags & DF_NO_ITEMS) && it.pickup == Pickup_Powerup)
			G_FreeEdict(ent);
		else if (((dm_flags)dmflags & DF_NO_HEALTH) && (it.pickup == Pickup_Health || it.pickup == Pickup_Adrenaline || it.pickup == Pickup_AncientHead))
			G_FreeEdict(ent);
		else if (((dm_flags)dmflags & DF_INFINITE_AMMO) && (it.flags == IT_AMMO || it.id == ITEM_BFG))
			G_FreeEdict(ent);
#ifdef GROUND_ZERO
		else if ((dmflags.intVal & DF_NO_MINES) && (ent.classname == "ammo_prox" || ent.classname == "ammo_tesla"))
			G_FreeEdict (ent);
		else if ((dmflags.intVal & DF_NO_NUKES) && ent.classname == "ammo_nuke")
			G_FreeEdict (ent);
#endif
#ifdef SINGLE_PLAYER
	}
#endif
#ifdef GROUND_ZERO
	else if (ent.classname == "ammo_nuke")
		G_FreeEdict(ent);
#endif

	// freed by the above
	if (!ent.inuse)
		return;

	PrecacheItem(it);

#ifdef SINGLE_PLAYER
	if (coop && it.id == ITEM_POWER_CUBE)
	{
		ent.spawnflags |= (spawn_flag)(1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if (coop && (it.flags & IT_STAY_COOP))
		// FIXME
		const_cast<gitem_t &>(it).drop = nullptr;
#endif

#ifdef CTF
//Don't spawn the flags unless enabled
	if (!ctf.intVal &&
		(ent->classname == "item_flag_team1" ||
		ent->classname == "item_flag_team2"))
	{
		G_FreeEdict(ent);
		return;
	}
#endif

	ent.item = it;
	ent.nextthink = level.framenum + 2;    // items start after other solids
	ent.think = droptofloor_savable;
	ent.s.effects = it.world_model_flags;
	ent.s.renderfx = RF_GLOW;
	ent.type = ET_ITEM;

	if (ent.model)
		gi.modelindex(ent.model);

#ifdef GROUND_ZERO
	if (ent.spawnflags & ITEM_TRIGGER_SPAWN)
		SetTriggeredSpawn (ent);
#endif

#ifdef CTF
	//flags are server animated and have special handling
	if (ent.classname == "item_flag_team1" ||
		ent.classname == "item_flag_team2")
		ent.think = CTFFlagSetup;
#endif
}

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags)dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags)dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/medium/tris.md2";
	self.count = 10;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/n_health.wav");
}

REGISTER_ENTITY(item_health, ET_ITEM);

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_small(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags)dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags)dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/stimpack/tris.md2";
	self.count = 2;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	self.style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
}

REGISTER_ENTITY(item_health_small, ET_ITEM);

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_large(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags)dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags)dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/healing/large/tris.md2";
	self.count = 25;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/l_health.wav");
}

REGISTER_ENTITY(item_health_large, ET_ITEM);

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
static void SP_item_health_mega(entity &self)
{
#ifdef SINGLE_PLAYER
	if (deathmatch && ((dm_flags)dmflags & DF_NO_HEALTH))
#else
	if ((dm_flags)dmflags & DF_NO_HEALTH)
#endif
	{
		G_FreeEdict(self);
		return;
	}

	self.model = "models/items/mega_h/tris.md2";
	self.count = 100;
	SpawnItem(self, GetItemByIndex(ITEM_HEALTH));
	gi.soundindex("items/m_health.wav");
	self.style = HEALTH_IGNORE_MAX | HEALTH_TIMED;
}

REGISTER_ENTITY(item_health_mega, ET_ITEM);

void SetItemNames()
{
	for (auto &it : item_list())
		gi.configstring((config_string)(CS_ITEMS + (config_string)it.id), it.pickup_name);
}