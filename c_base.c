#include "g_local.h"
#include "c_botnav.h"
#include "c_cam.h"
#include "m_player.h"

void Cmd_Say_f(edict_t* ent, qboolean team, qboolean arg0);
void ClientBeginDeathmatch(edict_t* ent);
void TossClientWeapon(edict_t* self);
void ThrowUpNow(edict_t* self);
void ClientCommand2(edict_t* ent);
void FakeDeath(edict_t* self);
void Drop_Weapon(edict_t* ent, gitem_t* item);
void Cmd_Hook_f(edict_t* ent);
void Toggle_Scanner(edict_t* ent);
qboolean ClientConnect(edict_t* ent, char* userinfo);
void ClientDisconnect(edict_t* ent);
void ClientBegin(edict_t* ent);

///------------------------------------------------------------------------------------------
/// Misc functions
///------------------------------------------------------------------------------------------

qboolean visible(edict_t* self, edict_t* other)
{
	vec3_t	spot1 = { 0 }, spot2 = { 0 };
	trace_t	trace;
	/* MrG{DRGN} sanity check */
	if (!self || !other)
		return false;
	/* END */

	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);

	if (trace.fraction == 1.0)
		return true;
	return false;
}

qboolean infront(edict_t* self, edict_t* other)
{
	vec3_t	vec = { 0 };
	float	dot;
	vec3_t	forward;

	/* MrG{DRGN} sanity check */
	if (!self || !other)
		return false;
	/* END */

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3)
		return true;
	return false;
}

void ShowGun(edict_t* ent)	//vwep
{
	int		nIndex;
	char* pszIcon;

	/* MrG{DRGN} sanity check */
	if (!ent)
		return;
	/* END */

	if (!ent->client->pers.weapon)
	{
		ent->s.modelindex2 = 0;
		return;
	}

	// Determine the weapon's precache index.

	nIndex = 0;
	pszIcon = ent->client->pers.weapon->icon;

	if (strcmp(pszIcon, "w_ak42") == 0)
		nIndex = 1;
	else if (strcmp(pszIcon, "w_sword") == 0)
		nIndex = 2;
	else if (strcmp(pszIcon, "w_chainsaw") == 0)
		nIndex = 3;
	else if (strcmp(pszIcon, "w_sshotgun") == 0
		|| strcmp(pszIcon, "w_esshotgun") == 0)
		nIndex = 4;
	else if (strcmp(pszIcon, "w_bow") == 0
		|| strcmp(pszIcon, "w_pbow") == 0
		|| strcmp(pszIcon, "w_ebow") == 0)
		nIndex = 5;
	else if (strcmp(pszIcon, "w_airfist") == 0)
		nIndex = 6;
	else if (strcmp(pszIcon, "a_grenades1") == 0
		|| strcmp(pszIcon, "a_fgrenades") == 0
		|| strcmp(pszIcon, "a_pgrenades") == 0
		|| strcmp(pszIcon, "a_lmines") == 0)
		nIndex = 7;
	else if (strcmp(pszIcon, "w_glauncher1") == 0
		|| strcmp(pszIcon, "w_flauncher") == 0
		|| strcmp(pszIcon, "w_plauncher") == 0)
		nIndex = 8;
	else if (strcmp(pszIcon, "w_xlauncher") == 0)
		nIndex = 9;
	else if (strcmp(pszIcon, "w_rlauncher") == 0
		|| strcmp(pszIcon, "w_grlauncher") == 0)
		nIndex = 10;
	else if (strcmp(pszIcon, "w_hyperblaster") == 0)
		nIndex = 11;
	else if (strcmp(pszIcon, "w_railgun") == 0)
		nIndex = 12;
	else if (strcmp(pszIcon, "w_buzzsaw") == 0)
		nIndex = 13;
	else if (strcmp(pszIcon, "w_bfg") == 0)
		nIndex = 14;
	else if (strcmp(pszIcon, "a_vortex") == 0)
		nIndex = 15;
	else if (strcmp(pszIcon, "a_rturret") == 0
		|| strcmp(pszIcon, "a_lturret") == 0)
		nIndex = 16;
	else
		nIndex = 0;

	// Clear previous weapon model.
	ent->s.skinnum &= 255;

	// Set new weapon model.
	ent->s.skinnum |= (nIndex << 8);
	ent->s.modelindex2 = (VWEP_MODEL); /* MrG{DRGN} VWEP_MAX  */
}

qboolean TouchingLadder(edict_t* self)
{
	/* MrG{DRGN} sanity check */
	if (!self)
		return false;
	/* END */

	vec3_t org = { 0 };

	VectorCopy(self->s.origin, org);

	org[2] += 48;

	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += 8; org[1] += 8;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += -16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[1] += -16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += 16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] -= 8; org[1] += 8;
	org[2] -= 88;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	return false;
}

void ClearMaplist(void)
{
	int i;

	maplist.currentmap = -1;
	maplist.nummaps = 0;
	maplist.mlflag = 0; //off

	for (i = 0; i < MAX_MAPS; i++)
	{
		int p;

		maplist.ctf[i] = '0';
		maplist.lightsoff[i] = '0';

		for (p = 0; p < MAX_MAPNAME_LEN; p++)
		{
			maplist.mapnames[i][p] = 0;
		}
	}
}

void LoadMaplist(char* filename)
{
	FILE* fp = NULL;
	int		i = 0;
	char	file[MAX_QPATH];
	char	line[128];

	Com_strcpy(file, sizeof file, "./");
	Com_strcat(file, sizeof file, game_dir->string);
	Com_strcat(file, sizeof file, "/maplists/");
	Com_strcat(file, sizeof file, filename);
	if (!(strrchr(filename, '.')))	// user didn't add extension
		Com_strcat(file, sizeof file, ".txt");	// add a default

	//open the file
	if ((fp = fopen(file, "r")) == NULL)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Could not find file \"%s\".\n\n", file, strerror(errno));
		return;
	}

	ClearMaplist();

	i = 0;

	while ((!feof(fp)) && (i < MAX_MAPS))
	{
		size_t	len;

		if ((fgets(line, sizeof line, fp) == NULL) && feof(fp))
			gi.dprintf("%s file read error %s\n", __func__, file);
		len = strlen(line);

		if (len < 5) //invalid
			continue;
		if (line[0] == ';') //comment
			continue;

		len--;
		while (line[len] == '\n' || line[len] == '\r')
			len--;

		// lightsoff
		maplist.lightsoff[i] = line[len--];

		// ctf
		maplist.ctf[i] = line[len--];

		// mapname
		strncpy(maplist.mapnames[i], line, len);

		gi.cprintf(NULL, PRINT_HIGH, "...%s,ctf=%c,lightsoff=%c\n", maplist.mapnames[i], maplist.ctf[i], maplist.lightsoff[i]);
		i++;
	}

	maplist.nummaps = i;

	if (maplist.nummaps > 0)
		maplist.mlflag = g_maplistmode->value; //set per configured mode.
	fclose(fp);

	if (maplist.nummaps == 0)
	{
		gi.cprintf(NULL, PRINT_HIGH, "No maps listed in %s\n", file);
	}
	else
	{
		gi.cprintf(NULL, PRINT_HIGH, "%i map(s) loaded.\n\n", i);
	}
	gi.cprintf(NULL, PRINT_HIGH, "Map rotation is %s mode %i\n\n", maplist.mlflag ? "ON" : "OFF", maplist.mlflag);
}

void GetSettings()
{
	node_debug = gi.cvar("node_debug", "0", CVAR_USERINFO);
	blindtime = gi.cvar("blindtime", "20", CVAR_SERVERINFO);
	poisontime = gi.cvar("poisontime", "15", CVAR_SERVERINFO);
	lasertime = gi.cvar("lasertime", "60", CVAR_SERVERINFO);
	proxytime = gi.cvar("proxytime", "60", CVAR_SERVERINFO); /* MrG{DRGN} this was mistakenly also controlled by lasertime.*/
	defence_turret_ammo = gi.cvar("defence_turret_ammo", "1000", CVAR_SERVERINFO);
	rocket_turret_ammo = gi.cvar("rocket_turret_ammo", "90", CVAR_SERVERINFO);
	lasermine_health = gi.cvar("lasermine_health", "150", CVAR_LATCH);
	// FWP Set ex arrow strength and radius from server var
	ex_arrow_damage = gi.cvar("ex_arrow_damage", "80", CVAR_LATCH);
	ex_arrow_radius = gi.cvar("ex_arrow_radius", "200", CVAR_LATCH);

	dntg = gi.cvar("dntg", "1", CVAR_SERVERINFO);
	cosg = gi.cvar("cosg", "0", CVAR_SERVERINFO);
	start_invulnerable_time = gi.cvar("start_invulnerable_time", "3", CVAR_SERVERINFO);
	lightsoff = gi.cvar("lightsoff", "0", CVAR_SERVERINFO);
	botchat = gi.cvar("botchat", "1", CVAR_SERVERINFO);

	ban_sword = gi.cvar("ban_sword", "0", CVAR_LATCH);
	ban_chainsaw = gi.cvar("ban_chainsaw", "0", CVAR_LATCH);
	ban_supershotgun = gi.cvar("ban_supershotgun", "0", CVAR_LATCH);
	ban_crossbow = gi.cvar("ban_crossbow", "0", CVAR_LATCH);
	ban_airgun = gi.cvar("ban_airgun", "0", CVAR_LATCH);
	ban_grenadelauncher = gi.cvar("ban_grenadelauncher", "0", CVAR_LATCH);
	ban_proxylauncher = gi.cvar("ban_proxylauncher", "0", CVAR_LATCH);
	ban_rocketlauncher = gi.cvar("ban_rocketlauncher", "0", CVAR_LATCH);
	ban_hyperblaster = gi.cvar("ban_hyperblaster", "0", CVAR_LATCH);
	ban_railgun = gi.cvar("ban_railgun", "0", CVAR_LATCH);
	ban_buzzsaw = gi.cvar("ban_buzzsaw", "0", CVAR_LATCH);
	ban_defenseturret = gi.cvar("ban_defenseturret", "0", CVAR_LATCH);
	ban_rocketturret = gi.cvar("ban_rocketturret", "0", CVAR_LATCH);
	ban_vortex = gi.cvar("ban_vortex", "0", CVAR_LATCH);
	ban_bfg = gi.cvar("ban_bfg", "0", CVAR_LATCH);

#ifdef	CHAOS_RETAIL
	ban_grapple = gi.cvar("ban_grapple", "1", CVAR_LATCH);
	ban_jetpack = gi.cvar("ban_jetpack", "1", CVAR_LATCH);
#else
	ban_grapple = gi.cvar("ban_grapple", "0", CVAR_LATCH);
	ban_jetpack = gi.cvar("ban_jetpack", "0", CVAR_LATCH);
#endif

	ban_bodyarmor = gi.cvar("ban_bodyarmor", "0", CVAR_LATCH);
	ban_combatarmor = gi.cvar("ban_combatarmor", "0", CVAR_LATCH);
	ban_jacketarmor = gi.cvar("ban_jacketarmor", "0", CVAR_LATCH);
	ban_armorshard = gi.cvar("ban_armorshard ", "0", CVAR_LATCH);
	ban_powerscreen = gi.cvar("ban_powerscreen", "0", CVAR_LATCH);
	ban_powershield = gi.cvar("ban_powershield", "0", CVAR_LATCH);
	ban_ammo_grenades = gi.cvar("ban_ammo_grenades", "0", CVAR_LATCH);
	ban_ammo_flashgrenades = gi.cvar("ban_ammo_flashgrenades", "0", CVAR_LATCH);
	ban_ammo_lasergrenades = gi.cvar("ban_ammo_lasergrenades", "0", CVAR_LATCH);
	ban_ammo_poisongrenades = gi.cvar("ban_ammo_poisongrenades", "0", CVAR_LATCH);
	ban_ammo_proximitymines = gi.cvar("ban_ammo_proximitymines", "0", CVAR_LATCH);
	ban_ammo_shells = gi.cvar("ban_ammo_shells", "0", CVAR_LATCH);
	ban_ammo_explosiveshells = gi.cvar("ban_ammo_explosiveshells", "0", CVAR_LATCH);
	ban_ammo_arrows = gi.cvar("ban_ammo_arrows", "0", CVAR_LATCH);
	ban_ammo_explosivearrows = gi.cvar("ban_ammo_explosivearrows", "0", CVAR_LATCH);
	ban_ammo_poisonarrows = gi.cvar("ban_ammo_poisonarrows", "0", CVAR_LATCH);
	ban_ammo_cells = gi.cvar("ban_ammo_cells", "0", CVAR_LATCH);
	ban_ammo_rockets = gi.cvar("ban_ammo_rockets", "0", CVAR_LATCH);
	ban_ammo_homingmissiles = gi.cvar("ban_ammo_homingmissiles", "0", CVAR_LATCH);
	ban_ammo_buzzes = gi.cvar("ban_ammo_buzzes", "0", CVAR_LATCH);
	ban_ammo_slugs = gi.cvar("ban_ammo_slugs", "0", CVAR_LATCH);
	ban_quaddamage = gi.cvar("ban_quaddamage", "0", CVAR_LATCH);
	ban_invulnerability = gi.cvar("ban_invulnerability", "0", CVAR_LATCH);
	ban_invisibility = gi.cvar("ban_invisibility", "0", CVAR_LATCH);
	ban_adrenaline = gi.cvar("ban_adrenaline", "0", CVAR_LATCH);
	ban_silencer = gi.cvar("ban_silencer", "0", CVAR_LATCH);
	ban_rebreather = gi.cvar("ban_rebreather", "0", CVAR_LATCH);
	ban_environmentsuit = gi.cvar("ban_environmentsuit", "0", CVAR_LATCH);

	ban_health = gi.cvar("ban_health", "0", CVAR_LATCH);
	ban_health_small = gi.cvar("ban_health_small", "0", CVAR_LATCH);
	ban_health_large = gi.cvar("ban_health_large", "0", CVAR_LATCH);
	ban_health_mega = gi.cvar("ban_health_mega", "0", CVAR_LATCH);

	start_sword = gi.cvar("start_sword", "0", CVAR_LATCH);

	start_chainsaw = gi.cvar("start_chainsaw", "0", CVAR_LATCH);
	start_supershotgun = gi.cvar("start_supershotgun", "0", CVAR_LATCH);
	start_crossbow = gi.cvar("start_crossbow", "0", CVAR_LATCH);
	start_airgun = gi.cvar("start_airgun", "0", CVAR_LATCH);
	start_grenadelauncher = gi.cvar("start_grenadelauncher", "0", CVAR_LATCH);
	start_proxylauncher = gi.cvar("start_proxylauncher", "0", CVAR_LATCH);
	start_rocketlauncher = gi.cvar("start_rocketlauncher", "0", CVAR_LATCH);
	start_hyperblaster = gi.cvar("start_hyperblaster", "0", CVAR_LATCH);
	start_railgun = gi.cvar("start_railgun", "0", CVAR_LATCH);
	start_buzzsaw = gi.cvar("start_buzzsaw", "0", CVAR_LATCH);
	start_bfg = gi.cvar("start_bfg", "0", CVAR_LATCH);

	start_grapple = gi.cvar("start_grapple", "0", CVAR_LATCH);
	start_jetpack = gi.cvar("start_jetpack", "0", CVAR_LATCH);
	start_gravityvortex = gi.cvar("start_gravityvortex", "0", CVAR_LATCH);
	start_defenseturret = gi.cvar("start_defenseturret", "0", CVAR_LATCH);
	start_rocketturret = gi.cvar("start_rocketturret", "0", CVAR_LATCH);
	start_bodyarmor = gi.cvar("start_bodyarmor", "0", CVAR_LATCH);
	start_combatarmor = gi.cvar("start_combatarmor", "0", CVAR_LATCH);
	start_jacketarmor = gi.cvar("start_jacketarmor", "0", CVAR_LATCH);
	start_armorshard = gi.cvar("start_armorshard", "0", CVAR_LATCH);
	start_powerscreen = gi.cvar("start_powerscreen", "0", CVAR_LATCH);
	start_powershield = gi.cvar("start_powershield", "0", CVAR_LATCH);
	start_ammo_grenades = gi.cvar("start_ammo_grenades", "0", CVAR_LATCH);
	start_ammo_flashgrenades = gi.cvar("start_ammo_flashgrenades", "0", CVAR_LATCH);
	start_ammo_lasergrenades = gi.cvar("start_ammo_lasergrenades", "0", CVAR_LATCH);
	start_ammo_poisongrenades = gi.cvar("start_ammo_poisongrenades", "0", CVAR_LATCH);
	start_ammo_proximitymines = gi.cvar("start_ammo_proximitymines", "0", CVAR_LATCH);
	start_ammo_shells = gi.cvar("start_ammo_shells", "0", CVAR_LATCH);
	start_ammo_explosiveshells = gi.cvar("start_ammo_explosiveshells", "0", CVAR_LATCH);
	start_ammo_arrows = gi.cvar("start_ammo_arrows", "0", CVAR_LATCH);
	start_ammo_poisonarrows = gi.cvar("start_ammo_poisonarrows", "0", CVAR_LATCH);
	start_ammo_explosivearrows = gi.cvar("start_ammo_explosivearrows", "0", CVAR_LATCH);
	start_ammo_cells = gi.cvar("start_ammo_cells", "0", CVAR_LATCH);
	start_ammo_rockets = gi.cvar("start_ammo_rockets", "0", CVAR_LATCH);
	start_ammo_homingmissiles = gi.cvar("start_ammo_homingmissiles", "0", CVAR_LATCH);
	start_ammo_buzzes = gi.cvar("start_ammo_buzzes", "0", CVAR_LATCH);
	start_ammo_slugs = gi.cvar("start_ammo_slugs", "0", CVAR_LATCH);
	start_quaddamage = gi.cvar("start_quaddamage", "0", CVAR_LATCH);
	start_invulnerability = gi.cvar("start_invulnerability", "0", CVAR_LATCH);
	start_silencer = gi.cvar("start_silencer", "0", CVAR_LATCH);
	start_rebreather = gi.cvar("start_rebreather", "0", CVAR_LATCH);
	start_environmentsuit = gi.cvar("start_environmentsuit", "0", CVAR_LATCH);
}

qboolean infront2(edict_t* self, edict_t* other)
{
	vec3_t	vec = { 0 };
	float	dot;
	vec3_t	forward;

	/* MrG{DRGN} sanity check */
	if (!self || !other)
		return false;
	/* END */

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.4)
		return true;
	return false;
}

qboolean infront3(edict_t* self, edict_t* other)
{
	vec3_t	vec = { 0 };
	float	dot;
	vec3_t	forward;

	/* MrG{DRGN} sanity check */
	if (!self || !other)
		return false;
	/* END */

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.7)
		return true;
	return false;
}

qboolean infront4(edict_t* self, edict_t* other)
{
	vec3_t	vec = { 0 };
	float	dot;
	vec3_t	forward;

	/* MrG{DRGN} sanity check */
	if (!self || !other)
		return false;
	/* END */

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.85)
		return true;
	return false;
}

void PreCacheAll()
{
	//pics
	gi.imageindex("scanner/dot");
	gi.imageindex("scanner/down");
	gi.imageindex("scanner/up");
	gi.imageindex("scanner/scan01");
	gi.imageindex("scanner/scan02");
	gi.imageindex("scanner/scan03");
	gi.imageindex("scanner/scan04");
	gi.imageindex("scanner/scan05");
	gi.imageindex("scanner/scan06");
	gi.imageindex("scanner/scan07");
	gi.imageindex("scanner/scan08");
	gi.imageindex("scanner/scan09");
	gi.imageindex("scanner/scan10");
	gi.imageindex("scanner/scan11");
	gi.imageindex("scanner/scan12");
	gi.imageindex("scanner/scan13");
	gi.imageindex("scanner/scan14");
	gi.imageindex("scanner/scan15");
	gi.imageindex("scanner/scan16");
	gi.imageindex("scanner/scan17");
	gi.imageindex("scanner/scan18");
	gi.imageindex("a_eshells");
	gi.imageindex("a_arrows");
	gi.imageindex("a_grockets");
	gi.imageindex("a_parrows");
	gi.imageindex("a_earrows");
	gi.imageindex("a_buzz");
	gi.imageindex("a_fgrenades");
	gi.imageindex("a_lmines");
	gi.imageindex("a_lturret");
	gi.imageindex("a_pgrenades");
	gi.imageindex("a_xgrenades");
	gi.imageindex("a_rturret");
	gi.imageindex("a_vortex");
	gi.imageindex("i_grapple");
	gi.imageindex("p_invis");
	gi.imageindex("p_jet");
	gi.imageindex("p_kamikaze");
	gi.imageindex("w_airfist");
	gi.imageindex("w_bow");
	gi.imageindex("w_buzzsaw");
	gi.imageindex("w_ebow");
	gi.imageindex("w_esshotgun");
	gi.imageindex("w_flauncher");
	gi.imageindex("w_grlauncher");
	gi.imageindex("w_plauncher");
	gi.imageindex("w_pbow");
	gi.imageindex("w_xlauncher");
	gi.imageindex("w_sword");
	gi.imageindex("w_chainsaw");
	//models
	gi.modelindex("models/weapons/v_sword/tris.md2");
	gi.modelindex("models/weapons/g_sword/tris.md2");
	gi.modelindex("models/weapons/v_chsaw/tris.md2");
	gi.modelindex("models/weapons/g_chsaw/tris.md2");
	gi.modelindex("models/weapons/v_eshot2/tris.md2");
	gi.modelindex("models/weapons/g_eshot2/tris.md2");
	gi.modelindex("models/weapons/v_bow/tris.md2");
	gi.modelindex("models/weapons/g_bow/tris.md2");
	gi.modelindex("models/weapons/v_pbow/tris.md2");
	gi.modelindex("models/weapons/g_pbow/tris.md2");
	gi.modelindex("models/weapons/v_ebow/tris.md2");
	gi.modelindex("models/weapons/g_ebow/tris.md2");
	gi.modelindex("models/weapons/v_air/tris.md2");
	gi.modelindex("models/weapons/g_air/tris.md2");
	gi.modelindex("models/weapons/v_flalau/tris.md2");
	gi.modelindex("models/weapons/g_flalau/tris.md2");
	gi.modelindex("models/weapons/v_poilau/tris.md2");
	gi.modelindex("models/weapons/g_poilau/tris.md2");
	gi.modelindex("models/weapons/v_proxyl/tris.md2");
	gi.modelindex("models/weapons/g_proxyl/tris.md2");
	gi.modelindex("models/weapons/v_guided/tris.md2");
	gi.modelindex("models/weapons/g_guided/tris.md2");
	gi.modelindex("models/weapons/v_buzzsw/tris.md2");
	gi.modelindex("models/weapons/g_buzzsw/tris.md2");
	gi.modelindex("models/weapons/v_flashg/tris.md2");
	gi.modelindex("models/weapons/v_poison/tris.md2");
	gi.modelindex("models/weapons/v_laserg/tris.md2");
	gi.modelindex("models/weapons/v_vortx/tris.md2");
	gi.modelindex("models/weapons/v_ltur/tris.md2");
	gi.modelindex("models/weapons/v_rtur/tris.md2");
	gi.modelindex("models/objects/dummy/tris.md2");
	gi.modelindex("models/objects/t_base/tris.md2");
	gi.modelindex("models/objects/t_rocket/tris.md2");
	gi.modelindex("models/objects/lturret/tris.md2");
	gi.modelindex("models/objects/rturret/tris.md2");
	gi.modelindex("models/objects/rtrthrow/tris.md2");
	gi.modelindex("models/objects/ltrthrow/tris.md2");
	gi.modelindex("models/objects/arngs/tris.md2");
	gi.modelindex("models/objects/arrow/tris.md2");
	gi.modelindex("models/objects/avortex/tris.md2");
	gi.modelindex("models/objects/buzz/tris.md2");
	gi.modelindex("models/objects/earrow/tris.md2");
	gi.modelindex("models/objects/gflash/tris.md2");
	gi.modelindex("models/objects/gpoison/tris.md2");
	//gi.modelindex("models/objects/hgevilpr/tris.md2");
	gi.modelindex("models/objects/hgflash/tris.md2");
	gi.modelindex("models/objects/hglaser/tris.md2");
	gi.modelindex("models/objects/hgpoison/tris.md2");
	gi.modelindex("models/objects/hgproxy/tris.md2");
	gi.modelindex("models/objects/homing/tris.md2");
	gi.modelindex("models/objects/parrow/tris.md2");
	gi.modelindex("models/objects/rings/tris.md2");
	gi.modelindex("models/objects/selftp/tris.md2");
	gi.modelindex("models/items/ammo/arrows/tris.md2");
	gi.modelindex("models/items/ammo/bullets/explo/tris.md2");
	gi.modelindex("models/items/ammo/buzz/tris.md2");
	gi.modelindex("models/items/ammo/earrows/tris.md2");
	gi.modelindex("models/items/ammo/parrows/tris.md2");
	gi.modelindex("models/items/ammo/lturret/tris.md2");
	gi.modelindex("models/items/ammo/rturret/tris.md2");
	gi.modelindex("models/items/ammo/grenades/flashg/tris.md2");
	gi.modelindex("models/items/ammo/grenades/laserg/tris.md2");
	gi.modelindex("models/items/ammo/grenades/poison/tris.md2");
	gi.modelindex("models/items/ammo/grenades/proxy/tris.md2");
	gi.modelindex("models/items/ammo/rockets/guided/tris.md2");
	gi.modelindex("models/items/grapple/tris.md2");
	gi.modelindex("models/items/invis/tris.md2");
	gi.modelindex("models/items/jet/tris.md2");


	//sound
	gi.soundindex("hook/hit.wav");
	gi.soundindex("hook/chain1.wav");
	gi.soundindex("hook/chain2.wav");
	gi.soundindex("hook/retract.wav");
	gi.soundindex("hook/smack.wav");
	gi.soundindex("weapons/buzz/buzzfire.wav");
	gi.soundindex("weapons/buzz/buzzflsh.wav");
	gi.soundindex("weapons/buzz/buzzrico.wav");
	gi.soundindex("weapons/buzz/buzzwhrl.wav");
	gi.soundindex("weapons/air/agfail.wav");
	gi.soundindex("weapons/air/agfire.wav");
	gi.soundindex("weapons/air/agwater.wav");
	gi.soundindex("weapons/air/fly.wav");
	gi.soundindex("weapons/crossbow/catch.wav");
	gi.soundindex("weapons/crossbow/fly.wav");
	gi.soundindex("weapons/crossbow/hit1.wav");
	gi.soundindex("weapons/crossbow/hit3.wav");
	gi.soundindex("weapons/crossbow/release1.wav");
	gi.soundindex("items/invis.wav");
	gi.soundindex("items/invis2.wav");
	gi.soundindex("weapons/sword/swingl.wav");
	gi.soundindex("weapons/sword/swingr.wav");
	gi.soundindex("weapons/sword/hitwall.wav");
	gi.soundindex("weapons/turret/online.wav");
	gi.soundindex("weapons/turret/rockfly.wav");
	gi.soundindex("weapons/turret/rockshot.wav");
	gi.soundindex("weapons/turret/throw.wav");
	gi.soundindex("weapons/vortex/ready.wav");
	gi.soundindex("weapons/vortex/storm.wav");
	gi.soundindex("weapons/vortex/throw.wav");
	gi.soundindex("weapons/chainsw/chaincu1.wav");
	gi.soundindex("weapons/chainsw/chaincu2.wav");
	gi.soundindex("weapons/chainsw/chaincu3.wav");
	gi.soundindex("weapons/chainsw/chainend.wav");
	gi.soundindex("weapons/chainsw/chainidl.wav");
	gi.soundindex("weapons/chainsw/chainstr.wav");
	gi.soundindex("misc/fakedeath.wav");
	gi.soundindex("misc/heartbeat.wav");
	gi.soundindex("misc/kamikaze.wav");
	gi.soundindex("misc/kick.wav");
	gi.soundindex("misc/placetelep.wav");
	gi.soundindex("misc/proxy.wav");
	gi.soundindex("misc/radar.wav");
	gi.soundindex("misc/selfteleport.wav");
	gi.soundindex("misc/sneeze1.wav");
	gi.soundindex("misc/sneeze2.wav");
	gi.soundindex("misc/sonar.wav");
	gi.soundindex("misc/vomit1.wav");
	gi.soundindex("misc/vomit2.wav");
	gi.soundindex("misc/vomit3.wav");
	gi.soundindex("misc/vomit4.wav");
	gi.soundindex("misc/bubbles1.wav");
	gi.soundindex("misc/bubbles2.wav");
	gi.soundindex("misc/burp.wav");
}

qboolean TeamMembers(edict_t* p1, edict_t* p2)
{
	if (!p1)
		return 0;
	if (!p1->client)
		return 0;
	if (!p2)
		return 0;
	if (!p2->client)
		return 0;

	if (ctf->value)
	{
		return (p1->client->resp.ctf_team == p2->client->resp.ctf_team);
	}
	else
	{
		// neutral player
		if (!p1->client->resp.team || !p2->client->resp.team)
			return false;

		return (p1->client->resp.team == p2->client->resp.team);
	}
}

void LoadMOTD(void)
{
	FILE* fp;
	char file[MAX_QPATH];
	char line[80];
	size_t i;

	Com_strcpy(file, sizeof file, "./");
	Com_strcat(file, sizeof file, game_dir->string);
	Com_strcat(file, sizeof file, "/motd.txt");

	/* 	if ((fp = fopen(file, "r")) != NULL)
	{
		if (fgets(motd, 500, fp) )
		{
				charcnt = 0;
			while (charcnt < 559 && ( fgets(line, 80,fp) ))
			{
				strcat(motd, line);
				charcnt += 80;
			}
				if (charcnt >= 559)
			   gi.dprintf("MOTD length exceeded maximum (560) , may be truncated.\n");
		}
		fclose(fp);
	}

	*/


	if ((fp = fopen(file, "r")) == NULL)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Could not find file \"%s\".\n\n", file, strerror(errno));
		return;
	}

	i = 0;

	while ((!feof(fp)) && (i < sizeof motd))
	{
		size_t	len;
		if (fgets(line, sizeof line, fp) == NULL) {
			if (!feof(fp))
				gi.dprintf("%s problem reading %s\n", __func__, file);
		}
		len = strlen(line);
		while (line[len] == '\n' || line[len] == '\r')
			len--;

		if ((i + len) < sizeof motd)
			strncpy(motd + i, line, len);
		else
			gi.dprintf("MOTD is too long (> %u chars), truncated\n", sizeof motd);
		i += len;
	}

	//close file
	fclose(fp);
}

void FakeDeath(edict_t* self)
{
	vec3_t              mins = { -16, -16, -24 };
	vec3_t              maxs = { 16, 16, 32 };

	/* MrG{DRGN} sanity check */
	if (!self)
		return;
	/* END */

	/* MrG{DRGN} replaced with integer comparision
	if (Q_strcasecmp (self->classname, "camera") == 0)
	*/
	if (self->classindex == CAMPLAYER || self->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't fakedeath! */
		return;

	if (self->client->fakedeath == 0)	//fake
	{
		if (self->client->PoisonTime > 0)
		{
			cprintf2(self, PRINT_HIGH, "You can't fake death because you are poisoned!\n");
			return;
		}

		self->client->fakedeath = 1;

		VectorClear(self->avelocity);

		self->takedamage = DAMAGE_YES;
		self->movetype = MOVETYPE_TOSS;

		self->s.modelindex2 = 0;	// remove linked weapon model
		self->s.modelindex3 = 0;	// remove linked ctf flag
		//self->client->ps.gunindex = 0;

		//self->client->killer_yaw = self->s.angles[YAW];
		self->s.angles[0] = 0;
		self->s.angles[2] = 0;



		self->maxs[2] = -8;

		self->client->ps.pmove.pm_type = PM_DEAD;

		if (self->client->pers.weapon != it_ak42)
			TossClientWeapon(self);

		CTFDeadDropFlag(self);
		CTFDeadDropTech(self);

		self->s.sound = 0;
		self->client->weapon_sound = 0;
		self->client->quad_framenum = 0;
		self->client->invincible_framenum = 0;
		self->client->invisible_framenum = 0;
		self->client->breather_framenum = 0;
		self->client->enviro_framenum = 0;
		self->client->jet_framenum = 0;
		self->client->flashlightactive = 0;
		if (self->client->flashlight)
		{
			self->client->flashlight->think = G_FreeEdict;
			G_FreeEdict(self->client->flashlight);
		}

		// start a death animation
		self->client->anim_priority = ANIM_DEATH;

		int i = 0; /* MrG{DRGN} moved here */
		if (random() < 0.33)
			i = 1;
		if (random() < 0.33)
			i = 2;

		if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			self->s.frame = FRAME_crdeath1 - 1;
			self->client->anim_end = FRAME_crdeath5;
		}
		else switch (i)
		{
		case 0:
			self->s.frame = FRAME_death101 - 1;
			self->client->anim_end = FRAME_death106;
			break;
		case 1:
			self->s.frame = FRAME_death201 - 1;
			self->client->anim_end = FRAME_death206;
			break;
		case 2:
			self->s.frame = FRAME_death301 - 1;
			self->client->anim_end = FRAME_death308;
			break;
		}
		if (random() > 0.2)
			gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1, ATTN_NORM, 0);
		else
			gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fakedeath.wav"), 1, ATTN_NORM, 0);

		self->s.modelindex = (PLAYER_MODEL); /* MrG{DRGN} no Magic Number 255 */
		self->deadflag = DEAD_DEAD;



		gi.linkentity(self);
	}
	else	// revive
	{
		self->client->fakedeath = 0;

		self->takedamage = DAMAGE_AIM;
		self->viewheight = 22;
		self->inuse = true;
		self->solid = SOLID_BBOX;
		self->deadflag = DEAD_NO;
		self->clipmask = MASK_PLAYERSOLID;

		VectorCopy(mins, self->mins);
		VectorCopy(maxs, self->maxs);
		VectorClear(self->velocity);
		VectorClear(self->avelocity);

		if (/* MrG{DRGN} always DM deathmatch->value && */((int)dmflags->value & DF_FIXED_FOV))
		{
			self->client->ps.fov = 90;
		}
		else
		{
			self->client->ps.fov = atoi(Info_ValueForKey(self->client->pers.userinfo, "fov"));
			if (self->client->ps.fov < 1)
				self->client->ps.fov = 90;
			else if (self->client->ps.fov > 160)
				self->client->ps.fov = 160;
		}

		self->client->ps.gunindex = gi.modelindex(self->client->pers.weapon->view_model);

		self->s.effects = 0;
		self->s.modelindex = (PLAYER_MODEL); /* MrG{DRGN} no Magic Number 255 */

		self->s.origin[2] += 1;  // make sure off ground

		self->s.frame = FRAME_stand01;
		self->client->anim_end = FRAME_stand40;

		self->client->pers.weapon = it_ak42;
		self->client->newweapon = self->client->pers.weapon;
		ChangeWeapon(self);
		ShowGun(self);	//vwep

		self->movetype = MOVETYPE_WALK;

		//self->svflags 	= 0;
		self->client->ps.pmove.pm_type = PM_NORMAL;

		gi.linkentity(self);

		// force the current weapon up
		self->client->newweapon = self->client->pers.weapon;
		ChangeWeapon(self);
	}
}

void FlashLightThink(edict_t* ent)
{
	vec3_t start, end, endp, offset = { 0 };
	vec3_t forward, right, up;
	trace_t tr;

	/* MrG{DRGN} sanity check */
	if (!ent)
		return;
	/* END */
	if (!ent->owner ||
		!ent->owner->client)
		return;

	AngleVectors(ent->owner->client->v_angle, forward, right, up);

	VectorSet(offset, 24, 6, ent->owner->viewheight - 7.0F); /* MrG{DRGN} Explicitly now a float */
	G_ProjectSource(ent->owner->s.origin, offset, forward, right, start);
	VectorMA(start, 8192, forward, end);

	tr = gi.trace(start, NULL, NULL, end, ent->owner, CONTENTS_SOLID);

	if ((tr.fraction != 1) || tr.startsolid)
	{
		VectorMA(tr.endpos, -10, forward, endp);
		VectorCopy(endp, tr.endpos);
	}

	vectoangles(tr.plane.normal, ent->s.angles);
	VectorCopy(tr.endpos, ent->s.origin);

	gi.linkentity(ent);
	ent->nextthink = level.time + 0.1F; /* MrG{DRGN} Explicitly now a float */
}

void T_RadiusDamage2(edict_t* attacker, vec3_t position, float damage, float radius, int mod)
{
	float	points;
	edict_t* ent = NULL;
	vec3_t	v = { 0 };
	vec3_t	dir = { 0 };

	while ((ent = findradius(ent, position, radius)) != NULL)
	{
		if (!ent->takedamage)
			continue;

		VectorSubtract(ent->s.origin, position, v);
		points = damage - 0.5F * VectorLength(v); /* MrG{DRGN} Explicitly now a float */
		//		if (ent == attacker)
			//	points = points * 0.5;
		if (points > 0)
		{
			//			if (CanDamage (ent, attacker))
			//	{
			VectorSubtract(ent->s.origin, position, dir);
			T_Damage(ent, attacker, attacker, dir, position, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			//	}
		}
	}
}

edict_t* findradius2(edict_t* from, vec3_t org, float rad)	//find all entities
{
	vec3_t	eorg = { 0 };
	int		j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		//if (from->solid != SOLID_NOT)
		//	continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5F); /* MrG{DRGN} Explicitly now a float */
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}

static char	BPrint2Buff[0x2000]; /*  MrG{DRGN} move this here  and reduce the size*/

void bprintf2(int printlevel, char* fmt, ...)
{
	int i;
	/*  MrG{DRGN} unused!
	char	bigbuffer[0x10000];
	int		len; */
	va_list		argptr;
	edict_t* cl_ent;

	va_start(argptr, fmt);
	/*len = vsprintf (bigbuffer,fmt,argptr);MrG{DRGN} use vsnprintf */
	vsnprintf(BPrint2Buff, sizeof(BPrint2Buff), fmt, argptr);
	va_end(argptr);

	if (dedicated->value)
		gi.cprintf(NULL, printlevel, BPrint2Buff);

	for (i = 0; i < maxclients->value; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || cl_ent->bot_player) /* MrG{DRGN} */
			continue;

		gi.cprintf(cl_ent, printlevel, BPrint2Buff);
	}
}
static char	CPrint2Buff[0x2000]; /*  MrG{DRGN} move this here  and reduce the size*/

void cprintf2(edict_t* ent, int printlevel, char* fmt, ...)
{
	//char	bigbuffer[0x10000];
	/* int		len;  MrG{DRGN} unused! */
	va_list		argptr;

	if (!ent || ent->bot_player)/* MrG{DRGN} */
		return;

	va_start(argptr, fmt);
	/*len = vsprintf (bigbuffer,fmt,argptr);MrG{DRGN} use vsnprintf */
	vsnprintf(CPrint2Buff, sizeof(CPrint2Buff), fmt, argptr);
	va_end(argptr);

	if (ent->inuse) /* MrG{DRGN} */
	{
		gi.cprintf(ent, printlevel, CPrint2Buff);
	}
}
static char	NPrintBuff[0x2000]; /*  MrG{DRGN} move this here  and reduce the size*/
void nprintf(int printlevel, char* fmt, ...)
{
	int i;
	//char	bigbuffer[0x10000];
	/* int		len;  MrG{DRGN} unused! */
	va_list		argptr;
	edict_t* cl_ent;

	if (!node_debug->value)
		return;

	va_start(argptr, fmt);
	/*len = vsprintf (bigbuffer,fmt,argptr);MrG{DRGN} use vsnprintf */
	vsnprintf(NPrintBuff, sizeof(NPrintBuff), fmt, argptr);
	va_end(argptr);

	for (i = 0; i < maxclients->value; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || cl_ent->bot_player)
			continue;

		gi.cprintf(cl_ent, printlevel, NPrintBuff);
	}
}

void ThrowUpNow(edict_t* self)
{
	vec3_t  forward, right;
	vec3_t  mouth_pos, spew_vector;
	float	rn;

	AngleVectors(self->client->v_angle, forward, right, NULL);

	VectorScale(forward, 24, mouth_pos);
	VectorAdd(mouth_pos, self->s.origin, mouth_pos);
	mouth_pos[2] += self->viewheight;

	VectorScale(forward, 24, spew_vector);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BLOOD);
	gi.WritePosition(mouth_pos);
	gi.WriteDir(spew_vector);
	gi.multicast(mouth_pos, MULTICAST_PVS);

	mouth_pos[2] -= 10;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BLOOD);
	gi.WritePosition(mouth_pos);
	gi.WriteDir(spew_vector);
	gi.multicast(mouth_pos, MULTICAST_PVS);

	mouth_pos[2] -= 10;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BLOOD);
	gi.WritePosition(mouth_pos);
	gi.WriteDir(spew_vector);
	gi.multicast(mouth_pos, MULTICAST_PVS);

	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit1.wav"), 1, ATTN_NORM, 0);

	rn = random();
	if (rn < 0.25)
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit1.wav"), 1, ATTN_NORM, 0);
	else if (0.25 <= rn && rn < 0.5) /* MrG{DRGN} made this work properly by adding && rn 09/23/2020*/
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit2.wav"), 1, ATTN_NORM, 0);
	else if (0.5 <= rn && rn < 0.75) /* MrG{DRGN} made this work properly by adding && rn 09/23/2020*/
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit4.wav"), 1, ATTN_NORM, 0);
}

void Teleport_Think(edict_t* ent)
{
	ent->s.frame += 1;
	if (ent->s.frame > 9)
		ent->s.frame = 0;

	ent->nextthink = level.time + 0.1F; /* MrG{DRGN} Explicitly now a float */
}

void Teleport(edict_t* ent)
{
	edict_t* telep = NULL;
	vec3_t spawn_origin = { 0 };

	/* MrG{DRGN} sanity check */
	if (!ent || !ent->client)
	{
		return;
	}
	/* END */

	if (ent->client->teleporter)	//teleport
	{
		int i; /* MrG{DRGN} */
		if (ctf->value)
			CTFDeadDropFlag(ent);

		telep = ent->client->teleporter;

		VectorCopy(telep->s.origin, spawn_origin);
		spawn_origin[2] += 9;

		/* MrG{DRGN}
		ent->client->ps.pmove.origin[0] = spawn_origin[0] * 8;
		ent->client->ps.pmove.origin[1] = spawn_origin[1] * 8;
		ent->client->ps.pmove.origin[2] = spawn_origin[2] * 8;
		*/

		for (i = 0; i < 3; i++) {
			ent->client->ps.pmove.origin[i] = COORD2SHORT(spawn_origin[i]);
		}
		/* END */
		ent->s.event = EV_ITEM_RESPAWN;
		spawn_origin[2] += 1;
		VectorCopy(spawn_origin, ent->s.origin);

		if (!KillBox(ent))
		{	// could't spawn in?
		}

		gi.sound(telep, CHAN_VOICE, gi.soundindex("misc/selfteleport.wav"), 1, ATTN_NORM, 0);

		//kill teleport point
		telep->s.event = EV_ITEM_RESPAWN; // Make an item respawn effect ! Look cool ! :)
		G_FreeEdict(telep);
		ent->client->teleporter = NULL;
	}
	else	//create teleporter
	{
		telep = G_Spawn();
		telep->owner = ent;
		telep->solid = SOLID_BBOX;
		telep->classname = "selfteleporter";
		telep->classindex = SELFTELEPORTER;
		telep->s.event = EV_ITEM_RESPAWN;
		telep->s.renderfx = RF_TRANSLUCENT;
		telep->s.effects |= EF_FLAG2;
		telep->clipmask = MASK_SHOT;
		VectorClear(telep->mins);
		VectorClear(telep->maxs);
		/*	telep->owner = ent; redundant reassignment! */

		telep->movetype = MOVETYPE_FLYMISSILE;
		telep->model = "models/objects/selftp/tris.md2";
		gi.setmodel(telep, telep->model);
		VectorCopy(ent->s.origin, telep->s.origin);
		telep->s.origin[2] -= 10;
		telep->think = Teleport_Think;
		telep->nextthink = level.time + 0.1F; /* MrG{DRGN} Explicitly now a float */

		gi.linkentity(telep);
		ent->client->teleporter = telep;

		gi.sound(telep, CHAN_VOICE, gi.soundindex("misc/placetelep.wav"), 1, ATTN_NORM, 0);
	}
}

///------------------------------------------------------------------------------------------
/// Weapon switching
///------------------------------------------------------------------------------------------

void Use_Class2(edict_t* ent)
{
	if (ent->client->pers.weapon == it_sword)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_chainsaw)] > 0)
			ent->client->newweapon = it_chainsaw;
	}
	else if (ent->client->pers.weapon == it_chainsaw)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_sword)] > 0)
			ent->client->newweapon = it_sword;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_sword)] > 0)
			ent->client->newweapon = it_sword;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_chainsaw)] > 0)
			ent->client->newweapon = it_chainsaw;
	}
}

void Use_Class3(edict_t* ent)
{
	if (ent->client->pers.weapon == it_supershotgun)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_esupershotgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_eshells)] > 1)
			ent->client->newweapon = it_esupershotgun;
	}
	else if (ent->client->pers.weapon == it_esupershotgun)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_supershotgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_shells)] > 1)
			ent->client->newweapon = it_supershotgun;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_supershotgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_shells)] > 1)
			ent->client->newweapon = it_supershotgun;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_esupershotgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_eshells)] > 1)
			ent->client->newweapon = it_esupershotgun;
	}
}

void Use_Class4(edict_t* ent)
{
	if (ent->client->pers.weapon == it_crossbow)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_poisonarrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] > 0)
			ent->client->newweapon = it_poisoncrossbow;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_explosivearrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] > 0)
			ent->client->newweapon = it_explosivecrossbow;
	}
	else if (ent->client->pers.weapon == it_poisoncrossbow)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_explosivearrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] > 0)
			ent->client->newweapon = it_explosivecrossbow;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_arrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_crossbow)] > 0)
			ent->client->newweapon = it_crossbow;
	}
	else if (ent->client->pers.weapon == it_explosivecrossbow)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_arrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_crossbow)] > 0)
			ent->client->newweapon = it_crossbow;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisonarrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] > 0)
			ent->client->newweapon = it_poisoncrossbow;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_arrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_crossbow)] > 0)
			ent->client->newweapon = it_crossbow;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisonarrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] > 0)
			ent->client->newweapon = it_poisoncrossbow;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_explosivearrows)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] > 0)
			ent->client->newweapon = it_explosivecrossbow;
	}
}

void Use_Class5(edict_t* ent)
{
	if (ent->client->pers.weapon != it_airfist)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_airfist)] > 0)
			ent->client->newweapon = it_airfist;
	}
}
void Use_Class6(edict_t* ent)
{
	if (ent->client->pers.weapon == it_grenadelauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] > 0)
			ent->client->newweapon = it_flashgrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] > 0)
			ent->client->newweapon = it_poisongrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_proxymines)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)] > 0)
			ent->client->newweapon = it_proxyminelauncher;
	}
	else if (ent->client->pers.weapon == it_flashgrenadelauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] > 0)
			ent->client->newweapon = it_poisongrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_proxymines)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)] > 0)
			ent->client->newweapon = it_proxyminelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_grenadelauncher)] > 0)
			ent->client->newweapon = it_grenadelauncher;
	}
	else if (ent->client->pers.weapon == it_poisongrenadelauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_proxymines)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)] > 0)
			ent->client->newweapon = it_proxyminelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_grenadelauncher)] > 0)
			ent->client->newweapon = it_grenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] > 0)
			ent->client->newweapon = it_flashgrenadelauncher;
	}
	else if (ent->client->pers.weapon == it_proxyminelauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_grenadelauncher)] > 0)
			ent->client->newweapon = it_grenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] > 0)
			ent->client->newweapon = it_flashgrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] > 0)
			ent->client->newweapon = it_poisongrenadelauncher;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_grenadelauncher)] > 0)
			ent->client->newweapon = it_grenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] > 0)
			ent->client->newweapon = it_flashgrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] > 0)
			ent->client->newweapon = it_poisongrenadelauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_proxymines)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)] > 0)
			ent->client->newweapon = it_proxyminelauncher;
	}
}
void Use_Class7(edict_t* ent)
{
	if (ent->client->pers.weapon == it_rocketlauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_hominglauncher)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_homings)] > 0)
			ent->client->newweapon = it_hominglauncher;
	}
	else if (ent->client->pers.weapon == it_hominglauncher)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_rocketlauncher)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_rockets)] > 0)
			ent->client->newweapon = it_rocketlauncher;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_rocketlauncher)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_rockets)] > 0)
			ent->client->newweapon = it_rocketlauncher;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_hominglauncher)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_homings)] > 0)
			ent->client->newweapon = it_hominglauncher;
	}
}
void Use_Class8(edict_t* ent)
{
	if (ent->client->pers.weapon != it_hyperblaster)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_hyperblaster)] > 0)
			ent->client->newweapon = it_hyperblaster;
	}
}
void Use_Class9(edict_t* ent)
{
	if (ent->client->pers.weapon == it_railgun)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_buzzsaw)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_buzzes)] > 0)
			ent->client->newweapon = it_buzzsaw;
	}
	else if (ent->client->pers.weapon == it_buzzsaw)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_railgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_slugs)] > 0)
			ent->client->newweapon = it_railgun;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_railgun)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_slugs)] > 0)
			ent->client->newweapon = it_railgun;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_buzzsaw)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_buzzes)] > 0)
			ent->client->newweapon = it_buzzsaw;
	}
}

void Use_Class0(edict_t* ent)
{
	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	*/
	if (ent->client->pers.weapon == it_bfg)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_rturret)] > 0)
			ent->client->newweapon = it_rturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lturret)] > 0)
			ent->client->newweapon = it_lturret;
	}
	else if (ent->client->pers.weapon == it_vortex)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_rturret)] > 0)
			ent->client->newweapon = it_rturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lturret)] > 0)
			ent->client->newweapon = it_lturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) /* MrG{DRGN} was 0 */
			ent->client->newweapon = it_bfg;
	}
	else if (ent->client->pers.weapon == it_rturret)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_lturret)] > 0)
			ent->client->newweapon = it_lturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) /* MrG{DRGN} was 0 */
			ent->client->newweapon = it_bfg;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
	}
	else if (ent->client->pers.weapon == it_lturret)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) /* MrG{DRGN} was 0 */
			ent->client->newweapon = it_bfg;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_rturret)] > 0)
			ent->client->newweapon = it_rturret;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) /* MrG{DRGN} was 0 */
			ent->client->newweapon = it_bfg;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_rturret)] > 0)
			ent->client->newweapon = it_rturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lturret)] > 0)
			ent->client->newweapon = it_lturret;
	}
}

void Use_Grenades(edict_t* ent)
{
	if (ent->client->pers.weapon == it_grenades)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0)
			ent->client->newweapon = it_flashgrenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lasermines)] > 0)
			ent->client->newweapon = it_lasermines;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0)
			ent->client->newweapon = it_poisongrenades;
	}
	else if (ent->client->pers.weapon == it_flashgrenades)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_lasermines)] > 0)
			ent->client->newweapon = it_lasermines;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0)
			ent->client->newweapon = it_poisongrenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0)
			ent->client->newweapon = it_flashgrenades;
	}
	else if (ent->client->pers.weapon == it_lasermines)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0)
			ent->client->newweapon = it_poisongrenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0)
			ent->client->newweapon = it_grenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0)
			ent->client->newweapon = it_flashgrenades;
	}
	else if (ent->client->pers.weapon == it_poisongrenades)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0)
			ent->client->newweapon = it_grenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0)
			ent->client->newweapon = it_flashgrenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lasermines)] > 0)
			ent->client->newweapon = it_lasermines;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_grenades)] > 0)
			ent->client->newweapon = it_grenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] > 0)
			ent->client->newweapon = it_flashgrenades;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_lasermines)] > 0)
			ent->client->newweapon = it_lasermines;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] > 0)
			ent->client->newweapon = it_poisongrenades;
	}
}


/* MrG{DRGN} for testing */
void ED_CallSpawn(edict_t* ent);
void Cmd_Spawn_f(edict_t* ent)
{
	char* param = NULL;
	edict_t* spawn = NULL;


	param = gi.argv(1);

	if (param && strlen(param))
	{
		vec3_t forward;
		spawn = G_Spawn();
		spawn->classname = param;
		AngleVectors(ent->s.angles, forward, NULL, NULL);
		VectorMA(ent->s.origin, 256, forward, spawn->s.origin);
		spawn->s.origin[2] += 32;
		/* spawn->s.angles[3] = (0,0,0); */
		VectorCopy(ent->s.angles, spawn->s.angles);
		ED_CallSpawn(spawn);
		gi.unlinkentity(spawn);
		KillBox(spawn);
		gi.linkentity(spawn);
		spawn->s.renderfx |= RF_IR_VISIBLE;
	}
}
/* END */
///------------------------------------------------------------------------------------------
/// Command handling
///------------------------------------------------------------------------------------------

void ClientCommand2(edict_t* ent)
{
	char* cmd;

	if (!ent->client)
		return;

	cmd = gi.argv(0);

	if (!Q_strcasecmp(cmd, "grapple"))
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't use the hook! */
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;

		if (ent->client->pers.inventory[ITEM_INDEX(it_grapple)] >= 1)
			Cmd_Hook_f(ent);
		else
		{
			if (!ent->bot_player)/* MrG{DRGN} */
				gi.centerprintf(ent, "\nYou don't have a grappling hook!\n");//MATTHIAS

			return;
		}
	}
	else if (Q_strcasecmp(cmd, "class2") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class2(ent);
	}
	else if (Q_strcasecmp(cmd, "class3") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class3(ent);
	}
	else if (Q_strcasecmp(cmd, "class4") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class4(ent);
	}
	else if (Q_strcasecmp(cmd, "class5") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class5(ent);
	}
	else if (Q_strcasecmp(cmd, "class6") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class6(ent);
	}
	else if (Q_strcasecmp(cmd, "class7") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class7(ent);
	}
	else if (Q_strcasecmp(cmd, "class8") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class8(ent);
	}
	else if (Q_strcasecmp(cmd, "class9") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class9(ent);
	}
	else if (Q_strcasecmp(cmd, "class0") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class0(ent);
	}
	else if (Q_strcasecmp(cmd, "grenades") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Grenades(ent);
	}
	else if (Q_strcasecmp(cmd, "throwup") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't puke! */
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		if (level.time > ent->client->nextvomit)
		{
			ThrowUpNow(ent);
			ent->client->nextvomit = level.time + 1;
		}
	}
	else if (Q_strcasecmp(cmd, "zoom") == 0)
	{
		int zoomtype = atoi(gi.argv(1));

		if (ent->health <= 0)
			return;

		if (zoomtype == 0)
		{
			ent->client->ps.fov = 90;
		}
		else if (zoomtype == 1)
		{
			if (ent->client->ps.fov == 90) ent->client->ps.fov = 40;
			else if (ent->client->ps.fov == 40) ent->client->ps.fov = 20;
			else if (ent->client->ps.fov == 20) ent->client->ps.fov = 10;
			else ent->client->ps.fov = 90;
		}
	}
	else if (Q_strcasecmp(cmd, "camera") == 0)
	{
		if (Q_strcasecmp(gi.argv(1), "0") == 0)	//cam off
		{
			if (ent->client->camera)
			{
				char name[MAX_INFO_KEY], skin[MAX_INFO_KEY], hand[MAX_INFO_KEY], fov[MAX_INFO_KEY]; /* MrG{DRGN} fix for camera not returning players fov to normal*/

				Com_Printf(name, Info_ValueForKey(ent->client->pers.userinfo, "name"));
				Com_Printf(skin, Info_ValueForKey(ent->client->pers.userinfo, "skin"));
				Com_Printf(hand, Info_ValueForKey(ent->client->pers.userinfo, "hand"));
				Com_Printf(fov, Info_ValueForKey(ent->client->pers.userinfo, "fov"));/* MrG{DRGN} "" */

				ClientDisconnect(ent);
				ClientConnect(ent, ent->client->pers.userinfo);
				Info_SetValueForKey(ent->client->pers.userinfo, "name", name);
				Info_SetValueForKey(ent->client->pers.userinfo, "skin", skin);
				Info_SetValueForKey(ent->client->pers.userinfo, "hand", hand);
				Info_SetValueForKey(ent->client->pers.userinfo, "fov", fov); /* MrG{DRGN} "" */

				ClientBegin(ent);
				cprintf2(ent, PRINT_HIGH, "Camera OFF!\n");
			}
		}
		else if (Q_strcasecmp(gi.argv(1), "1") == 0)	//intelli mode
		{
			if (!ent->client->camera)
				CreateCamera(ent);

			ent->client->cammode = 1;
			cprintf2(ent, PRINT_HIGH, "IntelliCam Mode!\n");
		}
		else if (Q_strcasecmp(gi.argv(1), "2") == 0)	//chase cam mode
		{
			if (!ent->client->camera)
				CreateCamera(ent);

			ent->client->cammode = 2;
			cprintf2(ent, PRINT_HIGH, "ChaseCam Mode!\n");
		}
		else if (Q_strcasecmp(gi.argv(1), "3") == 0)	// birdview chase cam
		{
			if (!ent->client->camera)
				CreateCamera(ent);

			ent->client->cammode = 3;
			cprintf2(ent, PRINT_HIGH, "Birdview ChaseCam Mode!\n");
		}
		else if (Q_strcasecmp(gi.argv(1), "4") == 0)	// TV cam mode
		{
			if (!ent->client->camera)
				CreateCamera(ent);

			ent->client->cammode = 4;
			cprintf2(ent, PRINT_HIGH, "TV-Cam Mode!\n");
		}
	}
	else if (Q_strcasecmp(cmd, "pathdebug") == 0)
	{
		if (!ent->client->b_target)
		{
			ent->client->b_target = G_Spawn();
			ent->client->b_target->movetype = MOVETYPE_NONE;
			ent->client->b_target->solid = SOLID_NOT;
			ent->client->b_target->s.modelindex = gi.modelindex("models/objects/gibs/skull/tris.md2");
			VectorCopy(ent->s.origin, ent->client->b_target->s.origin);
			gi.linkentity(ent->client->b_target);
			cprintf2(ent, PRINT_HIGH, "Pathdebug ON!\n");
		}
		else
		{
			G_FreeEdict(ent->client->b_target);
			ent->client->b_target = NULL;
			cprintf2(ent, PRINT_HIGH, "Pathdebug OFF!\n");
		}


	}
	else if (Q_strcasecmp(cmd, "scanner") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't puke! */
			return;

		Toggle_Scanner(ent);
	}
	else if (Q_strcasecmp(cmd, "placenode") == 0)
	{
		if (dntg->value)
		{
			vec3_t	end = { 0 }, spot = { 0 };
			trace_t	tr;

			//check if node is in air
			VectorCopy(ent->s.origin, end);
			end[2] -= 40;

			tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_SOLID);

			/*
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BFG_LASER);
			gi.WritePosition (nodes[nindex].origin);
			gi.WritePosition (end);
			gi.multicast (nodes[nindex].origin, MULTICAST_PHS);
			*/

			if (!tr.startsolid && (tr.fraction == 1))
			{
				Bot_PlaceNode(ent->s.origin, INAIR_NODE, 1);
			}
			else
			{
				VectorCopy(ent->s.origin, spot);

				if (!(ent->client->ps.pmove.pm_flags & PMF_DUCKED))
				{
					spot[2] += 5;
					Bot_PlaceNode(spot, NORMAL_NODE, 0);
				}
				else
				{
					Bot_PlaceNode(spot, NORMAL_NODE, 1);
				}
			}

			Bot_CalcNode(ent, numnodes);
		}
		else
			cprintf2(ent, PRINT_HIGH, "Dynamic Node Table Generation is off activate it with <set dntg 1>!\n");
	}
	else if (Q_strcasecmp(cmd, "belt") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't puke! */
			return;

		if (ent->client->fakedeath > 0)
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->beltactive > 0)
		{
			cprintf2(ent, PRINT_HIGH, "Anti gravity belt OFF\n");
			ent->client->beltactive = 0;
			ent->client->nextbeltcell = level.time;
		}
		else
		{
			if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] <= 0)
			{
				cprintf2(ent, PRINT_HIGH, "You don't have enough cells to run your anti gravity belt!\n");
				ent->client->beltactive = 0;
				ent->client->nextbeltcell = level.time;
			}
			else
			{
				cprintf2(ent, PRINT_HIGH, "Anti gravity belt ON\n");
				ent->client->beltactive = 1;
				ent->client->nextbeltcell = level.time + 2;
			}
		}
	}
	else if (Q_strcasecmp(cmd, "flashlight") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't use the flashlight! */
			return;

		if (ent->client->fakedeath > 0)
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->flashlightactive)
		{
			ent->client->flashlightactive = false;
			if (ent->client->flashlight)
				ent->client->flashlight->think = G_FreeEdict;
			cprintf2(ent, PRINT_HIGH, "Flashlight OFF\n");
		}
		else
		{
			vec3_t  start, forward, right, end = { 0 };

			ent->client->flashlightactive = true;

			AngleVectors(ent->client->v_angle, forward, right, NULL);

			VectorSet(end, 100, 0, 0);
			G_ProjectSource(ent->s.origin, end, forward, right, start);

			ent->client->flashlight = G_Spawn();
			ent->client->flashlight->think = FlashLightThink;
			ent->client->flashlight->nextthink = level.time + 0.1F; /* MrG{DRGN} Explicitly now a float */
			ent->client->flashlight->s.effects = EF_HYPERBLASTER;
			ent->client->flashlight->s.modelindex = gi.modelindex("models/objects/dummy/tris.md2");
			ent->client->flashlight->solid = SOLID_NOT;
			ent->client->flashlight->owner = ent;
			ent->client->flashlight->classname = "flashlight";
			ent->client->flashlight->classindex = FLASHLIGHT;
			ent->client->flashlight->movetype = MOVETYPE_NOCLIP;
			ent->client->flashlight->clipmask = MASK_SHOT;
			VectorCopy(end, ent->client->flashlight->s.origin);

			gi.linkentity(ent->client->flashlight);

			cprintf2(ent, PRINT_HIGH, "Flashlight ON\n");
		}
	}
	else if (Q_strcasecmp(cmd, "teleport") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't teleport! */
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		if (ent->client->teleporter)	//teleport
		{
			Teleport(ent);
		}
		else	//create teleporter
		{
			if (ent->client->pers.inventory[ITEM_INDEX(it_cells)] < 100)
			{
				cprintf2(ent, PRINT_HIGH, "You need 100 cells to place a self teleporter!\n");
			}
			else
			{
				ent->client->pers.inventory[ITEM_INDEX(it_cells)] -= 100;
				Teleport(ent);
				cprintf2(ent, PRINT_HIGH, "Self Teleporter placed! Use cmd teleport again to use it.\n");
			}
		}
	}
	else if (Q_strcasecmp(cmd, "kamikaze") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't puke! */
			return;
		if (ent->health <= 0)
			return;
		if (ent->client->kamikazetime != 0)
			return;
		if (ent->flags & FL_GODMODE)
		{
			cprintf2(ent, PRINT_MEDIUM, "You can't go kamikaze in god mode, cheater!\n");
			return;
		}
		if (ent->client->pers.inventory[ITEM_INDEX(it_rockets)] + ent->client->pers.inventory[ITEM_INDEX(it_grenades)] + ent->client->pers.inventory[ITEM_INDEX(it_homings)] < 10)
		{
			cprintf2(ent, PRINT_MEDIUM, "You need at least 10 rockets or grenades to go kamikaze!\n");
			return;
		}

		ent->client->kamikazetime = 50;
		ent->s.effects = EF_ROCKET;
		gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/kamikaze.wav"), 1, ATTN_NORM, 0);
	}
	else if (Q_strcasecmp(cmd, "togglegrenades") == 0)
	{
		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't toggle nades */
			return;
		if (ent->client->grenadesactive == 1)
		{
			ent->client->grenadesactive = 0;
			cprintf2(ent, PRINT_HIGH, "Grenades OFF\n");
		}
		else
		{
			cprintf2(ent, PRINT_HIGH, "Grenades ON\n");
			ent->client->grenadesactive = 1;
		}
	}
	else if (Q_strcasecmp(cmd, "fakedeath") == 0)
	{
		if (ent->health <= 0)
			return;

		FakeDeath(ent);
	}
	else if (Q_strcasecmp(cmd, "kick") == 0)
	{
		edict_t* blip = NULL;
		vec3_t	forward;

		if (ent->classindex == CAMPLAYER || ent->movetype == MOVETYPE_NOCLIP)/* MrG{DRGN} if you haven't joined a team yet. you can't kick */
			return;
		if (ent->client->fakedeath > 0)
			return;
		if (ent->health <= 0)
			return;

		while ((blip = findradius2(blip, ent->s.origin, 100)) != NULL)
		{
			/*if (blip->client
				|| blip->item
				|| Q_strcasecmp(blip->classname, "bolt") == 0
				|| Q_strcasecmp(blip->classname, "arrow") == 0
				|| Q_strcasecmp(blip->classname, "grenade") == 0
				|| Q_strcasecmp(blip->classname, "hgrenade") == 0
				|| Q_strcasecmp(blip->classname, "flashgrenade") == 0
				|| Q_strcasecmp(blip->classname, "lasermine") == 0
				|| Q_strcasecmp(blip->classname, "poisongrenade") == 0
				|| Q_strcasecmp(blip->classname, "proxymine") == 0
				|| Q_strcasecmp(blip->classname, "rocket") == 0
				|| Q_strcasecmp(blip->classname, "homing") == 0
				|| Q_strcasecmp(blip->classname, "buzz") == 0
				|| Q_strcasecmp(blip->classname, "bfg blast") == 0
				|| Q_strcasecmp(blip->classname, "item_flag_team1") == 0 // MrG{DRGN} to kick, or not to kick... that is the question? left here on mistake by prior coder!
				|| Q_strcasecmp(blip->classname, "item_flag_team2") == 0// MrG{DRGN} to kick, or not to kick... that is the question?

				|| Q_strcasecmp(blip->classname, "bodyque") == 0)

				if (Q_strcasecmp(blip->classname, "laser_turret_base") != 0 // MrG{DRGN} This doesn't exist! Should have simply been "turret_base".
					&& Q_strcasecmp(blip->classname, "rocket_turret_base") != 0 // MrG{DRGN} This doesn't exist! Should have simply been "turret_base".
					&& Q_strcasecmp(blip->classname, "laser_turret") != 0
					&& Q_strcasecmp(blip->classname, "rocket_turret") != 0
					&& Q_strcasecmp(blip->classname, "item_flag_team1") != 0 // MrG{DRGN} Not kickable!
					&& Q_strcasecmp(blip->classname, "item_flag_team2") != 0) // MrG{DRGN} Not kickable!
					*/
					/* MrG{DRGN} much faster than a string comparison!*/
			if (blip->client
				|| blip->item
				|| blip->classindex == BOLT
				|| blip->classindex == ARROW
				|| blip->classindex == GRENADE
				|| blip->classindex == HGRENADE
				|| blip->classindex == FLASHGRENADE
				|| blip->classindex == LASERMINE
				|| blip->classindex == PGRENADE
				|| blip->classindex == PROXYMINE
				|| blip->classindex == ROCKET
				|| blip->classindex == HOMING
				|| blip->classindex == BUZZ
				|| blip->classindex == BFG_BLAST
				|| blip->classindex == ITEM_FLAG_TEAM1
				|| blip->classindex == ITEM_FLAG_TEAM2
				|| blip->classindex == BODYQUE
				|| blip->classindex == GIBHEAD)

				if (blip->classindex != TBASE
					&& blip->classindex != LTURRET
					&& blip->classindex != RTURRET
					&& blip->classindex != ITEM_FLAG_TEAM1
					&& blip->classindex != ITEM_FLAG_TEAM2)
				{
					{
						if (blip == ent)
							continue;
						if (!visible(ent, blip))
							continue;
						if (!infront(ent, blip))
							continue;

						AngleVectors(ent->client->v_angle, forward, NULL, NULL);

						VectorScale(forward, 400, blip->velocity);
						blip->velocity[2] = 400;

						gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/kick.wav"), 1, ATTN_NORM, 0);

						if (blip->client && blip->client->camera)
							ent->client->b_waittime = level.time + 3;

						return;
					}
				}/* MrG{DRGN} END */
		}
	}
	else if (Q_strcasecmp(cmd, "load_nodes") == 0)
	{
		Bot_LoadNodes();
	}
	else if (Q_strcasecmp(cmd, "save_nodes") == 0)
	{
		Bot_SaveNodes();
	}
	else if (Q_strcasecmp(cmd, "nums") == 0)
	{
		cprintf2(ent, PRINT_HIGH, "numplayers=%d\n", numplayers);
		cprintf2(ent, PRINT_HIGH, "numbots=%d\n", numbots);
		cprintf2(ent, PRINT_HIGH, "numred=%d\n", numred);
		cprintf2(ent, PRINT_HIGH, "numblue=%d\n", numblue);
		cprintf2(ent, PRINT_HIGH, "red_base=%d\n", red_base);
		cprintf2(ent, PRINT_HIGH, "blue_base=%d\n", blue_base);
		cprintf2(ent, PRINT_HIGH, "numturrets=%d\n", numturrets);
	}
	else if (Q_strcasecmp(cmd, "join_team") == 0)
	{
		int team = atoi(gi.argv(1));

		if (team < 0 || team > 99)
			cprintf2(ent, PRINT_HIGH, "\nPlease select a team between 1 and 99!\nSelect 0 for no team!\n");
		else
		{
			if (team == 0)
				cprintf2(ent, PRINT_HIGH, "\nYou have joined team 0 that means you are in NO team!\n");
			else
				cprintf2(ent, PRINT_HIGH, "\nYou have joined team %d!\n", team);

			ent->client->resp.team = team;
		}
	}
	else if (Q_strcasecmp(cmd, "playerlist") == 0)
	{
		int		i;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (players[i])
				if (players[i]->client)
					cprintf2(ent, PRINT_HIGH, "%d: %s\n", i, players[i]->client->pers.netname);
		}
	}
	else if (Q_strcasecmp(cmd, "turretlist") == 0)
	{
		int		i;

		for (i = 0; i < numturrets; i++)
		{
			if (turrets[i] && turrets[i]->inuse)
				cprintf2(ent, PRINT_HIGH, "turret %d active\n", i);
		}
	}
	else if (Q_strcasecmp(cmd, "weaponlist") == 0)
	{
		edict_t* current = NULL;

		current = weapon_list;	// start with the head

		//go through all items in the list
		while (current)
		{
			cprintf2(ent, PRINT_HIGH, "%s\n", current->classname);
			current = current->next_listitem;	//go to next item in list
		}
	}
	else if (Q_strcasecmp(cmd, "healthlist") == 0)
	{
		edict_t* current = NULL;

		current = health_list;	// start with the head

		//go through all items in the list
		while (current)
		{
			cprintf2(ent, PRINT_HIGH, "%s\n", current->classname);
			current = current->next_listitem;	//go to next item in list
		}
	}
	else if (Q_strcasecmp(cmd, "ammolist") == 0)
	{
		edict_t* current = NULL;

		current = ammo_list;	// start with the head

		//go through all items in the list
		while (current)
		{
			cprintf2(ent, PRINT_HIGH, "%s\n", current->classname);
			current = current->next_listitem;	//go to next item in list
		}
	}
	else if (Q_strcasecmp(cmd, "poweruplist") == 0)
	{
		edict_t* current = NULL;

		current = powerup_list;	// start with the head

		//go through all items in the list
		while (current)
		{
			cprintf2(ent, PRINT_HIGH, "%s\n", current->classname);
			current = current->next_listitem;	//go to next item in list
		}
	}/* MrG{DRGN} for debugging */
	else if (Q_strcasecmp(cmd, "spawn") == 0)
	{
		Cmd_Spawn_f(ent);
		return;
	}

	else if (Q_strcasecmp(cmd, "gameversion") == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "%s : %s : %s\n", GAMEVERSION, __DATE__, __TIME__);
	}
	/* END */
	else
		Cmd_Say_f(ent, false, true);
}
