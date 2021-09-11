#include "g_local.h"
#include "c_botai.h"
#include "c_base.h"


void	Svcmd_Test_f(void)
{
	gi.cprintf(NULL, PRINT_HIGH, "Svcmd_Test_f()\n");
}


/* FWP Move to next map in the current rotation */

// Force the next map in queue
void Svcmd_nextmap_f()

{
	bprintf2(PRINT_HIGH, "Advancing to next level.\n");
	EndDMLevel();
}

// Turn on sequential map rotation
void Svcmd_sequential_f(void)
{
	if (maplist.nummaps > 0)  // does a maplist exist? 
	{
		maplist.mlflag = 1; //sequential
		maplist.currentmap = -1;
		gi.cprintf(NULL, PRINT_HIGH, "Sequential map rotation ON!\n\n");
		EndDMLevel();
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
}

// Turn on randomized map rotation
void Svcmd_random_f(void)
{
	if (maplist.nummaps > 0)  // does a maplist exist? 
	{
		maplist.mlflag = 2; //random
		maplist.currentmap = -1;
		gi.cprintf(NULL, PRINT_HIGH, "Random map rotation ON!\n\n");
		EndDMLevel();
	}
	else
	{
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
	}
}

void Svcmd_goto_f(char *mapnum)
{
	if (maplist.nummaps > 0)  // does a maplist exist?
	{
		if (maplist.mlflag > 0)
		{
			int num = atoi(mapnum);
			if (num < maplist.nummaps)
			{
				maplist.currentmap = num;
				gi.cprintf(NULL, PRINT_HIGH, "Map is changing to %d! %s\n\n", num, maplist.mapnames[num]);
				EndDMLevel();
			}
			else
				gi.cprintf(NULL, PRINT_HIGH, "Map number %d not found!\n\n", num);
		}
		else
			gi.cprintf(NULL, PRINT_HIGH, "You have to start the map rotation with <sv ml 1 or 2> first!\n\n");
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "You have to load a maplist first!\n\n");
}

void Svcmd_show_f(void)
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
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/

void ServerCommand(void)
{
	char* cmd;

	cmd = gi.argv(1);
	if (Q_stricmp(cmd, "test") == 0)
		Svcmd_Test_f();
	else if (Q_stricmp(cmd, "addbots") == 0)
		Svcmd_addbots_f();
	else if (Q_stricmp(cmd, "killbot") == 0)
		Svcmd_killbot_f(gi.argv(2));
	else if (Q_stricmp(cmd, "nextmap") == 0)
		Svcmd_nextmap_f();
	else if (Q_stricmp(cmd, "ml") == 0)
	{
		if (Q_stricmp(gi.argv(2), "0") == 0)	//map rotation off
		{
			ClearMaplist();
		}
		else if (Q_stricmp(gi.argv(2), "1") == 0)	//start sequential rotation
		{
			Svcmd_sequential_f();
		}
		else if (Q_stricmp(gi.argv(2), "2") == 0)	//start random rotation
		{
			Svcmd_random_f();
		}
		else if (Q_stricmp(gi.argv(2), "goto") == 0)	//jump to map X in list
		{
			Svcmd_goto_f(gi.argv(3));
		}
		else if (Q_stricmp(gi.argv(2), "") == 0)	//print maplist
		{
			Svcmd_show_f();
		}
		else	//load maplist
		{
			LoadMaplist(gi.argv(2));
		}
	}
	else
		gi.cprintf(NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}





