
#ifndef C_MAPLIST_H
#define C_MAPLIST_H

#define MAX_MAPS           512
#define MAX_MAPNAME_LEN    32

#define ML_OFF				0
#define ML_ROTATE_SEQ		1
#define ML_ROTATE_RANDOM	2

typedef struct maplist_s
{
	int		nummaps;
	char	mapnames[MAX_MAPS][MAX_MAPNAME_LEN];
	char	ctf[MAX_MAPS];
	char	lightsoff[MAX_MAPS];
	int		mlflag;
	int		currentmap;
	int		index; //QW// for random map indexing
	int		map_random[MAX_MAPS];
} maplist_t;

maplist_t maplist;

// Maplist handling Cvars
cvar_t* g_maplistfile;
cvar_t* g_maplistmode;// 1 for sequential, 2 for random map selection

// public functions
void	MaplistInitVars(void);
void	MaplistClear(void); // clear the maplist
void	Maplis(void); //shuffle the rotation
void	MaplistLoad(char* filename); // load the maplist from the file
int		MaplistNext(void); // Choose next map in rotation based on current mode.
void	MaplistMapModeSetup(void);	// ctf and lightsoff flags

#endif
