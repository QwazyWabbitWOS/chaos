#ifndef C_BELT_H
#define C_BELT_H

gitem_t* it_belt; //always possessed, never spawns in game

void Cmd_Belt_f(edict_t* ent);
void Belt_Think(edict_t* ent);

#endif
