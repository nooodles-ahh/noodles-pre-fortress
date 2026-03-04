#include "cbase.h"


#ifdef CLIENT_DLL

void MainMenuForceTeamCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ExecuteClientCmd( "pf_mainmenu_reload" );
}

ConVar pf_muzzleflash( "pf_muzzleflash", "0", FCVAR_ARCHIVE, "Enable/Disable beta muzzleflash");
ConVar pf_muzzlelight( "pf_muzzlelight", "0", FCVAR_ARCHIVE, "Enable dynamic lights for muzzleflashes and the flamethrower" );
ConVar pf_projectilelight( "pf_projectilelight", "0", FCVAR_ARCHIVE, "Enable dynamic lights for grenades and rockets" );
ConVar pf_muzzlelightsprops( "pf_muzzlelightsprops", "0", FCVAR_ARCHIVE, "Enable dynamic lights affecting props" );
ConVar pf_burningplayerlight( "pf_burningplayerlight", "0", FCVAR_ARCHIVE, "Enable burning players emitting light" );
ConVar pf_showchatbubbles( "pf_showchatbubbles", "1", FCVAR_ARCHIVE, "Show bubble icons over typing players" );
ConVar pf_cheapbulletsplash( "pf_cheapbulletsplash", "0", FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Use the new, less intensive, bullet splash particle" );
ConVar pf_team_colored_spy_cloak( "pf_team_colored_spy_cloak", "1", FCVAR_ARCHIVE | FCVAR_DONTRECORD );
ConVar pf_blood_impact_disable( "pf_blood_impact_disable", "0", FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Disable blood impact effects");
ConVar pf_accessibility_concussion("pf_accessibility_concussion", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL/*, TODO - description*/);
ConVar pf_mainmenu_forceteam( "pf_mainmenu_forceteam", "-1", FCVAR_DONTRECORD | FCVAR_DEVELOPMENTONLY, "Force mainmenu to use a specific team", true, -1, true, 3, MainMenuForceTeamCallBack );

ConVar pf_alerts_armor( "pf_alerts_armor", "0", FCVAR_ARCHIVE );
ConVar pf_alerts_armor_percentage( "pf_alerts_armor_percentage", "49", FCVAR_ARCHIVE );
ConVar pf_alerts_armor_volume( "pf_alerts_armor_volume", "0.4", FCVAR_ARCHIVE, "", true, 0.0f, true, 1.0f );

ConVar pf_alerts_health( "pf_alerts_health", "0", FCVAR_ARCHIVE );
ConVar pf_alerts_health_percentage( "pf_alerts_health_percentage", "49", FCVAR_ARCHIVE );
ConVar pf_alerts_health_volume( "pf_alerts_health_volume", "1.0", FCVAR_ARCHIVE, "", true, 0.0f, true, 1.0f );
#else
ConVar pf_grenadepack_respawn_time( "pf_grenadepack_respawn_time", "15", FCVAR_NOTIFY, "Respawn time for grenade packs." );
ConVar pf_grenades_infinite("pf_grenades_infinite", "0", FCVAR_NOTIFY, "Player can throw an unlimited amount of grenades");
ConVar pf_concuss_effect_disable("pf_concuss_effect_disable", "0", FCVAR_NOTIFY | FCVAR_SERVER_CAN_EXECUTE, "Disables the camera shake from concussion grenade");
ConVar pf_healthkit_armor_repair( "pf_healthkit_armor_repair", "-1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enables armor repairing from health pickups: -1=auto, 0=off , 1=on" );
#endif
//ConVar pf_aprilfools("pf_aprilfools", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the April Fools holiday");
ConVar pf_delayed_knife( "pf_delayed_knife", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE, "Toggles the old delayed knife backstab" );
ConVar pf_allow_special_class( "pf_allow_special_class", "0", FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE | FCVAR_NOTIFY, "Enables the civilian class in non-escort gamemodes." );
ConVar pf_grenades_holstering("pf_grenades_holstering", "1", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar pf_haul_buildings("pf_haul_buildings", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow Engineers to haul buildings.");
ConVar pf_upgradable_buildings("pf_upgradable_buildings", "1", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE, "Allow Engineers to upgrade dispensers and teleporters.");
ConVar pf_flag_allow_throwing( "pf_flag_allow_throwing", "1", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE,  "Allows players to throw the flag." );
ConVar pf_flag_throw_force( "pf_flag_throw_force", "500", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE, "Max flag throwing force" );
ConVar pf_infinite_ammo( "pf_infinite_ammo", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_REPLICATED |FCVAR_SERVER_CAN_EXECUTE, "Enable infinite ammo." );
ConVar pf_force_crits( "pf_force_crits", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE, "Force crits ON/OFF" );

ConVar pf_tranq_speed_ratio( "pf_tranq_speed_ratio", "0.4", FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE );
ConVar pf_brokenleg_speed_ratio( "pf_brokenleg_speed_ratio", "0.4", FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE );
