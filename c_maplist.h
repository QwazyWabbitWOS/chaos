#pragma once

void MaplistClear(void); // clear the maplist
void MaplistShuffle(void); //shuffle the rotation
void MaplistLoad(char* filename); // load the maplist from the file
int MaplistNext(void); // Choose next map in rotation based on current mode.
void MaplistMapModeSetup(void);	// ctf and lightsoff flags