// Basic functions

void T_RadiusDamage2(edict_t* attacker, vec3_t position, float damage, float radius, int mod);
edict_t* findradius2(edict_t* from, vec3_t org, float rad);
qboolean TeamMembers(edict_t* p1, edict_t* p2);
qboolean infront2(edict_t* self, edict_t* other);
qboolean infront3(edict_t* self, edict_t* other);
qboolean infront4(edict_t* self, edict_t* other);
qboolean TouchingLadder(edict_t* self);
//HAVOC
void AddItemToList(edict_t* ent);
void RemoveFromList(edict_t* ent);
void		Bot_InitNodes(void);
qboolean	Bot_LoadNodes(void);
qboolean	Bot_SaveNodes(void);
void FakeDeath(edict_t* self);
void ClientCommand2(edict_t* ent); //Chaos commands
void ThrowUpNow(edict_t* self);
void ShowGun(edict_t* ent);	//vwep
void PreCacheAll(void);	//MATTHIAS
void GetSettings(void);
void FakeDeath(edict_t* self);
void ThrowUpNow(edict_t* self);
void Drop_Weapon(edict_t* ent, gitem_t* item);
void Cmd_Hook_f(edict_t* ent);

// Weapon banning console variables

cvar_t* ban_sword, * ban_chainsaw, * ban_supershotgun,
* ban_crossbow, * ban_airgun, * ban_grenadelauncher,
* ban_proxylauncher, * ban_rocketlauncher, * ban_hyperblaster,
* ban_railgun, * ban_buzzsaw, * ban_vortex, * ban_defenseturret,
* ban_rocketturret, * ban_bfg;

// Item banning console variables

cvar_t* ban_bodyarmor, * ban_combatarmor, * ban_jacketarmor,
* ban_armorshard, * ban_powerscreen, * ban_powershield,
* ban_ammo_grenades, * ban_ammo_flashgrenades,
* ban_ammo_lasergrenades, * ban_ammo_poisongrenades,
* ban_ammo_proximitymines, * ban_ammo_shells,
* ban_ammo_explosiveshells, * ban_ammo_arrows,
* ban_ammo_explosivearrows, * ban_ammo_poisonarrows,
* ban_ammo_cells, * ban_ammo_rockets, * ban_ammo_homingmissiles,
* ban_ammo_buzzes, * ban_ammo_slugs, * ban_quaddamage,
* ban_jetpack, * ban_invulnerability, * ban_invisibility,
* ban_silencer, * ban_rebreather, * ban_environmentsuit,
* ban_adrenaline, * ban_health, * ban_health_small,
* ban_health_large, * ban_health_mega;

// Startup weapon console variables

cvar_t* start_sword, * start_chainsaw, * start_supershotgun, * start_crossbow,
* start_airgun, * start_grenadelauncher, * start_proxylauncher,
* start_rocketlauncher, * start_hyperblaster, * start_railgun,
* start_buzzsaw, * start_bfg;

// Startup item console variables

cvar_t* start_defenseturret, * start_rocketturret,
* start_autosentry, * start_gravityvortex, * start_bodyarmor,
* start_combatarmor, * start_jacketarmor, * start_armorshard,
* start_powerscreen, * start_powershield, * start_ammo_grenades,
* start_ammo_flashgrenades, * start_ammo_lasergrenades,
* start_ammo_poisongrenades, * start_ammo_proximitymines,
* start_ammo_shells, * start_ammo_explosiveshells,
* start_ammo_arrows, * start_ammo_explosivearrows,
* start_ammo_poisonarrows, * start_ammo_cells,
* start_ammo_rockets, * start_ammo_homingmissiles,
* start_ammo_buzzes, * start_ammo_slugs, * start_quaddamage,
* start_jetpack, * start_invulnerability, * start_silencer,
* start_rebreather, * start_environmentsuit;

// Item definitions

gitem_t
/* MrG{DRGN}  Items converted by Chaos not in game */
* it_shotgun, * it_machinegun, * it_chaingun, * it_bullets,
/* MrG{DRGN} Weapons */
* it_ak42, * it_sword, * it_chainsaw, * it_supershotgun,
* it_esupershotgun, * it_crossbow, * it_poisoncrossbow,
* it_explosivecrossbow, * it_airfist, * it_grenadelauncher,
* it_flashgrenadelauncher, * it_poisongrenadelauncher,
* it_proxyminelauncher, * it_rocketlauncher, * it_hominglauncher,
* it_hyperblaster, * it_railgun, * it_buzzsaw, * it_bfg, * it_vortex, * it_rturret, * it_lturret,
/* MrG{DRGN} Ammo */
* it_shells, * it_eshells, * it_arrows, * it_poisonarrows,
* it_explosivearrows, * it_grenades, * it_flashgrenades,
* it_poisongrenades, * it_proxymines, * it_lasermines, * it_rockets,
* it_homings, * it_cells, * it_slugs, * it_buzzes,
/* MrG{DRGN} Health */
* it_health_generic, * it_health, * it_health_small,
* it_health_large, * it_health_mega,
/* MrG{DRGN} CTF items */
* it_tech1, * it_tech2, * it_tech3, * it_tech4, * it_flag_red, * it_flag_blue,
/* MrG{DRGN} Powerups */
* it_ancient_head, * it_quaddamage, * it_silencer, * it_adrenaline,
* it_invulnerability, * it_breather, * it_enviro, * it_bandolier,
* it_pack, * it_invisibility;

/* MrG{DRGN} TODO: Missing? */

cvar_t* node_debug;
cvar_t* lightsoff;
cvar_t* botchat;
cvar_t* blindtime;
cvar_t* poisontime;
cvar_t* lasertime;
cvar_t* proxytime;
cvar_t* defence_turret_ammo;
cvar_t* rocket_turret_ammo;
cvar_t* dntg;
cvar_t* lasermine_health;
cvar_t* ex_arrow_damage;
cvar_t* ex_arrow_radius;
cvar_t* start_invulnerable_time;

/* MrG{DRGN} Chaos DM Lives cvars */
cvar_t* drop_tech;	/* MrG{DRGN} tech drop prevention */
cvar_t* allow_flagdrop;	/* MrG{DRGN} allow flag dropping */
cvar_t* weapon_kick; /* MrG{DRGN} kickable weapons toggle */
cvar_t* tele_fire; /* MrG{DRGN} allow certain non-client projectiles to pass through teleporters */
cvar_t* do_respawn;	/* MrG{DRGN} base item respawn time default 60 */
cvar_t* do_respawn_rnd;	/* MrG{DRGN} random item respawn time default 80 */

#define LAYOUT_MAX_LENGTH              1400
