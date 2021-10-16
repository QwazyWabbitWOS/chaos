#include "g_local.h"
#include "c_base.h"

#include "c_botai.h"
#include "c_cam.h"
#include "m_player.h"
#include "gslog.h"

///------------------------------------------------------------------------------------------
/// Command handling
///------------------------------------------------------------------------------------------

// Adds "num" bots with specified skill, team, name and model/skin.
// sv addbots <amount> <skill> <team> <name> <model/skin>
void SVCmd_addbots_f(void)
{
	int		i, num, bot_skill, team;
	char	name[64], model[128];

	num = atoi(gi.argv(2)); // quantity
	bot_skill = atoi(gi.argv(3)); // skill
	team = atoi(gi.argv(4)); // team number
	Com_strcpy(name, sizeof name, gi.argv(5)); // name
	int max_bots = ((int)maxclients->value - 2);  // reserve slots

	if (bot_skill == 0)
		bot_skill = 3;

	if (num == 0) //QW// if num is 0 or no args given, print instructions.
	{
		gi.cprintf(NULL, PRINT_HIGH, "sv addbots <amount> <skill> <team> <name> <model/skin>\n");
		return;
	}

	if (num > max_bots)
	{
		gi.cprintf(NULL, PRINT_HIGH, "You can't spawn more than %i Havoc-Bots!\n", max_bots);
		return;
	}

	else if (num == 1) //QW// adding single bot accepts optional name model/skin
	{
		// set the model
		if (Q_stricmp(gi.argv(6), "") == 0)
		{
			Com_sprintf(model, sizeof model, Get_RandomBotSkin());
		}
		else
			Com_sprintf(model, sizeof model, gi.argv(6)); // specific model

		// set the name
		if (Q_stricmp(name, "") == 0 || Q_stricmp(name, " ") == 0)
		{
			// if no name, it's the skin of the model
			Com_strcpy(name, sizeof name, (strchr(model, '/') + 1));
		}

		Bot_Create(bot_skill, team, name, model);
	}
	else // num > 1, accept only models, names will be placeholder.
	{
		for (i = 0; i < num; i++)
		{
			// set the model, random if no argv
			if (Q_stricmp(gi.argv(6), "") == 0)
			{
				Com_sprintf(model, sizeof model, Get_RandomBotSkin());
			}
			else
				Com_sprintf(model, sizeof model, gi.argv(6));

			Com_strcpy(name, sizeof name, (strchr(model, '/') + 1));

			Bot_Create(bot_skill, team, name, model);
		}
	}
}

void SVCmd_killbot_f(char* name)
{
	int i, k, count;

	count = numplayers + 1;

	if (Q_stricmp(name, "all") == 0)	//kill all bots
	{
		for (i = 0; i < count; i++)
		{
			for (k = 0; k < numplayers; k++)
			{
				if (!players[k])
					continue;
				if (!players[k]->client)
					continue;
				/* MrG{DRGN}
				if (Q_stricmp(players[k]->classname, "bot") == 0)
				*/
				if (players[k]->bot_player)
				{
					ClientDisconnect(players[k]);
					numbots--;
				}
			}
		}
	}
	else	//kill a specific bot
	{
		for (i = 0; i < count; i++)
		{
			if (!players[i])
				continue;
			if (!players[i]->client)
				continue;
			/* MrG{DRGN}
			if ((Q_stricmp(players[i]->classname, "bot") == 0) && (Q_stricmp(players[i]->client->pers.netname, name) == 0))
			*/
			if ((players[i]->bot_player) && (Q_stricmp(players[i]->client->pers.netname, name) == 0))
			{
				ClientDisconnect(players[i]);
				numbots--;
			}
		}
	}
}

///------------------------------------------------------------------------------------------
/// bot create/spawn/respawn/die
///------------------------------------------------------------------------------------------

void Bot_Create(int accuracy_level, int team, char* name, char* skin)
{
	int       i;
	char      userinfo[MAX_INFO_STRING];
	edict_t* bot = NULL; /* MrG{DRGN} initialized */
	int mcv = (int)maxclients->value;/* MrG{DRGN} check once */

	for (i = (mcv); i > 0; i--) /* MrG{DRGN} use here */
	{
		bot = g_edicts + i + 1;
		if (!bot->inuse)
			break;
	}

	if (bot && bot->inuse)
		bot = NULL;

	if (!bot)
	{
		bprint_botsafe(PRINT_HIGH, "%s cant connect, server is full!\n", name);
		return;
	}

	memset(userinfo, 0, MAX_INFO_STRING);

	Info_SetValueForKey(userinfo, "name", name);
	Info_SetValueForKey(userinfo, "skin", skin);
	Info_SetValueForKey(userinfo, "hand", "2");

	ClientConnect(bot, userinfo);
	G_InitEdict(bot);
	InitClientResp(bot->client);

	Bot_Spawn(bot);

	if (accuracy_level > 5)
		accuracy_level = 5;
	if (accuracy_level < 1)
		accuracy_level = 1;

	bot->client->b_botlevel = accuracy_level;

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(bot - g_edicts);
	gi.WriteByte(MZ_LOGIN);
	gi.multicast(bot->s.origin, MULTICAST_PVS);

	bprint_botsafe(PRINT_HIGH, "%s entered the game\n", bot->client->pers.netname);
	ClientEndServerFrame(bot);
	players[numplayers] = bot;
	numplayers++;
	numbots++;

	//TEAMPLAY
	if (ctf->value)
	{
		if (bot->team == CTF_NOTEAM)
		{
			if (numblue < numred)
				CTFBotJoinTeam(bot, CTF_TEAM2);
			else
				CTFBotJoinTeam(bot, CTF_TEAM1);
		}
		else if (rand() & 1)
			CTFBotJoinTeam(bot, CTF_TEAM1);
		else
			CTFBotJoinTeam(bot, CTF_TEAM2);
	}
	else
	{
		if (team == CTF_NOTEAM)
			bot->client->resp.team = CTF_NOTEAM;
		else if ((team > CTF_NOTEAM) && (team < 100))
		{
			bot->client->resp.team = team;
			bprint_botsafe(PRINT_HIGH, "%s has joined team %d!\n", name, team);
		}
		else
		{
			bot->client->resp.team = CTF_NOTEAM;
			bprint_botsafe(PRINT_HIGH, "Invalid team number! %s has joined NO team!\n", name);
		}
	}
}

void Bot_Spawn(edict_t* ent)
{
	vec3_t               origin, angles;
	vec3_t               mins = { -16, -16, -24 };
	vec3_t               maxs = { 16, 16, 32 };
	int                 i, index;
	client_persistant_t  pers;
	client_respawn_t     resp = { 0 };

	if (!deathmatch->value)
	{
		gi.dprintf("Must be in Deathmatch to spawn as Bot!\n");
		return;
	}

	SelectSpawnPoint(ent, origin, angles);

	index = ent - g_edicts - 1;

	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = ent->client->resp;
		memcpy(userinfo, ent->client->pers.userinfo, MAX_INFO_STRING);
		InitClientPersistant(ent->client);
		ClientUserinfoChanged(ent, userinfo);
	}
	else
		memset(&resp, 0, sizeof(client_respawn_t));

	pers = ent->client->pers;
	memset(ent->client, 0, sizeof(gclient_t));
	ent->client->pers = pers;
	ent->client->resp = resp;

	FetchClientEntData(ent);

	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_BOT;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->bot_player = true;  /* MrG{DRGN} this is a lot easier than checking the classname to track if we're dealing wiith a bot or not! */
	ent->classname = "bot";
	ent->classindex = BOT;
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->think = Bot_Think;
	ent->touch = NULL;
	ent->pain = bot_pain;
	ent->die = bot_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->enemy = NULL;
	ent->movetarget = NULL;

	ent->client->b_respawntime = 0;
	ent->client->b_nextshot = 0;
	ent->client->b_nextwchange = 0;
	ent->client->b_goalitem = NULL;
	ent->client->b_strafechange = level.time + 1;
	ent->client->b_runchange = level.time + 1;
	ent->client->b_nextrandjump = 0;
	ent->client->b_waittime = 0;
	ent->client->b_pausetime = 0;
	ent->client->grenadesactive = 1;
	ent->client->b_nodetime = 0;
	ent->client->b_nextroam = 0;
	ent->client->b_closeitem = NULL;
	ent->client->b_nopathitem = NULL;

	if (random() >= 0.5)
		ent->client->b_strafedir = 0;
	else
		ent->client->b_strafedir = 1;

	if (random() >= 0.5)
		ent->client->b_rundir = 0;
	else
		ent->client->b_rundir = 1;

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);
	VectorClear(ent->velocity);

	memset(&ent->client->ps, 0, sizeof(player_state_t));

	for (i = 0; i < 3; i++)
	{
		ent->client->ps.pmove.origin[i] = (short)origin[i] * 8; /* MrG{DRGN} explicit cast to shut up compiler! */
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(angles[i] - ent->client->resp.cmd_angles[i]);
	}

	ent->client->ps.fov = 90;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	ent->s.effects = 0;
	ent->s.skinnum = index;
	ent->s.modelindex = (PLAYER_MODEL); /* MrG{DRGN} */

	ShowGun(ent); //vwep
	//ent->s.modelindex2    = 255;
	ent->s.frame = 0;
	ent->enemy = NULL;
	ent->client->b_currentnode = -1;
	/* MrG{DRGN} make their aim speed a little more reasonable
	(20 degrees per frame base, + 14 degrees per level)
	vs the hardcoded 90 it was! Fixed so they're just as nasty at level 5 */
	ent->yaw_speed = 20 + (ent->client->b_botlevel * 14);

	VectorCopy(origin, ent->s.origin);
	ent->s.origin[2]++;

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = angles[YAW];
	ent->s.angles[ROLL] = 0;

	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
	VectorCopy(ent->s.angles, ent->client->v_angle);

	gi.unlinkentity(ent);
	KillBox(ent);
	gi.linkentity(ent);

	ent->client->newweapon = ent->client->pers.weapon;
	ChangeWeapon(ent);

	ent->nextthink = level.time + FRAMETIME;
}

void Bot_Respawn(edict_t* ent)
{
	CopyToBodyQue(ent);

	Bot_Spawn(ent);

	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;
}

void bot_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	VectorClear(self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = REMOVED_MODEL;	// remove linked weapon model

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 1.0F; /* MrG{DRGN} Explicit float */
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker);

		sl_WriteStdLogDeath(&gi, level, self, inflictor, attacker);	// StdLog - Mark Davies

		/* MrG{DRGN} move CTF specific stuff here */
		if (ctf->value)
		{	//ZOID
			self->s.modelindex3 = REMOVED_MODEL;	// remove linked ctf flag
			CTFFragBonuses(self, inflictor, attacker);
			CTFDeadDropFlag(self);
		}
		TossClientWeapon(self);

		CTFDeadDropTech(self);
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->client->invisible_framenum = 0;

	self->s.effects = 0;

	if (Jet_Active(self))
	{
		Jet_BecomeExplosion(self, damage);
		self->client->jet_framenum = 0;
	}

	// clear inventory
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	self->client->b_respawntime = level.time + 1.5F; /* MrG{DRGN} Explicit float */

	// check for gib
	if (self->health < -40)
	{	// gib
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGib(self, "models/objects/gibs/sm_gib1/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_gib2/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_gib2/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_gib3/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_gib4/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_gib5/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/bone2/tris.md2", damage, GIB_ORGANIC);

		ThrowClientHead(self, damage);
		//ZOID
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = 0;
		//ZOID
		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i + 1) % 3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
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
			gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1, ATTN_NORM, 0);
			self->s.modelindex = PLAYER_MODEL;
		}
	}

	self->deadflag = DEAD_DEAD;

	self->client->BlindTime = 0;
	self->client->PoisonTime = 0;
	self->client->invisible = 0;

	self->client->flashlightactive = 0;
	if (self->client->flashlight)
	{
		self->client->flashlight->think = G_FreeEdict;
		G_FreeEdict(self->client->flashlight);
	}
	if (self->client->teleporter)
		G_FreeEdict(self->client->teleporter);

	gi.linkentity(self);
	//DbgPrintf("%s %s index: %d\n", __func__, self->classname, self->classindex);
}

///------------------------------------------------------------------------------------------
/// bot misc functions
///------------------------------------------------------------------------------------------

float Bot_Fire_Freq(edict_t* ent)
{
	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	*/

	if (ent->client->pers.weapon == it_ak42)
		return AK42_FREQ;
	else if (ent->client->pers.weapon == it_supershotgun
		|| ent->client->pers.weapon == it_esupershotgun)
		return SUPERSHOTGUN_FREQ;
	else if (ent->client->pers.weapon == it_crossbow
		|| ent->client->pers.weapon == it_poisoncrossbow
		|| ent->client->pers.weapon == it_explosivecrossbow)
		return CROSSBOW_FREQ;
	else if (ent->client->pers.weapon == it_airfist)
		return AIRFIST_FREQ;
	else if (ent->client->pers.weapon == it_grenades
		|| ent->client->pers.weapon == it_flashgrenades
		|| ent->client->pers.weapon == it_lasermines
		|| ent->client->pers.weapon == it_poisongrenades
		|| ent->client->pers.weapon == it_proxymines)
		return GRENADE_FREQ;
	else if (ent->client->pers.weapon == it_grenadelauncher
		|| ent->client->pers.weapon == it_flashgrenadelauncher
		|| ent->client->pers.weapon == it_poisongrenadelauncher
		|| ent->client->pers.weapon == it_proxyminelauncher)
		return GRENADELAUNCHER_FREQ;
	else if (ent->client->pers.weapon == it_rocketlauncher)
		return ROCKETLAUNCHER_FREQ;
	else if (ent->client->pers.weapon == it_hominglauncher)
		return HOMINGLAUNCHER_FREQ;
	else if (ent->client->pers.weapon == it_hyperblaster)
		return HYPERBLASTER_FREQ;
	else if (ent->client->pers.weapon == it_railgun)
		return RAILGUN_FREQ;
	else if (ent->client->pers.weapon == it_buzzsaw)
		return BUZZSAW_FREQ;
	else if (ent->client->pers.weapon == it_bfg)
		return BFG_FREQ;
	else if (ent->client->pers.weapon == it_vortex)
		return VORTEX_FREQ;
	else if (ent->client->pers.weapon == it_sword)
		return SWORD_FREQ;
	else if (ent->client->pers.weapon == it_chainsaw)
		return CHAINSAW_FREQ;
	else if (ent->client->pers.weapon == it_rturret
		|| ent->client->pers.weapon == it_lturret)
		return TURRET_FREQ;
	else
		return  1;
}

void Bot_BestMidWeapon(edict_t* self)
{
	gclient_t* client;
	gitem_t* oldweapon;
	qboolean	turret = 0;

	client = self->client;
	oldweapon = client->pers.weapon;

	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	it_airfist = FindItem("airgun");	//bugfix
	*/

	// Is our enemy a turret ?
	if (self->enemy
		&& self->enemy->inuse
		&& (self->enemy->health > 0)
		&& (self->enemy->die == Turret_Die))
	{
		turret = 1;
	}

	// Rocket Turret
	if (client->pers.inventory[ITEM_INDEX(it_rturret)])
	{
		if (oldweapon != it_rturret)
			self->client->newweapon = it_rturret;
		return;
	}
	// Laser Turret
	if (client->pers.inventory[ITEM_INDEX(it_lturret)])
	{
		if (oldweapon != it_lturret)
			self->client->newweapon = it_lturret;
		return;
	}
	// Buzzsaw
	if (client->pers.inventory[ITEM_INDEX(it_buzzes)] && client->pers.inventory[ITEM_INDEX(it_buzzsaw)])
	{
		if (oldweapon != it_buzzsaw)
			self->client->newweapon = it_buzzsaw;
		return;
	}
	// Railgun
	if (client->pers.inventory[ITEM_INDEX(it_slugs)] && client->pers.inventory[ITEM_INDEX(it_railgun)])
	{
		if (oldweapon != it_railgun)
			self->client->newweapon = it_railgun;
		return;
	}
	// Rocket Launcher
	if (client->pers.inventory[ITEM_INDEX(it_rockets)] && client->pers.inventory[ITEM_INDEX(it_rocketlauncher)])
	{
		if (oldweapon != it_rocketlauncher)
			self->client->newweapon = it_rocketlauncher;
		return;
	}
	// Hyperblaster
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] > 10) && client->pers.inventory[ITEM_INDEX(it_hyperblaster)])
	{
		if (oldweapon != it_hyperblaster)
			self->client->newweapon = it_hyperblaster;
		return;
	}
	// Vortex
	if (client->pers.inventory[ITEM_INDEX(it_vortex)])
	{
		if (oldweapon != it_vortex)
			self->client->newweapon = it_vortex;
		return;
	}
	// BFG
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] >= 50) && client->pers.inventory[ITEM_INDEX(it_bfg)])
	{
		if (oldweapon != it_bfg)
			self->client->newweapon = it_bfg;
		return;
	}
	// Explosive Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] && client->pers.inventory[ITEM_INDEX(it_explosivearrows)])
	{
		if (oldweapon != it_explosivecrossbow)
			self->client->newweapon = it_explosivecrossbow;
		return;
	}
	// Proxymine Launcher
	if (client->pers.inventory[ITEM_INDEX(it_proxymines)] && client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)])
	{
		if (oldweapon != it_proxyminelauncher)
			self->client->newweapon = it_proxyminelauncher;
		return;
	}
	// Homing Missile Launcher
	if (client->pers.inventory[ITEM_INDEX(it_homings)] && client->pers.inventory[ITEM_INDEX(it_hominglauncher)])
	{
		if (oldweapon != it_hominglauncher)
			self->client->newweapon = it_hominglauncher;
		return;
	}
	// Poison Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] && client->pers.inventory[ITEM_INDEX(it_poisonarrows)])
	{
		if (oldweapon != it_poisonarrows)
			self->client->newweapon = it_poisoncrossbow;
		return;
	}
	// Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_crossbow)] && client->pers.inventory[ITEM_INDEX(it_arrows)])
	{
		if (oldweapon != it_crossbow)
			self->client->newweapon = it_crossbow;
		return;
	}
	// Explosive Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_eshells)] && client->pers.inventory[ITEM_INDEX(it_esupershotgun)])
	{
		if (oldweapon != it_esupershotgun)
			self->client->newweapon = it_esupershotgun;
		return;
	}
	// Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_shells)] && client->pers.inventory[ITEM_INDEX(it_supershotgun)])
	{
		if (oldweapon != it_supershotgun)
			self->client->newweapon = it_supershotgun;
		return;
	}
	// Grenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_grenades)] && client->pers.inventory[ITEM_INDEX(it_grenadelauncher)])
	{
		if (oldweapon != it_grenadelauncher)
			self->client->newweapon = it_grenadelauncher;
		return;
	}
	// Airfist
	if (client->pers.inventory[ITEM_INDEX(it_airfist)] && !turret)
	{
		if (oldweapon != it_airfist)
			self->client->newweapon = it_airfist;
		return;
	}
	// Flashgrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] && !turret)
	{
		if (oldweapon != it_flashgrenadelauncher)
			self->client->newweapon = it_flashgrenadelauncher;
		return;
	}
	// Poisongrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] && !turret)
	{
		if (oldweapon != it_poisongrenadelauncher)
			self->client->newweapon = it_poisongrenadelauncher;
		return;
	}
	// Sword
	if (client->pers.inventory[ITEM_INDEX(it_sword)])
	{
		if (oldweapon != it_sword)
			self->client->newweapon = it_sword;
		return;
	}
	// Chainsaw
	if (client->pers.inventory[ITEM_INDEX(it_chainsaw)])
	{
		if (oldweapon != it_chainsaw)
			self->client->newweapon = it_chainsaw;
		return;
	}
	// Flash Grenades
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && !turret)
	{
		if (oldweapon != it_flashgrenades)
			self->client->newweapon = it_flashgrenades;
		return;
	}
	// Poison Grenades
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && !turret)
	{
		if (oldweapon != it_poisongrenades)
			self->client->newweapon = it_poisongrenades;
		return;
	}

	// found nothing so use ak42
	self->client->newweapon = it_ak42;
}

void Bot_BestCloseWeapon(edict_t* self)
{
	gclient_t* client;
	gitem_t* oldweapon;
	qboolean	turret = 0;

	client = self->client;
	oldweapon = client->pers.weapon;

	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	it_airfist = FindItem("airgun");	//bugfix
	*/

	// Is our enemy a turret ?
	if (self->enemy
		&& self->enemy->inuse
		&& (self->enemy->health > 0)
		&& (self->enemy->die == Turret_Die))
	{
		turret = 1;
	}

	// Sword
	if (client->pers.inventory[ITEM_INDEX(it_sword)])
	{
		if (oldweapon != it_sword)
			self->client->newweapon = it_sword;
		return;
	}
	// Chainsaw
	if (client->pers.inventory[ITEM_INDEX(it_chainsaw)])
	{
		if (oldweapon != it_chainsaw)
			self->client->newweapon = it_chainsaw;
		return;
	}
	// Buzzsaw
	if (client->pers.inventory[ITEM_INDEX(it_buzzes)] && client->pers.inventory[ITEM_INDEX(it_buzzsaw)])
	{
		if (oldweapon != it_buzzsaw)
			self->client->newweapon = it_buzzsaw;
		return;
	}
	// Railgun
	if (client->pers.inventory[ITEM_INDEX(it_slugs)] && client->pers.inventory[ITEM_INDEX(it_railgun)])
	{
		if (oldweapon != it_railgun)
			self->client->newweapon = it_railgun;
		return;
	}
	// Rocket Launcher
	if (client->pers.inventory[ITEM_INDEX(it_rockets)] && client->pers.inventory[ITEM_INDEX(it_rocketlauncher)])
	{
		if (oldweapon != it_rocketlauncher)
			self->client->newweapon = it_rocketlauncher;
		return;
	}
	// Hyperblaster
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] > 10) && client->pers.inventory[ITEM_INDEX(it_hyperblaster)])
	{
		if (oldweapon != it_hyperblaster)
			self->client->newweapon = it_hyperblaster;
		return;
	}
	// Vortex
	if (client->pers.inventory[ITEM_INDEX(it_vortex)])
	{
		if (oldweapon != it_vortex)
			self->client->newweapon = it_vortex;
		return;
	}
	// BFG
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] >= 50) && client->pers.inventory[ITEM_INDEX(it_bfg)])
	{
		if (oldweapon != it_bfg)
			self->client->newweapon = it_bfg;
		return;
	}
	// Poison Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] && client->pers.inventory[ITEM_INDEX(it_poisonarrows)])
	{
		if (oldweapon != it_poisonarrows)
			self->client->newweapon = it_poisoncrossbow;
		return;
	}
	// Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_crossbow)] && client->pers.inventory[ITEM_INDEX(it_arrows)])
	{
		if (oldweapon != it_crossbow)
			self->client->newweapon = it_crossbow;
		return;
	}
	// Rocket Turret
	if (client->pers.inventory[ITEM_INDEX(it_rturret)])
	{
		if (oldweapon != it_rturret)
			self->client->newweapon = it_rturret;
		return;
	}
	// Laser Turret
	if (client->pers.inventory[ITEM_INDEX(it_lturret)])
	{
		if (oldweapon != it_lturret)
			self->client->newweapon = it_lturret;
		return;
	}
	// Proxymine Launcher
	if (client->pers.inventory[ITEM_INDEX(it_proxymines)] && client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)])
	{
		if (oldweapon != it_proxyminelauncher)
			self->client->newweapon = it_proxyminelauncher;
		return;
	}
	// Homing Missile Launcher
	if (client->pers.inventory[ITEM_INDEX(it_homings)] && client->pers.inventory[ITEM_INDEX(it_hominglauncher)])
	{
		if (oldweapon != it_hominglauncher)
			self->client->newweapon = it_hominglauncher;
		return;
	}
	// Explosive Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] && client->pers.inventory[ITEM_INDEX(it_explosivearrows)])
	{
		if (oldweapon != it_explosivecrossbow)
			self->client->newweapon = it_explosivecrossbow;
		return;
	}
	// Explosive Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_eshells)] && client->pers.inventory[ITEM_INDEX(it_esupershotgun)])
	{
		if (oldweapon != it_esupershotgun)
			self->client->newweapon = it_esupershotgun;
		return;
	}
	// Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_shells)] && client->pers.inventory[ITEM_INDEX(it_supershotgun)])
	{
		if (oldweapon != it_supershotgun)
			self->client->newweapon = it_supershotgun;
		return;
	}
	// Grenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_grenades)] && client->pers.inventory[ITEM_INDEX(it_grenadelauncher)])
	{
		if (oldweapon != it_grenadelauncher)
			self->client->newweapon = it_grenadelauncher;
		return;
	}
	// Airfist
	if (client->pers.inventory[ITEM_INDEX(it_airfist)] && !turret)
	{
		if (oldweapon != it_airfist)
			self->client->newweapon = it_airfist;
		return;
	}
	// Flashgrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] && !turret)
	{
		if (oldweapon != it_flashgrenadelauncher)
			self->client->newweapon = it_flashgrenadelauncher;
		return;
	}
	// Poisongrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] && !turret)
	{
		if (oldweapon != it_poisongrenadelauncher)
			self->client->newweapon = it_poisongrenadelauncher;
		return;
	}
	// Flash Grenades
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && !turret)
	{
		if (oldweapon != it_flashgrenades)
			self->client->newweapon = it_flashgrenades;
		return;
	}
	// Poison Grenades
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && !turret)
	{
		if (oldweapon != it_poisongrenades)
			self->client->newweapon = it_poisongrenades;
		return;
	}

	// found nothing so use ak42
	self->client->newweapon = it_ak42;
}

void Bot_BestFarWeapon(edict_t* self)
{
	gclient_t* client;
	gitem_t* oldweapon;
	qboolean	turret = 0;

	client = self->client;
	oldweapon = client->pers.weapon;

	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	it_airfist = FindItem("airgun");	//bugfix
	*/

	// Is our enemy a turret ?
	if (self->enemy
		&& self->enemy->inuse
		&& (self->enemy->health > 0)
		&& (self->enemy->die == Turret_Die))
	{
		turret = 1;
	}

	// No enemy but we want to place some proxies or turrets
	if (!self->enemy)
	{
		if (self->client->pers.inventory[ITEM_INDEX(it_rturret)]
			&& self->client->pers.weapon == it_rturret)
		{
			return;	//we have the rturret and no enemy...return;
		}

		if (self->client->pers.inventory[ITEM_INDEX(it_lturret)]
			&& self->client->pers.weapon == it_lturret)
		{
			return;	//we have the lturret and no enemy...return;
		}

		if (self->client->pers.inventory[ITEM_INDEX(it_proxymines)]
			&& self->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)]
			&& self->client->pers.weapon == it_proxyminelauncher)
		{
			return;	//we have proxies and no enemy...return;
		}
	}

	// Railgun
	if (client->pers.inventory[ITEM_INDEX(it_slugs)] && client->pers.inventory[ITEM_INDEX(it_railgun)])
	{
		if (oldweapon != it_railgun)
			self->client->newweapon = it_railgun;
		return;
	}
	// Buzzsaw
	if (client->pers.inventory[ITEM_INDEX(it_buzzes)] && client->pers.inventory[ITEM_INDEX(it_buzzsaw)])
	{
		if (oldweapon != it_buzzsaw)
			self->client->newweapon = it_buzzsaw;
		return;
	}
	// BFG
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] >= 50) && client->pers.inventory[ITEM_INDEX(it_bfg)])
	{
		if (oldweapon != it_bfg)
			self->client->newweapon = it_bfg;
		return;
	}
	// Explosive Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)] && client->pers.inventory[ITEM_INDEX(it_explosivearrows)])
	{
		if (oldweapon != it_explosivecrossbow)
			self->client->newweapon = it_explosivecrossbow;
		return;
	}
	// Homing Missile Launcher
	if (client->pers.inventory[ITEM_INDEX(it_homings)] && client->pers.inventory[ITEM_INDEX(it_hominglauncher)])
	{
		if (oldweapon != it_hominglauncher)
			self->client->newweapon = it_hominglauncher;
		return;
	}
	// Hyperblaster
	if ((client->pers.inventory[ITEM_INDEX(it_cells)] > 10) && client->pers.inventory[ITEM_INDEX(it_hyperblaster)])
	{
		if (oldweapon != it_hyperblaster)
			self->client->newweapon = it_hyperblaster;
		return;
	}
	// Rocket Launcher
	if (client->pers.inventory[ITEM_INDEX(it_rockets)] && client->pers.inventory[ITEM_INDEX(it_rocketlauncher)])
	{
		if (oldweapon != it_rocketlauncher)
			self->client->newweapon = it_rocketlauncher;
		return;
	}
	// Poison Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)] && client->pers.inventory[ITEM_INDEX(it_poisonarrows)])
	{
		if (oldweapon != it_poisonarrows)
			self->client->newweapon = it_poisoncrossbow;
		return;
	}
	// Crossbow
	if (client->pers.inventory[ITEM_INDEX(it_crossbow)] && client->pers.inventory[ITEM_INDEX(it_arrows)])
	{
		if (oldweapon != it_crossbow)
			self->client->newweapon = it_crossbow;
		return;
	}
	// Rocket Turret
	if (client->pers.inventory[ITEM_INDEX(it_rturret)])
	{
		if (oldweapon != it_rturret)
			self->client->newweapon = it_rturret;
		return;
	}
	// Laser Turret
	if (client->pers.inventory[ITEM_INDEX(it_lturret)])
	{
		if (oldweapon != it_lturret)
			self->client->newweapon = it_lturret;
		return;
	}
	// Explosive Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_eshells)] && client->pers.inventory[ITEM_INDEX(it_esupershotgun)])
	{
		if (oldweapon != it_esupershotgun)
			self->client->newweapon = it_esupershotgun;
		return;
	}
	// Super Shotgun
	if (client->pers.inventory[ITEM_INDEX(it_shells)] && client->pers.inventory[ITEM_INDEX(it_supershotgun)])
	{
		if (oldweapon != it_supershotgun)
			self->client->newweapon = it_supershotgun;
		return;
	}
	// Vortex
	if (client->pers.inventory[ITEM_INDEX(it_vortex)])
	{
		if (oldweapon != it_vortex)
			self->client->newweapon = it_vortex;
		return;
	}
	// Proxymine Launcher
	if (client->pers.inventory[ITEM_INDEX(it_proxymines)] && client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)])
	{
		if (oldweapon != it_proxyminelauncher)
			self->client->newweapon = it_proxyminelauncher;
		return;
	}
	// Grenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_grenades)] && client->pers.inventory[ITEM_INDEX(it_grenadelauncher)])
	{
		if (oldweapon != it_grenadelauncher)
			self->client->newweapon = it_grenadelauncher;
		return;
	}
	// Airfist
	if (client->pers.inventory[ITEM_INDEX(it_airfist)] && !turret)
	{
		if (oldweapon != it_airfist)
			self->client->newweapon = it_airfist;
		return;
	}
	// Flashgrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)] && !turret)
	{
		if (oldweapon != it_flashgrenadelauncher)
			self->client->newweapon = it_flashgrenadelauncher;
		return;
	}
	// Poisongrenade Launcher
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)] && !turret)
	{
		if (oldweapon != it_poisongrenadelauncher)
			self->client->newweapon = it_poisongrenadelauncher;
		return;
	}
	// Sword
	if (client->pers.inventory[ITEM_INDEX(it_sword)])
	{
		if (oldweapon != it_sword)
			self->client->newweapon = it_sword;
		return;
	}
	// Chainsaw
	if (client->pers.inventory[ITEM_INDEX(it_chainsaw)])
	{
		if (oldweapon != it_chainsaw)
			self->client->newweapon = it_chainsaw;
		return;
	}
	// Flash Grenades
	if (client->pers.inventory[ITEM_INDEX(it_flashgrenades)] && !turret)
	{
		if (oldweapon != it_flashgrenades)
			self->client->newweapon = it_flashgrenades;
		return;
	}
	// Poison Grenades
	if (client->pers.inventory[ITEM_INDEX(it_poisongrenades)] && !turret)
	{
		if (oldweapon != it_poisongrenades)
			self->client->newweapon = it_poisongrenades;
		return;
	}

	// found nothing so use ak42
	self->client->newweapon = it_ak42;
}

qboolean Bot_CanPickupAmmo(edict_t* ent, edict_t* eitem)
{
	gitem_t* item;

	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	*/

	item = eitem->item;

	if (item->tag == AMMO_SHELLS
		&& ent->client->pers.inventory[ITEM_INDEX(it_shells)] >= ent->client->pers.max_shells)
		return 0;
	if (item->tag == AMMO_EXPLOSIVESHELLS
		&& ent->client->pers.inventory[ITEM_INDEX(it_eshells)] >= ent->client->pers.max_eshells)
		return 0;
	if (item->tag == AMMO_ROCKETS
		&& ent->client->pers.inventory[ITEM_INDEX(it_rockets)] >= ent->client->pers.max_rockets)
		return 0;
	if (item->tag == AMMO_GRENADES
		&& ent->client->pers.inventory[ITEM_INDEX(it_grenades)] >= ent->client->pers.max_grenades)
		return 0;
	if (item->tag == AMMO_CELLS
		&& ent->client->pers.inventory[ITEM_INDEX(it_cells)] >= ent->client->pers.max_cells)
		return 0;
	if (item->tag == AMMO_SLUGS
		&& ent->client->pers.inventory[ITEM_INDEX(it_slugs)] >= ent->client->pers.max_slugs)
		return 0;
	if (item->tag == AMMO_ARROWS
		&& ent->client->pers.inventory[ITEM_INDEX(it_arrows)] >= ent->client->pers.max_arrows)
		return 0;
	if (item->tag == AMMO_POISONARROWS
		&& ent->client->pers.inventory[ITEM_INDEX(it_arrows)] >= ent->client->pers.max_poisonarrows)
		return 0;
	if (item->tag == AMMO_EXPLOSIVEARROWS
		&& ent->client->pers.inventory[ITEM_INDEX(it_arrows)] >= ent->client->pers.max_explosivearrows)
		return 0;
	if (item->tag == AMMO_FLASHGRENADES
		&& ent->client->pers.inventory[ITEM_INDEX(it_flashgrenades)] >= ent->client->pers.max_flashgrenades)
		return 0;
	if (item->tag == AMMO_LASERGRENADES
		&& ent->client->pers.inventory[ITEM_INDEX(it_lasermines)] >= ent->client->pers.max_lasergrenades)
		return 0;
	if (item->tag == AMMO_POISONGRENADES
		&& ent->client->pers.inventory[ITEM_INDEX(it_poisongrenades)] >= ent->client->pers.max_poisongrenades)
		return 0;
	if (item->tag == AMMO_PROXYMINES
		&& ent->client->pers.inventory[ITEM_INDEX(it_proxymines)] >= ent->client->pers.max_proxymines)
		return 0;
	if (item->tag == AMMO_HOMING
		&& ent->client->pers.inventory[ITEM_INDEX(it_homings)] >= ent->client->pers.max_homing)
		return 0;
	if (item->tag == AMMO_BUZZES
		&& ent->client->pers.inventory[ITEM_INDEX(it_buzzes)] >= ent->client->pers.max_buzzes)
		return 0;
	if (item->tag == AMMO_RTURRET
		&& ent->client->pers.inventory[ITEM_INDEX(it_rturret)] >= ent->client->pers.max_rturret)
		return 0;
	if (item->tag == AMMO_LTURRET
		&& ent->client->pers.inventory[ITEM_INDEX(it_lturret)] >= ent->client->pers.max_lturret)
		return 0;
	if (item->tag == AMMO_VORTEX
		&& ent->client->pers.inventory[ITEM_INDEX(it_vortex)] >= ent->client->pers.max_vortex)
		return 0;

	return 1;
}

qboolean Bot_CanPickupItem(edict_t* ent, edict_t* eitem)
{
	gitem_t* item;

	/* MrG{DRGN} no longer needed
	it_lturret = FindItem("automatic defence turret");	//bugfix
	*/

	item = eitem->item;

	if (item == FindItemByClassindex(ITEM_FLAG_TEAM1)
		|| item == FindItemByClassindex(ITEM_FLAG_TEAM2))
		return 0;

	/* MrG{DRGN} don't go for keys (which should probably be removed eventually), or multiple grappling hooks or Jetpacks */
	if (item->flags & IT_KEY)
		return 0;
	if (item == it_grapple && ent->client->pers.inventory[ITEM_INDEX(it_grapple)])
		return 0;
	if (item == it_jetpack && ent->client->pers.inventory[ITEM_INDEX(it_jetpack)] > 0 && ent->client->jet_remaining == 600)
		return 0;

	// MrG{DRGN} if they already have a weapon don't go for duplicates!
	// class 10
	if (item == it_bfg && ent->client->pers.inventory[ITEM_INDEX(it_bfg)])
		return 0;

	// class 9
	if (item == it_railgun && ent->client->pers.inventory[ITEM_INDEX(it_railgun)])
		return 0;
	if (item == it_buzzsaw && ent->client->pers.inventory[ITEM_INDEX(it_buzzsaw)])
		return 0;

	// class 8
	if (item == it_hyperblaster && ent->client->pers.inventory[ITEM_INDEX(it_hyperblaster)])
		return 0;

	// class 7
	if (item == it_rocketlauncher && ent->client->pers.inventory[ITEM_INDEX(it_rocketlauncher)])
		return 0;
	if (item == it_hominglauncher && ent->client->pers.inventory[ITEM_INDEX(it_hominglauncher)])
		return 0;

	// class 6
	if (item == it_grenadelauncher && ent->client->pers.inventory[ITEM_INDEX(it_grenadelauncher)])
		return 0;
	if (item == it_proxyminelauncher && ent->client->pers.inventory[ITEM_INDEX(it_proxyminelauncher)])
		return 0;
	if (item == it_poisongrenadelauncher && ent->client->pers.inventory[ITEM_INDEX(it_poisongrenadelauncher)])
		return 0;
	if (item == it_flashgrenadelauncher && ent->client->pers.inventory[ITEM_INDEX(it_flashgrenadelauncher)])
		return 0;
	
	// class 5
	if (item == it_airfist && ent->client->pers.inventory[ITEM_INDEX(it_airfist)])
		return 0;

	// class 4
	if (item == it_crossbow && ent->client->pers.inventory[ITEM_INDEX(it_crossbow)])
		return 0;
	if (item == it_explosivecrossbow && ent->client->pers.inventory[ITEM_INDEX(it_explosivecrossbow)])
		return 0;
	if (item == it_poisoncrossbow && ent->client->pers.inventory[ITEM_INDEX(it_poisoncrossbow)])
		return 0;

	// class 3
	if (item == it_supershotgun && ent->client->pers.inventory[ITEM_INDEX(it_supershotgun)])
		return 0;
	if (item == it_esupershotgun && ent->client->pers.inventory[ITEM_INDEX(it_esupershotgun)])
		return 0;
	// class 2
	if (item == it_sword && ent->client->pers.inventory[ITEM_INDEX(it_sword)])
		return 0;
	if (item == it_chainsaw && ent->client->pers.inventory[ITEM_INDEX(it_chainsaw)])



	/* MrG{DRGN} Tweak Havoc bot health hunting routine
	if (item == FindItem("Health") && ent->health >= ent->max_health)
		return 0; */
	if (item == it_health ||
		item == it_health_large && ent->health >= ent->max_health)
		return 0;

	if (item == it_tech2
		|| item == it_tech3
		|| item == it_tech4
		|| item == it_tech1
		)

	{
		if (ent->client->pers.inventory[ITEM_INDEX(it_tech2)]
			|| ent->client->pers.inventory[ITEM_INDEX(it_tech3)]
			|| ent->client->pers.inventory[ITEM_INDEX(it_tech4)]
			|| ent->client->pers.inventory[ITEM_INDEX(it_tech1)])
			return 0;
	}

	if (!Bot_CanPickupAmmo(ent, eitem))
		return 0;
	/* MrG{DRGN} */
	if (eitem->classindex == AR_BODY
		|| eitem->classindex == AR_JACKET
		|| eitem->classindex == AR_COMBAT)
	{
		if (!Bot_CanPickupArmor(ent, eitem))/* MrG{DRGN} there was an unintended ; here. */
			return 0;
	}

	return 1;
}

int Bot_CanPickupArmor(edict_t* self, edict_t* ent)
{
	int				old_armor_index;
	gitem_armor_t* oldinfo;
	gitem_armor_t* newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;
	gclient_t* client;

	if (self->client)
		client = self->client;
	else
		return 0;

	// get info on new armor
	newinfo = (gitem_armor_t*)ent->item->info;

	old_armor_index = ArmorIndex(self);

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		return 1;
	}
	else if (!old_armor_index)
	{
		return 1;
	}
	else
	{
		if (old_armor_index == jacket_armor_index)
			oldinfo = &jacketarmor_info;
		else if (old_armor_index == combat_armor_index)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = (int)(salvage * client->pers.inventory[old_armor_index]); /* MrG{DRGN} explicit cast to shut up compiler! */
			newcount = newinfo->base_count + salvagecount;
			if (newcount > newinfo->max_count)
				newcount = newinfo->max_count;

			return 1;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = (int)(salvage * newinfo->base_count); /* MrG{DRGN} explicit cast to shut up compiler! */
			newcount = client->pers.inventory[old_armor_index] + salvagecount;
			if (newcount > oldinfo->max_count)
				newcount = oldinfo->max_count;

			// if we're already maxed out then we don't need the new armor
			if (client->pers.inventory[old_armor_index] >= newcount)
				return 0;

			return 1;
		}
	}
}

qboolean Bot_CanReachSpotDirectly(edict_t* ent, vec3_t target)
{
	vec3_t	dir = { 0 }, midpos, end_trace = { 0 }, mins = { 0 };
	trace_t	tr;
	//float	dist, progressive_dist;

	// water
	if (ent->waterlevel && (gi.pointcontents(target) & MASK_WATER))
		return true;

	VectorAdd(ent->mins, tv(0, 0, 12), mins);
	//VectorCopy(ent->s.origin, start);

	VectorSubtract(target, ent->s.origin, dir);
	VectorMA(ent->s.origin, 0.5, dir, midpos);
	VectorSubtract(midpos, tv(0, 0, 40), end_trace);

	// check if the way to target is free
	tr = gi.trace(ent->s.origin, NULL, NULL, target, ent, MASK_SOLID);

	if (tr.ent && Q_stricmp(tr.ent->classname, "func_door") == 0)
	{
		// we are standing in front of a door
	}
	else if (tr.fraction != 1 || tr.startsolid)
	{
		return false;
	}

	// is the midpos is on ground
	tr = gi.trace(midpos, mins, ent->maxs, end_trace, ent, MASK_SOLID);

	if (tr.fraction == 1)
		return false;

	//if ((dist = VectorLength(dir)) < 32)
	return true;
}

qboolean Bot_CheckObstacle(edict_t* self)
{
	vec3_t  mins = { -16, -16, -10 }, maxs = { 16, 16, 0 };
	vec3_t	start = { 0 }, forward, dir;
	trace_t	tr;

	AngleVectors(self->client->v_angle, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(self->s.origin, 30, forward, dir);

	VectorCopy(self->s.origin, start);
	start[2] += 10;
	/* MrG{DRGN} operator has equivalent nested operands
	tr = gi.trace(start, mins, maxs, dir, self, MASK_SOLID | MASK_PLAYERSOLID);	*/
	tr = gi.trace(start, mins, maxs, dir, self, MASK_PLAYERSOLID);
	if ((tr.fraction != 1) || tr.startsolid)
		return true;

	return false;
}

void AddItemToList(edict_t* ent)
{
	edict_t* current = NULL;

	if (!ent->item)
		return;

	if (!ent->item->pickup)
		return;

	// Don't add these to the list!
	/* MrG{DRGN} classindex instead of classname */
	if (ent->classindex == AR_SHARD
		|| ent->classindex == ITEM_FLAG_TEAM1
		|| ent->classindex == ITEM_FLAG_TEAM2
		|| ent->classindex == FREED
		|| ent->classindex == ITEM_HEALTH_SMALL)
		return;

	//find the list head
	/* MrG{DRGN} classindex instead of classname */
	if (ent->item->pickup == Pickup_Weapon
		|| ent->item->pickup == Pickup_NoAmmoWeapon
		|| ent->classindex == AM_VORTEX
		|| ent->classindex == AM_LTURRET
		|| ent->classindex == AM_RTURRET
		)
	{
		if (!weapon_list)	//list is empty so the first item becomes the head
		{
			weapon_list = ent;
			return;
		}
		else
			current = weapon_list;	// start with the head
	}
	else if (ent->item->pickup == Pickup_Health)
	{
		if (!health_list)	//list is empty so the first item becomes the head
		{
			health_list = ent;
			return;
		}
		else
			current = health_list;	// start with the head
	}
	else if (ent->item->pickup == Pickup_Ammo)
		if (!ammo_list)	//list is empty so the first item becomes the head
		{
			ammo_list = ent;
			return;
		}
		else
			current = ammo_list;	// start with the head
	else
	{
		if (!powerup_list)	//list is empty so the first item becomes the head
		{
			powerup_list = ent;
			return;
		}
		else
			current = powerup_list;	// start with the head
	}

	//go through all items in the list
	while (current)
	{
		if (current == ent) //items already is in the list
			return;

		if (current->next_listitem)	//list go's on
			current = current->next_listitem;	//go to next item in list
		else	// next position is free so add current item there
		{
			current->next_listitem = ent;
			return;
		}
	}
}

void RemoveFromList(edict_t* ent)
{
	edict_t* current = NULL;

	if (!ent->item)
		return;

	if (!ent->item->pickup)
		return;

	if (ent->classindex == AR_SHARD
		|| ent->classindex == ITEM_FLAG_TEAM1
		|| ent->classindex == ITEM_FLAG_TEAM2
		|| ent->classindex == FREED
		|| ent->classindex == ITEM_HEALTH_SMALL)
		return;

	//find the list head
	if (ent->item->pickup == Pickup_Weapon)
	{
		if (!weapon_list)	//list is empty
		{
			return;
		}
		else
			current = weapon_list;	// start with the head
	}
	else if (ent->item->pickup == Pickup_Health)
	{
		if (!health_list)	//list is empty
		{
			return;
		}
		else
			current = health_list;
	}
	else if (ent->item->pickup == Pickup_Ammo)
	{
		if (!ammo_list)	//list is empty so the first item becomes the head
		{
			return;
		}
		else
			current = ammo_list;
	}
	else
	{
		if (!powerup_list)	//list is empty so the first item becomes the head
		{
			return;
		}
		else
			current = powerup_list;	// start with the head
	}

	while (current)
	{
		if (current->next_listitem == ent)	//found it
		{
			break;
		}

		current = current->next_listitem;
	}

	if (current)
	{
		if (current->next_listitem->next_listitem)
			current->next_listitem = current->next_listitem->next_listitem;
		else
			current->next_listitem = NULL;
	}
}

void Load_BotChat(void)
{
	/* FILE* fp; MrG{DRGN} moved below */
	int		section, line = 0, i; /* MrG{DRGN} initialized 09/23/2020 */
	char	filename[MAX_QPATH];
	char	buffer;

	memset(chat_text, 0, sizeof(chat_text));

	Com_strcpy(filename, sizeof filename, "./");
	Com_strcat(filename, sizeof filename, game_dir->string);
	Com_strcat(filename, sizeof filename, "/botchat.txt");

	FILE* fp = fopen(filename, "r");

	if (!fp)
	{
		//QwazyWabbit gracefully fail if file doesn't exist.
		gi.dprintf("Unable to open file! %s.\n", strerror(errno));
		gi.dprintf("Forcing botchat OFF\n");
		gi.cvar_set("botchat", "0");
		return;
	}

	section = -1;

	while (!feof(fp))
	{
		// QwazyWabbit if file exists but doesn't parse, fall out and disable bot chat.
		if (fscanf(fp, "%c", &buffer) == EOF && section == -1) /* MrG{DRGN check return */
		{
			gi.dprintf("Chaos: ERROR reading %s\n", filename);
			gi.dprintf("Forcing botchat OFF and aborting file load.\n\n");
			gi.cvar_set("botchat", "0");
			fclose(fp);
			return;
		}

		if (buffer == ';')	//comment, strip the rest of the line
		{
			// skip to end of line
			while (!feof(fp) && (buffer != '\n') && fscanf(fp, "%c", &buffer) != EOF);
		}
		else if (section > NUM_CHATSECTIONS - 1)
		{
			fclose(fp);
			return;
		}
		else if (buffer == '[')	// section begin
		{
			section++;
			line = -1;

			while (!feof(fp) && (buffer != '\n') && fscanf(fp, "%c", &buffer) != EOF);	// read the end of the line
		}
		else if ((((buffer >= 'a') && (buffer <= 'z')) ||	// a chat line...read it
			((buffer >= 'A') && (buffer <= 'Z')) ||
			(buffer == '%')) && (section >= 0))/* MrG{DRGN} added section >= 0 check to avoid array overrun*/
		{
			i = 0;
			line++;

			// allow for potential resizes of botchat files
			int size = NUM_CHATSECTIONS * MAX_LINES_PER_SECTION;
			// allocate memory
			chat_text[section][line] = gi.TagMalloc(size, TAG_GAME);
			memset(chat_text[section][line], 0, size);

			while (!feof(fp) && (buffer != '\n'))
			{
				chat_text[section][line][i++] = buffer;

				if (fscanf(fp, "%c", &buffer) != EOF)
					; /* Empty body */
			}

			if (i > 0)
			{
				chat_text[section][line][i] = '\0';	//append nul
				//DbgPrintf("%s S%i L%i\n", chat_text[section][line], section, line);
			}
			else	//empty line ?
				line--;

			chat_linecount[section] = line;
		}
	}

	gi.dprintf("%s successfully loaded %u sections.\n", filename, section + 1);
	fclose(fp);
}

char* Get_RandomBotSkin(void)
{
	switch ((int)(random() * 28))
	{
	case 0:  	return "male\cipher"; break;
	case 1:	return "male\claymore"; break;
	case 2:	return "male\flak"; break;
	case 3:	return "male\grunt"; break;
	case 4:	return "male\howitzer"; break;
	case 5:	return "male\major"; break;
	case 6:	return "male\nightops"; break;
	case 7:	return "male\pointman"; break;
	case 8:	return "male\psycho"; break;
	case 9:	return "male\rampage"; break;
	case 10:	return "male\razor"; break;
	case 11:	return "male\recon"; break;
	case 12:	return "male\scout"; break;
	case 13:	return "male\sniper"; break;
	case 14:	return "male\viper"; break;
	case 15: 	return "female\athena"; break;
	case 16: 	return "female\brianna"; break;
	case 17:	return "female\cobalt"; break;
	case 18:	return "female\ensign"; break;
	case 19:	return "female\jezebel"; break;
	case 20:	return "female\jungle"; break;
	case 21:	return "female\lotus"; break;
	case 22:	return "female\stiletto"; break;
	case 23:	return "female\venus"; break;
	case 24:	return "female\voodoo"; break;
	case 25:	return "cyborg\oni911"; break;
	case 26:	return "cyborg\ps9000"; break;
	case 27:	return "cyborg\tyr574"; break;
	default:	return "male\major"; break;
	}
}
