﻿#include "g_local.h"
#include "c_cam.h"
#include "gslog.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;

edict_t* g_edicts;

cvar_t* deathmatch;
cvar_t* coop;
cvar_t* dmflags;
cvar_t* skill;
cvar_t* fraglimit;
cvar_t* timelimit;
//ZOID
cvar_t* capturelimit;
cvar_t* instantweap;
//ZOID
cvar_t* password;
cvar_t* maxclients;
cvar_t* maxentities;
cvar_t* g_select_empty;
cvar_t* dedicated;
cvar_t* game_dir;
cvar_t* g_maplistfile;
cvar_t* g_maplistmode;// 1 for sequential, 2 for random map selection

cvar_t* sv_maxvelocity;
cvar_t* sv_gravity;

cvar_t* sv_rollspeed;
cvar_t* sv_rollangle;
cvar_t* gun_x;
cvar_t* gun_y;
cvar_t* gun_z;

cvar_t* run_pitch;
cvar_t* run_roll;
cvar_t* bob_up;
cvar_t* bob_pitch;
cvar_t* bob_roll;

// MrG{DRGN} 
cvar_t* flood_msgs;
cvar_t* flood_persecond;
cvar_t* flood_waitdelay;
cvar_t* filterban;
cvar_t* developer;

void SVCmd_killbot_f(char* name);

cvar_t* sv_cheats;

//===================================================================

void  ShutdownGame(void)
{
	gi.dprintf("==== Shutdown Chaos DM Lives ====\n");

	sl_GameEnd(&gi, level);	// StdLog - Mark Davies
	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);

#ifdef _WIN32
	OutputDebugString("ShutdownGame() was called.\n");
	OutputDebugString("Memory stats since startup.\n");
	_CrtMemDumpStatistics(&startup1);
	_CrtDumpMemoryLeaks();
#endif
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t* GetGameAPI(game_import_t* import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(const char* error, ...) /* MrG{DRGN} changed to const */
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	gi.error("%s", text);
}

void Com_Printf(char* msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	gi.dprintf("%s", text);
}

#endif

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames(void)
{
	int		i;
	edict_t* ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame(ent);
	}
}

/*
Choose next map in rotation based on current mode.
*/
int MaplistNext(void)
{
	switch (maplist.mlflag)
	{
	case ML_ROTATE_SEQ:
		maplist.currentmap = (maplist.currentmap + 1) % maplist.nummaps;
		break;
	case ML_ROTATE_RANDOM:
		maplist.lastmap = maplist.currentmap; // MrG{DRGN} store this here
		maplist.currentmap = (int)(random() * maplist.nummaps);	// assign a new map
		while (maplist.currentmap == maplist.lastmap) // check we're not going to repeat the lastmap right away.
		{
			maplist.currentmap = (int)(random() * maplist.nummaps);	// if we are, spin again
		}
		break;
	default:
		maplist.mlflag = ML_OFF;
		maplist.currentmap = maplist.currentmap;
		break;
	}

	return maplist.currentmap;
}

void MaplistMapModeSetup(void) 
{
	if (maplist.ctf[maplist.currentmap] == '1')
		gi.cvar_set("ctf", "1");
	else
		gi.cvar_set("ctf", "0");

	if (maplist.lightsoff[maplist.currentmap] == '1')
		gi.cvar_set("lightsoff", "1");
	else if (maplist.lightsoff[maplist.currentmap] == '2')
		gi.cvar_set("lightsoff", "2");
	else
		gi.cvar_set("lightsoff", "0");
}

/*
=================
Returns the created target changelevel.
=================
*/
edict_t* CreateTargetChangeLevel(char* map)
{
	edict_t* ent;

	ent = G_Spawn();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}
/*
=================
The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel(void)
{
	edict_t* ent;

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
		ent = CreateTargetChangeLevel(level.mapname);
	
	// maplist rotation select
	else if (maplist.mlflag > 0)
	{
		int i = MaplistNext();
		ent = CreateTargetChangeLevel(maplist.mapnames[i]);
	}
	else if (level.nextmap[0]) // operator has commanded a map
	{	// go to a specific map
		ent = CreateTargetChangeLevel(level.nextmap);
	}
	else
	{	// search for a changelevel
		ent = G_Find(NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			ent = CreateTargetChangeLevel(level.mapname);
		}
	}

	BeginIntermission(ent);
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules(void)
{
	int			i;
	gclient_t* cl;

	if (level.intermissiontime)
		return;

	//ZOID
	if (ctf->value && CTFCheckRules()) {
		EndDMLevel();
		return;
	}
	if (CTFInMatch())
		return; // no checking in match mode
//ZOID

	if (timelimit->value)
	{
		if (level.time >= timelimit->value * 60)
		{
			bprint_botsafe(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i = 0; i < maxclients->value; i++)
		{
			cl = game.clients + i;
			if (!g_edicts[i + 1].inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				bprint_botsafe(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel();
				return;
			}
		}
	}
}

/*
=============
ExitLevel
=============
*/
void ExitLevel(void)
{
	int		i;
	edict_t* ent;
	char	command[256];

	Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames();

	// clear some things before going to next level
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}
	SVCmd_killbot_f("all");
	//ZOID
	if (ctf->value)
		CTFInit();
	//ZOID
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/

void G_RunFrame(void)
{
	int		i;
	edict_t* ent;

	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i = 0; i < globals.num_edicts; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy(ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame(ent);
			if ((Q_stricmp(ent->classname, "bot") != 0)) /* MrG{DRGN} simplify */
				continue;							//MATTHIAS
		}

		G_RunEntity(ent);
	}

	// see if it is time to end a deathmatch
	CheckDMRules();

	// build the playerstate_t structures for all players
	ClientEndServerFrames();
}
