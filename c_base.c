#include "g_local.h"
#include "c_botnav.h"
#include "m_player.h"

///------------------------------------------------------------------------------------------
/// Misc functions
///------------------------------------------------------------------------------------------

qboolean visible(edict_t* self, edict_t* other)
{
	vec3_t	spot1 = { 0 }, spot2 = { 0 };
	trace_t	trace;

	if (!self || !other)
		return false;

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

	if (!self || !other)
		return false;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3)
		return true;
	return false;
}

// ### Hentai ### BEGIN
int vwep_index; //QW offset for vwep list in cached models

// Compute a player's current vwep index and combine it with his skin index
void ShowGun(edict_t* ent)	//QW from WOD:LOX for vwep
{
	char heldmodel[128];
	int n;

	if (!ent->client->pers.weapon)
	{
		ent->client->ps.gunindex = 0;		// WI: seems to be missing?
		ent->s.modelindex2 = REMOVED_MODEL;
		if (!ent->client->resp.spectator) // MrG{DRGN} Don't complain about camera using players not having a gun
			gi.dprintf("ShowGun: Oops! Weapon Index missing! %s\n", ent->client->pers.netname);
		return;
	}

	ent->s.modelindex2 = VWEP_MODEL;
	strcpy(heldmodel, "#");	//safe, don't change
	strcat(heldmodel, ent->client->pers.weapon->icon);
	strcat(heldmodel, ".md2");

	// held model is mapped onto the player skinnum
	n = (gi.modelindex(heldmodel) - vwep_index) << 8;
	ent->s.skinnum &= 0xFF;
	ent->s.skinnum |= n;
	//DbgPrintf("ShowGun: %s skinum 0x%x\n", heldmodel, n);
}
// ### Hentai ### END

qboolean TouchingLadder(edict_t* self)
{
	if (!self)
		return false;

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

void GetSettings(void)
{
	// Server settings
	node_debug = gi.cvar("node_debug", "0", CVAR_USERINFO);
	blindtime = gi.cvar("blindtime", "20", CVAR_USERINFO);
	poisontime = gi.cvar("poisontime", "15", CVAR_USERINFO);
	lasertime = gi.cvar("lasertime", "60", CVAR_USERINFO);
	proxytime = gi.cvar("proxytime", "60", CVAR_USERINFO); /* MrG{DRGN} this was also controlled by lasertime.*/
	defence_turret_ammo = gi.cvar("defence_turret_ammo", "1000", CVAR_USERINFO);
	rocket_turret_ammo = gi.cvar("rocket_turret_ammo", "90", CVAR_USERINFO);
	lasermine_health = gi.cvar("lasermine_health", "150", CVAR_LATCH);
	// FWP Set ex arrow strength and radius from server var
	ex_arrow_damage = gi.cvar("ex_arrow_damage", "80", CVAR_LATCH);
	ex_arrow_radius = gi.cvar("ex_arrow_radius", "200", CVAR_LATCH);
	dntg = gi.cvar("dntg", "0", CVAR_SERVERINFO);
	start_invulnerable_time = gi.cvar("start_invulnerable_time", "3", CVAR_USERINFO);
	lightsoff = gi.cvar("lightsoff", "0", CVAR_USERINFO);
	botchat = gi.cvar("botchat", "1", CVAR_LATCH);
	drop_tech = gi.cvar("drop_tech", "1", CVAR_LATCH); // MrG{DRGN} tech drop prevention
	allow_flagdrop = gi.cvar("allow_flagdrop", "1", CVAR_LATCH); // MrG{DRGN} allow flag dropping
	weapon_kick = gi.cvar("weapon_kick", "1", CVAR_LATCH); // MrG{DRGN} kickable weapons toggle
	do_respawn = gi.cvar("do_respawn", "60", CVAR_LATCH); /* MrG{DRGN} base item respawn time default 60 */
	do_respawn_rnd = gi.cvar("do_respawn_rnd", "80", CVAR_LATCH); /* MrG{DRGN} random item respawn time default 80 */

	// Weapons
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

	// Ammo
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

	// Armor
	ban_bodyarmor = gi.cvar("ban_bodyarmor", "0", CVAR_LATCH);
	ban_combatarmor = gi.cvar("ban_combatarmor", "0", CVAR_LATCH);
	ban_jacketarmor = gi.cvar("ban_jacketarmor", "0", CVAR_LATCH);
	ban_armorshard = gi.cvar("ban_armorshard ", "0", CVAR_LATCH);

	// Power Armor
	ban_powerscreen = gi.cvar("ban_powerscreen", "0", CVAR_LATCH);
	ban_powershield = gi.cvar("ban_powershield", "0", CVAR_LATCH);

	// Powerups
	ban_quaddamage = gi.cvar("ban_quaddamage", "0", CVAR_LATCH);
	ban_invulnerability = gi.cvar("ban_invulnerability", "0", CVAR_LATCH);
	ban_adrenaline = gi.cvar("ban_adrenaline", "0", CVAR_LATCH);
	ban_silencer = gi.cvar("ban_silencer", "0", CVAR_LATCH);
	ban_rebreather = gi.cvar("ban_rebreather", "0", CVAR_LATCH);
	ban_environmentsuit = gi.cvar("ban_environmentsuit", "0", CVAR_LATCH);

	// Health
	ban_health = gi.cvar("ban_health", "0", CVAR_LATCH);
	ban_health_small = gi.cvar("ban_health_small", "0", CVAR_LATCH);
	ban_health_large = gi.cvar("ban_health_large", "0", CVAR_LATCH);
	ban_health_mega = gi.cvar("ban_health_mega", "0", CVAR_LATCH);

	// Weapons
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
	start_gravityvortex = gi.cvar("start_gravityvortex", "0", CVAR_LATCH);
	start_defenseturret = gi.cvar("start_defenseturret", "0", CVAR_LATCH);
	start_rocketturret = gi.cvar("start_rocketturret", "0", CVAR_LATCH);

	// Ammo
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

	GrappleInitVars();
	InvisibilityInitVars();
	JetpackInitVars();
	/* Armor - 0|1|2 Initiate 0%, 100% or 200% of max allowed in each class */
	start_bodyarmor = gi.cvar("start_bodyarmor", "0", CVAR_LATCH);
	start_combatarmor = gi.cvar("start_combatarmor", "0", CVAR_LATCH);
	start_jacketarmor = gi.cvar("start_jacketarmor", "0", CVAR_LATCH);
	start_armorshard = gi.cvar("start_armorshard", "0", CVAR_LATCH);

	// Grant only the highest armor selected at init. Exclude lower ones.
	if (start_bodyarmor->value > 0)
	{
		gi.cvar_set("start_combatarmor", "0");
		gi.cvar_set("start_jacketarmor", "0");
		gi.cvar_set("start_armorshard", "0");
	}
	if (start_combatarmor->value > 0)
	{
		gi.cvar_set("start_jacketarmor", "0");
		gi.cvar_set("start_armorshard", "0");
	}
	if (start_jacketarmor->value > 0)
	{
		gi.cvar_set("start_armorshard", "0");
	}

	// Power Armor
	start_powerscreen = gi.cvar("start_powerscreen", "0", CVAR_LATCH);
	start_powershield = gi.cvar("start_powershield", "0", CVAR_LATCH);

	// Powerups

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

	if (!self || !other)
		return false;

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

	if (!self || !other)
		return false;

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

	if (!self || !other)
		return false;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.85)
		return true;
	return false;
}

void PreCacheAll(void)
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
	gi.imageindex("w_xlauncher"); //proxy mine launcher
	gi.imageindex("w_sword");
	gi.imageindex("w_chainsaw");
	//models
	gi.modelindex("models/objects/shell1/tris.md2"); // ejceted shell casing
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
	gi.modelindex("models/objects/bfgball/tris.md2");
	gi.modelindex("models/objects/earrow/tris.md2");
	gi.modelindex("models/objects/gflash/tris.md2");
	gi.modelindex("models/objects/gpoison/tris.md2");
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

	// VWEP STUFF This section must be contiguous and in weapon select order.
	// Hard limit for this is 19 models, index 0 is default blaster vwep defined in client code.
	vwep_index = gi.modelindex("#w_ak42.md2") - 1;//QW vwep_index sets the base offset for ShowGun.
	gi.modelindex("#w_sword.md2");
	gi.modelindex("#w_chainsaw.md2");
	gi.modelindex("#w_sshotgun.md2");
	gi.modelindex("#w_bow.md2");
	gi.modelindex("#w_airfist.md2");
	gi.modelindex("#a_grenades1.md2");
	gi.modelindex("#w_glauncher1.md2");
	gi.modelindex("#w_xlauncher.md2");
	gi.modelindex("#w_rlauncher.md2");
	gi.modelindex("#w_hyperblaster.md2");
	gi.modelindex("#w_railgun.md2");
	gi.modelindex("#w_buzzsaw.md2");
	gi.modelindex("#w_bfg.md2");
	gi.modelindex("#a_vortex.md2");

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
	if (!p1 || !p1->client)
		return 0;
	if (!p2 || !p2->client)
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

void FakeDeath(edict_t* self)
{
	vec3_t              mins = { -16, -16, -24 };
	vec3_t              maxs = { 16, 16, 32 };

	if (!self)
		return;

	if (self->client->resp.spectator)// MrG{DRGN} if you haven't joined a team yet or are a spectator, you can't fakedeath!
		return;

	if (self->client->fakedeath == 0)	//fake
	{
		if (self->client->PoisonTime > 0)
		{
			cprint_botsafe(self, PRINT_HIGH, "You can't fake death because you are poisoned!\n");
			return;
		}

		self->client->fakedeath = 1;

		VectorClear(self->avelocity);

		self->takedamage = DAMAGE_YES;
		self->movetype = MOVETYPE_TOSS;

		self->s.modelindex2 = REMOVED_MODEL;	// remove linked weapon model
		self->s.modelindex3 = REMOVED_MODEL;	// remove linked ctf flag
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

		ShutOff_Flashlight(self);

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

		self->s.modelindex = PLAYER_MODEL;
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

		if (((int)dmflags->value & DF_FIXED_FOV))
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
		self->s.modelindex = PLAYER_MODEL;
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
		points = damage - 0.5F * VectorLength(v);
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
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5F);
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}

void bprint_botsafe(int printlevel, char* fmt, ...)
{
	int i;

	va_list		argptr;
	edict_t* cl_ent;
	char	BPrint2Buff[0x2000];

	va_start(argptr, fmt);
	vsnprintf(BPrint2Buff, sizeof(BPrint2Buff), fmt, argptr);
	va_end(argptr);

	if (dedicated->value)
		gi.cprintf(NULL, printlevel, BPrint2Buff);

	for (i = 0; i < maxclients->value; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || !cl_ent->client || cl_ent->bot_player)
			continue;

		gi.cprintf(cl_ent, printlevel, BPrint2Buff);
	}
}

// bot-safe cprint
void cprint_botsafe(edict_t* ent, int printlevel, char* fmt, ...)
{
	va_list		argptr;
	char	CPrint2Buff[0x2000];

	if (!ent || !ent->client || ent->bot_player)// MrG{DRGN}
		return;

	va_start(argptr, fmt);

	vsnprintf(CPrint2Buff, sizeof(CPrint2Buff), fmt, argptr);
	va_end(argptr);

	if (ent->inuse) // MrG{DRGN}
	{
		gi.cprintf(ent, printlevel, CPrint2Buff);
	}
}

void nprintf(int printlevel, char* fmt, ...)
{
	int i;
	char	NPrintBuff[0x2000];
	va_list		argptr;
	edict_t* cl_ent;

	if (!node_debug->value)
		return;

	va_start(argptr, fmt);

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

	rn = random();
	if (rn < 0.25)
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit1.wav"), 1, ATTN_NORM, 0);
	else if (0.25 <= rn && rn < 0.5) /* MrG{DRGN} made this work properly by adding && rn 09/23/2020 */
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit2.wav"), 1, ATTN_NORM, 0);
	else if (0.5 <= rn && rn < 0.75) /* MrG{DRGN} made this work properly by adding && rn 09/23/2020 */
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/vomit4.wav"), 1, ATTN_NORM, 0);
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
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) // MrG{DRGN} was 0
			ent->client->newweapon = it_bfg;
	}
	else if (ent->client->pers.weapon == it_rturret)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_lturret)] > 0)
			ent->client->newweapon = it_lturret;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) // MrG{DRGN} was 0
			ent->client->newweapon = it_bfg;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
	}
	else if (ent->client->pers.weapon == it_lturret)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) // MrG{DRGN} was 0
			ent->client->newweapon = it_bfg;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_vortex)] > 0)
			ent->client->newweapon = it_vortex;
		else if (ent->client->pers.inventory[ITEM_INDEX(it_rturret)] > 0)
			ent->client->newweapon = it_rturret;
	}
	else
	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_bfg)] > 0
			&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] > 49) // MrG{DRGN} was 0
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
			ent->client->newweapon = it_grenades;
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

void Cmd_Zoom_f(edict_t* ent)
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
		if (ent->client->ps.fov == atoi(Info_ValueForKey(ent->client->pers.userinfo, "fov")))
			ent->client->ps.fov = 40;	/* MrG{DRGN} fix zoom */
		else if (ent->client->ps.fov == 40)
			ent->client->ps.fov = 20;
		else if (ent->client->ps.fov == 20)
			ent->client->ps.fov = 10;
		else
			ent->client->ps.fov = atoi(Info_ValueForKey(ent->client->pers.userinfo, "fov")); /* MrG{DRGN} fix zoom */
	}
}

void Cmd_Camera_f(edict_t* ent)
{
	if (Q_stricmp(gi.argv(1), "") == 0)
	{
		if (ent->client->camera == 1)
			gi.cprintf(ent, PRINT_HIGH, "IntelliCam Mode!\n");
		if (ent->client->camera == 2)
			gi.cprintf(ent, PRINT_HIGH, "Chasecam Mode!\n");
		if (ent->client->camera == 3)
			gi.cprintf(ent, PRINT_HIGH, "Birdview ChaseCam Mode!\n");
		if (ent->client->camera == 4)
			gi.cprintf(ent, PRINT_HIGH, "TV-Cam Mode!\n");
		else
			gi.cprintf(ent, PRINT_HIGH, "Your camera mode is %d.\n", ent->client->camera);
	}

	if (Q_stricmp(gi.argv(1), "0") == 0)	//cam off
	{
		if (ent->client->camera)
		{
			ent->client->resp.spectator = false; // Not Spectator
			PutClientInServer(ent);
			ClientBegin(ent);
			gi.cprintf(ent, PRINT_HIGH, "Camera OFF!\n");
		}
		return;
	}

	if (Q_stricmp(gi.argv(1), "1") == 0)	//intelli mode
	{
		if (!ent->client->camera)
			CreateCamera(ent);

		ent->client->camera = 1;
		gi.cprintf(ent, PRINT_HIGH, "IntelliCam Mode!\n");
	}
	else if (Q_stricmp(gi.argv(1), "2") == 0)	//chase cam mode
	{
		if (!ent->client->camera)
			CreateCamera(ent);

		ent->client->camera = 2;
		gi.cprintf(ent, PRINT_HIGH, "ChaseCam Mode!\n");
	}
	else if (Q_stricmp(gi.argv(1), "3") == 0)	// birdview chase cam
	{
		if (!ent->client->camera)
			CreateCamera(ent);

		ent->client->camera = 3;
		gi.cprintf(ent, PRINT_HIGH, "Birdview ChaseCam Mode!\n");
	}
	else if (Q_stricmp(gi.argv(1), "4") == 0)	// TV cam mode
	{
		if (!ent->client->camera)
			CreateCamera(ent);

		ent->client->camera = 4;
		gi.cprintf(ent, PRINT_HIGH, "TV-Cam Mode!\n");
	}
}

/* Node Table Tools */
void Cmd_Pathdebug_f(edict_t* ent)
{
	if (!ent->client->b_target)
	{
		ent->client->b_target = G_Spawn();
		ent->client->b_target->movetype = MOVETYPE_NONE;
		ent->client->b_target->solid = SOLID_NOT;
		ent->client->b_target->s.modelindex = gi.modelindex("models/objects/gibs/skull/tris.md2");
		VectorCopy(ent->s.origin, ent->client->b_target->s.origin);
		gi.linkentity(ent->client->b_target);
		cprint_botsafe(ent, PRINT_HIGH, "Pathdebug ON!\n");
	}
	else
	{
		G_FreeEdict(ent->client->b_target);
		ent->client->b_target = NULL;
		cprint_botsafe(ent, PRINT_HIGH, "Pathdebug OFF!\n");
	}
}

void Cmd_PlaceNode_f(edict_t* ent)
{
	if (dntg->value)
	{
		vec3_t	end = { 0 }, spot = { 0 };
		trace_t	tr;

		//check if node is in air
		VectorCopy(ent->s.origin, end);
		end[2] -= 40;

		tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_SOLID);

		//	gi.WriteByte (svc_temp_entity);
		//	gi.WriteByte (TE_BFG_LASER);
		//	gi.WritePosition (nodes[nindex].origin);
		//	gi.WritePosition (end);
		//	gi.multicast (nodes[nindex].origin, MULTICAST_PHS);

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
		cprint_botsafe(ent, PRINT_HIGH, "Dynamic Node Table Generation is off activate it with <set dntg 1>!\n");
}
/* END Node Table Tools */

void Cmd_Join_Team_f(edict_t* ent)
{
	int team = atoi(gi.argv(1));

	if (team < 0 || team > 99)
		cprint_botsafe(ent, PRINT_HIGH, "\nPlease select a team between 1 and 99!\nSelect 0 for no team!\n");
	else
	{
		if (team == 0)
			cprint_botsafe(ent, PRINT_HIGH, "\nYou have joined team 0 that means you are in NO team!\n");
		else
			cprint_botsafe(ent, PRINT_HIGH, "\nYou have joined team %d!\n", team);

		ent->client->resp.team = team;
	}
}

void Cmd_Nums_f(edict_t* ent)
{
	cprint_botsafe(ent, PRINT_HIGH, "numplayers=%d\n", numplayers);
	cprint_botsafe(ent, PRINT_HIGH, "numbots=%d\n", numbots);
	cprint_botsafe(ent, PRINT_HIGH, "numred=%d\n", numred);
	cprint_botsafe(ent, PRINT_HIGH, "numblue=%d\n", numblue);
	cprint_botsafe(ent, PRINT_HIGH, "red_base=%d\n", red_base);
	cprint_botsafe(ent, PRINT_HIGH, "blue_base=%d\n", blue_base);
	cprint_botsafe(ent, PRINT_HIGH, "numturrets=%d\n", numturrets);
}

void Cmd_PlayerList_f(edict_t* ent)
{
	int i;
	char string[80];
	char text[LAYOUT_MAX_LENGTH] = { 0 };
	edict_t* ed = NULL;
	/* connect time, ping, score, name */

	// Start with a nice header.
	Com_sprintf(string, sizeof string, "%s\n", "entry ping score      name");
	Q_strncatz(text, sizeof text, string);
	Com_sprintf(string, sizeof string, "%s\n", "----- ---- -----  -------------");
	Q_strncatz(text, sizeof text, string);

	for (i = 0, ed = g_edicts + 1; i < maxclients->value; i++, ed++) {
		if (!ed->inuse || ed->bot_player)
			continue;

		Com_sprintf(string, sizeof string, "%02d:%02d %3d %4d    %-s %-12s\n",
			(level.framenum - ed->client->resp.enterframe) / 600,
			((level.framenum - ed->client->resp.enterframe) % 600) / 10,
			ed->client->ping,
			ed->client->resp.score,
			ed->client->pers.netname,
			ed->client->resp.spectator ? "(spectator)" : "");

		// While we might think we have the full frame to use
		// we can't take all of it for a console message, or it
		// gets dropped. 1000 seems a hard limit here.
		// The number of lines listed will depend on
		// lengths of names and spectator status. //QW//
		//DbgPrintf("length: %u\n", strlen(text));
		if (strlen(text) + strlen(string) > 1000)
		{
			Q_strncatz(text, sizeof text, "And more...\n");
			cprint_botsafe(ent, PRINT_HIGH, "%s\n", text);
			return;
		}
		Q_strncatz(text, sizeof text, string);
	}
	cprint_botsafe(ent, PRINT_HIGH, "%s\n", text);
}

void Cmd_Turretlist_f(edict_t* ent)
{
	int		i;

	for (i = 0; i < numturrets; i++)
	{
		if (turrets[i] && turrets[i]->inuse)
			cprint_botsafe(ent, PRINT_HIGH, "turret %d active\n", i);
	}
}

void Cmd_Weaponlist_f(edict_t* ent)
{
	edict_t* current = NULL;

	current = weapon_list;	// start with the head

	//go through all items in the list
	while (current)
	{
		cprint_botsafe(ent, PRINT_HIGH, "%s\n", current->classname);
		current = current->next_listitem;	//go to next item in list
	}
}

void Cmd_Healthlist_f(edict_t* ent)
{
	edict_t* current = NULL;

	current = health_list;	// start with the head

	//go through all items in the list
	while (current)
	{
		cprint_botsafe(ent, PRINT_HIGH, "%s\n", current->classname);
		current = current->next_listitem;	//go to next item in list
	}
}

void Cmd_Ammolist_f(edict_t* ent)
{
	edict_t* current = NULL;

	current = ammo_list;	// start with the head

	//go through all items in the list
	while (current)
	{
		cprint_botsafe(ent, PRINT_HIGH, "%s\n", current->classname);
		current = current->next_listitem;	//go to next item in list
	}
}

void Cmd_Poweruplist_f(edict_t* ent)
{
	edict_t* current = NULL;

	current = powerup_list;	// start with the head

	//go through all items in the list
	while (current)
	{
		cprint_botsafe(ent, PRINT_HIGH, "%s\n", current->classname);
		current = current->next_listitem;	//go to next item in list
	}
}

void Cmd_Throwup_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't puke! */
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

void Cmd_Kamikaze_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't Kamikaze! */
		return;
	if (ent->health <= 0)
		return;
	if (ent->client->kamikazetime != 0)
		return;
	if (ent->flags & FL_GODMODE)
	{
		cprint_botsafe(ent, PRINT_MEDIUM, "You can't go kamikaze in god mode, cheater!\n");
		return;
	}
	if (ent->client->pers.inventory[ITEM_INDEX(it_rockets)] + ent->client->pers.inventory[ITEM_INDEX(it_grenades)]
		+ ent->client->pers.inventory[ITEM_INDEX(it_homings)] < 10)
	{
		cprint_botsafe(ent, PRINT_MEDIUM, "You need at least 10 rockets or grenades to go kamikaze!\n");
		return;
	}

	ent->client->kamikazetime = 50;
	ent->s.effects = EF_ROCKET;
	gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/kamikaze.wav"), 1, ATTN_NORM, 0);
}

void Cmd_ToggleGrenades_f(edict_t* ent)
{
	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't toggle nades */
		return;
	if (ent->client->grenadesactive == 1)
	{
		ent->client->grenadesactive = 0;
		cprint_botsafe(ent, PRINT_HIGH, "Grenades OFF\n");
	}
	else
	{
		cprint_botsafe(ent, PRINT_HIGH, "Grenades ON\n");
		ent->client->grenadesactive = 1;
	}
}

void Cmd_Kick_f(edict_t* ent)
{
	edict_t* blip = NULL;
	vec3_t	forward;

	if (ent->client->resp.spectator)/* MrG{DRGN} if you haven't joined a team yet. you can't kick */
		return;
	if (ent->client->fakedeath > 0)
		return;
	if (ent->health <= 0)
		return;

	while ((blip = findradius2(blip, ent->s.origin, 100)) != NULL)
	{
		if ((blip->classindex >= W_BLASTER && blip->classindex <= W_BFG) && (!weapon_kick->value))
			return;

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
			}
	}
}

///------------------------------------------------------------------------------------------
/// Command handling
///------------------------------------------------------------------------------------------
void ClientCommand2(edict_t* ent)
{
	char* cmd;

	if (!ent->client)
		return;

	cmd = gi.argv(0);

	if (!Q_stricmp(cmd, "grapple"))
	{
		Cmd_Grapple_f(ent);
	}
	else if (Q_stricmp(cmd, "class2") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class2(ent);
	}
	else if (Q_stricmp(cmd, "class3") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class3(ent);
	}
	else if (Q_stricmp(cmd, "class4") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class4(ent);
	}
	else if (Q_stricmp(cmd, "class5") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class5(ent);
	}
	else if (Q_stricmp(cmd, "class6") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class6(ent);
	}
	else if (Q_stricmp(cmd, "class7") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class7(ent);
	}
	else if (Q_stricmp(cmd, "class8") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class8(ent);
	}
	else if (Q_stricmp(cmd, "class9") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class9(ent);
	}
	else if (Q_stricmp(cmd, "class0") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Class0(ent);
	}
	else if (Q_stricmp(cmd, "grenades") == 0)
	{
		if (ent->health <= 0)
			return;
		if (ent->client->fakedeath > 0)
			return;
		Use_Grenades(ent);
	}

	else if (Q_stricmp(cmd, "zoom") == 0)
	{
		Cmd_Zoom_f(ent);
	}
	else if (Q_stricmp(cmd, "camera") == 0)
	{
		Cmd_Camera_f(ent);
	}
	else if (Q_stricmp(cmd, "pathdebug") == 0)
	{
		Cmd_Pathdebug_f(ent);
	}
	else if (Q_stricmp(cmd, "placenode") == 0)
	{
		Cmd_PlaceNode_f(ent);
	}
	else if (Q_stricmp(cmd, "load_nodes") == 0)
	{
		Bot_LoadNodes();
	}
	else if (Q_stricmp(cmd, "save_nodes") == 0)
	{
		Bot_SaveNodes();
	}
	else if (Q_stricmp(cmd, "nums") == 0)
	{
		Cmd_Nums_f(ent);
	}
	else if (Q_stricmp(cmd, "join_team") == 0)
	{
		Cmd_Join_Team_f(ent);
	}
	else if (Q_stricmp(cmd, "playerlist") == 0)
	{
		if (ctf->value)
			CTFPlayerList(ent);
		else
			Cmd_PlayerList_f(ent);
	}
	else if (Q_stricmp(cmd, "ctfplayerlist") == 0)
	{
		CTFPlayerList(ent);
	}
	else if (Q_stricmp(cmd, "turretlist") == 0)
	{
		Cmd_Turretlist_f(ent);
	}
	else if (Q_stricmp(cmd, "weaponlist") == 0)
	{
		Cmd_Weaponlist_f(ent);
	}
	else if (Q_stricmp(cmd, "healthlist") == 0)
	{
		Cmd_Healthlist_f(ent);
	}
	else if (Q_stricmp(cmd, "ammolist") == 0)
	{
		Cmd_Ammolist_f(ent);
	}
	else if (Q_stricmp(cmd, "poweruplist") == 0)
	{
		Cmd_Poweruplist_f(ent);
	}
	else if (Q_stricmp(cmd, "gameversion") == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "%s : %s : %s\n", GAMEVERSION, __DATE__, __TIME__);
	}
	else if (Q_stricmp(cmd, "throwup") == 0)
	{
		Cmd_Throwup_f(ent);
	}
	else if (Q_stricmp(cmd, "scanner") == 0)
	{
		Toggle_Scanner(ent);
	}
	else if (Q_stricmp(cmd, "belt") == 0)
	{
		Cmd_Belt_f(ent);
	}
	else if (Q_stricmp(cmd, "flashlight") == 0)
	{
		Cmd_Flashlight_f(ent);
	}
	else if (Q_stricmp(cmd, "teleport") == 0)
	{
		Cmd_Teleport_f(ent);
	}
	else if (Q_stricmp(cmd, "kamikaze") == 0)
	{
		Cmd_Kamikaze_f(ent);
	}
	else if (Q_stricmp(cmd, "togglegrenades") == 0)
	{
		Cmd_ToggleGrenades_f(ent);
	}
	else if (Q_stricmp(cmd, "fakedeath") == 0)
	{
		if (ent->health > 0)
			FakeDeath(ent);
	}
	else if (Q_stricmp(cmd, "kick") == 0)
	{
		Cmd_Kick_f(ent);
	}
	else
		Cmd_Say_f(ent, false, true);
}
