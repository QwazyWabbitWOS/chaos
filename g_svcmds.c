#include "g_local.h"
#include "c_botai.h"

/* This command appears to be a dummy function for
 testing that your rcon_password is correct.
 */
static void	SVCmd_Test_f(void)
{
	gi.cprintf(NULL, PRINT_HIGH, "SVCmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be extended at a later date.  The filter lists are not saved and restored by default, because I believe it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

==============================================================================
*/

typedef struct
{
	unsigned	mask;
	unsigned	compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

static ipfilter_t	ipfilters[MAX_IPFILTERS];
static int			numipfilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter(char* s, ipfilter_t* f)
{
	char	num[128] = { 0 };
	int		i;
	byte	b[4] = { 0 };
	byte	m[4] = { 0 };

	for (i = 0; i < 4; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++)
	{
		int j;

		if (*s < '0' || *s > '9')
		{
			gi.cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = *(unsigned*)m;
	f->compare = *(unsigned*)b;

	return true;
}

/*
=================
SVCmd_FilterPacket
=================
*/
qboolean SVCmd_FilterPacket(char* from)
{
	int		i;
	unsigned	in;
	byte m[4] = { 0 };
	char* p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	in = *(unsigned*)m;

	for (i = 0; i < numipfilters; i++)
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
			return (int)filterban->value;

	return (int)!filterban->value;
}

/*
=================
SVCmd_AddIP_f
sv addip - add addresses from the filter list with:
sv addip <ip>
The ip address is specified in dot format, and any unspecified digits will match any value,
so you can specify an entire class C network with "sv addip 192.246.40".
=================
*/
static void SVCmd_AddIP_f(void)
{
	int		i;

	if (gi.argc() < 3) {
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  addip <ip-mask>\n");
		return;
	}

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		/* free spot */
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			gi.cprintf(NULL, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter(gi.argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SVCmd_RemoveIP_f
sv addip - remove addresses from the filter list with:
sv removeip <ip>
sv removeip will only remove an address specified exactly the same way.
You cannot addip a subnet, then removeip a single host.
=================
*/
static void SVCmd_RemoveIP_f(void)
{
	ipfilter_t	f;
	int			i, j;

	if (gi.argc() < 3) {
		gi.cprintf(NULL, PRINT_HIGH, "Usage:  sv removeip <ip-mask>\n");
		return;
	}

	if (!StringToFilter(gi.argv(2), &f))
		return;

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].mask == f.mask
			&& ipfilters[i].compare == f.compare)
		{
			for (j = i + 1; j < numipfilters; j++)
				ipfilters[j - 1] = ipfilters[j];
			numipfilters--;
			gi.cprintf(NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	gi.cprintf(NULL, PRINT_HIGH, "Didn't find %s.\n", gi.argv(2));
}

/*
=================
SVCmd_ListIP_f
sv listip Prints the current list of filters.
=================
*/
static void SVCmd_ListIP_f(void)
{
	int		i;
	byte	b[4] = { 0 };

	gi.cprintf(NULL, PRINT_HIGH, "Filter list:\n");
	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		gi.cprintf(NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SVCmd_WriteIP_f
sv writeip -	Dumps "addip <ip>" commands to listip.cfg so it can be extended at a later date.
				The filter lists are not saved and restored by default, because I believe it would cause too much confusion.
=================
*/
static void SVCmd_WriteIP_f(void)
{
	FILE* f;
	char	name[MAX_OSPATH];
	byte	b[4] = { 0 };
	int		i;

	Com_strcpy(name, sizeof name, "./");
	Com_strcat(name, sizeof name, game_dir->string);
	Com_strcat(name, sizeof name, "/listip.cfg");

	if ((f = fopen(name, "wb")) == NULL)    /* MrG{DRGN} check the return */
	{
		gi.cprintf(NULL, PRINT_HIGH, "Couldn't open %s\n", name, strerror(errno));
		return;
	}

	gi.cprintf(NULL, PRINT_HIGH, "Writing %s.\n", name);
	fprintf(f, "set filterban %d\n", (int)filterban->value);

	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		fprintf(f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	fclose(f);
}

/* FWP Move to next map in the current rotation */

// Force the next map in queue
static void SVCmd_nextmap_f(void)
{
	bprint_botsafe(PRINT_HIGH, "Advancing to next level.\n");
	EndDMLevel();
}

// Turn on sequential map rotation
static void SVCmd_sequential_f(void)
{
	if (maplist.nummaps > 0)  // does a maplist exist?
	{
		maplist.mlflag = ML_ROTATE_SEQ;
		maplist.currentmap = -1;
		gi.cprintf(NULL, PRINT_HIGH, "Sequential map rotation ON!\n\n");
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
}

// Turn on randomized map rotation
static void SVCmd_random_f(void)
{
	if (maplist.nummaps > 0)  // does a maplist exist?
	{
		maplist.mlflag = ML_ROTATE_RANDOM;
		maplist.currentmap = -1;
		gi.cprintf(NULL, PRINT_HIGH, "Random map rotation ON!\n\n");
	}
	else
	{
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
	}
}

static void SVCmd_goto_f(char* mapnum)
{
	char command[MAX_QPATH] = { 0 };

	if (maplist.nummaps > 0)  // does a maplist exist?
	{
		if (maplist.mlflag > 0)
		{
			// mapnum is 1 to n for users
			// we convert it here for zero based array
			int num = atoi(mapnum) - 1;
			if (num < maplist.nummaps && num >= 0) // range check
			{
				sprintf(command, "gamemap %s", maplist.mapnames[num]);
				gi.cprintf(NULL, PRINT_HIGH, "Map is changing to %d! %s\n\n", num + 1, maplist.mapnames[num]);
				gi.AddCommandString(command);
			}
			else
				gi.cprintf(NULL, PRINT_HIGH, "Map number %d not found!\n\n", num + 1);
		}
		else
			gi.cprintf(NULL, PRINT_HIGH, "You have to start the map rotation with <sv ml 1 or 2> first!\n\n");
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
}

static void SVCmd_show_f(void)
{
	if (maplist.nummaps > 0)  // does a maplist exist?
	{
		int i;

		gi.cprintf(NULL, PRINT_HIGH, "Current maplist:\n");

		for (i = 0; i < maplist.nummaps; i++)
		{
			gi.cprintf(NULL, PRINT_HIGH, "...%s,ctf=%c,lightsoff=%c\n", maplist.mapnames[i], maplist.ctf[i], maplist.lightsoff[i]);
		}
		gi.cprintf(NULL, PRINT_HIGH, "\n");
	}
	else
	{
		gi.cprintf(NULL, PRINT_HIGH, "The maplist is empty!\n");
	}
	gi.cprintf(NULL, PRINT_HIGH, "Load a maplist with <sv ml maplistname|show>!\n");
	gi.cprintf(NULL, PRINT_HIGH, "Control maplist mode with <sv ml 0|1|2 off|sequential|random>!\n");
	gi.cprintf(NULL, PRINT_HIGH, "Skip to a map in the maplist with with <sv ml goto mapnumber!\n\n");
}
/*
=================
ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() and gi.argv() commands to get the rest
of the parameters.
=================
*/
void ServerCommand(void)
{
	char* cmd;

	cmd = gi.argv(1);
	if (Q_stricmp(cmd, "test") == 0)
		SVCmd_Test_f();
	else if (Q_stricmp(cmd, "addip") == 0)
		SVCmd_AddIP_f();
	else if (Q_stricmp(cmd, "removeip") == 0)
		SVCmd_RemoveIP_f();
	else if (Q_stricmp(cmd, "listip") == 0)
		SVCmd_ListIP_f();
	else if (Q_stricmp(cmd, "writeip") == 0)
		SVCmd_WriteIP_f();
	else if (Q_stricmp(cmd, "addbots") == 0)
		SVCmd_addbots_f();
	else if (Q_stricmp(cmd, "killbot") == 0)
		SVCmd_killbot_f(gi.argv(2));
	else if (Q_stricmp(cmd, "nextmap") == 0)
		SVCmd_nextmap_f();
	else if (Q_stricmp(cmd, "ml") == 0)
	{
		if (Q_stricmp(gi.argv(2), "0") == 0)	//map rotation off
		{
			MaplistClear();
		}
		else if (Q_stricmp(gi.argv(2), "1") == 0)	//start sequential rotation
		{
			SVCmd_sequential_f();
		}
		else if (Q_stricmp(gi.argv(2), "2") == 0)	//start random rotation
		{
			SVCmd_random_f();
		}
		else if (Q_stricmp(gi.argv(2), "goto") == 0)	//jump to map X in list
		{
			SVCmd_goto_f(gi.argv(3));
		}
		else if (Q_stricmp(gi.argv(2), "shuffle") == 0)	//shuffle the rotation
		{
			gi.cprintf(NULL, PRINT_HIGH, "Shuffling the rotation of the random maplist!\n");
			MaplistShuffle();
		}
		else if (Q_stricmp(gi.argv(2), "") == 0)	//print maplist
		{
			SVCmd_show_f();
		}
		else	//load maplist
		{
			MaplistLoad(gi.argv(2));
		}
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}
