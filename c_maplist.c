#include "g_local.h"
#include "c_maplist.h"

void MaplistInitVars(void)
{
	g_maplistfile = gi.cvar("g_maplistfile", "chaosdm.txt", 0);
	g_maplistmode = gi.cvar("g_maplistmode", "1", 0); // 1 for sequential, 2 for random map selection
}

void MaplistClear(void)
{
	memset(&maplist, 0, sizeof maplist);
	maplist.currentmap = -1;
}

void MaplistLoad(char* filename)
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

	MaplistClear();

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
	fclose(fp);

	MaplistShuffle();

	if (maplist.nummaps > 0) {
		maplist.mlflag = g_maplistmode->value; //set per configured mode.
	}
	if (maplist.nummaps == 1) {
		maplist.mlflag = ML_OFF; // no rotation if only 1 map
	}
	if (maplist.nummaps == 0)
	{
		gi.cprintf(NULL, PRINT_HIGH, "No maps listed in %s\n", file);
	}
	else
	{
		gi.cprintf(NULL, PRINT_HIGH, "%i map(s) loaded.\n\n", maplist.nummaps);
	}
	gi.cprintf(NULL, PRINT_HIGH, "Map rotation is %s mode %i\n\n", maplist.mlflag ? "ON" : "OFF", maplist.mlflag);
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
		maplist.currentmap = (maplist.map_random[maplist.index]) % maplist.nummaps;
		maplist.index = (maplist.index + 1) % maplist.nummaps;
		break;
	default:
		maplist.mlflag = ML_OFF;
		maplist.currentmap = maplist.currentmap;
		break;
	}

	return maplist.currentmap;
}

void MaplistShuffle(void)
{
	int i;

	for (i = 0; i < maplist.nummaps; i++)
		maplist.map_random[i] = i;

	for (i = 0; i < maplist.nummaps; i++)
	{
		int temp = maplist.map_random[i];
		int random = random() * maplist.nummaps;
		maplist.map_random[i] = maplist.map_random[random];
		maplist.map_random[random] = temp;
	}
}
