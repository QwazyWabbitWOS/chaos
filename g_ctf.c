﻿#include "g_local.h"
#include "stdlog.h"
#include "m_player.h"

typedef enum match_s {
	MATCH_NONE,
	MATCH_SETUP,
	MATCH_PREGAME,
	MATCH_GAME,
	MATCH_POST
} match_t;

typedef enum {
	ELECT_NONE,
	ELECT_MATCH,
	ELECT_ADMIN,
	ELECT_MAP
} elect_t;

typedef struct ctfgame_s {
	int team1, team2;
	int total1, total2;	// these are only set when going into intermission!
	float last_flag_capture;
	int last_capture_team;

	match_t match;		// match state
	float matchtime;	// time for match start/end (depends on state)
	int lasttime;		// last time update
	qboolean countdown;	// has audio countdown started?

	elect_t election;	// election type
	edict_t* etarget;	// for admin election, who's being elected
	char elevel[32];	// for map election, target level
	int evotes;		// votes so far
	int needvotes;		// votes needed
	float electtime;	// remaining time until election times out
	char emsg[256];		// election name
	int warnactive;		// true if stat string 30 is active

	ghost_t ghosts[MAX_CLIENTS];	// ghost codes
} ctfgame_t;

ctfgame_t ctfgame;

cvar_t* ctf;
cvar_t* ctf_forcejoin;

cvar_t* competition;
cvar_t* matchlock;
cvar_t* electpercentage;
cvar_t* matchtime;
cvar_t* matchsetuptime;
cvar_t* matchstarttime;
cvar_t* admin_password;
cvar_t* allow_admin;
cvar_t* warp_list;
cvar_t* warn_unbalanced;

ctfgame_t ctfgame;
qboolean techspawn = false;

cvar_t* ctf;
cvar_t* ctf_forcejoin;

char* ctf_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"xv 246 "
"num 2 10 "
"xv 296 "
"pic 9 "
"endif "

//  help / weapon icon
"if 11 "
"xv 148 "
"pic 11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "

//tech
"yb -129 "
"if 26 "
"xr -26 "
"pic 26 "
"endif "

// red team
"yb -102 "
"if 17 "
"xr -26 "
"pic 17 "
"endif "
"xr -62 "
"num 2 18 "
//joined overlay
"if 22 "
"yb -104 "
"xr -28 "
"pic 22 "
"endif "

// blue team
"yb -75 "
"if 19 "
"xr -26 "
"pic 19 "
"endif "
"xr -62 "
"num 2 20 "
"if 23 "
"yb -77 "
"xr -28 "
"pic 23 "
"endif "

// have flag graph
"if 21 "
"yt 26 "
"xr -24 "
"pic 21 "
"endif "

// id view state
"if 27 "
"xv 0 "
"yb -58 "
"string \"Viewing\" "
"xv 64 "
"stat_string 27 "
"endif "
;

static char* tnames[] = {
	"item_tech1", "item_tech2", "item_tech3", "item_tech4",
	NULL
};

void stuffcmd(edict_t* ent, char* s)
{
	gi.WriteByte(11);
	gi.WriteString(s);
	gi.unicast(ent, true);
}

/*--------------------------------------------------------------------------*/

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static edict_t* loc_findradius(edict_t* from, vec3_t org, float rad)
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
#if 0
		if (from->solid == SOLID_NOT)
			continue;
#endif
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5F); /* MrG{DRGN} This was 0.5, I changed it to an explicit float */
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}

static void loc_buildboxpoints(vec3_t p[8], vec3_t org, vec3_t mins, vec3_t maxs)
{
	VectorAdd(org, mins, p[0]);
	VectorCopy(p[0], p[1]);
	p[1][0] -= mins[0];
	VectorCopy(p[0], p[2]);
	p[2][1] -= mins[1];
	VectorCopy(p[0], p[3]);
	p[3][0] -= mins[0];
	p[3][1] -= mins[1];
	VectorAdd(org, maxs, p[4]);
	VectorCopy(p[4], p[5]);
	p[5][0] -= maxs[0];
	VectorCopy(p[0], p[6]);
	p[6][1] -= maxs[1];
	VectorCopy(p[0], p[7]);
	p[7][0] -= maxs[0];
	p[7][1] -= maxs[1];
}

static qboolean loc_CanSee(edict_t* targ, edict_t* inflictor)
{
	trace_t	trace;
	vec3_t	targpoints[8];
	int i;
	vec3_t viewpoint = { 0 };

	// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
		return false; // bmodels not supported

	loc_buildboxpoints(targpoints, targ->s.origin, targ->mins, targ->maxs);

	VectorCopy(inflictor->s.origin, viewpoint);
	viewpoint[2] += inflictor->viewheight;

	for (i = 0; i < 8; i++) {
		trace = gi.trace(viewpoint, vec3_origin, vec3_origin, targpoints[i], inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
	}

	return false;
}

/*--------------------------------------------------------------------------*/
/* MrG{DRGN} this is done in SetItemNames, now.
static gitem_t* it_flag1_item;
static gitem_t* it_flag2_item;
*/

void CTFInit_old(void)
{
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO); //MATTHIAS
	ctf_forcejoin = gi.cvar("ctf_forcejoin", "1", CVAR_LATCH);
	/*
	if (!it_flag_red)
		it_flag_red = FindItemByClassindex(ITEM_FLAG_TEAM1);
	if (!it_flag_blue)
		it_flag_blue = FindItemByClassindex(ITEM_FLAG_TEAM1);
	*/
	memset(&ctfgame, 0, sizeof(ctfgame));
	techspawn = false;
}

void CTFInit(void)
{
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO);
	ctf_forcejoin = gi.cvar("ctf_forcejoin", "1", 0);
	competition = gi.cvar("competition", "0", CVAR_SERVERINFO);
	matchlock = gi.cvar("matchlock", "1", CVAR_SERVERINFO);
	electpercentage = gi.cvar("electpercentage", "66", 0);
	matchtime = gi.cvar("matchtime", "20", CVAR_SERVERINFO);
	matchsetuptime = gi.cvar("matchsetuptime", "10", 0);
	matchstarttime = gi.cvar("matchstarttime", "20", 0);
	admin_password = gi.cvar("admin_password", "", 0);
	allow_admin = gi.cvar("allow_admin", "1", 0);
	warp_list = gi.cvar("warp_list", "q2ctf1 q2ctf2 q2ctf3 q2ctf4 q2ctf5", 0);
	warn_unbalanced = gi.cvar("warn_unbalanced", "1", 0);
	memset(&ctfgame, 0, sizeof(ctfgame));
	techspawn = false;
}

/*
/*--------------------------------------------------------------------------*/

char* CTFTeamName(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return "RED";
	case CTF_TEAM2:
		return "BLUE";
	}
	return "UKNOWN";
}

char* CTFOtherTeamName(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return "BLUE";
	case CTF_TEAM2:
		return "RED";
	}
	return "UKNOWN";
}

int CTFOtherTeam(int team)
{
	switch (team) {
	case CTF_TEAM1:
		return CTF_TEAM2;
	case CTF_TEAM2:
		return CTF_TEAM1;
	}
	return -1; // invalid value
}

/*--------------------------------------------------------------------------*/



void CTFAssignSkin(edict_t* ent, char* s)
{
	int playernum = ent - g_edicts - 1;
	char* p;
	char t[64];

	Com_sprintf(t, sizeof(t), "%s", s);

	/* MrG{DRGN} Bugfix!
	if ((p = strrchr(t, '/')) != NULL)
	*/
	if ((p = strchr(t, '/')) != NULL)
		p[1] = 0;
	else 		
		Com_strcpy(t, sizeof(t), "male/");

	switch (ent->client->resp.ctf_team) {
	case CTF_TEAM1:
		gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s%s",
			ent->client->pers.netname, t, CTF_TEAM1_SKIN));
		break;
	case CTF_TEAM2:
		gi.configstring(CS_PLAYERSKINS + playernum,
			va("%s\\%s%s", ent->client->pers.netname, t, CTF_TEAM2_SKIN));
		break;
	default:
		gi.configstring(CS_PLAYERSKINS + playernum,
			va("%s\\%s", ent->client->pers.netname, s));
		break;
	}
	//	cprint_botsafe(ent, PRINT_HIGH, "You have been assigned to %s team.\n", ent->client->pers.netname);
}

void CTFAssignTeam(gclient_t* who)
{
	edict_t* player;
	int i;
	int team1count = 0, team2count = 0;

	who->resp.ctf_state = CTF_STATE_START;

	if (!((int)dmflags->value & DF_CTF_FORCEJOIN)) {
		who->resp.ctf_team = CTF_NOTEAM;
		return;
	}

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse || player->client == who)
			continue;

		switch (player->client->resp.ctf_team) {
		case CTF_TEAM1:
			team1count++;
			break;
		case CTF_TEAM2:
			team2count++;
		}
	}
	if (team1count < team2count) /* MrG{DRGN} bugfix was team1count on both sides of < */
		who->resp.ctf_team = CTF_TEAM1;
	else if (team2count < team1count)
		who->resp.ctf_team = CTF_TEAM2;
	else if (rand() & 1)
		who->resp.ctf_team = CTF_TEAM1;
	else
		who->resp.ctf_team = CTF_TEAM2;
}

/*
================
SelectCTFSpawnPoint

go to a ctf point, but NOT the two points closest
to other players
================
*/
edict_t* SelectCTFSpawnPoint(edict_t* ent)
{
	edict_t* spot, * spot1, * spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;
	char* cname;

	if (ent->client->resp.ctf_state != CTF_STATE_START)
	{
		if ((int)(dmflags->value) & DF_SPAWN_FARTHEST)
			return SelectFarthestDeathmatchSpawnPoint();
		else
			return SelectRandomDeathmatchSpawnPoint();
	}

	ent->client->resp.ctf_state = CTF_STATE_PLAYING;

	switch (ent->client->resp.ctf_team) {
	case CTF_TEAM1:
		cname = "info_player_team1";
		break;
	case CTF_TEAM2:
		cname = "info_player_team2";
		break;
	default:
		return SelectRandomDeathmatchSpawnPoint();
	}

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find(spot, FOFS(classname), cname)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return SelectRandomDeathmatchSpawnPoint();

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find(spot, FOFS(classname), cname);
		if (spot == spot1 || spot == spot2)
			selection++;
	} while (selection--);

	return spot;
}

/*------------------------------------------------------------------------*/
/*
CTFFragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
*/
void CTFFragBonuses(edict_t* targ, edict_t* inflictor, edict_t* attacker)
{
	int i;
	edict_t* ent;
	gitem_t* flag_item, * enemy_flag_item;
	int otherteam;
	edict_t* flag, * carrier = NULL;
	char* c;
	vec3_t v1 = { 0 }, v2 = { 0 };

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	otherteam = CTFOtherTeam(targ->client->resp.ctf_team);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (targ->client->resp.ctf_team == CTF_TEAM1) {
		flag_item = it_flag_red;
		enemy_flag_item = it_flag_blue;
	}
	else {
		flag_item = it_flag_blue;
		enemy_flag_item = it_flag_red;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->pers.inventory[ITEM_INDEX(enemy_flag_item)]) {
		attacker->client->resp.ctf_lastfraggedcarrier = level.time;
		attacker->client->resp.score += CTF_FRAG_CARRIER_BONUS;
		cprint_botsafe(attacker, PRINT_MEDIUM, "BONUS: %d points for fragging enemy flag carrier.\n",
			CTF_FRAG_CARRIER_BONUS);

		// Log Flag Carrier Frag - Mark Davies
		sl_LogScore(&gi,
			attacker->client->pers.netname,
			NULL,
			"FC Frag",
			NULL,
			CTF_FRAG_CARRIER_BONUS,
			level.time);

		// The target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 1; i <= maxclients->value; i++) {
			ent = g_edicts + i;
			if (ent->inuse && ent->client->resp.ctf_team == otherteam)
				ent->client->resp.ctf_lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
		level.time - targ->client->resp.ctf_lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->pers.inventory[ITEM_INDEX(flag_item)]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		attacker->client->resp.score += CTF_CARRIER_DANGER_PROTECT_BONUS;
		bprint_botsafe(PRINT_MEDIUM, "%s defends %s's flag carrier against an aggressive enemy\n",
			attacker->client->pers.netname,
			CTFTeamName(attacker->client->resp.ctf_team));

		// Log Flag Danger Carrier Protect Frag - Mark Davies
		sl_LogScore(&gi,
			attacker->client->pers.netname,
			NULL,
			"FC Def",
			NULL,
			CTF_CARRIER_DANGER_PROTECT_BONUS,
			level.time);

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->resp.ctf_team) {
	case CTF_TEAM1:
		c = "item_flag_team1";
		break;
	case CTF_TEAM2:
		c = "item_flag_team2";
		break;
	default:
		return;
	}

	flag = NULL;
	while ((flag = G_Find(flag, FOFS(classname), c)) != NULL) {
		if (!(flag->spawnflags & DROPPED_ITEM))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	carrier = NULL; /* MrG{DRGN} make sure! */

	// find attacker's team's flag carrier
	for (i = 1; i <= maxclients->value; i++) {
		carrier = g_edicts + i;
		if (carrier->inuse &&
			carrier->client->pers.inventory[ITEM_INDEX(flag_item)])
			break;
		carrier = NULL;
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract(targ->s.origin, flag->s.origin, v1);
	VectorSubtract(attacker->s.origin, flag->s.origin, v2);

	if (VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS ||
		VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS ||
		loc_CanSee(flag, targ) || loc_CanSee(flag, attacker)) {
		// we defended the base flag
		attacker->client->resp.score += CTF_FLAG_DEFENSE_BONUS;
		if (flag->solid == SOLID_NOT)
			bprint_botsafe(PRINT_MEDIUM, "%s defends the %s base.\n",
				attacker->client->pers.netname,
				CTFTeamName(attacker->client->resp.ctf_team));
		else
			bprint_botsafe(PRINT_MEDIUM, "%s defends the %s flag.\n",
				attacker->client->pers.netname,
				CTFTeamName(attacker->client->resp.ctf_team));

		// Log Flag Defense - Mark Davies
		sl_LogScore(&gi,
			attacker->client->pers.netname,
			NULL,
			"F Def",
			NULL,
			CTF_FLAG_DEFENSE_BONUS,
			level.time);
		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->s.origin, carrier->s.origin, v1);
		VectorSubtract(attacker->s.origin, carrier->s.origin, v1);

		if (VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS ||
			VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS ||
			loc_CanSee(carrier, targ) || loc_CanSee(carrier, attacker)) {
			attacker->client->resp.score += CTF_CARRIER_PROTECT_BONUS;
			bprint_botsafe(PRINT_MEDIUM, "%s defends the %s's flag carrier.\n",
				attacker->client->pers.netname,
				CTFTeamName(attacker->client->resp.ctf_team));

			// Log Flag Carrier Protect Frag - Mark Davies
			sl_LogScore(&gi,
				attacker->client->pers.netname,
				NULL,
				"FC Def",
				NULL,
				CTF_CARRIER_PROTECT_BONUS,
				level.time);

			return;
		}
	}
}

void CTFCheckHurtCarrier(edict_t* targ, edict_t* attacker)
{
	gitem_t* flag_item;

	if (!targ || !attacker)
		return;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->resp.ctf_team == CTF_TEAM1)
		flag_item = it_flag_blue;
	else
		flag_item = it_flag_red;

	if (targ->client->pers.inventory[ITEM_INDEX(flag_item)] &&
		targ->client->resp.ctf_team != attacker->client->resp.ctf_team)
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
}

/*------------------------------------------------------------------------*/

void CTFResetFlag(int ctf_team)
{
	char* c;
	edict_t* ent;

	switch (ctf_team) {
	case CTF_TEAM1:
		c = "item_flag_team1";
		break;
	case CTF_TEAM2:
		c = "item_flag_team2";
		break;
	default:
		return;
	}

	ent = NULL;
	while ((ent = G_Find(ent, FOFS(classname), c)) != NULL) {
		if (ent->spawnflags & DROPPED_ITEM)
			G_FreeEdict(ent);
		else {
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

void CTFResetFlags(void)
{
	CTFResetFlag(CTF_TEAM1);
	CTFResetFlag(CTF_TEAM2);
}

qboolean CTFPickup_Flag(edict_t* ent, edict_t* other)
{
	int ctf_team;
	int i;
	edict_t* player;
	gitem_t* flag_item, * enemy_flag_item;

	// figure out what team this flag is
	if (strcmp(ent->classname, "item_flag_team1") == 0)
		ctf_team = CTF_TEAM1;
	else if (strcmp(ent->classname, "item_flag_team2") == 0)
		ctf_team = CTF_TEAM2;
	else {
		cprint_botsafe(ent, PRINT_HIGH, "Don't know what team the flag is on.\n");
		return false;
	}

	// same team, if the flag at base, check to he has the enemy flag
	if (ctf_team == CTF_TEAM1) {
		flag_item = it_flag_red;
		enemy_flag_item = it_flag_blue;
	}
	else {
		flag_item = it_flag_blue;
		enemy_flag_item = it_flag_red;
	}

	if (ctf_team == other->client->resp.ctf_team) {
		if (!(ent->spawnflags & DROPPED_ITEM)) {
			// the flag is at home base.  if the player has the enemy
			// flag, he's just won!

			if (other->client->pers.inventory[ITEM_INDEX(enemy_flag_item)]) {
				bprint_botsafe(PRINT_HIGH, "%s captured the %s flag!\n",
					other->client->pers.netname, CTFOtherTeamName(ctf_team));

				// Log Flag Capture - Mark Davies
				sl_LogScore(&gi,
					other->client->pers.netname,
					NULL,
					"F Capture",
					NULL,
					CTF_CAPTURE_BONUS,
					level.time);

				other->client->pers.inventory[ITEM_INDEX(enemy_flag_item)] = 0;

				ctfgame.last_flag_capture = level.time;
				ctfgame.last_capture_team = ctf_team;
				if (ctf_team == CTF_TEAM1)
					ctfgame.team1++;
				else
					ctfgame.team2++;

				gi.sound(ent, CHAN_RELIABLE + CHAN_NO_PHS_ADD + CHAN_VOICE, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);

				// other gets another 10 frag bonus
				other->client->resp.score += CTF_CAPTURE_BONUS;

				// Ok, let's do the player loop, hand out the bonuses
				for (i = 1; i <= maxclients->value; i++) {
					player = &g_edicts[i];
					if (!player->inuse)
						continue;

					if (player->client->resp.ctf_team != other->client->resp.ctf_team)
						player->client->resp.ctf_lasthurtcarrier = -5;
					else/* if (player->client->resp.ctf_team == other->client->resp.ctf_team) MrG{DRGN} always true if we got here */ {
						if (player != other)
						{
							player->client->resp.score += CTF_TEAM_BONUS;

							// Log Flag Capture Team Score - Mark Davies
							sl_LogScore(&gi,
								player->client->pers.netname,
								NULL,
								"Team Score",
								NULL,
								CTF_TEAM_BONUS,
								level.time);
						}
						// award extra points for capture assists
						if (player->client->resp.ctf_lastreturnedflag + CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
							bprint_botsafe(PRINT_HIGH, "%s gets an assist for returning the flag!\n", player->client->pers.netname);
							player->client->resp.score += CTF_RETURN_FLAG_ASSIST_BONUS;

							// Log Flag Capture Team Score - Mark Davies
							sl_LogScore(&gi,
								player->client->pers.netname,
								NULL,
								"F Return Assist",
								NULL,
								CTF_RETURN_FLAG_ASSIST_BONUS,
								level.time);
						}
						if (player->client->resp.ctf_lastfraggedcarrier + CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
							bprint_botsafe(PRINT_HIGH, "%s gets an assist for fragging the flag carrier!\n", player->client->pers.netname);
							player->client->resp.score += CTF_FRAG_CARRIER_ASSIST_BONUS;

							// Log Flag Capture Team Score - Mark Davies
							sl_LogScore(&gi,
								player->client->pers.netname,
								NULL,
								"FC Frag Assist",
								NULL,
								CTF_FRAG_CARRIER_ASSIST_BONUS,
								level.time);
						}
					}
				}

				CTFResetFlags();
				return false;
			}
			return false; // it's at home base already
		}
		// hey, it's not home.  return it by teleporting it back
		bprint_botsafe(PRINT_HIGH, "%s returned the %s flag!\n",
			other->client->pers.netname, CTFTeamName(ctf_team));

		// Log Flag Recover - Mark Davies
		sl_LogScore(&gi,
			other->client->pers.netname,
			NULL,
			"F Return",
			NULL,
			CTF_RECOVERY_BONUS,
			level.time);

		other->client->resp.score += CTF_RECOVERY_BONUS;
		other->client->resp.ctf_lastreturnedflag = level.time;
		gi.sound(ent, CHAN_RELIABLE + CHAN_NO_PHS_ADD + CHAN_VOICE, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
		//CTFResetFlag will remove this entity!  We must return false
		CTFResetFlag(ctf_team);
		return false;
	}

	// hey, it's not our flag, pick it up
	bprint_botsafe(PRINT_HIGH, "%s got the %s flag!\n",
		other->client->pers.netname, CTFTeamName(ctf_team));
	other->client->resp.score += CTF_FLAG_BONUS;

	// Log Flag Pickup - Mark Davies
	sl_LogScore(&gi,
		other->client->pers.netname,
		NULL,
		"F Pickup",
		NULL,
		CTF_FLAG_BONUS,
		level.time);

	other->client->pers.inventory[ITEM_INDEX(flag_item)] = 1;
	other->client->resp.ctf_flagsince = level.time;

	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnflags & DROPPED_ITEM)) {
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}

static void CTFDropFlagTouch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > CTF_AUTO_FLAG_RETURN_TIMEOUT - 2)
		return;

	Touch_Item(ent, other, plane, surf);
}

static void CTFDropFlagThink(edict_t* ent)
{
	// auto return the flag
	// reset flag will remove ourselves
	if (strcmp(ent->classname, "item_flag_team1") == 0) {
		CTFResetFlag(CTF_TEAM1);
		bprint_botsafe(PRINT_HIGH, "The %s flag has returned!\n",
			CTFTeamName(CTF_TEAM1));
	}
	else if (strcmp(ent->classname, "item_flag_team2") == 0) {
		CTFResetFlag(CTF_TEAM2);
		bprint_botsafe(PRINT_HIGH, "The %s flag has returned!\n",
			CTFTeamName(CTF_TEAM2));
	}
}

// Called from PlayerDie, to drop the flag from a dying player
void CTFDeadDropFlag(edict_t* self)
{
	edict_t* dropped = NULL;

	if (!it_flag_red || !it_flag_blue)
		CTFInit();

	if (self->client->pers.inventory[ITEM_INDEX(it_flag_red)]) /* MrG{DRGN}*/
	{
		dropped = Drop_Item(self, it_flag_red);
		self->client->pers.inventory[ITEM_INDEX(it_flag_red)] = 0;/* MrG{DRGN}*/
		bprint_botsafe(PRINT_HIGH, "%s lost the %s flag!\n",
			self->client->pers.netname, CTFTeamName(CTF_TEAM1));
	}
	else if (self->client->pers.inventory[ITEM_INDEX(it_flag_blue)])/* MrG{DRGN}*/
	{
		dropped = Drop_Item(self, it_flag_blue);
		self->client->pers.inventory[ITEM_INDEX(it_flag_blue)] = 0;/* MrG{DRGN}*/
		bprint_botsafe(PRINT_HIGH, "%s lost the %s flag!\n",
			self->client->pers.netname, CTFTeamName(CTF_TEAM2));
	}

	if (dropped) {
		dropped->think = CTFDropFlagThink;
		dropped->nextthink = level.time + CTF_AUTO_FLAG_RETURN_TIMEOUT;
		dropped->touch = CTFDropFlagTouch;
	}
}

void CTFDrop_Flag(edict_t* ent, gitem_t* item)
{	  	
	if (!allow_flagdrop->value)
	{
		if (rand() & 1)																 
			cprint_botsafe(ent, PRINT_HIGH, "Only lusers drop flags.\n");
		else
			cprint_botsafe(ent, PRINT_HIGH, "Winners don't drop flags.\n");
		return;
	}
	else
	{
		edict_t* dropped = NULL;

		if (!it_flag_red || !it_flag_blue)
			CTFInit();
		if (ent->client->pers.inventory[ITEM_INDEX(it_flag_red)]) /* MrG{DRGN}*/
		{
			dropped = Drop_Item(ent, it_flag_red);
			ent->client->pers.inventory[ITEM_INDEX(it_flag_red)] = 0;/* MrG{DRGN}*/
			bprint_botsafe(PRINT_HIGH, "%s dropped the %s flag!\n",
				ent->client->pers.netname, CTFTeamName(CTF_TEAM1));
		}
		else if (ent->client->pers.inventory[ITEM_INDEX(it_flag_blue)])/* MrG{DRGN}*/
		{
			dropped = Drop_Item(ent, it_flag_blue);
			ent->client->pers.inventory[ITEM_INDEX(it_flag_blue)] = 0;/* MrG{DRGN}*/
			bprint_botsafe(PRINT_HIGH, "%s dropped the %s flag!\n",
				ent->client->pers.netname, CTFTeamName(CTF_TEAM2));
		}

		if (dropped) {
			dropped->think = CTFDropFlagThink;
			dropped->nextthink = level.time + CTF_AUTO_FLAG_RETURN_TIMEOUT;
			dropped->touch = CTFDropFlagTouch;
		}
	}
}

static void CTFFlagThink(edict_t* ent)
{
	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextthink = level.time + FRAMETIME;
}

void CTFFlagSetup(edict_t* ent)
{
	trace_t		tr;
	vec3_t		dest = { 0 };
	float* v;

	v = tv(-15, -15, -15);
	VectorCopy(v, ent->mins);
	v = tv(15, 15, 15);
	VectorCopy(v, ent->maxs);

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else
		gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	v = tv(0, 0, -128);
	VectorAdd(ent->s.origin, v, dest);

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf("CTFFlagSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	VectorCopy(tr.endpos, ent->s.origin);

	gi.linkentity(ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = CTFFlagThink;
}

void CTFEffects(edict_t* player)
{
	player->s.effects &= (EF_FLAG1 | EF_FLAG2);
	if (player->health > 0) {
		if (player->client->pers.inventory[ITEM_INDEX(it_flag_red)]) /* MrG{DRGN}*/
		{
			player->s.effects |= EF_FLAG1;
		}
		if (player->client->pers.inventory[ITEM_INDEX(it_flag_blue)]) /* MrG{DRGN}*/
		{
			player->s.effects |= EF_FLAG2;
		}
	}

	if (player->client->pers.inventory[ITEM_INDEX(it_flag_red)])
		player->s.modelindex3 = gi.modelindex("players/male/flag1.md2");
	else if (player->client->pers.inventory[ITEM_INDEX(it_flag_blue)])
		player->s.modelindex3 = gi.modelindex("players/male/flag2.md2");
	else
		player->s.modelindex3 = REMOVED_MODEL;
}

// called when we enter the intermission
void CTFCalcScores(void)
{
	int i;

	ctfgame.total1 = ctfgame.total2 = 0;
	for (i = 0; i < maxclients->value; i++) {
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			ctfgame.total1 += game.clients[i].resp.score;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			ctfgame.total2 += game.clients[i].resp.score;
	}
}

void CTFID_f(edict_t* ent)
{
	if (ent->client->resp.id_state) {
		cprint_botsafe(ent, PRINT_HIGH, "Disabling player identication display.\n");
		ent->client->resp.id_state = false;
	}
	else {
		cprint_botsafe(ent, PRINT_HIGH, "Activating player identication display.\n");
		ent->client->resp.id_state = true;
	}
}

static void CTFSetIDView(edict_t* ent)
{
	vec3_t	forward, dir = { 0 };
	trace_t	tr;
	edict_t* who, * best;
	float	bd = 0, d;
	int i;

	ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorScale(forward, 1024, forward);
	VectorAdd(ent->s.origin, forward, forward);
	tr = gi.trace(ent->s.origin, NULL, NULL, forward, ent, MASK_SOLID);
	if (tr.fraction < 1 && tr.ent && tr.ent->client) {
		ent->client->ps.stats[STAT_CTF_ID_VIEW] =
			CS_PLAYERSKINS + (ent - g_edicts - 1);
		return;
	}

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	best = NULL;
	for (i = 1; i <= maxclients->value; i++) {
		who = g_edicts + i;
		if (!who->inuse)
			continue;
		VectorSubtract(who->s.origin, ent->s.origin, dir);
		VectorNormalize(dir);
		d = DotProduct(forward, dir);
		if (d > bd && loc_CanSee(ent, who)) {
			bd = d;
			best = who;
		}
	}
	if (bd > 0.90)
		ent->client->ps.stats[STAT_CTF_ID_VIEW] =
		CS_PLAYERSKINS + (best - g_edicts - 1);
}

void SetCTFStats(edict_t* ent)
{
	gitem_t* tech;
	int i;
	int p1, p2;
	edict_t* e;

	// logo headers for the frag display
	ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = gi.imageindex("ctfsb1");
	ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = gi.imageindex("ctfsb2");

	// if during intermission, we must blink the team header of the winning team
	if (level.intermissiontime && (level.framenum & 8)) { // blink 1/8th second
		// note that ctfgame.total[12] is set when we go to intermission
		if (ctfgame.team1 > ctfgame.team2)
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
		else if (ctfgame.team2 > ctfgame.team1)
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		else if (ctfgame.total1 > ctfgame.total2) // frag tie breaker
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
		else if (ctfgame.total2 > ctfgame.total1)
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		else { // tie game!
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		}
	}

	// tech icon
	i = 0;
	ent->client->ps.stats[STAT_CTF_TECH] = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			ent->client->pers.inventory[ITEM_INDEX(tech)]) {
			ent->client->ps.stats[STAT_CTF_TECH] = gi.imageindex(tech->icon);
			break;
		}
		i++;
	}

	// figure out what icon to display for team logos
	// three states:
	//   flag at base
	//   flag taken
	//   flag dropped
	p1 = gi.imageindex("i_ctf1");
	e = G_Find(NULL, FOFS(classname), "item_flag_team1");
	if (e != NULL) {
		if (e->solid == SOLID_NOT) {
			//int i;

			// not at base
			// check if on player
			p1 = gi.imageindex("i_ctf1d"); // default to dropped
			for (i = 1; i <= maxclients->value; i++)
				if (g_edicts[i].inuse &&
					g_edicts[i].client->pers.inventory[ITEM_INDEX(it_flag_red)]) {
					// enemy has it
					p1 = gi.imageindex("i_ctf1t");
					break;
				}
		}
		else if (e->spawnflags & DROPPED_ITEM)
			p1 = gi.imageindex("i_ctf1d"); // must be dropped
	}
	p2 = gi.imageindex("i_ctf2");
	e = G_Find(NULL, FOFS(classname), "item_flag_team2");
	if (e != NULL) {
		if (e->solid == SOLID_NOT) {
			//int i;

			// not at base
			// check if on player
			p2 = gi.imageindex("i_ctf2d"); // default to dropped
			for (i = 1; i <= maxclients->value; i++)
				if (g_edicts[i].inuse &&
					g_edicts[i].client->pers.inventory[ITEM_INDEX(it_flag_blue)]) {
					// enemy has it
					p2 = gi.imageindex("i_ctf2t");
					break;
				}
		}
		else if (e->spawnflags & DROPPED_ITEM)
			p2 = gi.imageindex("i_ctf2d"); // must be dropped
	}

	ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = p1;
	ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = p2;

	if (ctfgame.last_flag_capture && level.time - ctfgame.last_flag_capture < 5) {
		if (ctfgame.last_capture_team == CTF_TEAM1)
			if (level.framenum & 8)
				ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = p1;
			else
				ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = 0;
		else
			if (level.framenum & 8)
				ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = p2;
			else
				ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = 0;
	}

	ent->client->ps.stats[STAT_CTF_TEAM1_CAPS] = ctfgame.team1;
	ent->client->ps.stats[STAT_CTF_TEAM2_CAPS] = ctfgame.team2;

	ent->client->ps.stats[STAT_CTF_FLAG_PIC] = 0;
	if (ent->client->resp.ctf_team == CTF_TEAM1 &&
		ent->client->pers.inventory[ITEM_INDEX(it_flag_blue)] &&
		(level.framenum & 8))
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = gi.imageindex("i_ctf2");

	else if (ent->client->resp.ctf_team == CTF_TEAM2 &&
		ent->client->pers.inventory[ITEM_INDEX(it_flag_red)] &&
		(level.framenum & 8))
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = gi.imageindex("i_ctf1");

	ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC] = 0;
	ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC] = 0;
	if (ent->client->resp.ctf_team == CTF_TEAM1)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC] = gi.imageindex("i_ctfj");
	else if (ent->client->resp.ctf_team == CTF_TEAM2)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC] = gi.imageindex("i_ctfj");

	ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;
	if (ent->client->resp.id_state)
		CTFSetIDView(ent);
}

/*------------------------------------------------------------------------*/

/*QUAKED info_player_team1 (1 0 0) (-16 -16 -24) (16 16 32)
potential team1 spawning position for ctf games
*/
void SP_info_player_team1(edict_t* self)
{
}

/*QUAKED info_player_team2 (0 0 1) (-16 -16 -24) (16 16 32)
potential team2 spawning position for ctf games
*/
void SP_info_player_team2(edict_t* self)
{
}

void CTFTeam_f(edict_t* ent)
{
	char* t, * s;
	int desired_team;

	t = gi.args();
	if (!*t) {
		cprint_botsafe(ent, PRINT_HIGH, "You are on the %s team.\n",
			CTFTeamName(ent->client->resp.ctf_team));
		return;
	}
	if (Q_stricmp(t, "red") == 0)
		desired_team = CTF_TEAM1;
	else if (Q_stricmp(t, "blue") == 0)
		desired_team = CTF_TEAM2;
	else {
		cprint_botsafe(ent, PRINT_HIGH, "Unknown team %s.\n", t);
		return;
	}

	if (ent->client->resp.ctf_team == desired_team) {
		cprint_botsafe(ent, PRINT_HIGH, "You are already on the %s team.\n",
			CTFTeamName(ent->client->resp.ctf_team));
		return;
	}

	////
	ent->svflags = 0;
	ent->flags &= ~FL_GODMODE;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	// Log Team Change - Mark Davies
	sl_LogPlayerTeamChange(&gi,
		ent->client->pers.netname,
		CTFTeamName(ent->client->resp.ctf_team),
		level.time);

	if (ent->solid == SOLID_NOT) { // spectator
		PutClientInServer(ent);
		// add a teleportation effect
		ent->s.event = EV_PLAYER_TELEPORT;
		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
		bprint_botsafe(PRINT_HIGH, "%s joined the %s team.\n",
			ent->client->pers.netname, CTFTeamName(desired_team));
		return;
	}

	ent->health = 0;
	player_die(ent, ent, ent, 100000, vec3_origin);
	// don't even bother waiting for death frames
	ent->deadflag = DEAD_DEAD;
	respawn(ent);

	ent->client->resp.score = 0;

	bprint_botsafe(PRINT_HIGH, "%s changed to the %s team.\n",
		ent->client->pers.netname, CTFTeamName(desired_team));
}

/*
==================
CTFScoreboardMessage
==================
*/
void CTFScoreboardMessage(edict_t* ent, edict_t* killer)
{
	char	entry[1024] = { 0 };
	char	string[1400] = { 0 };
	size_t	len;/* MrG{DRGN} changed to fix conversion from 'size_t' to 'unsigned int', possible loss of data */
	int		i, j, k, n;
	int		sorted[2][MAX_CLIENTS] = { 0 };
	int		sortedscores[2][MAX_CLIENTS] = { 0 };
	int		score, total[2] = { 0 }, totalscore[2] = { 0 };
	int		last[2] = { 0 };
	gclient_t* cl;
	edict_t* cl_ent;
	int team;
	int maxsize = 1400;

	// sort the clients by team and score
	total[0] = total[1] = 0;
	last[0] = last[1] = 0;
	totalscore[0] = totalscore[1] = 0;
	for (i = 0; i < game.maxclients; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			team = 0;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			team = 1;
		else
			continue; // unknown team?

		score = game.clients[i].resp.score;
		for (j = 0; j < total[team]; j++)
		{
			if (score > sortedscores[team][j])
				break;
		}
		for (k = total[team]; k > j; k--)
		{
			sorted[team][k] = sorted[team][k - 1];
			sortedscores[team][k] = sortedscores[team][k - 1];
		}
		sorted[team][j] = i;
		sortedscores[team][j] = score;
		totalscore[team] += score;
		total[team]++;
	}

	// print level name and exit rules
	// add the clients in sorted order
	*string = 0;
	len = 0;

	// team one
	sprintf(string, "if 24 xv 8 yv 8 pic 24 endif "
		"xv 40 yv 28 string \"%4d/%-3d\" "
		"xv 98 yv 12 num 2 18 "
		"if 25 xv 168 yv 8 pic 25 endif "
		"xv 200 yv 28 string \"%4d/%-3d\" "
		"xv 256 yv 12 num 2 20 ",
		totalscore[0], total[0],
		totalscore[1], total[1]);
	len = strlen(string);

	for (i = 0; i < 16; i++)
	{
		if (i >= total[0] && i >= total[1])
			break; // we're done

#if 0 //ndef NEW_SCORE
		// set up y
		sprintf(entry, "yv %d ", 42 + i * 8);
		if (maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
#else
		* entry = 0;
#endif

		// left side
		if (i < total[0]) {
			cl = &game.clients[sorted[0][i]];
			cl_ent = g_edicts + 1 + sorted[0][i];

#if 0 //ndef NEW_SCORE
			sprintf(entry + strlen(entry),
				"xv 0 %s \"%3d %3d %-12.12s\" ",
				(cl_ent == ent) ? "string2" : "string",
				cl->resp.score,
				(cl->ping > 999) ? 999 : cl->ping,
				cl->pers.netname);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(it_flag_blue)])
				strcat(entry, "xv 56 picn sbfctf2 ");
#else
			sprintf(entry + strlen(entry),
				"ctf 0 %d %d %d %d ",
				42 + i * 8,
				sorted[0][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(it_flag_blue)])
				sprintf(entry + strlen(entry), "xv 56 yv %d picn sbfctf2 ",
					42 + i * 8);
#endif

			if (maxsize - len > strlen(entry)) {				
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
				last[0] = i;
			}
		}

		// right side
		if (i < total[1]) {
			cl = &game.clients[sorted[1][i]];
			cl_ent = g_edicts + 1 + sorted[1][i];

#if 0 //ndef NEW_SCORE
			sprintf(entry + strlen(entry),
				"xv 160 %s \"%3d %3d %-12.12s\" ",
				(cl_ent == ent) ? "string2" : "string",
				cl->resp.score,
				(cl->ping > 999) ? 999 : cl->ping,
				cl->pers.netname);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(it_flag_red)])
				strcat(entry, "xv 216 picn sbfctf1 ");

#else

			sprintf(entry + strlen(entry),
				"ctf 160 %d %d %d %d ",
				42 + i * 8,
				sorted[1][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(it_flag_red)])
				sprintf(entry + strlen(entry), "xv 216 yv %d picn sbfctf1 ",
					42 + i * 8);
#endif
			if (maxsize - len > strlen(entry)) {				
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
				last[1] = i;
			}
		}
	}

	// put in spectators if we have enough room
	if (last[0] > last[1])
		j = last[0];
	else
		j = last[1];
	j = (j + 2) * 8 + 42;

	k = n = 0;
	if (maxsize - len > 50) {
		for (i = 0; i < maxclients->value; i++) {
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			if (!cl_ent->inuse ||
				(cl_ent->solid != SOLID_TRIGGER && cl_ent->solid != SOLID_NOT )||
				cl_ent->client->resp.ctf_team != CTF_NOTEAM)
				continue;

			if (!k) {
				k = 1;
				sprintf(entry, "xv 0 yv %d string2 \"Spectators\" ", j);				
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
				j += 8;
			}

			sprintf(entry + strlen(entry),
				"ctf %d %d %d %d %d ",
				(n & 1) ? 160 : 0, // x
				j, // y
				i, // playernum
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);
			if (maxsize - len > strlen(entry)) {  				
				Com_strcat(string, sizeof(string), entry);
				len = strlen(string);
			}

			if (n & 1)
				j += 8;
			n++;
		}
	}

	if (total[0] - last[0] > 1) // couldn't fit everyone
		sprintf(string + strlen(string), "xv 8 yv %d string \"..and %d more\" ",
			42 + (last[0] + 1) * 8, total[0] - last[0] - 1);
	if (total[1] - last[1] > 1) // couldn't fit everyone
		sprintf(string + strlen(string), "xv 168 yv %d string \"..and %d more\" ",
			42 + (last[1] + 1) * 8, total[1] - last[1] - 1);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
}

/*------------------------------------------------------------------------*/
/* TECH																	  */
/*------------------------------------------------------------------------*/

/*/
void CTFHasTech(edict_t* who)
{
	if (level.time - who->client->ctf_lasttechmsg > 2){
		if (!strcmp(who->classname, "bot") == 0)
		{
			gi.centerprintf(who, "You already have a TECH powerup.");
			who->client->ctf_lasttechmsg = level.time;
		}
	}
}
*/
/* MrG{DRGN} if it's a bot abort imediately.*/
void CTFHasTech(edict_t* who)
{
	if (who->bot_player)
		return;

	if (level.time - who->client->ctf_lasttechmsg > 2) {
		gi.centerprintf(who, "You already have a TECH powerup.");
		who->client->ctf_lasttechmsg = level.time;
	}
}

gitem_t* CTFWhat_Tech(edict_t* ent)
{
	gitem_t* tech;
	int i;

	i = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			ent->client->pers.inventory[ITEM_INDEX(tech)]) {
			return tech;
		}
		i++;
	}
	return NULL;
}

qboolean CTFPickup_Tech(edict_t* ent, edict_t* other)
{
	gitem_t* tech;
	int i;

	i = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			other->client->pers.inventory[ITEM_INDEX(tech)]) {
			CTFHasTech(other);
			return false; // has this one
		}
		i++;
	}

	// client only gets one tech
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	other->client->ctf_regentime = level.time;
	RemoveFromList(ent);
	return true;
}

static void SpawnTech(gitem_t* item, edict_t* spot);

//static edict_t* FindTechSpawn(void)
//{
//	edict_t* spot = NULL;
//	int i = rand() % 16;

//	while (i--)
//		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
//	if (!spot)
//		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
//	return spot;
//}
/*
MrG{DRGN} - A better way to pick a random spawnpoint than FindTechSpawn
*/
static edict_t* FindTechDest(void)
{
	edict_t* spot;
	int        count = 0;
	int        selection;

	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
		count++;

	if (!count)
		return NULL;

	selection = rand() % count;
	spot = NULL;

	do
	{
		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
	} while (selection--);

	return spot;
}


static void TechThink(edict_t* tech)
{
	CTFTech_teleport(tech);
	tech->nextthink = level.time + CTF_TECH_TIMEOUT;
	tech->think = TechThink;
}

void CTFDrop_Tech(edict_t* ent, gitem_t* item)
{
	edict_t* tech;

	tech = Drop_Item(ent, item);
	AddItemToList(tech);
	tech->nextthink = level.time + CTF_TECH_TIMEOUT;
	tech->think = TechThink;
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
}

void CTFDeadDropTech(edict_t* ent)
{
	gitem_t* tech;
	edict_t* dropped;
	int i;

	i = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			ent->client->pers.inventory[ITEM_INDEX(tech)]) {
			dropped = Drop_Item(ent, tech);
			AddItemToList(dropped);
			// hack the velocity to make it bounce random
			dropped->velocity[0] = (rand() % 600) - 300;
			dropped->velocity[1] = (rand() % 600) - 300;
			dropped->nextthink = level.time + CTF_TECH_TIMEOUT;
			dropped->think = TechThink;
			dropped->owner = NULL;
			ent->client->pers.inventory[ITEM_INDEX(tech)] = 0;
		}
		i++;
	}
}

static void SpawnTech(gitem_t* item, edict_t* spot)
{
	edict_t* ent;
	vec3_t	forward, right;
	vec3_t  angles = { 0 };

	ent = G_Spawn();

	ent->classname = item->classname;
	ent->classindex = item->classindex;
	ent->item = item;
	ent->spawnflags = DROPPED_ITEM;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	VectorSet(ent->mins, -15, -15, -15);
	VectorSet(ent->maxs, 15, 15, 15);
	gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->owner = ent;

	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;

	AngleVectors(angles, forward, right, NULL);
	VectorCopy(spot->s.origin, ent->s.origin);
	ent->s.origin[2] += 16;
	VectorScale(forward, 100, ent->velocity);
	ent->velocity[2] = 300;

	ent->nextthink = level.time + CTF_TECH_TIMEOUT;
	ent->think = TechThink;

	gi.linkentity(ent);

	DbgPrintf("%s %s index: %d\n", __func__, ent->classname, ent->classindex);

	//HAVOC
	AddItemToList(ent);
}

static void SpawnTechs(edict_t* ent)
{
	gitem_t* tech;
	edict_t* spot;
	int i;

	i = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			(spot = FindTechDest()) != NULL)
			SpawnTech(tech, spot);
		i++;
	}
	if (ent)
		G_FreeEdict(ent);
}

// frees the passed edict!
void CTFRespawnTech(edict_t* ent)
{
	edict_t* spot;

	if ((spot = FindTechDest()) != NULL)
		SpawnTech(ent->item, spot);
	DbgPrintf("%s %s %d\n", __func__, ent->classname, ent->classindex);
	G_FreeEdict(ent);
}

void CTFSetupTechSpawn(void)
{
	edict_t* ent;

	if (((int)dmflags->value & DF_CTF_NO_TECH))
		return;

	ent = G_Spawn();
	ent->nextthink = level.time + 2;
	ent->think = SpawnTechs;
}

void CTFResetTech(void)
{
	edict_t* ent;
	int i;

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++) {
		if (ent->inuse)
			if (ent->item && (ent->item->flags & IT_TECH))
				G_FreeEdict(ent);
	}
	SpawnTechs(NULL);
}

/*
* 	 MrG{DRGN} working out a function for the techs to travel, vs be freed.
*/
void CTFTech_teleport( edict_t* tech)
{
	edict_t* spot;
	vec3_t	forward, right;
	vec3_t  angles = { 0 };


	/* END */
	if ((spot = FindTechDest()) == NULL)
		return;

	gi.unlinkentity(tech);

	VectorCopy(spot->s.origin, tech->s.origin);
	VectorCopy(spot->s.origin, tech->s.old_origin);

	tech->spawnflags = DROPPED_ITEM;
	tech->s.renderfx = RF_GLOW;
	VectorSet(tech->mins, -15, -15, -15);
	VectorSet(tech->maxs, 15, 15, 15);
	gi.setmodel(tech, tech->item->world_model);
	tech->solid = SOLID_TRIGGER;
	tech->movetype = MOVETYPE_TOSS;
	tech->touch = Touch_Item;
	tech->owner = tech;

	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;

	AngleVectors(angles, forward, right, NULL);
	VectorCopy(spot->s.origin, tech->s.origin);
	tech->s.origin[2] += 16;

	tech->s.event = EV_ITEM_RESPAWN;
	VectorScale(forward, 100, tech->velocity);
	tech->velocity[2] = 300;

	tech->nextthink = level.time + CTF_TECH_TIMEOUT;
	tech->think = TechThink;

	gi.linkentity(tech);
}




int CTFApplyResistance(edict_t* ent, int dmg)
{
	static gitem_t* tech = NULL;
	float volume = 1.0F; /* MrG{DRGN} Explicit float. */

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2F; /* MrG{DRGN} added  F, as this was causing truncation from double to float. */

	if (!tech)
		tech = (it_tech1);
	if (dmg && tech && ent->client && ent->client->pers.inventory[ITEM_INDEX(tech)]) {
		// make noise
		gi.sound(ent, CHAN_VOICE, gi.soundindex("ctf/tech1.wav"), volume, ATTN_NORM, 0);
		return dmg / 2;
	}
	return dmg;
}

int CTFApplyStrength(edict_t* ent, int dmg)
{
	static gitem_t* tech = NULL;

	if (!tech)
		tech = it_tech2;
	if (dmg && tech && ent && ent->client && ent->client->pers.inventory[ITEM_INDEX(tech)]) {
		return dmg * 2;
	}
	return dmg;
}

qboolean CTFApplyStrengthSound(edict_t* ent)
{
	static gitem_t* tech = NULL;
	float volume = 1.0F; /* MrG{DRGN} Explicit float. */

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2F; /* MrG{DRGN} added  F, as this was causing truncation from double to float. */

	if (!tech)
		tech = it_tech2;
	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)]) {
		if (ent->client->ctf_techsndtime < level.time) {
			ent->client->ctf_techsndtime = level.time + 1;
			if (ent->client->quad_framenum > level.framenum)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("ctf/tech2x.wav"), volume, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("ctf/tech2.wav"), volume, ATTN_NORM, 0);
		}
		return true;
	}
	return false;
}

qboolean CTFApplyHaste(edict_t* ent)
{
	static gitem_t* tech = NULL;

	if (!tech)
		tech = it_tech3;
	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
		return true;
	return false;
}

void CTFApplyHasteSound(edict_t* ent)
{
	static gitem_t* tech = NULL;
	float volume = 1.0F; /* MrG{DRGN} Explicit float. */

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2F; /* MrG{DRGN} Explicit float. */

	if (!tech)
		tech = it_tech3;
	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)] &&
		ent->client->ctf_techsndtime < level.time) {
		ent->client->ctf_techsndtime = level.time + 1;
		gi.sound(ent, CHAN_VOICE, gi.soundindex("ctf/tech3.wav"), volume, ATTN_NORM, 0);
	}
}

void CTFApplyRegeneration(edict_t* ent)
{
	static gitem_t* tech = NULL;
	qboolean noise = false;
	gclient_t* client;
	float volume = 1.0;

	client = ent->client;
	if (!client)
		return;

	if (ent->client->silencer_shots)
		volume = 0.2F; /* MrG{DRGN} Explicit float */

	if (!tech)
		tech = it_tech4;
	if (tech && client->pers.inventory[ITEM_INDEX(tech)]) {
		if (client->ctf_regentime < level.time) {
			client->ctf_regentime = level.time;
			if (ent->health < 150) {
				ent->health += 5;
				if (ent->health > 150)
					ent->health = 150;
				client->ctf_regentime += 0.5F;/* MrG{DRGN} explicit float */
				noise = true;
			}
		}
		if (noise && ent->client->ctf_techsndtime < level.time) {
			ent->client->ctf_techsndtime = level.time + 1;
			gi.sound(ent, CHAN_VOICE, gi.soundindex("ctf/tech4.wav"), volume, ATTN_NORM, 0);
		}
	}
}

qboolean CTFHasRegeneration(edict_t* ent)
{
	static gitem_t* tech = NULL;

	if (!tech)
		tech = it_tech4;
	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
		return true;
	return false;
}

/*
======================================================================

SAY_TEAM

======================================================================
*/

// This array is in 'importance order', it indicates what items are
// more important when reporting their names.
struct {
	char* classname;
	int priority;
} loc_names[] =
{
	{	"item_flag_team1",			1 },
	{	"item_flag_team2",			1 },
	{	"item_quad",				2 },
	{	"item_invulnerability",		2 },
	{	"weapon_bfg",				3 },
	{	"weapon_railgun",			4 },
	{	"weapon_rocketlauncher",	4 },
	{	"weapon_hyperblaster",		4 },
	{	"weapon_chaingun",			4 },
	{	"weapon_grenadelauncher",	4 },
	{	"weapon_machinegun",		4 },
	{	"weapon_supershotgun",		4 },
	{	"weapon_shotgun",			4 },
	{	"item_power_screen",		5 },
	{	"item_power_shield",		5 },
	{	"item_armor_body",			6 },
	{	"item_armor_combat",		6 },
	{	"item_armor_jacket",		6 },
	{	"item_silencer",			7 },
	{	"item_breather",			7 },
	{	"item_enviro",				7 },
	{	"item_adrenaline",			7 },
	{	"item_bandolier",			8 },
	{	"item_pack",				8 },
	{ NULL, 0 }
};


static void CTFSay_Team_Location(edict_t* who, char* buf)
{
	edict_t* what = NULL;
	edict_t* hot = NULL;
	float hotdist = 999999, newdist;
	vec3_t v = { 0 };
	int hotindex = 999;
	int i;
	gitem_t* item;
	int nearteam = -1;
	edict_t* flag1, * flag2;
	qboolean hotsee = false;
	qboolean cansee;

	while ((what = loc_findradius(what, who->s.origin, 1024)) != NULL) {
		// find what in loc_classnames
		for (i = 0; loc_names[i].classname; i++)
			if (strcmp(what->classname, loc_names[i].classname) == 0)
				break;
		if (!loc_names[i].classname)
			continue;
		// something we can see get priority over something we can't
		cansee = loc_CanSee(what, who);
		if (cansee && !hotsee) {
			hotsee = true;
			hotindex = loc_names[i].priority;
			hot = what;
			VectorSubtract(what->s.origin, who->s.origin, v);
			hotdist = VectorLength(v);
			continue;
		}
		// if we can't see this, but we have something we can see, skip it
		if (hotsee && !cansee)
			continue;
		if (hotsee && hotindex < loc_names[i].priority)
			continue;
		VectorSubtract(what->s.origin, who->s.origin, v);
		newdist = VectorLength(v);
		if (newdist < hotdist ||
			(cansee && loc_names[i].priority < hotindex)) {
			hot = what;
			hotdist = newdist;
			hotindex = i;
			hotsee = loc_CanSee(hot, who);
		}
	}

	if (!hot) {
		strcpy(buf, "nowhere");
		return;
	}

	// we now have the closest item
	// see if there's more than one in the map, if so
	// we need to determine what team is closest
	what = NULL;
	while ((what = G_Find(what, FOFS(classname), hot->classname)) != NULL) {
		if (what == hot)
			continue;
		// if we are here, there is more than one, find out if hot
		// is closer to red flag or blue flag
		if ((flag1 = G_Find(NULL, FOFS(classname), "item_flag_team1")) != NULL &&
			(flag2 = G_Find(NULL, FOFS(classname), "item_flag_team2")) != NULL) {
			VectorSubtract(hot->s.origin, flag1->s.origin, v);
			hotdist = VectorLength(v);
			VectorSubtract(hot->s.origin, flag2->s.origin, v);
			newdist = VectorLength(v);
			if (hotdist < newdist)
				nearteam = CTF_TEAM1;
			else if (hotdist > newdist)
				nearteam = CTF_TEAM2;
		}
		break;
	}

	if ((item = FindItemByClassname(hot->classname)) == NULL) {
		strcpy(buf, "nowhere");
		return;
	}

	// in water?
	if (who->waterlevel)
		strcpy(buf, "in the water ");
	else
		*buf = 0;

	// near or above
	VectorSubtract(who->s.origin, hot->s.origin, v);
	if (fabs(v[2]) > fabs(v[0]) && fabs(v[2]) > fabs(v[1]))
		if (v[2] > 0)
			strcat(buf, "above ");
		else
			strcat(buf, "below ");
	else
		strcat(buf, "near ");

	if (nearteam == CTF_TEAM1)
		strcat(buf, "the red ");
	else if (nearteam == CTF_TEAM2)
		strcat(buf, "the blue ");
	else
		strcat(buf, "the ");

	strcat(buf, item->pickup_name);
}

static void CTFSay_Team_Armor(edict_t* who, char* buf)
{
	gitem_t* item;
	int			index, cells;
	int			power_armor_type;

	*buf = 0;

	power_armor_type = PowerArmorType(who);
	if (power_armor_type)
	{
		cells = who->client->pers.inventory[ITEM_INDEX(FindItem("cells"))];
		if (cells)
			sprintf(buf + strlen(buf), "%s with %i cells ",
				(power_armor_type == POWER_ARMOR_SCREEN) ?
				"Power Screen" : "Power Shield", cells);
	}

	index = ArmorIndex(who);
	if (index)
	{
		item = GetItemByIndex(index);
		if (item) {
			if (*buf)
				strcat(buf, "and ");
			sprintf(buf + strlen(buf), "%i units of %s",
				who->client->pers.inventory[index], item->pickup_name);
		}
	}

	if (!*buf)
		strcpy(buf, "no armor");
}

static void CTFSay_Team_Health(edict_t* who, char* buf)
{
	if (who->health <= 0)
		strcpy(buf, "dead");
	else
		sprintf(buf, "%i health", who->health);
}

static void CTFSay_Team_Tech(edict_t* who, char* buf)
{
	gitem_t* tech;
	int i;

	// see if the player has a tech powerup
	i = 0;
	while (tnames[i]) {
		if ((tech = FindItemByClassname(tnames[i])) != NULL &&
			who->client->pers.inventory[ITEM_INDEX(tech)]) {
			sprintf(buf, "the %s", tech->pickup_name);
			return;
		}
		i++;
	}
	strcpy(buf, "no powerup");
}

static void CTFSay_Team_Weapon(edict_t* who, char* buf)
{
	if (who->client->pers.weapon)
		strcpy(buf, who->client->pers.weapon->pickup_name);
	else
		strcpy(buf, "none");
}

static void CTFSay_Team_Sight(edict_t* who, char* buf)
{
	int i;
	edict_t* targ;
	int n = 0;
	char s[1024] = { 0 };
	char s2[1024] = { 0 };

	*s = *s2 = 0;
	for (i = 1; i <= maxclients->value; i++) {
		targ = g_edicts + i;
		if (!targ->inuse ||
			targ == who ||
			!loc_CanSee(targ, who))
			continue;
		if (*s2) {
			if (strlen(s) + strlen(s2) + 3 < sizeof(s)) {
				if (n)
					strcat(s, ", ");
				strcat(s, s2);
				*s2 = 0;
			}
			n++;
		}
		strcpy(s2, targ->client->pers.netname);
	}
	if (*s2) {
		if (strlen(s) + strlen(s2) + 6 < sizeof(s)) {
			if (n)
				strcat(s, " and ");
			strcat(s, s2);
		}
		strcpy(buf, s);
	}
	else
		strcpy(buf, "no one");
}

void CTFSay_Team(edict_t* who, char* msg)
{
	char outmsg[256];
	char buf[1024];
	int i;
	char* p;
	edict_t* cl_ent;

	if (CheckFlood(who))
		return;

	outmsg[0] = 0;

	if (*msg == '\"') {
		msg[strlen(msg) - 1] = 0;
		msg++;
	}

	for (p = outmsg; *msg && (p - outmsg) < sizeof(outmsg) - 2; msg++) {
		if (*msg == '%') {
			switch (toupper(*++msg))
			{
			case 'L':
				CTFSay_Team_Location(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;
			case 'A':
				CTFSay_Team_Armor(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;
			case 'H':
				CTFSay_Team_Health(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;
			case 'T':
				CTFSay_Team_Tech(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;
			case 'W':
				CTFSay_Team_Weapon(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;
			case 'N':
				CTFSay_Team_Sight(who, buf);
				if (strlen(buf) + (p - outmsg) < sizeof(outmsg) - 2)
				{
					strcpy(p, buf);
					p += strlen(buf);
				}
				break;

			default:
				*p++ = *msg;
			}
		}
		else
			*p++ = *msg;
	}
	*p = 0;

	if (strlen(outmsg) > 150)
		outmsg[150] = 0;

	for (i = 0; i < maxclients->value; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->client->resp.ctf_team == who->client->resp.ctf_team)
			cprint_botsafe(cl_ent, PRINT_CHAT, "(%s): %s\n",
				who->client->pers.netname, outmsg);
	}

}

/*-----------------------------------------------------------------------*/
/*QUAKED misc_ctf_banner (1 .5 0) (-4 -64 0) (4 64 248) TEAM2
The origin is the bottom of the banner.
The banner is 248 tall.
*/
static void misc_ctf_banner_think(edict_t* ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_ctf_banner(edict_t* ent)
{
	if (!ctf->value) /* MrG{DRGN} let's conserver some resources if possible */
	{
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/tris.md2");
	if (ent->spawnflags & 1) // team2
		ent->s.skinnum = 1;

	ent->classindex = MISC_CTF_BANNER; /* MrG{DRGN} */
	ent->s.frame = rand() % 16;
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_ctf_small_banner (1 .5 0) (-4 -32 0) (4 32 124) TEAM2
The origin is the bottom of the banner.
The banner is 124 tall.
*/
void SP_misc_ctf_small_banner(edict_t* ent)
{
	if (!ctf->value) /* MrG{DRGN} let's conserver some resources if possible */
	{
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/small.md2");
	if (ent->spawnflags & 1) // team2
		ent->s.skinnum = 1;

	ent->classindex = MISC_CTF_SMALL_BANNER; /* MrG{DRGN} */
	ent->s.frame = rand() % 16;
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*-----------------------------------------------------------------------*/


/* ELECTIONS */

qboolean CTFBeginElection(edict_t* ent, elect_t type, char* msg)
{
	int i;
	int count;
	edict_t* e;

	if (electpercentage->value == 0) {
		gi.cprintf(ent, PRINT_HIGH, "Elections are disabled, only an admin can process this action.\n");
		return false;
	}


	if (ctfgame.election != ELECT_NONE) {
		gi.cprintf(ent, PRINT_HIGH, "Election already in progress.\n");
		return false;
	}

	// clear votes
	count = 0;
	for (i = 1; i <= maxclients->value; i++) {
		e = g_edicts + i;
		e->client->resp.voted = false;
		if (e->inuse)
			count++;
	}

	if (count < 2) {
		gi.cprintf(ent, PRINT_HIGH, "Not enough players for election.\n");
		return false;
	}

	ctfgame.etarget = ent;
	ctfgame.election = type;
	ctfgame.evotes = 0;
	ctfgame.needvotes = (count * electpercentage->value) / 100;
	ctfgame.electtime = level.time + 20; // twenty seconds for election
	strncpy(ctfgame.emsg, msg, sizeof(ctfgame.emsg) - 1);

	// tell everyone
	gi.bprintf(PRINT_CHAT, "%s\n", ctfgame.emsg);
	gi.bprintf(PRINT_HIGH, "Type YES or NO to vote on this request.\n");
	gi.bprintf(PRINT_HIGH, "Votes: %d  Needed: %d  Time left: %ds\n", ctfgame.evotes, ctfgame.needvotes,
		(int)(ctfgame.electtime - level.time));

	return true;
}

void DoRespawn(edict_t* ent);

void CTFResetAllPlayers(void)
{
	int i;
	edict_t* ent;

	for (i = 1; i <= maxclients->value; i++) {
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		if (ent->client->menu)
			PMenu_Close(ent);

		//CTFPlayerResetGrapple(ent);
		CTFDeadDropFlag(ent);
		CTFDeadDropTech(ent);

		ent->client->resp.ctf_team = CTF_NOTEAM;
		ent->client->resp.ready = false;

		ent->svflags = 0;
		ent->flags &= ~FL_GODMODE;
		PutClientInServer(ent);
	}

	// reset the level
	CTFResetTech();
	CTFResetFlags();

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++) {
		if (ent->inuse && !ent->client) {
			if (ent->solid == SOLID_NOT && ent->think == DoRespawn &&
				ent->nextthink >= level.time) {
				ent->nextthink = 0;
				DoRespawn(ent);
			}
		}
	}
	if (ctfgame.match == MATCH_SETUP)
		ctfgame.matchtime = level.time + matchsetuptime->value * 60;
}

void CTFAssignGhost(edict_t* ent)
{
	int ghost, i;

	for (ghost = 0; ghost < MAX_CLIENTS; ghost++)
		if (!ctfgame.ghosts[ghost].code)
			break;
	if (ghost == MAX_CLIENTS)
		return;
	ctfgame.ghosts[ghost].team = ent->client->resp.ctf_team;
	ctfgame.ghosts[ghost].score = 0;
	for (;;) {
		ctfgame.ghosts[ghost].code = 10000 + (rand() % 90000);
		for (i = 0; i < MAX_CLIENTS; i++)
			if (i != ghost && ctfgame.ghosts[i].code == ctfgame.ghosts[ghost].code)
				break;
		if (i == MAX_CLIENTS)
			break;
	}
	ctfgame.ghosts[ghost].ent = ent;
	strcpy(ctfgame.ghosts[ghost].netname, ent->client->pers.netname);
	ent->client->resp.ghost = ctfgame.ghosts + ghost;
	gi.cprintf(ent, PRINT_CHAT, "Your ghost code is **** %d ****\n", ctfgame.ghosts[ghost].code);
	gi.cprintf(ent, PRINT_HIGH, "If you lose connection, you can rejoin with your score "
		"intact by typing \"ghost %d\".\n", ctfgame.ghosts[ghost].code);
}

// start a match
void CTFStartMatch(void)
{
	int i;
	edict_t* ent;
	int ghost = 0;

	ctfgame.match = MATCH_GAME;
	ctfgame.matchtime = level.time + matchtime->value * 60;
	ctfgame.countdown = false;

	ctfgame.team1 = ctfgame.team2 = 0;

	memset(ctfgame.ghosts, 0, sizeof(ctfgame.ghosts));

	for (i = 1; i <= maxclients->value; i++) {
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		ent->client->resp.score = 0;
		ent->client->resp.ctf_state = CTF_STATE_START;
		ent->client->resp.ghost = NULL;

		gi.centerprintf(ent, "******************\n\nMATCH HAS STARTED!\n\n******************");

		if (ent->client->resp.ctf_team != CTF_NOTEAM) {
			// make up a ghost code
			CTFAssignGhost(ent);
			//CTFPlayerResetGrapple(ent);
			ent->svflags = SVF_NOCLIENT;
			ent->flags &= ~FL_GODMODE;

			ent->client->respawn_time = level.time + 1.0 + ((rand() % 30) / 10.0);
			ent->client->ps.pmove.pm_type = PM_DEAD;
			ent->client->anim_priority = ANIM_DEATH;
			ent->s.frame = FRAME_death308 - 1;
			ent->client->anim_end = FRAME_death308;
			ent->deadflag = DEAD_DEAD;
			ent->movetype = MOVETYPE_NOCLIP;
			ent->client->ps.gunindex = 0;
			gi.linkentity(ent);
		}
	}
}

void CTFEndMatch(void)
{
	ctfgame.match = MATCH_POST;
	gi.bprintf(PRINT_CHAT, "MATCH COMPLETED!\n");

	CTFCalcScores();

	gi.bprintf(PRINT_HIGH, "RED TEAM:  %d captures, %d points\n",
		ctfgame.team1, ctfgame.total1);
	gi.bprintf(PRINT_HIGH, "BLUE TEAM:  %d captures, %d points\n",
		ctfgame.team2, ctfgame.total2);

	if (ctfgame.team1 > ctfgame.team2)
		gi.bprintf(PRINT_CHAT, "RED team won over the BLUE team by %d CAPTURES!\n",
			ctfgame.team1 - ctfgame.team2);
	else if (ctfgame.team2 > ctfgame.team1)
		gi.bprintf(PRINT_CHAT, "BLUE team won over the RED team by %d CAPTURES!\n",
			ctfgame.team2 - ctfgame.team1);
	else if (ctfgame.total1 > ctfgame.total2) // frag tie breaker
		gi.bprintf(PRINT_CHAT, "RED team won over the BLUE team by %d POINTS!\n",
			ctfgame.total1 - ctfgame.total2);
	else if (ctfgame.total2 > ctfgame.total1)
		gi.bprintf(PRINT_CHAT, "BLUE team won over the RED team by %d POINTS!\n",
			ctfgame.total2 - ctfgame.total1);
	else
		gi.bprintf(PRINT_CHAT, "TIE GAME!\n");

	EndDMLevel();
}

qboolean CTFNextMap(void)
{
	if (ctfgame.match == MATCH_POST) {
		ctfgame.match = MATCH_SETUP;
		CTFResetAllPlayers();
		return true;
	}
	return false;
}

void CTFWinElection(void)
{
	switch (ctfgame.election) {
	case ELECT_MATCH:
		// reset into match mode
		if (competition->value < 3)
			gi.cvar_set("competition", "2");
		ctfgame.match = MATCH_SETUP;
		CTFResetAllPlayers();
		break;

	case ELECT_ADMIN:
		ctfgame.etarget->client->resp.admin = true;
		gi.bprintf(PRINT_HIGH, "%s has become an admin.\n", ctfgame.etarget->client->pers.netname);
		gi.cprintf(ctfgame.etarget, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
		break;

	case ELECT_MAP:
		gi.bprintf(PRINT_HIGH, "%s is warping to level %s.\n",
			ctfgame.etarget->client->pers.netname, ctfgame.elevel);
		strncpy(level.forcemap, ctfgame.elevel, sizeof(level.forcemap) - 1);
		EndDMLevel();
		break;
	}
	ctfgame.election = ELECT_NONE;
}

void CTFVoteYes(edict_t* ent)
{
	if (ctfgame.election == ELECT_NONE) {
		gi.cprintf(ent, PRINT_HIGH, "No election is in progress.\n");
		return;
	}
	if (ent->client->resp.voted) {
		gi.cprintf(ent, PRINT_HIGH, "You already voted.\n");
		return;
	}
	if (ctfgame.etarget == ent) {
		gi.cprintf(ent, PRINT_HIGH, "You can't vote for yourself.\n");
		return;
	}

	ent->client->resp.voted = true;

	ctfgame.evotes++;
	if (ctfgame.evotes == ctfgame.needvotes) {
		// the election has been won
		CTFWinElection();
		return;
	}
	gi.bprintf(PRINT_HIGH, "%s\n", ctfgame.emsg);
	gi.bprintf(PRINT_CHAT, "Votes: %d  Needed: %d  Time left: %ds\n", ctfgame.evotes, ctfgame.needvotes,
		(int)(ctfgame.electtime - level.time));
}

void CTFVoteNo(edict_t* ent)
{
	if (ctfgame.election == ELECT_NONE) {
		gi.cprintf(ent, PRINT_HIGH, "No election is in progress.\n");
		return;
	}
	if (ent->client->resp.voted) {
		gi.cprintf(ent, PRINT_HIGH, "You already voted.\n");
		return;
	}
	if (ctfgame.etarget == ent) {
		gi.cprintf(ent, PRINT_HIGH, "You can't vote for yourself.\n");
		return;
	}

	ent->client->resp.voted = true;

	gi.bprintf(PRINT_HIGH, "%s\n", ctfgame.emsg);
	gi.bprintf(PRINT_CHAT, "Votes: %d  Needed: %d  Time left: %ds\n", ctfgame.evotes, ctfgame.needvotes,
		(int)(ctfgame.electtime - level.time));
}

void CTFReady(edict_t* ent)
{
	int i, j;
	edict_t* e;
	int t1, t2;

	if (ent->client->resp.ctf_team == CTF_NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (ctfgame.match != MATCH_SETUP) {
		gi.cprintf(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (ent->client->resp.ready) {
		gi.cprintf(ent, PRINT_HIGH, "You have already commited.\n");
		return;
	}

	ent->client->resp.ready = true;
	gi.bprintf(PRINT_HIGH, "%s is ready.\n", ent->client->pers.netname);

	t1 = t2 = 0;
	for (j = 0, i = 1; i <= maxclients->value; i++) {
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->client->resp.ctf_team != CTF_NOTEAM && !e->client->resp.ready)
			j++;
		if (e->client->resp.ctf_team == CTF_TEAM1)
			t1++;
		else if (e->client->resp.ctf_team == CTF_TEAM2)
			t2++;
	}
	if (!j && t1 && t2) {
		// everyone has commited
		gi.bprintf(PRINT_CHAT, "All players have commited.  Match starting\n");
		ctfgame.match = MATCH_PREGAME;
		ctfgame.matchtime = level.time + matchstarttime->value;
		ctfgame.countdown = false;
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);
	}
}

void CTFNotReady(edict_t* ent)
{
	if (ent->client->resp.ctf_team == CTF_NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (ctfgame.match != MATCH_SETUP && ctfgame.match != MATCH_PREGAME) {
		gi.cprintf(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (!ent->client->resp.ready) {
		gi.cprintf(ent, PRINT_HIGH, "You haven't commited.\n");
		return;
	}

	ent->client->resp.ready = false;
	gi.bprintf(PRINT_HIGH, "%s is no longer ready.\n", ent->client->pers.netname);

	if (ctfgame.match == MATCH_PREGAME) {
		gi.bprintf(PRINT_CHAT, "Match halted.\n");
		ctfgame.match = MATCH_SETUP;
		ctfgame.matchtime = level.time + matchsetuptime->value * 60;
	}
}

void CTFGhost(edict_t* ent)
{
	int i;
	int n;

	if (gi.argc() < 2) {
		gi.cprintf(ent, PRINT_HIGH, "Usage:  ghost <code>\n");
		return;
	}

	if (ent->client->resp.ctf_team != CTF_NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You are already in the game.\n");
		return;
	}
	if (ctfgame.match != MATCH_GAME) {
		gi.cprintf(ent, PRINT_HIGH, "No match is in progress.\n");
		return;
	}

	n = atoi(gi.argv(1));

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (ctfgame.ghosts[i].code && ctfgame.ghosts[i].code == n) {
			gi.cprintf(ent, PRINT_HIGH, "Ghost code accepted, your position has been reinstated.\n");
			ctfgame.ghosts[i].ent->client->resp.ghost = NULL;
			ent->client->resp.ctf_team = ctfgame.ghosts[i].team;
			ent->client->resp.ghost = ctfgame.ghosts + i;
			ent->client->resp.score = ctfgame.ghosts[i].score;
			ent->client->resp.ctf_state = CTF_STATE_START;
			ctfgame.ghosts[i].ent = ent;
			ent->svflags = 0;
			ent->flags &= ~FL_GODMODE;
			PutClientInServer(ent);
			gi.bprintf(PRINT_HIGH, "%s has been reinstated to %s team.\n",
				ent->client->pers.netname, CTFTeamName(ent->client->resp.ctf_team));
			return;
		}
	}
	gi.cprintf(ent, PRINT_HIGH, "Invalid ghost code.\n");
}

qboolean CTFMatchSetup(void)
{
	if (ctfgame.match == MATCH_SETUP || ctfgame.match == MATCH_PREGAME)
		return true;
	return false;
}

qboolean CTFMatchOn(void)
{
	if (ctfgame.match == MATCH_GAME)
		return true;
	return false;
}


/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

void CTFBotJoinTeam(edict_t* ent, int desired_team)	//MATTHIAS
{
	char* s;

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	// Log Join Team - Mark Davies
	sl_LogPlayerName(&gi,
		ent->client->pers.netname,
		CTFTeamName(ent->client->resp.ctf_team),
		level.time);

	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;
	bprint_botsafe(PRINT_HIGH, "%s joined the %s team.\n", ent->client->pers.netname, CTFTeamName(desired_team));

	if (desired_team == 1)
	{
		numred++;
	}
	else if (desired_team == 2)
	{
		numblue++;
	}
}

void CTFJoinTeam_old(edict_t* ent, int desired_team)
{
	char* s;

	PMenu_Close(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	// Log Join Team - Mark Davies
	sl_LogPlayerName(&gi,
		ent->client->pers.netname,
		CTFTeamName(ent->client->resp.ctf_team),
		level.time);

	PutClientInServer(ent);
	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;
	bprint_botsafe(PRINT_HIGH, "%s joined the %s team.\n", ent->client->pers.netname, CTFTeamName(desired_team));

	if (desired_team == 1)
	{
		numred++;
	}
	else if (desired_team == 2)
	{
		numblue++;
	}
	if (!ent->bot_player)
		gi.centerprintf(ent, motd);//MATTHIAS
}
void CTFJoinTeam(edict_t* ent, int desired_team)
{
	char* s;

	PMenu_Close(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = CTF_STATE_START;
	s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	// Log Join Team - Mark Davies
	sl_LogPlayerName(&gi,
		ent->client->pers.netname,
		CTFTeamName(ent->client->resp.ctf_team),
		level.time);

	// assign a ghost if we are in match mode
	if (ctfgame.match == MATCH_GAME) {
		if (ent->client->resp.ghost)
			ent->client->resp.ghost->code = 0;
		ent->client->resp.ghost = NULL;
		CTFAssignGhost(ent);
	}

	PutClientInServer(ent);
	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;
	bprint_botsafe(PRINT_HIGH, "%s joined the %s team.\n", ent->client->pers.netname, CTFTeamName(desired_team));

	if (desired_team == 1)
	{
		numred++;
	}
	else if (desired_team == 2)
	{
		numblue++;
	}
	if (!ent->bot_player)
		gi.centerprintf(ent, motd);//MATTHIAS
	if (ctfgame.match == MATCH_SETUP) {
		if (!ent->bot_player)
		gi.centerprintf(ent, "***********************\n"
			"Type \"ready\" in console\n"
			"to ready up.\n"
			"***********************");
	}
}
void CTFJoinTeam1(edict_t* ent, pmenu_t* p)
{
	CTFJoinTeam(ent, CTF_TEAM1);
}

void CTFJoinTeam2(edict_t* ent, pmenu_t* p)
{
	CTFJoinTeam(ent, CTF_TEAM2);
}

void CTFChaseCam(edict_t* ent, pmenu_t* p)
{
	if (ent->client->camera)	//switch back to player mode
	{
		ClientBeginDeathmatch(ent);
		PMenu_Close(ent);
		return;
	}

	ent->client->cammode = 1;
	CreateCamera(ent);	//switch to intelli cam mode
	PMenu_Close(ent);
}

void CTFReturnToMain(edict_t* ent, pmenu_t* p)
{
	PMenu_Close(ent);
	CTFOpenJoinMenu(ent);
}

void CTFCredits(edict_t* ent, pmenu_t* p);



void CTFShowScores(edict_t* ent, pmenu_t* p)
{
	PMenu_Close(ent);

	ent->client->showscores = true;
	ent->client->showinventory = false;
	DeathmatchScoreboard(ent);
}

pmenu_t creditsmenu[] = {
	{ "*Quake II Chaos DM Lives v3.2b*",	PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{ NULL,				PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{  "*Programming",								PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{ "Flash (flash@telefragged.com)",					PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{ "MrG{DRGN} (aggravationq2@hotmail.com)",	PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{ "QwazyWabbit (gandalf.2004@gmail.com)",	PMENU_ALIGN_CENTER, NULL, NULL },/* MrG{DRGN} */
	{ "*Level Design", 					PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Nat (nnp@greennet.net)",			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Craig (car188@psu.edu)",			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*Art",							PMENU_ALIGN_CENTER, NULL, NULL },
	{ "SPA (spa@telefragged.com)",		PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*Sound",							PMENU_ALIGN_CENTER, NULL, NULL },
	{ "SPA (spa@telefragged.com)",		PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Nat (nnp@greennet.net)",			PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,								PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Return to Main Menu",			PMENU_ALIGN_LEFT, NULL, CTFReturnToMain }
};

pmenu_t joinmenu[] = {
	{ "*Quake II",			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "*Chaos Deathmatch",	PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,					PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,					PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Join Red Team",		PMENU_ALIGN_LEFT, NULL, CTFJoinTeam1 },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Join Blue Team",		PMENU_ALIGN_LEFT, NULL, CTFJoinTeam2 },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "IntelliCam",			PMENU_ALIGN_LEFT, NULL, CTFChaseCam },
	{ "Credits",			PMENU_ALIGN_LEFT, NULL, CTFCredits },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Use [ and ] to move cursor",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "ENTER to select",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "ESC to Exit Menu",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "(TAB to Return)",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "v" CTF_STRING_VERSION,	PMENU_ALIGN_RIGHT, NULL, NULL },
};

int CTFUpdateJoinMenu(edict_t* ent)
{
	static char levelname[32] = { 0 };
	static char team1players[32];
	static char team2players[32];
	int num1, num2, i;

	/* MrG{DRGN} something is broken with ctf camera this should prevent grapple firing and
	*  other things, but appropriate checks need to be made in g_cmds.c

	memset(ent->client->pers.inventory, 0, sizeof(ent->client->pers.inventory));*/

	joinmenu[4].text = "Join Red Team";
	joinmenu[4].SelectFunc = CTFJoinTeam1;
	joinmenu[6].text = "Join Blue Team";
	joinmenu[6].SelectFunc = CTFJoinTeam2;

	if (ctf_forcejoin->string && *ctf_forcejoin->string) {
		if (Q_stricmp(ctf_forcejoin->string, "red") == 0) {
			joinmenu[6].text = NULL;
			joinmenu[6].SelectFunc = NULL;
		}
		else if (Q_stricmp(ctf_forcejoin->string, "blue") == 0) {
			joinmenu[4].text = NULL;
			joinmenu[4].SelectFunc = NULL;
		}
	}

	if (ent->client->camera)
		joinmenu[8].text = "Leave IntelliCam";
	else
		joinmenu[8].text = "IntelliCam";

	levelname[0] = '*';
	if (g_edicts[0].message)
		strncpy(levelname + 1, g_edicts[0].message, sizeof(levelname) - 2);
	else
		strncpy(levelname + 1, level.mapname, sizeof(levelname) - 2);
	levelname[sizeof(levelname) - 1] = 0;

	num1 = num2 = 0;
	for (i = 0; i < maxclients->value; i++) {
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			num1++;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			num2++;
	}

	sprintf(team1players, "  (%d players)", num1);
	sprintf(team2players, "  (%d players)", num2);

	joinmenu[2].text = levelname;
	if (joinmenu[4].text)
		joinmenu[5].text = team1players;
	else
		joinmenu[5].text = NULL;
	if (joinmenu[6].text)
		joinmenu[7].text = team2players;
	else
		joinmenu[7].text = NULL;

	if (num1 > num2)
		return CTF_TEAM1;
	else if (num2 > num1)
		return CTF_TEAM1;
	return (rand() & 1) ? CTF_TEAM1 : CTF_TEAM2;
}

void CTFOpenJoinMenu(edict_t* ent)
{
	int team;

	team = CTFUpdateJoinMenu(ent);
	if (ent->client->camera)
		team = 8;
	else if (team == CTF_TEAM1)
		team = 4;
	else
		team = 6;
	PMenu_Open(ent, joinmenu, team, sizeof(joinmenu) / sizeof(pmenu_t));
}

void CTFCredits(edict_t* ent, pmenu_t* p)
{
	PMenu_Close(ent);
	PMenu_Open(ent, creditsmenu, -1, sizeof(creditsmenu) / sizeof(pmenu_t));
}

qboolean CTFStartClient(edict_t* ent)
{
	if (ent->client->resp.ctf_team != CTF_NOTEAM)
		return false;

	if (!((int)dmflags->value & DF_CTF_FORCEJOIN)) {
		// start as 'observer'
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->resp.ctf_team = CTF_NOTEAM;
		ent->client->ps.gunindex = 0;
		gi.linkentity(ent);

		CTFOpenJoinMenu(ent);
		return true;
	}
	return false;
}

qboolean CTFCheckRules_Old()
{
	if (capturelimit->value &&
		(ctfgame.team1 >= capturelimit->value ||
			ctfgame.team2 >= capturelimit->value)) {
		bprint_botsafe(PRINT_HIGH, "Capturelimit hit.\n");
		return true;
	}
	return false;
}

qboolean CTFInMatch(void)
{
	if (ctfgame.match > MATCH_NONE)
		return true;
	return false;
}

qboolean CTFCheckRules(void)
{
	int t;
	int i, j;
	char text[64];
	edict_t* ent;

	if (ctfgame.election != ELECT_NONE && ctfgame.electtime <= level.time) {
		gi.bprintf(PRINT_CHAT, "Election timed out and has been cancelled.\n");
		ctfgame.election = ELECT_NONE;
	}

	if (ctfgame.match != MATCH_NONE) {
		t = ctfgame.matchtime - level.time;

		// no team warnings in match mode
		ctfgame.warnactive = 0;

		if (t <= 0) { // time ended on something
			switch (ctfgame.match) {
			case MATCH_SETUP:
				// go back to normal mode
				if (competition->value < 3) {
					ctfgame.match = MATCH_NONE;
					gi.cvar_set("competition", "1");
					CTFResetAllPlayers();
				}
				else {
					// reset the time
					ctfgame.matchtime = level.time + matchsetuptime->value * 60;
				}
				return false;

			case MATCH_PREGAME:
				// match started!
				CTFStartMatch();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
				return false;

			case MATCH_GAME:
				// match ended!
				CTFEndMatch();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/bigtele.wav"), 1, ATTN_NONE, 0);
				return false;
			}
		}

		if (t == ctfgame.lasttime)
			return false;

		ctfgame.lasttime = t;

		switch (ctfgame.match) {
		case MATCH_SETUP:
			for (j = 0, i = 1; i <= maxclients->value; i++) {
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (ent->client->resp.ctf_team != CTF_NOTEAM &&
					!ent->client->resp.ready)
					j++;
			}

			if (competition->value < 3)
				sprintf(text, "%02d:%02d SETUP: %d not ready",
					t / 60, t % 60, j);
			else
				sprintf(text, "SETUP: %d not ready", j);

			gi.configstring(CONFIG_CTF_MATCH, text);
			break;


		case MATCH_PREGAME:
			sprintf(text, "%02d:%02d UNTIL START",
				t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);

			if (t <= 10 && !ctfgame.countdown) {
				ctfgame.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		case MATCH_GAME:
			sprintf(text, "%02d:%02d MATCH",
				t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);
			if (t <= 10 && !ctfgame.countdown) {
				ctfgame.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;
		}
		return false;

	}
	else {
		int team1 = 0, team2 = 0;

		if (level.time == ctfgame.lasttime)
			return false;
		ctfgame.lasttime = level.time;
		// this is only done in non-match (public) mode

		if (warn_unbalanced->value) {
			// count up the team totals
			for (i = 1; i <= maxclients->value; i++) {
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (ent->client->resp.ctf_team == CTF_TEAM1)
					team1++;
				else if (ent->client->resp.ctf_team == CTF_TEAM2)
					team2++;
			}

			if (team1 - team2 >= 2 && team2 >= 2) {
				if (ctfgame.warnactive != CTF_TEAM1) {
					ctfgame.warnactive = CTF_TEAM1;
					gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Red has too many players");
				}
			}
			else if (team2 - team1 >= 2 && team1 >= 2) {
				if (ctfgame.warnactive != CTF_TEAM2) {
					ctfgame.warnactive = CTF_TEAM2;
					gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Blue has too many players");
				}
			}
			else
				ctfgame.warnactive = 0;
		}
		else
			ctfgame.warnactive = 0;

	}



	if (capturelimit->value &&
		(ctfgame.team1 >= capturelimit->value ||
			ctfgame.team2 >= capturelimit->value)) {
		gi.bprintf(PRINT_HIGH, "Capturelimit hit.\n");
		return true;
	}
	return false;
}
/*--------------------------------------------------------------------------
 * just here to help old map conversions
 *--------------------------------------------------------------------------*/

static void old_teleporter_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	edict_t* dest;
	int			i;
	vec3_t		forward;

	/* MrG{DRGN}*/
	if (!self || !other) /* plane & surf are unused */
	{
		G_FreeEdict(self);
		return;
	}
	/* END */
	if (!other->client && !tele_fire->value)
		return;
	dest = G_Find(NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	//	other->s.origin[2] += 10;

		// clear the velocity and hold them in place briefly
	VectorClear(other->velocity);
	if (other->client)
	{
		other->client->ps.pmove.pm_time = 160 >> 3;		// hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	}
	// draw the teleport splash at source and on the player
	self->enemy->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	if (other->client)
	{
		for (i = 0; i < 3; i++)
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);
	}
	other->s.angles[PITCH] = 0;
	other->s.angles[YAW] = dest->s.angles[YAW];
	other->s.angles[ROLL] = 0;
	if (other->client)
	{
		VectorCopy(dest->s.angles, other->client->ps.viewangles);
		VectorCopy(dest->s.angles, other->client->v_angle);
	
	// give a little forward velocity
	AngleVectors(other->client->v_angle, forward, NULL, NULL);
	}
	VectorScale(forward, 200, other->velocity);

	// kill anything at the destination
	if (!KillBox(other))
	{
	}

	gi.linkentity(other);
}

/*QUAKED trigger_teleport (0.5 0.5 0.5) ?
Players touching this will be teleported
*/
void SP_trigger_teleport(edict_t* ent)
{
	edict_t* s;
	int i;

	if (!ent->target)
	{
		gi.dprintf("teleporter without a target.\n");
		G_FreeEdict(ent);
		return;
	}

	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	ent->touch = old_teleporter_touch;
	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);

	ent->classindex = TRIGGER_TELEPORT;
	// noisemaker and splash effect dude
	s = G_Spawn();
	ent->enemy = s;
	for (i = 0; i < 3; i++)
		s->s.origin[i] = ent->mins[i] + (ent->maxs[i] - ent->mins[i]) / 2;
	s->s.sound = gi.soundindex("world/hum1.wav");
	gi.linkentity(s);
}

/*QUAKED info_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32)
Point trigger_teleports at these.
*/
void SP_info_teleport_destination(edict_t* ent)
{
	ent->classindex = INFO_TELEPORT_DESTINATION;
	ent->s.origin[2] += 16;
}
