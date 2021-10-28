
#ifndef C_MAPLIST_H
#define C_MAPLIST_H


// Maplist handling Cvars
cvar_t* g_maplistfile;
cvar_t* g_maplistmode;// 1 for sequential, 2 for random map selection

// public functions
void	MaplistInitVars(void);
void	MaplistClear(void); // clear the maplist
void	MaplistShuffle(void); //shuffle the rotation
void	MaplistLoad(char* filename); // load the maplist from the file
int		MaplistNext(void); // Choose next map in rotation based on current mode.
void	MaplistMapModeSetup(void);	// ctf and lightsoff flags

#endif
