#include "config.h"
#include "itemlist.h"
#include "general.h"
#include "armor.h"
#include "weaponry.h"
#include "powerups.h"
#include "keys.h"
#include "health.h"

#ifdef GRAPPLE
#include "game/ctf/grapple.h"
#endif

#include "game/weaponry/blaster.h"
#include "game/weaponry/shotgun.h"
#include "game/weaponry/supershotgun.h"
#include "game/weaponry/machinegun.h"
#include "game/weaponry/chaingun.h"
#include "game/weaponry/handgrenade.h"
#include "game/weaponry/grenadelauncher.h"
#include "game/weaponry/rocketlauncher.h"
#include "game/weaponry/hyperblaster.h"
#include "game/weaponry/railgun.h"
#include "game/weaponry/bfg.h"

#ifdef THE_RECKONING
#include "game/xatrix/weaponry/trap.h"
#include "game/xatrix/weaponry/ionripper.h"
#include "game/xatrix/weaponry/phalanx.h"
#include "game/xatrix/items.h"
#endif

#ifdef GROUND_ZERO
#include "game/rogue/weaponry/chainfist.h"
#include "game/rogue/weaponry/disruptor.h"
#include "game/rogue/weaponry/etf_rifle.h"
#include "game/rogue/weaponry/heatbeam.h"
#include "game/rogue/weaponry/tesla.h"
#include "game/rogue/items.h"
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
		.vwep_model = "#w_chainfist.md2",
		.icon = "w_chainfist",
		.pickup_name = "Chainfist",
		.flags = IT_WEAPON_MASK,
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
		.vwep_model = "#w_etfrifle.md2",
		.icon = "w_etf_rifle",
		.pickup_name = "ETF Rifle",
		.quantity = 1,
		.ammo = ITEM_FLECHETTES,
		.flags = IT_WEAPON_MASK,
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
		.vwep_model = "#a_grenades.md2",
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
		.vwep_model = "#a_grenades.md2",
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
		.vwep_model = "#w_plauncher.md2",
		.icon = "w_proxlaunch",
		.pickup_name = "Prox Launcher",
		.quantity = 1,
		.ammo = ITEM_PROX,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav weapons/proxwarn.wav weapons/proxopen.wav models/weapons/g_prox/tris.md2"
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
		.vwep_model = "#w_ripper.md2",
		.icon = "w_ripper",
		.pickup_name = "Ionripper",
		.quantity = 2,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
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
		.vwep_model = "#w_plasma.md2",
		.icon = "w_heatbeam",
		.pickup_name = "Plasma Beam",
		.quantity = 2,
		.ammo = ITEM_CELLS,
		.flags = IT_WEAPON_MASK,
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
		.classname = "weapon_phalanx",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Phalanx,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_shotx/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_shotx/tris.md2",
		.vwep_model = "#w_phalanx.md2",
		.icon = "w_phallanx",
		.pickup_name = "Phalanx",
		.quantity = 1,
		.ammo = ITEM_MAGSLUG,
		.flags = IT_WEAPON_MASK,
		.precaches = "weapons/plasshot.wav"
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
		.classname = "weapon_disintegrator",
		.pickup = Pickup_Weapon,
		.use = Use_Weapon,
		.drop = Drop_Weapon,
		.weaponthink = Weapon_Disruptor,
		.pickup_sound = "misc/w_pkup.wav",
		.world_model = "models/weapons/g_dist/tris.md2",
		.world_model_flags = EF_ROTATE,
		.view_model = "models/weapons/v_dist/tris.md2",
		.vwep_model = "#w_disrupt.md2",
		.icon = "w_disintegrator",
		.pickup_name = "Disruptor",
		.quantity = 1,
		.ammo = ITEM_ROUNDS,
		.flags = IT_WEAPON_MASK,
		.precaches = "models/items/spawngro/tris.md2 models/proj/disintegrator/tris.md2 weapons/disrupt.wav weapons/disint2.wav weapons/disrupthit.wav"
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
		.classname = "ammo_magslug",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/objects/ammo/tris.md2",
		.icon = "a_mslugs",
		.pickup_name = "Mag Slug",
		.quantity = 10,
		.flags = IT_AMMO,
		.ammotag = AMMO_MAGSLUG
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED ammo_flechettes (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "ammo_flechettes",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/ammo/am_flechette/tris.md2",
		.icon = "a_flechettes",
		.pickup_name = "Flechettes",
		.quantity = 50,
		.flags = IT_AMMO,
		.ammotag = AMMO_FLECHETTES
	},

	/*QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "ammo_prox",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/ammo/am_prox/tris.md2",
		.icon = "a_prox",
		.pickup_name = "Prox",
		.quantity = 5,
		.flags = IT_AMMO,
		.ammotag = AMMO_PROX
	},

	/*QUAKED ammo_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "ammo_nuke",
		.pickup = Pickup_Nuke,
		.use = Use_Nuke,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/weapons/g_nuke/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_nuke",
		.pickup_name = "A-M Bomb",
		.quantity = 300,
		.flags = IT_POWERUP,	
		.precaches = "weapons/nukewarn2.wav world/rumble.wav"
	},

	/*QUAKED ammo_disruptor (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "ammo_disruptor",
		.pickup = Pickup_Ammo,
		.drop = Drop_Ammo,
		.pickup_sound = "misc/am_pkup.wav",
		.world_model = "models/ammo/am_disr/tris.md2",
		.icon = "a_disruptor",
		.pickup_name = "Rounds",
		.quantity = 15,
		.flags = IT_AMMO,
		.ammotag = AMMO_DISRUPTOR,
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
		.classname = "item_quadfire", 
		.pickup = Pickup_Powerup,
		.use = Use_QuadFire,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/quadfire/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_quadfire",
		.pickup_name = "DualFire Damage",
		.quantity = 60,
		.flags = IT_POWERUP,
		.precaches = "items/quadfire1.wav items/quadfire2.wav items/quadfire3.wav"
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED item_ir_goggles (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "item_ir_goggles",
		.pickup = Pickup_Powerup,
		.use = Use_IR,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/goggles/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_ir",
		.pickup_name = "IR Goggles",
		.quantity = 60,
		.flags = IT_POWERUP,
		.precaches = "misc/ir_start.wav"
	},

	/*QUAKED item_double (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "item_double", 
		.pickup = Pickup_Powerup,
		.use = Use_Double,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/ddamage/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "p_double",
		.pickup_name = "Double Damage",
		.quantity = 60,
		.flags = IT_POWERUP,
		.precaches = "misc/ddamage1.wav misc/ddamage2.wav misc/ddamage3.wav"
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
		.classname = "key_green_key",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/items/keys/green_key/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "k_green",
		.pickup_name = "Green Key",
		.flags = IT_STAY_COOP | IT_KEY
	},
#endif

#ifdef GROUND_ZERO
	/*QUAKED key_nuke_container (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "key_nuke_container",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/weapons/g_nuke/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_contain",
		.pickup_name = "Antimatter Pod",
		.flags = IT_STAY_COOP | IT_KEY
	},

	/*QUAKED key_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
	*/
	{
		.classname = "key_nuke",
		.pickup = Pickup_Key,
		.drop = Drop_General,
		.pickup_sound = "items/pkup.wav",
		.world_model = "models/weapons/g_nuke/tris.md2",
		.world_model_flags = EF_ROTATE,
		.icon = "i_nuke",
		.pickup_name = "Antimatter Bomb",
		.flags = IT_STAY_COOP | IT_KEY
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

	/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_health",
		.pickup = Pickup_Health,
		.pickup_sound = "items/n_health.wav",
		.world_model = "models/items/healing/medium/tris.md2",
		.icon = "i_health",
		.pickup_name = "Health",
		.quantity = 10,
		.flags = IT_HEALTH
	},

	/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_health_small",
		.pickup = Pickup_Health,
		.pickup_sound = "items/s_health.wav",
		.world_model = "models/items/healing/stimpack/tris.md2",
		.icon = "i_health",
		.pickup_name = "Health",
		.quantity = 2,
		.flags = IT_HEALTH | IT_HEALTH_IGNORE_MAX
	},

	/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_health_large",
		.pickup = Pickup_Health,
		.pickup_sound = "items/l_health.wav",
		.world_model = "models/items/healing/large/tris.md2",
		.icon = "i_health",
		.pickup_name = "Health",
		.quantity = 25,
		.flags = IT_HEALTH
	},

	/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_health_mega",
		.pickup = Pickup_Health,
		.pickup_sound = "items/m_health.wav",
		.world_model = "models/items/mega_h/tris.md2",
		.icon = "i_health",
		.pickup_name = "Health",
		.quantity = 100,
		.flags = IT_HEALTH | IT_HEALTH_IGNORE_MAX | IT_HEALTH_TIMED
	},

#ifdef THE_RECKONING
	/*QUAKED item_foodcube (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		.classname = "item_health_mega",
		.pickup = Pickup_Health,
		.pickup_sound = "items/s_health.wav",
		.world_model = "models/objects/trapfx/tris.md2",
		.icon = "i_health",
		.pickup_name = "Health",
		.flags = IT_HEALTH | IT_HEALTH_IGNORE_MAX
	}
#endif
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

itemref::itemref() :
	itemref(ITEM_NONE)
{
}

itemref::itemref(std::nullptr_t) :
	itemref(ITEM_NONE)
{
}

// initialize a reference to a specific item
itemref::itemref(const gitem_id &ref) :
	_ptr(&GetItemByIndex(ref))
{
}

// check whether this itemref is holding an item
// reference or not
bool itemref::has_value() const
{
	return _ptr->id;
}