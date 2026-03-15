#ifndef PF_CVARS_H
#define PF_CVARS_H

#ifdef CLIENT_DLL
extern ConVar pf_muzzleflash;
extern ConVar pf_muzzlelight;
extern ConVar pf_projectilelight;
extern ConVar pf_muzzlelightsprops;
extern ConVar pf_burningplayerlight;
extern ConVar pf_showchatbubbles;
extern ConVar pf_cheapbulletsplash;
extern ConVar pf_team_colored_spy_cloak;
extern ConVar pf_blood_impact_disable;
extern ConVar pf_accessibility_concussion;

extern ConVar pf_mainmenu_forceteam;

extern ConVar pf_alerts_armor;
extern ConVar pf_alerts_armor_percentage;
extern ConVar pf_alerts_armor_volume;

extern ConVar pf_alerts_health;
extern ConVar pf_alerts_health_percentage;
extern ConVar pf_alerts_health_volume;
#else
extern ConVar pf_grenadepack_respawn_time;
extern ConVar pf_grenades_infinite;
extern ConVar pf_concuss_effect_disable;
extern ConVar pf_healthkit_armor_repair;
#endif
//extern ConVar pf_aprilfools;
extern ConVar pf_delayed_knife;
extern ConVar pf_allow_special_class;
extern ConVar pf_grenades_holstering;
extern ConVar pf_haul_buildings;
extern ConVar pf_dismantle_buildings;
extern ConVar pf_upgradable_buildings;
extern ConVar pf_flag_allow_throwing;
extern ConVar pf_flag_throw_force;
extern ConVar pf_infinite_ammo;
extern ConVar pf_force_crits;
extern ConVar pf_tranq_speed_ratio;
extern ConVar pf_brokenleg_speed_ratio;
#endif