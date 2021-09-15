#include "g_local.h"
#include "c_botnav.h"
#include "gslog.h"

typedef struct
{
	unsigned int classindex; /* MrG{DRGN} */
	char* name;
	void	(*spawn)(edict_t* ent);
} spawn_t;

void PreCacheAll(void);	//MATTHIAS
void LoadMOTD(void);
void Load_BotChat(void);

void SP_item_health(edict_t* self);
void SP_item_health_small(edict_t* self);
void SP_item_health_large(edict_t* self);
void SP_item_health_mega(edict_t* self);

void SP_info_player_start(edict_t* ent);
void SP_info_player_deathmatch(edict_t* ent);
void SP_info_player_coop(edict_t* ent);
void SP_info_player_intermission(edict_t* ent);

void SP_func_plat(edict_t* ent);
void SP_func_rotating(edict_t* ent);
void SP_func_button(edict_t* ent);
void SP_func_door(edict_t* ent);
void SP_func_door_secret(edict_t* ent);
void SP_func_door_rotating(edict_t* ent);
void SP_func_water(edict_t* ent);
void SP_func_train(edict_t* ent);
void SP_func_conveyor(edict_t* self);
void SP_func_wall(edict_t* self);
void SP_func_object(edict_t* self);
void SP_func_explosive(edict_t* self);
void SP_func_timer(edict_t* self);
void SP_func_areaportal(edict_t* ent);
void SP_func_clock(edict_t* ent);
void SP_func_killbox(edict_t* ent);

void SP_trigger_always(edict_t* ent);
void SP_trigger_once(edict_t* ent);
void SP_trigger_multiple(edict_t* ent);
void SP_trigger_relay(edict_t* ent);
void SP_trigger_push(edict_t* ent);
void SP_trigger_hurt(edict_t* ent);
void SP_trigger_key(edict_t* ent);
void SP_trigger_counter(edict_t* ent);
void SP_trigger_elevator(edict_t* ent);
void SP_trigger_gravity(edict_t* ent);
void SP_trigger_monsterjump(edict_t* ent);

void SP_target_temp_entity(edict_t* ent);
void SP_target_speaker(edict_t* ent);
void SP_target_explosion(edict_t* ent);
void SP_target_changelevel(edict_t* ent);
void SP_target_secret(edict_t* ent);
void SP_target_goal(edict_t* ent);
void SP_target_splash(edict_t* ent);
void SP_target_spawner(edict_t* ent);
void SP_target_blaster(edict_t* ent);
void SP_target_crosslevel_trigger(edict_t* ent);
void SP_target_crosslevel_target(edict_t* ent);
void SP_target_laser(edict_t* self);
#if 0 /*MrG{DRGN} non-deathmatch stuff!*/
void SP_target_help(edict_t* ent);

void SP_target_lightramp(edict_t* self);
#endif
void SP_target_earthquake(edict_t* ent);
void SP_target_character(edict_t* ent);
void SP_target_string(edict_t* ent);

void SP_worldspawn(edict_t* ent);
void SP_viewthing(edict_t* ent);

void SP_light(edict_t* self);
void SP_light_mine1(edict_t* ent);
void SP_light_mine2(edict_t* ent);
void SP_info_null(edict_t* self);
void SP_info_notnull(edict_t* self);
void SP_path_corner(edict_t* self);
void SP_point_combat(edict_t* self);

void SP_misc_explobox(edict_t* self);
void SP_misc_banner(edict_t* self);
void SP_misc_satellite_dish(edict_t* self);
void SP_misc_gib_arm(edict_t* self);
void SP_misc_gib_leg(edict_t* self);
void SP_misc_gib_head(edict_t* self);
void SP_misc_deadsoldier(edict_t* self);
void SP_misc_viper(edict_t* self);
void SP_misc_viper_bomb(edict_t* self);
void SP_misc_bigviper(edict_t* self);
void SP_misc_strogg_ship(edict_t* self);
void SP_misc_teleporter(edict_t* self);
void SP_misc_teleporter_dest(edict_t* self);
void SP_misc_blackhole(edict_t* self);
void SP_misc_eastertank(edict_t* self);
void SP_misc_easterchick(edict_t* self);
void SP_misc_easterchick2(edict_t* self);

void SP_monster_commander_body(edict_t* self);


unsigned int getClassindexFromClassname(char* classname);
/* MrG{DRGN} added classindex field */
spawn_t	spawns[] = {
	{ITEM_HEALTH, "item_health", SP_item_health},
	{ITEM_HEALTH_SMALL, "item_health_small", SP_item_health_small},
	{ITEM_HEALTH_LARGE, "item_health_large", SP_item_health_large},
	{ITEM_HEALTH_MEGA, "item_health_mega", SP_item_health_mega},

	{INFO_PLAYER_START, "info_player_start", SP_info_player_start},
	{INFO_PLAYER_DEATHMATCH, "info_player_deathmatch", SP_info_player_deathmatch},
	{INFO_PLAYER_COOP, "info_player_coop", SP_info_player_coop},
	{INFO_PLAYER_INTERMISSION, "info_player_intermission", SP_info_player_intermission},
	//ZOID
	{INFO_PLAYER_TEAM1, "info_player_team1", SP_info_player_team1},
	{INFO_PLAYER_TEAM2, "info_player_team2", SP_info_player_team2},
	//ZOID

	{FUNC_PLAT, "func_plat", SP_func_plat},
	{FUNC_BUTTON,"func_button", SP_func_button},
	{FUNC_DOOR, "func_door", SP_func_door},
	{FUNC_DOOR_SECRET, "func_door_secret", SP_func_door_secret},
	{FUNC_DOOR_ROTATING, "func_door_rotating", SP_func_door_rotating},
	{FUNC_ROTATING, "func_rotating", SP_func_rotating},
	{FUNC_TRAIN, "func_train", SP_func_train},
	{FUNC_WATER, "func_water", SP_func_water},
	{FUNC_CONVEYOR, "func_conveyor", SP_func_conveyor},
	{FUNC_AREAPORTAL, "func_areaportal", SP_func_areaportal},
	{FUNC_CLOCK, "func_clock", SP_func_clock},
	{FUNC_WALL, "func_wall", SP_func_wall},
	{FUNC_OBJECT, "func_object", SP_func_object},
	{FUNC_TIMER, "func_timer", SP_func_timer},
	{FUNC_EXPLOSIVE, "func_explosive", SP_func_explosive},
	{FUNC_KILLBOX, "func_killbox", SP_func_killbox},

	{TRIGGER_ALWAYS, "trigger_always", SP_trigger_always},
	{TRIGGER_ONCE, "trigger_once", SP_trigger_once},
	{TRIGGER_MULTIPLE, "trigger_multiple", SP_trigger_multiple},
	{TRIGGER_RELAY, "trigger_relay", SP_trigger_relay},
	{TRIGGER_PUSH, "trigger_push", SP_trigger_push},
	{TRIGGER_HURT, "trigger_hurt", SP_trigger_hurt},
	{TRIGGER_KEY, "trigger_key", SP_trigger_key},
	{TRIGGER_COUNTER, "trigger_counter", SP_trigger_counter},
	{TRIGGER_ELEVATOR, "trigger_elevator", SP_trigger_elevator},
	{TRIGGER_GRAVITY, "trigger_gravity", SP_trigger_gravity},
	{TRIGGER_MONSTERJUMP, "trigger_monsterjump", SP_trigger_monsterjump},
	{TARGET_TEMP_ENTITY, "target_temp_entity", SP_target_temp_entity},
	{TARGET_SPEAKER, "target_speaker", SP_target_speaker},
	{TARGET_EXPLOSION, "target_explosion", SP_target_explosion},
	{TARGET_CHANGELEVEL, "target_changelevel", SP_target_changelevel},

#if 0 /* MrG{DRGN} these things free in deathmatch anyway */
	{TARGET_SECRET, "target_secret", SP_target_secret},
	{TARGET_GOAL, "target_goal", SP_target_goal},
#endif

	{TARGET_SPLASH, "target_splash", SP_target_splash},
	{TARGET_SPAWNER, "target_spawner", SP_target_spawner},
	{TARGET_BLASTER, "target_blaster", SP_target_blaster},
	{TARGET_CROSSLEVEL_TRIGGER, "target_crosslevel_trigger", SP_target_crosslevel_trigger},
	{TARGET_CROSSLEVEL_TARGET, "target_crosslevel_target", SP_target_crosslevel_target},
	{TARGET_LASER, "target_laser", SP_target_laser},

#if 0
	{TARGET_HELP, "target_help", SP_target_help},
	{TARGET_LIGHTRAMP, "target_lightramp", SP_target_lightramp},
#endif

	{TARGET_EARTHQUAKE, "target_earthquake", SP_target_earthquake},
	{TARGET_CHARACTER, "target_character", SP_target_character},
	{TARGET_STRING, "target_string", SP_target_string},

	{WORLDSPAWN, "worldspawn", SP_worldspawn},
	{VIEWTHING, "viewthing", SP_viewthing},

	{LIGHT, "light", SP_light},
	{LIGHT_MINE1, "light_mine1", SP_light_mine1},
	{LIGHT_MINE2, "light_mine2", SP_light_mine2},
	{INFO_NULL, "info_null", SP_info_null},
	{INFO_NULL, "func_group", SP_info_null},
	{INFO_NOTNULL, "info_notnull", SP_info_notnull},
	{PATH_CORNER, "path_corner", SP_path_corner},
	{POINT_COMBAT, "point_combat", SP_point_combat},

	{MISC_EXPLOBOX, "misc_explobox", SP_misc_explobox},
	{MISC_BANNER, "misc_banner", SP_misc_banner},
	//ZOID
	{MISC_CTF_BANNER, "misc_ctf_banner", SP_misc_ctf_banner},
	{MISC_CTF_SMALL_BANNER, "misc_ctf_small_banner", SP_misc_ctf_small_banner},
	//ZOID
	{MISC_SATELLITE_DISH, "misc_satellite_dish", SP_misc_satellite_dish},
	{MISC_GIB_ARM, "misc_gib_arm", SP_misc_gib_arm},
	{MISC_GIB_LEG, "misc_gib_leg", SP_misc_gib_leg},
	{MISC_GIB_HEAD, "misc_gib_head", SP_misc_gib_head},
	{MISC_DEADSOLDIER, "misc_deadsoldier", SP_misc_deadsoldier},
	{MISC_VIPER, "misc_viper", SP_misc_viper},
	{MISC_VIPER_BOMB, "misc_viper_bomb", SP_misc_viper_bomb},
	{MISC_BIGVIPER, "misc_bigviper", SP_misc_bigviper},
	{MISC_STROGG_SHIP, "misc_strogg_ship", SP_misc_strogg_ship},
	{MISC_TELEPORTER, "misc_teleporter", SP_misc_teleporter},
	{MISC_TELEPORTER_DEST, "misc_teleporter_dest", SP_misc_teleporter_dest},
	//ZOID
	{TRIGGER_TELEPORT, "trigger_teleport", SP_trigger_teleport},
	{INFO_TELEPORT_DESTINATION, "info_teleport_destination", SP_info_teleport_destination},
	//ZOID
	{ MISC_BLACKHOLE, "misc_blackhole", SP_misc_blackhole},
	{ MISC_EASTERTANK, "misc_eastertank", SP_misc_eastertank},
	{ MISC_EASTERCHICK, "misc_easterchick", SP_misc_easterchick},
	{ MISC_EASTERCHICK2, "misc_easterchick2", SP_misc_easterchick2},
	{MONSTER_COMMANDER_BODY,"monster_commander_body",SP_monster_commander_body },

	{ENT_NULL, NULL, NULL}
};

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/


unsigned int getClassindexFromClassname(char* classname) {
	spawn_t* s;
	gitem_t* item;
	int		i;

	for (i = 0, item = itemlist; i < game.num_items; i++, item++) {
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, classname)) {	// found it
			return item->classindex;
		}
	}
	for (s = spawns; s->name; s++) {
		if (!strcmp(s->name, classname)) {	// found it
			return s->classindex;
		}
	}
	return 0;
}

char* getClassnameFromClassindex(unsigned int classindex) {
	spawn_t* s;
	gitem_t* item;
	int		i;

	for (i = 0, item = itemlist; i < game.num_items; i++, item++) {
		if (!item->classname)
			continue;
		if (item->classindex == classindex) {	// found it
			return item->classname;
		}
	}
	for (s = spawns; s->name; s++) {
		if (s->classindex == classindex) // found it
			return s->name;
	}
	return NULL;
}

void ED_CallSpawn(edict_t* ent)
{
	spawn_t* s;
	gitem_t* item;
	int		i;

	/* If this fails, edict is freed. */
	if (!ent)
		return;

	if (!ent->classname)
	{
		gi.dprintf("ED_CallSpawn: NULL classname\n");
		G_FreeEdict(ent);
		return;
	}
	if (!ent->classname) {
		ent->classname = getClassnameFromClassindex(ent->classindex);
	}
	else {
		ent->classindex = getClassindexFromClassname(ent->classname);
	}

	if (!ent->classname) {
		gi.dprintf("ED_CallSpawn: NULL classname and invalid classindex %d\n", ent->classindex);
		return;
	}

	if (!ent->classindex) {
		ent->classindex = getClassindexFromClassname(ent->classname);
	}
	if (!ent->classindex) {
		//gi.dprintf("ED_CallSpawn: No classindex and invalid classname %s\n", ent->classname);
		return;
	}
	// check item spawn functions
	for (i = 0, item = itemlist; i < game.num_items; i++, item++)
	{
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, ent->classname))
		{	// found it
			item->classindex = getClassindexFromClassname(item->classname);
			SpawnItem(ent, item);
			return;
		}
	}

	// check normal spawn functions
	for (s = spawns; s->name; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{	// found it
			s->spawn(ent);
			return;
		}
	}
	//MATTHIAS
	gi.dprintf("%s doesn't have a spawn function\n", ent->classname);
}
/*
void ED_CallSpawn_Old(edict_t* ent)
{
	spawn_t* s;
	gitem_t* item;
	int		i;

	if (!ent->classname)
	{
		gi.dprintf("ED_CallSpawn: NULL classname\n");
		return;
	}

	// check item spawn functions
	for (i = 0, item = itemlist; i < game.num_items; i++, item++)
	{
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, ent->classname))
		{	// found it
			SpawnItem(ent, item);
			return;
		}
	}

	// check normal spawn functions
	for (s = spawns; s->name; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{	// found it
			s->spawn(ent);
			return;
		}
	}
	//MATTHIAS
	//gi.dprintf ("%s doesn't have a spawn function\n", ent->classname);
}*/

/*
=============
ED_NewString
=============

char* ED_NewString(char* string)
{
	char* newb, * new_p;
	int		i, l;

	l = strlen(string) + 1;

		newb = gi.TagMalloc(l, TAG_LEVEL);

		new_p = newb;

		for (i = 0; i < l; i++)
		{
			if (string[i] == '\\' && i < l - 1)
			{
				i++;
				if (string[i] == 'n')
					*new_p++ = '\n';
				else
					*new_p++ = '\\';
			}
			else
				*new_p++ = string[i];
		}

		return newb;
	}
	*/

static char* ED_NewString(const char* string)
{
	int l = (int)strlen(string) + 1;
	char* newb = (char*)gi.TagMalloc(l, TAG_LEVEL);
	char* new_p = newb;

	for (int i = 0; i < l; i++)
	{
		/* check for special chars and convert them */
		if (string[i] == '\\' && l - 1 > i) /* MrG{DRGN} The variable 'i' was used as an array index before it is checked that is within limits. This can mean that the array might be accessed out of bounds. */
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}
/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField(char* key, char* value, edict_t* ent)
{
	field_t* f;
	byte* b;
	float	v;
	vec3_t	vec = { 0 };

	for (f = fields; f->name; f++)
	{
		if (!(f->flags & FFL_NOSPAWN) && !Q_stricmp(f->name, key))
		{	/* found it */
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte*)&st;
			else
				b = (byte*)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*(char**)(b + f->ofs) = ED_NewString(value);
				break;
			case F_VECTOR:
				if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) {
					gi.dprintf("WARNING: Vector field incomplete in %s, map: %s, field: %s\n", __func__, level.mapname, f->name);
				}
				((float*)(b + f->ofs))[0] = vec[0];
				((float*)(b + f->ofs))[1] = vec[1];
				((float*)(b + f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int*)(b + f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float*)(b + f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float*)(b + f->ofs))[0] = 0;
				((float*)(b + f->ofs))[1] = v;
				((float*)(b + f->ofs))[2] = 0;
				break;
			case F_IGNORE:
				break;
				/* MrG{DREGN} added */
			case F_GSTRING:
			case F_EDICT:
			case F_ITEM:
			case F_CLIENT:
				break;
				/* END */
			default:
				break;
			}
			return;
		}
	}
	gi.dprintf("%s is not a field\n", key);
}


/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
char* ED_ParseEdict(char* data, edict_t* ent)
{
	qboolean	init;
	char		keyname[256];
	char* com_token;

	init = false;
	memset(&st, 0, sizeof(st));

	// go through all the dictionary pairs
	while (1)
	{
		// parse key
		com_token = COM_Parse((const char**)&data);
		if (com_token[0] == '}')
			break;
		if (!data)
			gi.error("ED_ParseEntity: EOF without closing brace");

		strncpy(keyname, com_token, sizeof(keyname) - 1);

		// parse value
		com_token = COM_Parse((const char**)&data);
		if (!data)
			gi.error("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			gi.error("ED_ParseEntity: closing brace without data");

		init = true;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		ED_ParseField(keyname, com_token, ent);
	}

	if (!init)
		memset(ent, 0, sizeof(*ent));

	return data;
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void)
{
	edict_t* e, * e2, * chain;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf("%i teams with %i entities\n", c, c2);
}

//MATTHIAS
void NodeCheck(edict_t* ent)
{
	if (numnodes >= MAX_NODES - 1)	//MAX_NODES = 512
		gi.cvar_set("dntg", "0");

	if (dntg->value > 0)
	{
		gi.dprintf("\nDynamic node table generation ON\n\n");
	}
	else
	{
		gi.dprintf("\nDynamic node table generation OFF\n\n");
	}

	G_FreeEdict(ent);
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(char* mapname, char* entities, char* spawnpoint)
{
	edict_t* ent, * dummy;
	int			inhibit;
	char* com_token;
	int			i;
	float		bot_skill;

#ifdef DEBUG
	LOGPRINTF("SpawnEntites\n");
#endif

	bot_skill = floor(skill->value);
	if (bot_skill < 0)
		bot_skill = 0;
	if (bot_skill > 3)
		bot_skill = 3;
	if (skill->value != bot_skill)
		gi.cvar_forceset("skill", va("%f", bot_skill));

	SaveClientData();

	//MATTHIAS

	weapon_list = NULL;
	ammo_list = NULL;
	health_list = NULL;
	powerup_list = NULL;

	gi.FreeTags(TAG_LEVEL);

	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

	strncpy(level.mapname, mapname, sizeof(level.mapname) - 1);
	strncpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint) - 1);

	gi.dprintf("Starting Map: %s\n", level.mapname);

	// set client fields on player ents
	for (i = 0; i < game.maxclients; i++)
		g_edicts[i + 1].client = game.clients + i;

	ent = NULL;
	inhibit = 0;

	// parse ents
	while (1)
	{
		// parse the opening brace
		com_token = COM_Parse((const char**)&entities);
		if (!entities)
			break;
		if (com_token[0] != '{')
			gi.error("ED_LoadFromFile: found %s when expecting {", com_token);

		if (!ent)
			ent = g_edicts;
		else
			ent = G_Spawn();


		if (!ent)/* MrG{DRGN} sanitiy check*/
		{
			gi.error("%s failed parsing entities.\n", __func__);
			return;
		}

		entities = ED_ParseEdict(entities, ent);

		// yet another map hack

		if (!Q_stricmp(level.mapname, "command") && !Q_stricmp(ent->classname, "trigger_once") && !Q_stricmp(ent->model, "*27"))
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch

		if (ent != g_edicts)
		{
			if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
			{
				G_FreeEdict(ent);
				inhibit++;
				continue;
			}

			ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);
		}

		ED_CallSpawn(ent);
	}

	gi.dprintf("%i entities inhibited\n", inhibit);

	G_FindTeams();

	//MATTHIAS
	dummy = G_Spawn();
	dummy->think = NodeCheck;
	dummy->nextthink = level.time + 1.5;

	//PlayerTrail_Init ();	//MATTHIAS

	sl_GameStart(&gi, level);	//	StdLog - Mark Davies

	CTFSetupTechSpawn();
}

//===================================================================

#if 0
	// cursor positioning
xl <value>
xr <value>
yb <value>
yt <value>
xv <value>
yv <value>

// drawing
statpic <name>
pic <stat>
num <fieldwidth> <stat>
string <stat>

// control
if <stat>
ifeq <stat> <value>
ifbit <stat> <value>
endif

#endif

char* dm_statusbar =
"yb	-24 "

//health
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
"yt 146 "
"xr	-50 "
"num 3 14 "

//tech
"yt 120 "
"if 26 "
"xr -26 "
"pic 26 "
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

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"	environment map name
"skyaxis"	vector axis for rotating sky
"skyrotate"	speed of rotation in degrees/second
"sounds"	music cd track number
"gravity"	800 is default gravity
"message"	text to print at user logon
*/
void SP_worldspawn(edict_t * ent)
{

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;			// since the world doesn't use G_Spawn()
	ent->s.modelindex = WORLD_MODEL;		// world model is always index 1
	ent->classindex = WORLDSPAWN; /* MrG{DRGN} */
	ent->classname = "worldspawn";
	/* MrG{DRGN}
	* this will create a different series of random numbers on every map load
	* by using current time as a seed for the  random generator
	*/
	//srand(time(0));
	srand((unsigned int)(time(0)));
	/* END */

	//---------------

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue();

	SetItemNames();

	if (st.nextmap)
		/*	MrG{DRGN} destination safe strcpy replacement*/
		Com_strcpy(level.nextmap, sizeof(level.nextmap), st.nextmap);
	// make some data visible to the server

	if (ent->message && ent->message[0])
	{
		gi.configstring(CS_NAME, ent->message);

		strncpy(level.level_name, ent->message, sizeof level.level_name - 1);
	}
	else
		strncpy(level.level_name, level.mapname, sizeof(level.level_name));

	if (st.sky && st.sky[0])
		gi.configstring(CS_SKY, st.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));

	gi.configstring(CS_SKYAXIS, va("%f %f %f",
		st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

	gi.configstring(CS_CDTRACK, va("%i", ent->sounds));

	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));

	// status bar program



	//---------------
	if (!st.gravity)
		gi.cvar_set("sv_gravity", "800");
	else
		gi.cvar_set("sv_gravity", st.gravity);

	if (ctf->value)
	{
		gi.configstring(CS_STATUSBAR, ctf_statusbar);
		//precaches
		gi.imageindex("sbctf1");
		gi.imageindex("sbctf2");
		gi.imageindex("i_ctf1");
		gi.imageindex("i_ctf2");
		gi.imageindex("i_ctf1d");
		gi.imageindex("i_ctf2d");
		gi.imageindex("i_ctf1t");
		gi.imageindex("i_ctf2t");
		gi.imageindex("i_ctfj");
	}
	else
		gi.configstring(CS_STATUSBAR, dm_statusbar);



	PrecacheItem(it_ak42);
	PreCacheAll();

	if (lightsoff->value == 0)
	{
		// 0 normal
		gi.configstring(CS_LIGHTS + 0, "m");

		// 1 FLICKER (first variety)
		gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

		// 2 SLOW STRONG PULSE
		gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

		// 3 CANDLE (first variety)
		gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

		// 4 FAST STROBE
		gi.configstring(CS_LIGHTS + 4, "mamamamamama");

		// 5 GENTLE PULSE 1
		gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

		// 6 FLICKER (second variety)
		gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");

		// 7 CANDLE (second variety)
		gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

		// 8 CANDLE (third variety)
		gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

		// 9 SLOW STROBE (fourth variety)
		gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

		// 10 FLUORESCENT FLICKER
		gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

		// 11 SLOW PULSE NOT FADE TO BLACK
		gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

		//JR 3/26/98
		//12 Wierd flashing
		gi.configstring(CS_LIGHTS + 12, "ahsbexcbkxerswaibldcwersxa");
	}
	else if (lightsoff->value == 1)
	{
		// 0 normal
		gi.configstring(CS_LIGHTS + 0, "c");

		// 1 FLICKER (first variety)
		gi.configstring(CS_LIGHTS + 1, "acacacccacccaccaccc");

		// 2 SLOW STRONG PULSE
		gi.configstring(CS_LIGHTS + 2, "aaacccaaacccaaaccc");

		// 3 CANDLE (first variety)
		gi.configstring(CS_LIGHTS + 3, "ccccacccccccaccccaacacacac");

		// 4 FAST STROBE
		gi.configstring(CS_LIGHTS + 4, "cacacacacacaca");

		// 5 GENTLE PULSE 1
		gi.configstring(CS_LIGHTS + 5, "cbacbcvacbacabcbacbac");

		// 6 FLICKER (second variety)
		gi.configstring(CS_LIGHTS + 6, "cbacbacbacbacba");

		// 7 CANDLE (second variety)
		gi.configstring(CS_LIGHTS + 7, "aaaaaaabbbccaaaaaaaacccbbb");

		// 8 CANDLE (third variety)
		gi.configstring(CS_LIGHTS + 8, "aaaacccccbbbbbaaaaccccbbbbb");

		// 9 SLOW STROBE (fourth variety)
		gi.configstring(CS_LIGHTS + 9, "aaaaaaaaccccccccc");

		// 10 FLUORESCENT FLICKER
		gi.configstring(CS_LIGHTS + 10, "ccaccaccaccaccaccacca");

		// 11 SLOW PULSE NOT FADE TO BLACK
		gi.configstring(CS_LIGHTS + 11, "bbcdbcbdcbdcbdbcbdcbdbc");

		//JR 3/26/98
		//12 Wierd flashing
		gi.configstring(CS_LIGHTS + 12, "bbcdedbcedbcedabcedbca");
	}
	else if (lightsoff->value == 2)
	{
		// 0 normal
		gi.configstring(CS_LIGHTS + 0, "a");

		// 1 FLICKER (first variety)
		gi.configstring(CS_LIGHTS + 1, "a");

		// 2 SLOW STRONG PULSE
		gi.configstring(CS_LIGHTS + 2, "a");

		// 3 CANDLE (first variety)
		gi.configstring(CS_LIGHTS + 3, "a");

		// 4 FAST STROBE
		gi.configstring(CS_LIGHTS + 4, "a");

		// 5 GENTLE PULSE 1
		gi.configstring(CS_LIGHTS + 5, "a");

		// 6 FLICKER (second variety)
		gi.configstring(CS_LIGHTS + 6, "a");

		// 7 CANDLE (second variety)
		gi.configstring(CS_LIGHTS + 7, "a");

		// 8 CANDLE (third variety)
		gi.configstring(CS_LIGHTS + 8, "a");

		// 9 SLOW STROBE (fourth variety)
		gi.configstring(CS_LIGHTS + 9, "a");

		// 10 FLUORESCENT FLICKER
		gi.configstring(CS_LIGHTS + 10, "a");

		// 11 SLOW PULSE NOT FADE TO BLACK
		gi.configstring(CS_LIGHTS + 11, "a");

		//JR 3/26/98
		//12 Wierd flashing
		gi.configstring(CS_LIGHTS + 12, "a");
	}


	LoadMOTD();

	if (botchat->value)/* MrG{DRGN} */
		Load_BotChat();

	Bot_InitNodes();		//init route table

	if (!Bot_LoadNodes())	//try to load route table
		Bot_InitNodes();	//init new route table if load not possible
}
